#include <sys/types.h>
#include <errno.h>

typedef enum{
    INTERNAL, EXTERNAL
}Type;

typedef enum{
    FG, BG 
}State;

typedef enum{
    APPLICATING, STOPPED, EXITED
}Status;

char* signals[] = {"-SIGHUP", "-SIGINT", "-SIGQUIT", "-SIGILL", "-SIGTRAP","-SIGABRT", "-SIGBUS", "-SIGFPE", "-SIGKILL", "-SIGUSR1", "-SIGSEGV", "-SIGUSR2", "-SIGPIPE", "-SIGALRM", "-SIGTERM", "-SIGSTKFLT", "-SIGCHLD", "-SIGCONT", "-SIGSTOP", "-SIGTSTP", "-SIGTTIN", "-SIGTTOU", "-SIGURG", "-SIGXCPU"};

typedef struct process{
    char** argv;
    pid_t pid;
    State state;
    Status status;
    Type type;
    struct process* next;
}process;

typedef struct job{
    process* conv;
    pid_t pgid;
    State state;
    Status status;
    struct job* next;
}job;

int is_stopped(job*);
int is_exited(job*);

void checkStatus(int status, process* process, pid_t pid){
    while(process != NULL){
        if(process->pid == pid){
            break;
        }
        process = process->next;
    }
    if(process == NULL){
        return;
    }
    if(WIFSIGNALED(status)){
        //printf("\nОтловил убийство\n");
        process->status = EXITED;
        return;
    }else if(WIFEXITED(status)){
        //printf("\nОтловил окончание процесса\n");
        process->status = EXITED;
        return;
    }else if(WIFSTOPPED(status)){
        //printf("\nОтловил остоновку процесса\n");
        process->status = STOPPED;
        return;
    }else if(WIFCONTINUED(status)){
        //printf("\nОтловил продолжение процесса\n");
        process->status = APPLICATING;
        return;
    }
}

void checkJobs(job* jobs){
    if(jobs == NULL){
        return;
    }
    int status;
    pid_t wpid;
    while(1){
        wpid = waitpid(-(jobs->pgid), &status, WUNTRACED | WCONTINUED | WNOHANG);
        if((wpid < 0 && errno == ECHILD)|| wpid == 0){
            break;
        }
        checkStatus(status, jobs->conv, wpid);
    }
    if(is_exited(jobs)){
        jobs->status = EXITED;
    }else if(is_stopped(jobs)){
        jobs->status = STOPPED;
    }else{
        jobs->status = APPLICATING;
    }
    checkJobs(jobs->next);
}

void addProcess(struct process** ptr, pid_t pid, char** command, State state, Type type){
    if(*ptr == NULL){
        *ptr = calloc(1, sizeof(struct process));
        (*ptr)->argv = command;
        (*ptr)->pid = pid;
        (*ptr)->state = state;
        (*ptr)->type = type;
        (*ptr)->next = NULL;
        return;
    }
    addProcess(&((*ptr)->next), pid, command, state, type);
}