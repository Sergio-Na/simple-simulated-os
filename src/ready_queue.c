#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "LRU_queue.h"

QueueNode *head = NULL;

void ready_queue_destory()
{
    if (!head)
        return;
    QueueNode *cur = head;
    QueueNode *tmp;
    while (cur->next != NULL)
    {
        tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    free(cur);
}

void ready_queue_add_to_tail(QueueNode *node)
{
    if (!head)
    {
        head = node;
        head->next = NULL;
    }
    else
    {
        QueueNode *cur = head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = node;
        cur->next->next = NULL;
    }
}

void ready_queue_add_to_head(QueueNode *node)
{
    if (!head)
    {
        head = node;
        head->next = NULL;
    }
    else
    {
        node->next = head;
        head = node;
    }
}

void print_ready_queue()
{
    if (!head)
    {
        printf("ready queue is empty\n");
        return;
    }
    QueueNode *cur = head;
    printf("Ready queue: \n");
    while (cur != NULL)
    {
        print_pcb(cur->pcb);
        cur = cur->next;
    }
}

void terminate_process(QueueNode *node)
{
    // node should not be in the ready queue
    mem_free_lines_between(node->pcb->start, node->pcb->end);
    free(node);
}

bool is_ready_empty()
{
    return head == NULL;
}

QueueNode *ready_queue_pop_head()
{
    QueueNode *tmp = head;
    if (head != NULL)
        head = head->next;
    return tmp;
}

void ready_queue_decrement_job_length_score()
{
    QueueNode *cur;
    cur = head;
    while (cur != NULL)
    {
        if (cur->pcb->job_length_score > 0)
            cur->pcb->job_length_score--;
        cur = cur->next;
    }
}

void ready_queue_swap_with_next(QueueNode *toSwap)
{
    QueueNode *next;
    QueueNode *afterNext;
    QueueNode *cur = head;
    if (head == toSwap)
    {
        next = head->next;
        head->next = next->next;
        next->next = head;
        head = next;
    }
    while (cur != NULL && cur->next != toSwap)
        cur = cur->next;
    if (cur == NULL)
        return;
    next = cur->next->next;
    afterNext = next->next;
    // cur toSwap next afterNext
    cur->next = next;
    next->next = toSwap;
    toSwap->next = afterNext;
}

bool swap_needed(QueueNode *cur)
{
    QueueNode *next = cur->next;
    if (!next)
        return false;
    if (cur->pcb->priority && next->pcb->priority)
    {
        if (cur->pcb->job_length_score > next->pcb->job_length_score)
        {
            return true;
        }
        else
            return false;
    }
    else if (cur->pcb->priority && !next->pcb->priority)
        return false;
    else if (!cur->pcb->priority && next->pcb->priority)
        return true;
    else
    {
        if (cur->pcb->job_length_score > next->pcb->job_length_score)
            return true;
        else
            return false;
    }
}

void sort_ready_queue()
{
    if (head == NULL)
        return;
    // bubble sort
    QueueNode *cur = head;
    bool sorted = false;
    while (!sorted)
    {
        sorted = true;
        while (cur->next != NULL)
        {
            if (swap_needed(cur))
            {
                sorted = false;
                ready_queue_swap_with_next(cur);
            }
            else
            {
                cur = cur->next;
            }
        }
    }
}

QueueNode *ready_queue_pop_shortest_job()
{
    sort_ready_queue();
    QueueNode *node = ready_queue_pop_head();
    return node;
}

int ready_queue_get_shortest_job_score()
{
    QueueNode *cur = head;
    int shortest = MAX_INT;
    while (cur != NULL)
    {
        if (cur->pcb->job_length_score < shortest)
        {
            shortest = cur->pcb->job_length_score;
        }
        cur = cur->next;
    }
    return shortest;
}

void ready_queue_promote(int score)
{
    if (head->pcb->job_length_score == score)
        return;
    QueueNode *cur = head;
    QueueNode *next;
    while (cur->next != NULL)
    {
        if (cur->next->pcb->job_length_score == score)
            break;
        cur = cur->next;
    }
    if (cur->next == NULL || cur->next->pcb->job_length_score != score)
        return;
    next = cur->next;
    cur->next = cur->next->next;
    next->next = head;
    head = next;
}

/**
 * Pops the process at the head of the LRU Queue.
 * Finds the least recently executed page in the
 * page table and cleares it from memory
 */
int empty_frame()
{

    QueueNode *LRU_node = LRU_queue_pop_head();

    PCB *curPCB = LRU_node->pcb;
    int *PCB_page_table = curPCB->pageTable;
    int page_table_len = curPCB->len;

    for (int i = 0; i < page_table_len; i++)
    {
        if (PCB_page_table[i] != -1)
        {

            printf("Page fault! Victim page contents:\n\n");
            for (int j = i; j < i + 3; j++)
            {
                if (PCB_page_table[j] != -1)
                {
                    printf("%s", mem_get_value_at_line(PCB_page_table[j]));
                    mem_free_lines_between(PCB_page_table[j], PCB_page_table[j]);
                    PCB_page_table[j] = -1;
                }
            }
            printf("\n");
            printf("End of victim page contents.\n");
            return 1;
        }
    }
}
