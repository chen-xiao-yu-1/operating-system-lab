#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
enum{
    judges=0,
    immgrants=1,
    specators=2,
};
const char* role_names[] = {
    "Judge",
    "Immigrant",
    "Spectator"
};
int NUM=10; 
sem_t judge;
sem_t mutex;
sem_t check_judge;
sem_t swear_sem;   
sem_t check_in;
int to_check; 

int judgenum=0;
int immgrantnum=0;
int specatornum=0;//ID

int judge_present=0;
int waiting_immigrants[100];  
int waiting_count = 0;       
int MAX_THREAD = 100;       

void leave(int specifiers,int id){
    if(specifiers == judges){
        printf("%s #%d leave\n", role_names[specifiers], id);
        sem_post(&judge);
    }
    else if(specifiers == immgrants){
        sem_wait(&mutex);
        if(judge_present) {
            sem_post(&mutex);
            sem_wait(&judge); 
            printf("%s #%d leave\n", role_names[specifiers], id); 
            sem_post(&judge);  
        }
        else {
            sem_post(&mutex);
            printf("%s #%d leave\n", role_names[specifiers], id); 
        }
    
    }
    else {
        printf("%s #%d leave\n", role_names[specifiers], id);
    }
    
}
void checkIn(int id){
    sem_wait(&check_in);
    to_check--;
    sleep(rand()%NUM); // Simulate time taken to check in
    printf("Immigrant #%d checkIn\n", id);
    if(to_check == 0){
        sem_post(&check_judge);//all check in then judge can confirm
    }
    sem_post(&check_in);
}

void enter(int specifiers,int id){
    if(specifiers == judges){
        sem_wait(&judge);
        judge_present = 1;  
        printf("%s #%d enter\n", role_names[specifiers], id);
    }
    else if(specifiers == immgrants){
        sem_wait(&judge);
        to_check++;
        if(to_check == 1){
            sem_post(&check_judge);
        }
        printf("%s #%d enter\n", role_names[specifiers], id);
        sem_post(&judge);
    }
    else{
        sem_wait(&judge);
        printf("%s #%d enter\n", role_names[specifiers], id);
        sem_post(&judge);
    }
}


void sitdown(int id){
    sem_wait(&mutex);
    printf("Immigrant #%d sitDown\n", id);
    waiting_immigrants[waiting_count++]=id;
    sem_post(&mutex);
}

void confirm_immigrant(int judge_id, int *waiting_immigrants){
    sem_wait(&check_judge);
    sem_wait(&mutex);

    for(int i=0;i<waiting_count;i++){
    printf("Judge #%d confirm the immigrant #%d\n", judge_id, waiting_immigrants[i] );
    sem_post(&swear_sem);
}   
    waiting_count = 0; 
    sem_post(&mutex);
    sem_post(&check_judge);
}


void *func_spectator(){
   int id=specatornum++;
    enter(specators, id);
    sleep(rand()%NUM); 
    printf("Spectator #%d spectate\n", id);
    sleep(rand()%NUM); 
    leave(specators, id);
    return NULL;
}
void *func_immigrant() {
    int id = immgrantnum++;
    enter(immgrants, id);
    sleep(rand() % NUM);
    checkIn(id);
    sleep(rand() % NUM);
    sitdown(id);
    sem_wait(&swear_sem);
    printf("Immigrant #%d swear\n", id) ;       
    sleep(rand() % NUM);
    printf("Immigrant #%d getCertificate\n", id);
    leave(immgrants, id);
    return NULL;
}
void *func_judge(){
    int id = judgenum++;
    enter(judges, id);
    sleep(rand()%NUM);
    confirm_immigrant(id, waiting_immigrants);
    sleep(rand()%NUM);
    leave(judges, id);
    return NULL;
}
int Nthread = 0;
pthread_t *threads;
void new_thread(int tid)
{
	switch(rand()%3)
	{
		case 0:pthread_create(&threads[tid], NULL, func_immigrant, NULL);break;
		case 1:pthread_create(&threads[tid], NULL, func_spectator, NULL);break;
		case 2:pthread_create(&threads[tid], NULL, func_judge, NULL);break;
	}
}
int main(int argc, char **argv){
	threads = malloc(MAX_THREAD *sizeof(pthread_t));
	sem_init(&judge, 0, 1);
	sem_init(&mutex, 0, 1);
	sem_init(&check_in, 0, 1);
	sem_init(&check_judge, 0, 1);
    sem_init(&swear_sem, 0, 0);
	while(1)
	{   if(Nthread >= MAX_THREAD) {
            break; 
        }
		new_thread(Nthread);
		++Nthread;
        sleep(rand()%NUM); 
	}
    free(threads);
	return 0;
}