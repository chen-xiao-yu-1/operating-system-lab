#include "header.h"
#define max 128
bool flag = false;
int parseLine(char *line, char *command_array[],  int *pipenum) {
    char *p;
    int count = 0;
    
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
        line[--len] = '\0';
    }
    
    p = strtok(line, " \t");
    while (p != NULL) {
        if(strcmp(p, "|") == 0) {
            command_array[count] = "|"; 
            (*pipenum)++;
            count++;
        } else {
            command_array[count] = p;
           
            count++;
        }
        p = strtok(NULL, " \t");
    }
    
    command_array[count] = NULL;
    
    return count;
}

void execute_cd(char *command[],int client){
	if(command[1]==NULL){
	const char* home=getenv("HOME");
	int k=strlen("Chdir failed.\n");
	if(home==NULL){
		send(client,"Chdir failed.\n",k,0);
		return;
	}
	int ret=chdir(home);
	if(ret==-1){
		send(client,"Chdir failed.\n",k,0);
		return;}
	}
	else
	{if(chdir(command[1])==-1){
		int k=strlen("Invalid Path.\n");
		send(client,"Invalid Path.\n",k,0);
		return;}
	char cwd[1024];
	char *p=getcwd(cwd,sizeof(cwd));
	if(p==NULL){
		int k=strlen("Failed to get path.\n");
                send(client,"Failed to get path.\n",k,0);
		return;
	}
	else{ char response[1024];
		snprintf(response,sizeof(response),"Your path now:\n%s\n",p);
		send(client,response,strlen(response),0);
		return;
	}
	}
}

void execute_pipe(char **args, int start, int end, int client_sockfd) {
    int pipe_pos = -1;
    char *command_array[max];
    for(int i = start; i < end; i++) {
        if(strcmp(args[i], "|") == 0) {
            pipe_pos = i;
            command_array[i-start] = NULL;
            break;
        }
        else{
            command_array[i-start] = args[i];
        }
    }

    if(pipe_pos == -1) {
        pid_t pid = fork();
        if(pid == 0) {
            dup2(client_sockfd, STDOUT_FILENO);
            dup2(client_sockfd, STDERR_FILENO);
            command_array[end-start] = NULL;
            execvp(command_array[0], command_array);
            perror("execvp failed");
            exit(1);
        } else {
            wait(NULL);
            return;
        }
    }

    int pipefd[2];
    if(pipe(pipefd) == -1) {
        perror("pipe failed");
        return;
    }
    pid_t pid1 = fork();
    if(pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        args[pipe_pos] = NULL; 
        execvp(command_array[0], command_array);
        perror("execvp failed");
        exit(1);
    }

    else{
	wait(NULL);
    pid_t pid2 = fork();
    if(pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        
        execute_pipe(args, pipe_pos + 1, end, client_sockfd);
        exit(1);
    }

	else{
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
	return;}
    }
}

int main(int argc,char *argv[]) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(2);
    }
    int port = atoi(argv[1]);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    listen(sockfd, 5);
    printf("Accepting connections...\n");
    
    int client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

	while(1){
        client_sockfd = accept(sockfd, (struct sockaddr *) &client_addr,&len);
	if(client_sockfd<0){
		perror("accept failed");
		continue;}

    // 添加新客户端连接提示
    printf("New client connected from %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), 
           ntohs(client_addr.sin_port));

	pid_t ForkPID;
	ForkPID=fork();
	switch(ForkPID){
		case -1:
		printf("Error:Failed to fork.\n");
		close(client_sockfd);
		break;
		case 0:
		close(sockfd);
		while(1){
       		char buf[1024];
		char *args[max]; 
	        int nread = read(client_sockfd, buf, 1024);
		buf[nread]='\0';

        // 添加命令执行提示
        printf("Client %s:%d executed command: %s\n", 
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buf);

		int pipenum=0;
		char **cmd_args = args;
		int num = parseLine(buf, cmd_args,  &pipenum);

		if(strcmp(args[0], "exit") == 0) {
		    printf("A client is disconnected\n");
		    close(client_sockfd);
		    exit(0);
		}

		if(strcmp(args[0], "cd") == 0) {
		    execute_cd(args, client_sockfd);
		}
		else if(pipenum!=0) {
            execute_pipe(args, 0, num, client_sockfd);
        }

	    else{
			int pipefd[2];
    		pid_t pid;
    
    	if (pipe(pipefd) == -1) {
        char error_msg[] = "Error: pipe failed.\n";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return 1;   
    }
    
    pid = fork();
    if (pid == 0) {  
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        if (execvp(args[0],args) == -1) { 
            char error_msg[1024];
            snprintf(error_msg, sizeof(error_msg), 
                    "Error: command not found: '%s'\n", args[0]);
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);  
        }
    } else if (pid > 0) {  
        close(pipefd[1]);
        char buffer[1024];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[count] = '\0';
            send(client_sockfd, buffer, count, 0);
        }
        close(pipefd[0]);
        int status;
        wait(&status);
        
        // 检查子进程是否正常退出
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            char error_msg[] = "Command execution failed.\n";
            send(client_sockfd, error_msg, strlen(error_msg), 0);
        }
    }
    }   
    }   

    default:
        close(client_sockfd);
        break;
    }   
      
    }  
    return 0;
}
