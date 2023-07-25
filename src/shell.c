#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "interpreter.h"
#include "shellmemory.h"
#include "pcb.h"
#include "kernel.h"
#include "shell.h"

#if defined(X) && defined(Y)
int framesize = X;
int varsize = Y;
#else
int framesize = 36;
int varsize = 36;
#endif

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

int main(int argc, char *argv[])
{
    printf("%s\n", "Shell version 1.2 Created January 2023\n");
    printf("Frame Store Size = %d; Variable Store Size = %d\n", framesize, varsize);

    char prompt = '$';              // Shell prompt
    char userInput[MAX_USER_INPUT]; // user's input stored here
    int errorCode = 0;              // zero means no error, default

    // init user input
    for (int i = 0; i < MAX_USER_INPUT; i++)
        userInput[i] = '\0';

    // init shell memory
    mem_init(framesize, varsize);

    // initialize backing store for paging
    create_backing_store();

    while (1)
    {

        if (isatty(fileno(stdin)))
            printf("%c ", prompt);

        char *str = fgets(userInput, MAX_USER_INPUT - 1, stdin);
        if (feof(stdin))
        {
            freopen("/dev/tty", "r", stdin);
        }

        if (strlen(userInput) > 0)
        {
            errorCode = parseInput(userInput);
            if (errorCode == -1)
                exit(99); // ignore all other errors
            memset(userInput, 0, sizeof(userInput));
        }
    }

    return 0;
}

int parseInput(char *ui)
{
    char tmp[200];
    char *words[100];
    int a = 0;
    int b;
    int w = 0; // wordID
    int errorCode;
    for (a = 0; ui[a] == ' ' && a < 1000; a++)
        ; // skip white spaces

    while (ui[a] != '\n' && ui[a] != '\0' && a < 1000 && a < strlen(ui))
    {
        while (ui[a] == ' ')
            a++;
        if (ui[a] == '\0')
            break;
        for (b = 0; ui[a] != ';' && ui[a] != '\0' && ui[a] != '\n' && ui[a] != ' ' && a < 1000; a++, b++)
            tmp[b] = ui[a];
        tmp[b] = '\0';
        if (strlen(tmp) == 0)
            continue;
        words[w] = strdup(tmp);
        if (ui[a] == ';')
        {
            w++;
            errorCode = interpreter(words, w);
            if (errorCode == -1)
                return errorCode;
            a++;
            w = 0;
            for (; ui[a] == ' ' && a < 1000; a++)
                ; // skip white spaces
            continue;
        }
        w++;
        a++;
    }
    errorCode = interpreter(words, w);
    return errorCode;
}