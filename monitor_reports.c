#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    if(sig == SIGINT) {
        keep_running = 0;
    }
    else if(sig == SIGUSR1){
        const char* message = "Received SIGUSR1: Report generation triggered.\n";
        write(STDOUT_FILENO, message, strlen(message));
    }
}

int main(){
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT,&sa,NULL) == -1){
        perror("Error registering SIGINT");
        return 1;
    }

    if(sigaction(SIGUSR1,&sa,NULL) == -1){
        perror("Error registering SIGUSR1");
        return 1;
    }

    pid_t mypid = getpid();

    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if(fd == -1){
        perror("Error creating .monitor_pid");
        return 1;
    }

    char pid_str[32];
    int len = snprintf(pid_str,sizeof(pid_str),"%d\n",mypid);
    write(fd,pid_str,len);
    close(fd);

    printf("Monitor process started with PID %d. Waiting for signals...\n", mypid);
    printf("Press Ctrl+C to stop the monitor.\n");

    while (keep_running){
        pause();   
    }

    const char* term_message = "\nSIGINT received: Terminating monitor process.\n";
    write(STDOUT_FILENO, term_message, strlen(term_message));

    if(unlink(".monitor_pid") == 0) {
        printf("PID file removed successfully.\n");
    } else {
        perror("Error removing PID file");
    }
    
    return 0;
}