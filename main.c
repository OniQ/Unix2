#include "shell.h"

int main(int argc, char* argv[])
{
    char *line = NULL;
    size_t lineSize;
    init();
    int result;
    printPromt();
    fflush(stdout);
    while((result = getline(&line, &lineSize, stdin)) != EOF){
        processLine(line);
        printPromt();
    };
#ifdef DEBUG    
    printf("\nShell terminated");
#endif
    putchar('\n');
    exit(EXIT_SUCCESS);
}
