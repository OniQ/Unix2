#include "shell.h"

void printPromt()
{
    time_t rawtime;
    struct tm *timeinfo;
    char timeBuffer[10];
    char pathBuffer[PATH_MAX];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%H.%M.%S", timeinfo);
    char *currentPath = getcwd(pathBuffer, sizeof(pathBuffer));
    printf("%s %s:", currentPath, timeBuffer);
}

void welcome(){
    printf("Welcome! Shell is ready to process commands.\n");
}

void init()
{
    signal(SIGCHLD, SIG_IGN);
    welcome();
}

char *trimwhitespace(char *str)
{
    char *end;
    while (isspace(*str))
        str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;
    *(end + 1) = 0;
    return str;
}

char **splitLine(char *line, const char *delimiter)
{
    char **lineWords = NULL;
    char *pch = strtok(line, delimiter);
    int wordCounter = 0;
    lineWords = malloc(sizeof(char *));
    while (pch != NULL) {
        pch = trimwhitespace(pch);
        lineWords =
            (void *) realloc(lineWords,
                             sizeof(char *) *wordCounter +
                             sizeof(char *));
        lineWords[wordCounter] = (char *) malloc(strlen(pch) + 1);
        lineWords[wordCounter] = pch;
        wordCounter++;
        pch = strtok(NULL, delimiter);
    };
    lineWords[wordCounter] = NULL;
    return lineWords;
}

// run the part of the pipeline
void runpipe(int *pfd, int direction, int *pfd_next, char **cmd)
{
    pid_t pid;
    switch (pid = fork()) {
    case 0: // child 
        dup2(pfd[direction], direction);  // bind std dsc to pipe
        close(pfd[!direction]);           // close unused pipe
        close(pfd[direction]);
        // sets pipe for next command if needed
        if (pfd_next) {
            dup2(pfd_next[1], 1);
            close(pfd_next[0]);
            close(pfd_next[1]);
        }
        execvp(cmd[0], cmd);    // execute command
        perror(cmd[0]);     // command failed
        _exit(EXIT_FAILURE);
    case -1:
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }
}

void processPipes(char **pipedCommands)
{
    int pfd[2];
    int pfd_next[2];
    int cmdNr = 0;
    char **command = splitLine(pipedCommands[cmdNr++], " ");
    pipe(pfd);
    runpipe(pfd, 1, NULL, command);
    command = splitLine(pipedCommands[cmdNr++], " ");
    while (pipedCommands[cmdNr] != NULL) {  // check if last command 
        pipe(pfd_next);
        runpipe(pfd, 0, pfd_next, command);
        close(pfd[0]);
        close(pfd[1]);
        pfd[0] = pfd_next[0];
        pfd[1] = pfd_next[1];
        command = splitLine(pipedCommands[cmdNr++], " ");
    };
    runpipe(pfd, 0, NULL, command);
    close(pfd[0]);
    close(pfd[1]);

    // pick up all the dead children
    int pid, status;
    while ((pid = wait(&status)) != -1)
        fprintf(stderr, "process %d exits with %d\n", pid,
            WEXITSTATUS(status));
}

void processCommand(char *line)
{
    char **commandArgs = splitLine(line, " ");
    pid_t pid = fork();
    if (pid == 0) {
        int status = execvp(commandArgs[0], commandArgs);
        fprintf(stderr, "%s failed to execute (%d)\n", commandArgs[0],
            status);
        _exit(status);
    }
    wait(NULL);
}

void checkForPipes(char *line)
{
    char **pipedCommands = splitLine(line, "|");
    
#ifdef DEBUG
    char *command;
    int i = 0;
    do {
        command = pipedCommands[i++];
        if (command != NULL)
            printf("{%s}\n", command);
        }
    while (command != NULL);
#endif
    
    if (pipedCommands != NULL) {
        if (pipedCommands[1] != NULL)
            processPipes(pipedCommands);
        else if (pipedCommands[0] != NULL)
            processCommand(pipedCommands[0]);
        else
            fprintf(stderr, "Failed to split line");
    }
}

int processInner(char *line)
{
    char *tempStr = strdup(line);
    char **args = splitLine(tempStr, " ");
    if (strcmp(args[0], "exit") == 0) {
#ifdef DEBUG 
        printf("\nExit shell");
#endif
        exit(EXIT_SUCCESS);
    } else if (strcmp(args[0], "help") == 0) {
        printf("Insert command to execute\n");
        printf("Chain processes with | to link them with pipe\n");
        printf("Press ^d or type \"exit\" to terminate program\n");
        return 1;
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL)
            chdir(getenv("HOME"));
        else if (chdir(args[1]) == -1)
            printf("cd: %s: No such directory\n", args[1]);
            return 1;
    } else if (strcmp(args[0], "\n") == 0){
        return 1;
    }
    return 0;
}

void removeLastSpace(char *line)
{
    if (line != NULL && strlen(line) > 1) {
        int last = strlen(line) - 1;
        if (isspace(line[last]))
            line[last] = '\0';
    }
}

void processLine(char *line)
{
    if (line == NULL)
        return;
    //remove new line symbol from buffer
    removeLastSpace(line);  
    //separate commands
    char **commands = splitLine(line, ";");
    while (*commands != NULL) {
        //if not build-in command try process standart
        if (!processInner(*commands)) {
            checkForPipes(*commands);
        }
        commands++;
    }
}
