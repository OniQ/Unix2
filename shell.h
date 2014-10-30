#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <linux/limits.h>

void printPromt();
void init();
void processLine(char* line);

#endif
