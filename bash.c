#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/stat.h>

#include "list.h"
#include "jobControl.h"
#include <bits/waitflags.h>

#define GREEN "\x1b[32m\x1b[1m"
#define BLUE "\x1b[34m"
#define WHITE "\x1b[0m\x1b[0m"

#define EMPTY 1
#define NOT_EMPTY 0

void getdata(char* ,char**, char**, char*);
void readString(char*);
void redirect(node*);
void cdFunction(char**);
int defaultApplication(node*, job*, job*, list*);
int conv(node*, job*, job*, list*);
list* parsString(char*);
job* applicate(list*, job*);
void killFunction(char*, char*);
void fgFunction(job*, char**, int);
void jobsFunction(job*);
void quitFunction(job*, list*);


int main(int argc, char** argv){
    char inputString[256];
    job* jobs = NULL;
    char* user;
    char* pwd;
    char host[256];
    char buf[256];
    signal(SIGINT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    while(1){
        getdata(buf, &user, &pwd, host);
        write(STDOUT_FILENO, buf, strlen(buf));
        readString(inputString);
        if(feof(stdin)){
            break;
        }
        list* dataBase = parsString(inputString);
        node* ptr = dataBase->data;
        //readList(dataBase);
        jobs = applicate(dataBase, jobs); 
        checkJobs(jobs);
    }
    printf("EXITTING\n");
}

void getdata(char* buf, char** user, char** pwd, char* host){
    *pwd = getcwd(NULL, 256);
    *user = getenv("USER");
    gethostname(host, _SC_HOST_NAME_MAX);
    *buf = '\0';
    strcat(buf, GREEN);
    strcat(buf, *user);
    strcat(buf, "@");
    strcat(buf, host);
    strcat(buf, ":");
    strcat(buf, BLUE);
    strcat(buf, *pwd);
    strcat(buf, "$");
    strcat(buf, WHITE);
    strcat(buf, " ");
}

void readString(char* string){
    int i = 0;
    char s = '\0';
    while(1){
        s = getchar();
        if(s == '\n' || s == EOF){
            string[i] = '\0';
            break;
        }
        string[i] = s;
        i++;
    }
}

int internalApplicate(node* command, job* currJob, job* firstJob, list* list){
    if(strcmp(command->data[0], "quit") == 0){
        quitFunction(firstJob, list);
        return 0;
    }
    if(strcmp(command->data[0], "cd") == 0){
        cdFunction(command->data);
        return 0;
    }
    if(strcmp(command->data[0], "kill") == 0){
        killFunction(command->data[1], command->data[2]);
        return 0;
    }
    if(strcmp(command->data[0], "fg") == 0){
        fgFunction(firstJob, command->data, FG);
        return 0;
    }
    if(strcmp(command->data[0], "bg") == 0){
        fgFunction(firstJob, command->data, BG);
        return 0;
    }
    if(strcmp(command->data[0], "jobs") == 0){
        jobsFunction(firstJob);
        return 0;
    }
    return 1;
}

list* parsString(char* string){
    int charCount = 0;
    int probelFlag = 1;
    for(int i = 0; i < strlen(string) + 1; i++){
        if(string[i] == ' '){
            if(probelFlag == 1){
                continue;
            }else{
                probelFlag = 1;
                string[charCount] = string[i];
                charCount++;
            }
        }else{
            probelFlag = 0;
            string[charCount] = string[i];
            charCount++;
        }
    }
    if(string[charCount - 2] == ' '){
        string[charCount - 2] = '\0';
    }else{
        string[charCount] = '\0';
    }
    int wordCount = 0;// Сколько слов в команде
    int commandCharCount = 0;// Сколько символов в команде
    int flag = 0;
    int exit_flag = 0;// Нужен специально для выхода при встрече перенаправлений, т.к. все что идет после воспринимается как одно слово
    node* list = NULL;
    struct list* commandList = NULL;
    for(int z= 0; z < strlen(string) + 1; z++){
        if(string[z] == ' '){
            wordCount++;
        }
        if(string[z] == '&' && string[z+1] != '&'){
            //printf("Встретил & \n");
            flag = 1;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '&' && string[z+1] == '&'){
            //printf("Встретил && \n ");
            flag = 2;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '|' && string[z+1] == '|'){
            //printf("Встретил || \n");
            flag = 3;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == ';'){
            //printf("Встретил ; \n");
            flag = 4;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '\0'){
            //printf("Встретил конец \n");
            flag = 5;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '>' && string[z+1] != '>' && string[z+1] != '&'){
            //printf("Встретил > \n");
            flag = 6;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '>' && string[z+1] == '&'){
            //printf("Встретил >& \n");
            flag = 7;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '>' && string[z+1] == '>' && string[z+2] != '&'){
            //printf("Встретил >> \n");
            flag = 8;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '>' && string[z+1] == '>' && string[z+2] == '&'){
            //printf("Встретил >>& \n");
            flag = 9;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '<' && string[z+1] != '<'){
            //printf("Встретил < \n");
            flag = 10;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '|' && string[z+1] != '|' && string[z+1] != '&'){
            //printf("Встретил | \n");
            flag = 11;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        if(string[z] == '|' && string[z+1] == '&'){
            //printf("Встретил |& \n");
            flag = 12;
            if(string[z - 1] == ' '){
                wordCount--;
            }
            wordCount++;
            commandCharCount--;
        }
        commandCharCount++;
//Создание list 
        if(flag != 0){
            char** pars = calloc((wordCount + 1), sizeof(char*));
            if(pars == NULL){
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            pars[wordCount] = NULL;

            int startPos = z - commandCharCount;
            //printf("\n\tParser\nСтартуем от сюда: %d \n", startPos);
            //Считаем и записываем в pars[][]
            for(int i = 0; i < wordCount; i++){
                charCount = 0; //Сколько символов в одном слове
                int rewrite = startPos;
                //Разделим на слова
                for(int j = startPos; j < z; j++){
                    if(string[j] == ' '){
                        startPos = j+1;
                        break;
                    }
                    charCount++;
                }
                //printf("CharCount is %d \n", charCount);
                pars[i] = malloc((charCount + 1) * sizeof(char));
                if(pars[i] == NULL){
                    perror("malloc");
                }
                //Добавление в pars 
                for(int j = 0; j < charCount + 1; j++){
                    pars[i][j] = string[rewrite];
                    rewrite++;
                }
                pars[i][charCount] = '\0';
                //printf("pars result %s \n", pars[i]);
            }
            add_node(&list, pars, flag);
            if(flag == 2 || flag == 3 || flag == 7 || flag == 8 || flag == 12){
                z++;
            }
            if(flag == 9){
                z+=2;
            }
            flag = 0;
            commandCharCount = 0;
            wordCount = 0;
            while(string[z+1] == ' '){
                z++;
            }
        }
        if(exit_flag == 1){
            break;
        }
    }
    node* ptr = list;
    node* ptrPrev = ptr;
    node* save = ptr;
    while(ptr != NULL){
        if(ptr->flag == 1 || ptr->flag == 2 || ptr->flag == 3 || ptr->flag == 4 || ptr->flag == 5){
            save = ptr->next;
            ptr->next = NULL;
            add_list(&commandList, ptrPrev, ptr->flag);
            if(ptr->flag == 1 || ptr-> flag == 2|| ptr->flag == 3 || ptr->flag == 4){
                ptr->flag = 5;
            }
            ptr = save;
            ptrPrev = ptr;
            continue;
        }
        ptr = ptr->next;
    }
    return commandList;
}

int conv(node* command, job* currJob, job* firstJob, list* list){
    job* jobs = currJob;
    node* ptr = command;
    int count = 0;
    int flag;
    int empty = EMPTY;
    while(true){
        if(ptr->flag == 11 || ptr->flag == 12){
            count++;
            ptr = ptr->next;
            continue;
        }
        count++;
        break;
    }
    int fd[count - 1][2];// coздается массив файловых дискрипторов
    for(int i = 0; i < count; i++){
        flag = 0;
        if(i < count - 1){
            pipe(fd[i]);
        }
        if(command->flag == 12){
            flag = 1;
        }
        pid_t cpid = fork();
        if(cpid == 0){
            // Если не первая команда, перенаправить стандартный ввод из предыдущего канала
            if(i > 0){
                dup2(fd[i-1][0], STDIN_FILENO);
                close(fd[i-1][0]);
            }
            // Если не последняя команда, перенаправить стандартный вывод в следующий канал
            if(i < count - 1){
                if(flag){
                    dup2(fd[i][1], STDERR_FILENO);
                }
                dup2(fd[i][1], STDOUT_FILENO);
                close(fd[i][1]);
                close(fd[i][0]);
            }
            if(jobs->pgid == 0){
                jobs->pgid = getpid();
            }
            setpgid(getpid(), jobs->pgid);

            if(jobs->state == FG){
                tcsetpgrp(STDIN_FILENO, jobs->pgid);
            }

            signal(SIGINT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if(internalApplicate(command, jobs, firstJob, list) == 0){
                exit(EXIT_SUCCESS);
            }

            execvp(command->data[0], command->data);
            exit(EXIT_FAILURE);
        }else if(cpid < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(i < count - 1){
            close(fd[i][1]);
        }
        if(i > 0){
            close(fd[i-1][0]);
        }
        if(jobs->pgid == 0){
            jobs->pgid = cpid;
        }
        addProcess(&(jobs->conv), cpid, command->data, jobs->state, EXTERNAL);
        command = command->next;
    }
    int status;
    int exitFlag = 1;
    if(jobs->state == FG){
        while(1){
            pid_t wpid = waitpid(-(jobs->pgid), &status, WUNTRACED);
            if(wpid == -1){
                break;
            }
            if(WIFEXITED(status) == 0){
                exitFlag = 0;
            }
            checkStatus(status, jobs->conv, wpid);
        }
    }else{
        jobs->status = APPLICATING;
    }
    if(jobs->state == FG){
        tcsetpgrp(STDIN_FILENO, getpid());
    }
    if(exitFlag){
        return 0;
    }else{
        return 1;
    }
}

int defaultApplication(node* command, job* currJob, job* firstJob, list* list){
    if(internalApplicate(command, currJob, firstJob, list) == 0){
        //цикл удаления ненужной ячейки в struct jobs
        job* prev = firstJob;
        while(firstJob != NULL){
            if(firstJob->pgid == 0){
                prev->next = NULL;
                free(firstJob);
                break;
            }
            prev = firstJob;
            firstJob = firstJob->next;
        }
        return 0;
    }else{
        pid_t cpid = fork();
            if(cpid == 0){
                setpgid(getpid(), currJob->pgid);
                if(currJob->state == FG && currJob->pgid == 0){
                    tcsetpgrp(STDIN_FILENO, getpid());
                }
                signal(SIGINT, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGCONT, SIG_DFL);

                execvp(command->data[0], command->data);
                exit(EXIT_FAILURE);
            }else if(cpid < 0){
                perror("fork");
                exit(EXIT_FAILURE);
            }
        int status;
        addProcess(&(currJob->conv), cpid, command->data, currJob->state, EXTERNAL);
        if(currJob->pgid == 0){
            currJob->pgid = cpid;
        }
        if(currJob->state == FG){
            pid_t wpid = waitpid(cpid, &status, WUNTRACED);
            checkStatus(status, currJob->conv, wpid);
            tcsetpgrp(STDIN_FILENO, getpid());
            if(WIFEXITED(status) == 1 && WEXITSTATUS(status) == 0){
                return 0;
            }else{
                return 1;
            }
        }else{
            currJob->status = APPLICATING;
        }
    }
}

void redirect(node* command){
    int file;
    int filePrev;
    int i = 0;
    char c;
    while(command->next != NULL){
        switch (command->flag){
            case 5:
                return;
            case 6:
            case 7:
                file = open(command->next->data[0], O_CREAT | O_RDWR , 0666);
                break;
            case 8:
            case 9:
                file = open(command->next->data[0], O_APPEND | O_RDWR | O_CREAT, 0666);
                break;
            case 10:
                file = open(command->next->data[0], O_CREAT | O_RDWR , 0666); 
                break;
        }
        if(i == 0 && command->flag != 10){
            pid_t pid = fork();
            if(pid == 0){
                dup2(file, STDOUT_FILENO);
                if(command->flag == 7 || command->flag == 9){
                    dup2(file, STDERR_FILENO);
                }
                execvp(command->data[0], command->data);
                exit(EXIT_FAILURE);
            }else if(pid < 0){
                perror("fork");
                exit(EXIT_FAILURE);
            }else{
                wait(NULL);
            }
            command = command->next;
            i++;
            continue;
        }
        filePrev = open(command->data[0], O_RDWR | O_CREAT, 0666);
        struct stat filestats;
        if(command->flag == 10){
            stat(command->next->data[0], &filestats);
            for(int i = 0; i < filestats.st_size; i++){
                read(file, &c, sizeof(char));
                write(filePrev, &c, sizeof(char));
            }
        }else{
            stat(command->data[0], &filestats);
            for(int i = 0; i < filestats.st_size; i++){
                read(filePrev, &c, sizeof(char));
                write(file, &c, sizeof(char));
            }
        }
        command = command->next;
    }
}

int pipeApplicate(node* commands, job* currJob, job* firstJob, list* list){
    node* ptr = commands;
    while(ptr != NULL){
        switch(ptr->flag){
            case 5:
                if(defaultApplication(ptr, currJob, firstJob, list) == 1){
                    return 1;
                }
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
                redirect(ptr);
                job* prev = firstJob;
                while(firstJob != NULL){
                    if(firstJob->pgid == 0){
                        prev->next = NULL;
                        free(firstJob);
                        break;
                    }
                    prev = firstJob;
                    firstJob = firstJob->next;
                }
                return 0;
            case 11:
                if(conv(ptr, currJob, firstJob, list) == 1){
                    return 1;
                }else{
                    return 0;
                }
            case 12:
                if(conv(ptr, currJob, firstJob, list) == 1){;
                   return 1;
                }else{
                    return 0;
                }
        }
        ptr = ptr->next;
    }
    return 0;
}

job* applicate(list* commands, job* jobs){
    job* save = NULL;
    job* rtrn = NULL;
    int jobs_status = EMPTY;
    if(jobs != NULL){
        rtrn = jobs;
        jobs_status = NOT_EMPTY;
    }
    int flag = 1;
    while(commands != NULL){
        if(jobs_status == NOT_EMPTY){
            while(jobs->next != NULL){
                jobs = jobs->next;
            }
            jobs->next = calloc(1, sizeof(struct job));
            jobs = jobs->next;
        }
        if(jobs_status == EMPTY){
            jobs = calloc(1, sizeof(struct job));
            rtrn = jobs;
            jobs_status = NOT_EMPTY;
        }
        if(flag){
            save = jobs;
            flag = 0;
        }
        jobs->conv = NULL;
        jobs->next = NULL;
        switch(commands->flag){
            case 1:
                (jobs)->state = BG;
                pipeApplicate(commands->data, jobs, rtrn, commands);
                break;
            case 2:
                (jobs)->state = FG;
                if(pipeApplicate(commands->data, jobs, rtrn, commands) == 1){
                    return rtrn;
                }
                break;
            case 3:
                (jobs)->state = FG;
                if(pipeApplicate(commands->data, jobs, rtrn, commands) == 0){
                    return rtrn;
                }
                break;
            case 4:
            case 5:
                (jobs)->state = FG;
                pipeApplicate(commands->data, jobs, rtrn, commands);
                break;
        }
        commands = commands->next;
    }
    return rtrn;
}

void cdFunction(char** command){
    char* path = calloc(1, sizeof(char));
    int size = 1;
    for(int i = 1; command[i] != NULL; i++){
        for(int j = 0; command[i][j] != '\0'; j++){
            path = realloc(path, size * sizeof(char));
            path[size - 1] = command[i][j];
            size++;
        }
    }
    path = realloc(path, size*sizeof(char));
    path[size - 1] = '\0';
    chdir(path);
    free(path);
}

void killFunction(char* signal, char* pid){
    if(signal == NULL || pid == NULL || signal[0] != '-'){
        printf("Uncorrect command");
        return;
    }
    int pidInt;
    int sigInt = 0;
    for(int i = 0; i < 24; i++){
        if(strcmp(signal, signals[i])==0){
            sigInt = i + 1;
            break;
        }
    }
    if(sigInt > 25){
        printf("SIGNAL WAS NOT FOUND");
        return;
    }
    if(pid[0] == '-'){
        pidInt = atoi(&(pid[1]));
        printf("PidInt:%d\n", pidInt);
        printf("SigInt:%d\n", sigInt);
        if(kill(-(pidInt), sigInt) == -1){
            printf("Signal wasn't sent");
        }
    }else{
        pidInt = atoi(pid);
        if(kill(pidInt, sigInt) == -1){
            printf("Signal wasn't sent");
        }
    }
}

void fgFunction(job* jobs, char** command, int flag){
    pid_t wpid = 1;
    int status;
    int count;
    int i = 1;
    if(command[1] == NULL){
        count = 1;
    }else{
        count = atoi(command[1]);
    }
    while(jobs != NULL){
        if(jobs->conv != NULL){
            if(jobs->status != EXITED){
                if(i < count){
                    i++;
                    jobs= jobs->next;
                    continue;
                }
                if(flag == FG){
                    tcsetpgrp(STDIN_FILENO, jobs->pgid);
                }
                kill(-(jobs->pgid), SIGCONT);
                process* ptr = jobs->conv;
                if(flag == FG){
                    while(1){
                        wpid = waitpid(-(jobs->pgid), &status, WUNTRACED);
                        checkStatus(status, jobs->conv, wpid);
                        if(wpid < 0){
                            break;
                        }else if(is_exited(jobs)){
                            jobs->status = EXITED;
                            break;
                        }else if(is_stopped(jobs)){
                            jobs->status = STOPPED;
                            break;
                        }
                    }
                }
                tcsetpgrp(STDIN_FILENO, getpid());
                return;
            }
        }
        jobs = jobs->next;
    }
}

void jobsFunction(job* jobs){
    int count = 0;
    int flag;
    while(jobs != NULL){
        /*flag = 0;
        if(jobs->conv == NULL){
            jobs = jobs->next;
            continue;
        }
        process* ptr = jobs->conv;
        while(ptr != NULL){
            if(ptr->status != EXITED){
                flag = 1;
                break;
            }
            ptr = ptr->next;
        }
        if(flag == 1){
            count++;
            printf(BLUE"[%d] PGID: %d STATUS: ", count, jobs->pgid);
            if(ptr->status == APPLICATING){
                printf("APPLICATING\n"WHITE);
            }else if(ptr->status == STOPPED){
                printf("STOPPED\n"WHITE);
            }
        }*/
        if(jobs->status == APPLICATING){
            printf(BLUE"[%d] PGID: %d STATUS: ", count, jobs->pgid);
            printf("APPLICATING\n"WHITE);
        }else if(jobs->status == STOPPED){
            printf(BLUE"[%d] PGID: %d STATUS: ", count, jobs->pgid);
            printf("STOPPED\n"WHITE);
        }
        jobs = jobs->next;
    }
}

void quitFunction(job* jobs, list* list){
    job* nextJob;
    process* nextProcess;
    while(jobs != NULL){
        nextJob = jobs->next;
        process* process = jobs->conv;
        while(process != NULL){
            nextProcess = process->next;
            kill(process->pid, SIGHUP);
            free(process);
            process = nextProcess;
        }
        free(jobs);
        jobs = nextJob;
    }
    struct list* nextList;
    node* nextNode;
    while(list != NULL){
        nextList = list->next;
        node* node = list->data;
        while(node != NULL){
            nextNode = node->next;
            char** data = node->data;
            char* nextData;
            int i = 0;
            while(data[i] != NULL){
                nextData = data[i + 1];
                free(data[i]);
                data[i] = nextData;
                i++;
            }
            free(node);
            node = nextNode;
        }
        free(list);
        list = nextList;
    }
    exit(0);
}

int is_exited(job* jobs){
    process* ptr = jobs->conv;
    while(ptr != NULL){
        if(ptr->status != EXITED){
            return 0;
        }
        ptr = ptr->next;
    }
    return 1;
}

int is_stopped(job* jobs){
    process* ptr = jobs->conv;
    int flag = 0;
    while(ptr != NULL){
        if(ptr->status == APPLICATING){
            flag = 0;
            break;
        }else if(ptr->status == STOPPED){
            flag = 1;
        }
        ptr = ptr->next;
    }
    return flag;
}