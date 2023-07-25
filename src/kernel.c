#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "LRU_queue.h"

bool multi_threading = false;
pthread_t worker1;
pthread_t worker2;
bool active = false;
bool debug = false;
bool in_background = false;
pthread_mutex_t queue_lock;
int copy_file(char *source_path, char *target_path);

int DUPLICATE_FILES = 0;

void lock_queue()
{
    if (multi_threading)
        pthread_mutex_lock(&queue_lock);
}

void unlock_queue()
{
    if (multi_threading)
        pthread_mutex_unlock(&queue_lock);
}

int process_initialize(char *filename)
{
    FILE *fp;
    int error_code = 0;
    int *start = (int *)malloc(sizeof(int));
    int *end = (int *)malloc(sizeof(int));

    // check if file has already been copied
    // if there are duplicates, copy file with diferent basename, else proceed as usual

    char destination[100];
    sprintf(destination, "./backing_store/%s", filename);

    if (access(destination, W_OK) == 0)
    {
        DUPLICATE_FILES++;
        memset(destination, 0, sizeof(destination));
        sprintf(destination, "./backing_store/%s_%d", filename, DUPLICATE_FILES);
    }

    copy_file(filename, destination);

    fp = fopen(destination, "r");

    if (fp == NULL)
    {
        error_code = 11; // 11 is the error code for file does not exist
        return error_code;
    }

    error_code = load_file(fp, start, end, destination);
    if (error_code != 0)
    {
        fclose(fp);
        return error_code;
    }

    PCB *newPCB = makePCB(*start, *end);
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_tail(node);
    unlock_queue();
    fclose(fp);
    return error_code;
}

int shell_process_initialize()
{
    // Note that "You can assume that the # option will only be used in batch mode."
    // So we know that the input is a file, we can directly load the file into ram
    int *start = (int *)malloc(sizeof(int));
    int *end = (int *)malloc(sizeof(int));
    int error_code = 0;
    error_code = load_file(stdin, start, end, "_SHELL");
    if (error_code != 0)
    {
        return error_code;
    }
    PCB *newPCB = makePCB(*start, *end);
    newPCB->priority = true;
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_head(node);
    unlock_queue();
    freopen("/dev/tty", "r", stdin);
    return 0;
}

int store_initialize(char *filename)
{
    FILE *fp;
    int error_code = 0;

    // check if file has already been copied
    // if there are duplicates, copy file with diferent basename, else proceed as usual

    char destination[100];
    sprintf(destination, "./backing_store/%s", filename);

    if (access(destination, W_OK) == 0)
    {
        DUPLICATE_FILES++;
        memset(destination, 0, sizeof(destination));
        sprintf(destination, "./backing_store/%s_%d", filename, DUPLICATE_FILES);
    }

    copy_file(filename, destination);

    int length = line_count(destination);
    fp = fopen(destination, "r");

    if (fp == NULL)
    {
        error_code = 11; // 11 is the error code for file does not exist
        return error_code;
    }

    PCB *newPCB = makePCBNew(destination);

    for (int i = 0; i < 2 && i < ((length / 3) + (length % 3 != 0)); i++)
    {
        error_code = load_page(fp, newPCB);
    }

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_tail(node);
    unlock_queue();
    fclose(fp);
    return error_code;
}

/**
 * Executes the next {quanta} instructions for
 * the given process. If the next instruction is
 * not loaded into memory. We either load it if there is
 * empty space or call empty_page to
 * make space for it in memory, load it, but we
 * do not execute the next instructions
 */
