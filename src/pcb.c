#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"
#include "kernel.h"

int pid_counter = 1;

int generatePID()
{
    return pid_counter++;
}

// In this implementation, Pid is the same as file ID
PCB *makePCB(int start, int end)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start = start;
    newPCB->end = end;
    newPCB->job_length_score = 1 + end - start;
    newPCB->priority = false;
    return newPCB;
}

PCB *makePCBNew(char *filename)
{
    int fileLen = line_count(filename);

    PCB *newPCB = malloc(sizeof(PCB));

    newPCB->pid = generatePID();
    newPCB->PC = 0;
    newPCB->len = fileLen;
    newPCB->priority = false;
    newPCB->pageTable = malloc(fileLen * sizeof(int));
    newPCB->filename = strdup(filename);

    for (int i = 0; i < fileLen; i++)
    {
        newPCB->pageTable[i] = -1; // Initialize array elements to -1
    }

    return newPCB;
}

void print_pcb(PCB *pcb)
{
    if (pcb == NULL)
    {
        printf("PCB is NULL\n");
        return;
    }

    printf("PCB Details:\n");
    printf("\tPID: %d\n", pcb->pid);
    printf("\tPriority: %s\n", pcb->priority ? "true" : "false");
    printf("\tPC: %d\n", pcb->PC);
    printf("\tStart: %d\n", pcb->start);
    printf("\tEnd: %d\n", pcb->end);
    printf("\tJob Length Score: %d\n", pcb->job_length_score);
    printf("\tFile Length: %d\n", pcb->len);
    printf("\tFilename: %s\n", pcb->filename);

    printf("\tPage Table:\n");
    for (int i = 0; i < pcb->len; i++)
    {
        printf("\t\t[%d] = %d\n", i, pcb->pageTable[i]);
    }
}