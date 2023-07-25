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

QueueNode *LRU_head = NULL;

/**
 * Creates new QueueNode by copying the one provided and
 * adds it to the tail of the LRU Queue. Copying is
 * necessary to not mess up the order order of
 * this queue since processess make multiple appearances
 * and using only one node per pcb would break the order
 */
void LRU_queue_add_to_tail(QueueNode *node)
{
    QueueNode *newNode = malloc(sizeof(QueueNode));
    newNode->pcb = node->pcb;

    if (!LRU_head)
    {
        LRU_head = newNode;
        LRU_head->next = NULL;
    }
    else
    {
        QueueNode *cur = LRU_head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = newNode;
        cur->next->next = NULL;
    }
}

void print_LRU_queue()
{
    if (!LRU_head)
    {
        printf("LRU queue is empty\n");
        return;
    }
    QueueNode *cur = LRU_head;
    printf("LRU queue: \n");
    while (cur != NULL)
    {
        print_pcb(cur->pcb);
        cur = cur->next;
    }
}

QueueNode *LRU_queue_pop_head()
{
    QueueNode *tmp = LRU_head;
    if (LRU_head != NULL)
        LRU_head = LRU_head->next;
    return tmp;
}