bool execute_process(QueueNode *node, int quanta)
{

    char *line = NULL;
    PCB *pcb = node->pcb;
    char *Countline;

    for (int i = 0; i < quanta; i++)
    {

        // If we find a page miss we either load the page into the frame store
        // or we evict a frame and then load a page into the frame store
        if (pcb->pageTable[pcb->PC] == -1)
        {

            FILE *fp = fopen(pcb->filename, "r");

            // Finding next instruction to load from file
            for (int x = 0; x < pcb->len; x++)
            {
                if (x == pcb->PC)
                {
                    break;
                }
                Countline = calloc(1, 1000);
                fgets(Countline, 999, fp);
                free(Countline);
            }

            // If the frame store is full we call the empty_frame function to clear a frame and then we load the next page in the frame store.
            // Returning false tells the scheduler to put the current process at the end of the ready queue.
            if (load_page(fp, pcb) == 21)
            {
                empty_frame();

                load_page(fp, pcb);

                fclose(fp);

                return false;
            }

            fclose(fp);
            return false;
        }

        if (i == 1)
        {

            // Adding the current process to the LRU_queue
            LRU_queue_add_to_tail(node);
        }
        line = mem_get_value_at_line(pcb->pageTable[pcb->PC++]);

        in_background = true;
        if (pcb->priority)
        {
            pcb->priority = false;
        }
        if (pcb->PC > pcb->len - 1)
        {
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        in_background = false;
    }
    return false;
}

/**
 * Same as normal execute_process but instead of returning
 * after loading the next instructions we continue execution
 */
bool execute_process_FCFS(QueueNode *node, int quanta)
{

    char *line = NULL;
    PCB *pcb = node->pcb;
    char *Countline;

    for (int i = 0; i < quanta; i++)
    {

        // If we find a page miss we either load the page into the frame store
        // or we evict a frame and then load a page into the frame store
        if (pcb->pageTable[pcb->PC] == -1)
        {

            FILE *fp = fopen(pcb->filename, "r");

            // Finding next instruction to load from file
            for (int x = 0; x < pcb->len; x++)
            {
                if (x == pcb->PC)
                {
                    break;
                }
                Countline = calloc(1, 1000);
                fgets(Countline, 999, fp);
                free(Countline);
            }

            // If the frame store is full we call the empty_frame function to clear a frame and then we load the next page in the frame store.
            // Returning false tells the scheduler to put the current process at the end of the ready queue.
            if (load_page(fp, pcb) == 21)
            {
                empty_frame();

                load_page(fp, pcb);

                fclose(fp);

                return execute_process_FCFS(node, MAX_INT);
            }

            fclose(fp);
            return execute_process_FCFS(node, MAX_INT);
        }

        if (i == 1)
        {

            // Adding the current process to the LRU_queue
            LRU_queue_add_to_tail(node);
        }
        line = mem_get_value_at_line(pcb->pageTable[pcb->PC++]);

        in_background = true;
        if (pcb->priority)
        {
            pcb->priority = false;
        }
        if (pcb->PC > pcb->len - 1)
        {
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS()
{
    QueueNode *cur;
    while (true)
    {
        lock_queue();
        if (is_ready_empty())
        {
            unlock_queue();
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        unlock_queue();
        execute_process_FCFS(cur, MAX_INT);
    }
    if (multi_threading)
        pthread_exit(NULL);
    return 0;
}

void *scheduler_SJF()
{
    QueueNode *cur;
    while (true)
    {
        lock_queue();
        if (is_ready_empty())
        {
            unlock_queue();
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        unlock_queue();
        execute_process(cur, MAX_INT);
    }
    if (multi_threading)
        pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING_alternative()
{
    QueueNode *cur;
    while (true)
    {
        lock_queue();
        if (is_ready_empty())
        {
            unlock_queue();
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if (!execute_process(cur, 1))
        {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }
    }
    if (multi_threading)
        pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING()
{
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while (true)
    {
        lock_queue();
        if (is_ready_empty())
        {
            unlock_queue();
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if (shortest < cur->pcb->job_length_score)
        {
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if (!execute_process(cur, 1))
        {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }
    }
    if (multi_threading)
        pthread_exit(NULL);
    return 0;
}

void *scheduler_RR(void *arg)
{
    int quanta = ((int *)arg)[0];
    QueueNode *cur;
    while (true)
    {
        lock_queue();
        if (is_ready_empty())
        {
            unlock_queue();
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        unlock_queue();
        if (!execute_process(cur, quanta))
        {
            lock_queue();
            ready_queue_add_to_tail(cur);
            unlock_queue();
        }
    }
    if (multi_threading)
        pthread_exit(NULL);
    return 0;
}

int threads_initialize(char *policy)
{
    active = true;
    multi_threading = true;
    int arg[1];
    pthread_mutex_init(&queue_lock, NULL);
    if (strcmp("FCFS", policy) == 0)
    {
        pthread_create(&worker1, NULL, scheduler_FCFS, NULL);
        pthread_create(&worker2, NULL, scheduler_FCFS, NULL);
    }
    else if (strcmp("SJF", policy) == 0)
    {
        pthread_create(&worker1, NULL, scheduler_SJF, NULL);
        pthread_create(&worker2, NULL, scheduler_SJF, NULL);
    }
    else if (strcmp("RR", policy) == 0)
    {
        arg[0] = 2;
        pthread_create(&worker1, NULL, scheduler_RR, (void *)arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *)arg);
    }
    else if (strcmp("AGING", policy) == 0)
    {
        pthread_create(&worker1, NULL, scheduler_AGING, (void *)arg);
        pthread_create(&worker2, NULL, scheduler_AGING, (void *)arg);
    }
    else if (strcmp("RR30", policy) == 0)
    {
        arg[0] = 30;
        pthread_create(&worker1, NULL, scheduler_RR, (void *)arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *)arg);
    }
}

void threads_terminate()
{
    if (!active)
        return;
    bool empty = false;
    while (!empty)
    {
        empty = is_ready_empty();
    }
    active = false;
    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);
}

int schedule_by_policy(char *policy, bool mt)
{
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0 && strcmp(policy, "RR30") != 0)
    {
        return 15;
    }
    if (active)
        return 0;
    if (in_background)
        return 0;
    int arg[1];
    if (mt)
        return threads_initialize(policy);
    else
    {
        if (strcmp("FCFS", policy) == 0)
        {
            scheduler_FCFS();
        }
        else if (strcmp("SJF", policy) == 0)
        {
            scheduler_SJF();
        }
        else if (strcmp("RR", policy) == 0)
        {

            arg[0] = 2;
            scheduler_RR((void *)arg);
        }
        else if (strcmp("AGING", policy) == 0)
        {
            scheduler_AGING();
        }
        else if (strcmp("RR30", policy) == 0)
        {
            arg[0] = 30;
            scheduler_RR((void *)arg);
        }
        return 0;
    }
}

int create_backing_store()
{
    struct stat st = {0};
    int error_code = 0;

    if (stat("backing_store", &st) == -1)
    {
        error_code = mkdir("backing_store", 0700);
    }
    else
    {
        system("rm -rf backing_store");
        error_code = mkdir("backing_store", 0700);
    }
    return error_code;
}

int delete_backing_store()
{
    return system("rm -rf backing_store");
}

int copy_file(char *source_path, char *target_path)
{
    FILE *source_file, *target_file;

    source_file = fopen(source_path, "r"); // "r" means read mode
    target_file = fopen(target_path, "w"); // "w" means write mode
    char buffer[1000];

    if (source_file == NULL || target_file == NULL)
    {
        return 11;
    }

    memset(buffer, 0, sizeof(buffer));

    while (fgets(buffer, sizeof(buffer), source_file))
    {
        int i = 0;
        while (buffer[i] != '\0')
        {
            if (buffer[i] == ';')
            {
                buffer[i] = '\n';
            }
            i++;
        }
        fputs(buffer, target_file);
        memset(buffer, 0, sizeof(buffer));
    }

    fclose(source_file);
    fclose(target_file);

    return 0;
}

int line_count(char *filename)
{
    FILE *fp;
    int count = 0; // Line counter (result)

    // Open the file
    fp = fopen(filename, "r");

    // Check if file exists
    if (fp == NULL)
    {
        return 0;
    }

    // Extract characters from file and store in character c
    for (char c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            count = count + 1;

    // Close the file
    fclose(fp);

    return count + 1;
}
