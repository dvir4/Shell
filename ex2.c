#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

#define MAX_INPUT_SIZE 512
#define PWD_SIZE 512
#define PROMPT "> "
#define SEPARATOR " "
#define SPACE ' '


// Jobs struct.
typedef struct {
    int processID;
    char command[MAX_INPUT_SIZE];
} process;

/*
 * print current jobs.
 */
void printProcess(process *processList, int arraySize) {
    int i;
    for (i = 0; i < arraySize; i++) {
        printf("%d ", processList[i].processID);
        printf("%s\n", processList[i].command);
    }
};

/*
 * add process to jobs list.
 */
void addProcess(process *processList, int processID, char *command, int *arraySize) {
    int location = *arraySize;
    processList[location].processID = processID;
    strcpy(processList[location].command, command);
    // add array to struct.
    *arraySize += 1;
};

/*
 * find process with pid, delete the process, and reorganize.
 */
void deleteProcess(process *processList, int i, int *arraySize) {
    int arrSize = *arraySize;
    int j;
    for (j = i; j < arrSize; j++) {
        processList[j].processID = processList[j + 1].processID;
        strcpy(processList[j].command, processList[j + 1].command);
     }
     *arraySize -= 1;
};

/*
 * remove process from struct array.
 */
void removeProcess(process *processList, int *arrSize) {
    int size = *arrSize;
    int status = 0;
    int i;
    for (i = 0; i < *arrSize; i++) {
        // check if process still alive. in case not,delete it.
        if (waitpid(processList[i].processID, &status, WNOHANG)){
            deleteProcess(processList, i, arrSize);
	    --i;
	}
    }
}

/*
 * copy cd argument, including spacial cases.
 */
void getCDinput(char *line) {
    int i = 0;
    int j = 0;
    int k = 0;
    int includeFlag = 0;
    char arg[MAX_INPUT_SIZE];
    memset(arg, 0, sizeof(arg));
    // find line length.
    int lineSize = strlen(line);
    char* start = NULL;
    int flag = 0;
    // find first argument in string.
    for(k =0; k < lineSize;k++){
        if(line[k] == SPACE)
            flag = 1;
        if(line[k] != SPACE && flag){
            start = &line[k];
            break;
        }
    }
    // in case of no argument, return null;
    if(start == NULL){
        strcpy(line,arg);
        return;
    }
    int argumentSize = strlen(start);
    // in case of '"', include spaces in string.
    for (j = 0; j < argumentSize; j++) {
        if (start[j] == '"' && includeFlag == 0) {
            includeFlag = 1;
            continue;
        }
        if (includeFlag) {
            if (start[j] == '"') {
                includeFlag = 0;
                continue;
            }
        } else if (start[j] == SPACE)
                break;
        arg[i] = start[j];
        i++;
    }
    strcpy(line,arg);
}

/*
 * run build in commands, return 1 in case of using one of the commands.
 */
int builtInCommands(char **parse, process *processList, int *arrSize, char *prefPwd, char* cdCommand) {
    if (!(strcmp(parse[0], "exit"))) {
        exit(0);
    } else if (!(strcmp(parse[0], "cd"))) {
        char s[PWD_SIZE];
        // string that represent argument.
        char argument[MAX_INPUT_SIZE];
        strcpy(argument,cdCommand);
        getCDinput(argument);

        printf("%d\n", getpid());
        // check for spacial cases in cd command.
        char *curr = getcwd(s, PWD_SIZE);
        if (parse[1] == NULL || strcmp(parse[1], "~") == 0) {
            chdir(getenv("HOME"));
        } else if (strcmp(parse[1], "-") == 0) {
            if (strcmp(prefPwd, "notSet") != 0)
                chdir(prefPwd);
        } else {
            chdir(argument);
        }
        // save current location.
        if (curr) {
            memset(prefPwd, 0, sizeof(prefPwd));
            strcpy(prefPwd, curr);
        }
        return 1;
        // in case of jobs command, print all running process.
    } else if (!(strcmp(parse[0], "jobs"))) {
        removeProcess(processList, arrSize);
        printProcess(processList, *arrSize);
        return 1;
    }
    return 0;
}

void executeCommand(process *processList, char *commandStr, int *arrSize, char *prefPwd) {
    removeProcess(processList, arrSize);
    int sizeStr = strlen(commandStr);
    int isBackground = 0;
    // check for background sign '&'.
    if (commandStr[sizeStr - 1] == '&') {
        commandStr[sizeStr - 1] = '\0';
        isBackground = 1;
    }
    char command[MAX_INPUT_SIZE];
    strcpy(command, commandStr);

    // create array for execvp input.
    char *parse[MAX_INPUT_SIZE];
    memset(parse, 0, sizeof(parse));
    int numOfArgs = 0;
    char *temp;
    temp = strtok(commandStr, SEPARATOR);

    while (temp != NULL) {
        parse[numOfArgs++] = temp;
        temp = strtok(NULL, SEPARATOR);
    }

    if (builtInCommands(parse, processList, arrSize, prefPwd,command)) {
        return;
    }
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Error in system call\n");
    } else if (pid == 0) {
        // in case of failure, print error massage.
        if (execvp(parse[0], parse) < 0) {
            fprintf(stderr, "Error in system call\n");
        }
        exit(0);
    } else {
        printf("%d\n", pid);
        // add process to struct array.
        addProcess(processList, pid, command, arrSize);
        // in case of background process, keep running without waiting.
        if (!(isBackground))
            waitpid(pid, NULL, 0);
        return;
    }

}

/*
 * get user input.
 */
int getInput(char *buffer) {
    fgets(buffer, MAX_INPUT_SIZE, stdin);
    // remove '\n' from string.
    buffer[strlen(buffer) - 1] = '\0';
    if (buffer[0] != '\0')
        return 1;
    return 0;
}

int main() {
    char *parse[MAX_INPUT_SIZE];
    int arraySize = 0;
    process processList[MAX_INPUT_SIZE];
    memset(processList, 0, sizeof(processList));
    int gotInput;
    // initialize previous location of command path, notSet as defult.
    char prevPWD[PWD_SIZE] = "notSet";
    while (1) {
        char buffer[MAX_INPUT_SIZE];
        memset(buffer, 0, sizeof(buffer));
        printf(PROMPT);
        gotInput = getInput(buffer);
        if (gotInput) {
            executeCommand(processList, buffer, &arraySize, prevPWD);
        }
    }
}

