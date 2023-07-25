#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
/*
 * Struct:  PCB
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 * len: Amount of instructions to be executed
 * pageTable: Stores the location of frames holding the the instructions to execute 
 * filename: name of file from which the script was loaded from
 */
typedef struct
{
    bool priority;
    int pid;
    int PC;
    int start;
    int end;
    int job_length_score;
    int len;
    int *pageTable;
    char *filename;
} PCB;

int generatePID();
PCB *makePCB(int start, int end);
PCB *makePCBNew(char *filename);
void print_pcb(PCB *pcb);
#endif