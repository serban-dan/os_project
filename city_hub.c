#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD_LEN 256
#define MAX_ARGS 20

//start_monitor command
void start_monitor(){
    pid_t hub_mon_pid = fork();

    if(hub_mon_pid < 0){
        perror("Error: Failed to fork hub_mon");
        return;
    }

    //hub_mon child process
    if(hub_mon_pid == 0) {
        int pipefd[2];

        if(pipe(pipefd) == -1){
            perror("Error creating pipe");
            _exit(EXIT_FAILURE);
        }

        pid_t monitor_pid = fork();

        if(monitor_pid < 0){
            perror("Error: Failed to fork monitor_reports");
            _exit(EXIT_FAILURE);
        }

        if(monitor_pid == 0){
            //inside the monitor child process
            
            close(pipefd[0]); //we close this as the monitor does not read from the pipe

            dup2(pipefd[1],STDOUT_FILENO);//dup stdout to WRITE pipe
            close(pipefd[1]); //close the original pipe

            execlp("./monitor_reports","./monitor_reports",(char *)NULL);

            perror("Failed to execute monitor_reports");
            _exit(EXIT_FAILURE);
        }
        else {
            //inside the hub_mon child process

            close(pipefd[1]); //we do not need write

            char buffer[256];
            ssize_t bytes_read;

            //read from the pipe until EOF
            while((bytes_read = read(pipefd[0],buffer,sizeof(buffer)-1)) > 0){
                buffer[bytes_read] = '\0';

                printf("\n>>> HUB_MON MSG: %s",buffer);

                //Detect error or term msg and break
                if(strstr(buffer,"[TERMINATED]") != NULL || strstr(buffer,"[ERROR]") != NULL){
                    printf(">>> HUB_MON ALERT: The monitor process has terminated.\n");
                    break;
                }

                printf("city_hub> ");
                fflush(stdout);
            }

            close(pipefd[0]);
            waitpid(monitor_pid, NULL, 0);
            _exit(EXIT_SUCCESS);
        }
    }
    printf("Monitor started in background (hub_mon PID: %d).\n",hub_mon_pid);
}

//calculate_scores command
void calculate_scores(int arg_count, char **args){
    if(arg_count < 2){
        printf("Usage: calculate_scores <district1> <district2> ...\n");
        return;
    }

    int num_districts = arg_count - 1;
    int pipes[MAX_ARGS][2];
    pid_t pids[MAX_ARGS];
    for(int i = 0; i < MAX_ARGS; i++) pids[i] = -1;

    printf("\nGenerating Combined Workload Report...\n");
    printf("========================================\n");

    for(int i = 0; i < num_districts; i++){
        if(pipe(pipes[i]) == -1) {
            perror("Error creating pipe");
            continue; //if a pipe fails just try for other districts
        }

        pids[i] = fork();

        if(pids[i] < 0){
            perror("Error forking scorer");
        
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        else if(pids[i] == 0){
            //inside the child scorer process

            close(pipes[i][0]); //close read

            dup2(pipes[i][1],STDOUT_FILENO); //copy write file
            close(pipes[i][1]); //close original write file

            //execute scorer for district i
            execlp("./scorer", "./scorer", args[i + 1], (char *)NULL);

            perror("Failed to execute scorer");
            _exit(EXIT_FAILURE);
        }
        else{
            //parent process
            close(pipes[i][1]); //close the write pipe for each process
        }
    }

    for(int i = 0; i < num_districts; i++){
        if(pids[i] > 0) {
            char buffer[1024];
            ssize_t bytes_read;

            //read from pipe and print in terminal
            while((bytes_read = read(pipes[i][0], buffer, sizeof(buffer) -1)) > 0){
                buffer[bytes_read] = '\0';
                printf("%s", buffer);
            }

             
            close(pipes[i][0]); //close read pipes in the parent
            waitpid(pids[i], NULL,0); //clean up zombies
        }

    }
    printf("=======================================\n\n");
}
    
int main(){
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS];

    printf("\n=== Welcome to the City Hub ===\n");
    printf("\nAvailable commands:\n");
    printf("  start_monitor\n");
    printf("  calculate_scores\n");
    
    while(1){
        //wait for any child without freezing the terminal
        waitpid(-1,NULL,WNOHANG);

        printf("city_hub> ");
        if(fgets(input,sizeof(input),stdin) == NULL){
            break; //Exit for CTRL + D
        }

        input[strcspn(input, "\n")] = 0; //remove newline

        if(strlen(input) == 0) continue; //skip no input

        //tokenize the string
        int arg_count = 0;
        char *token = strtok(input," ");
        while(token != NULL && arg_count < MAX_ARGS - 1){
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if(arg_count == 0){
            continue;
        }

        //commands
        if(strcmp(args[0],"exit") == 0){
            int check_fd = open(".monitor_pid", O_RDONLY);
            if (check_fd != -1){
                char buf[32] = {0};
                read(check_fd, buf, sizeof(buf)-1);
                close(check_fd);

                pid_t existing_pid = atoi(buf);
                if(existing_pid > 0){
                    printf("Shutting down background monitor (PID: %d)...\n",existing_pid);
                    kill(existing_pid,SIGINT);
                    sleep(1);
                }
            }
            break;
        }
        else if(strcmp(args[0],"start_monitor") == 0){
            start_monitor();
        }
        else if(strcmp(args[0], "calculate_scores") == 0){
            calculate_scores(arg_count,args);
        }
        else{
            printf("Unknown command: %s\n", args[0]);
        }
    }

    printf("Exiting City Hub. Goodbye!\n");
    return 0;
}