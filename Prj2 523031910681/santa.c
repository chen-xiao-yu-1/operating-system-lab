#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>     
#include <unistd.h>     
#include <time.h>       
#include <semaphore.h>

#define REINDEER_COUNT 9
#define ELF_COUNT 15
#define VACATION_TIME 20  
#define WORK_TIME 15  

int reindeer_num = 0;
int elf_num = 0;
int num=10; 
int MAX_THREAD = 100; 
int santa_state = 0;  
int elves_waiting;  
sem_t mutex;
sem_t santa_sleep;    
sem_t reindeer_wait;  
sem_t elves_wait;     
sem_t elf_queue;         
sem_t reindeer_done;  
sem_t elves_done;     
sem_t reindeer_barrier; 
int count = 0;
int program_finished = 0;  
void *func_santa() {
    while(1) {
        if(count == 30) {
            sem_wait(&mutex);
            program_finished = 1; 
            
        
            for(int i = 0; i < ELF_COUNT; i++) {
                sem_post(&elves_wait);
                sem_post(&elf_queue);
            }
            
        
            for(int i = 0; i < REINDEER_COUNT; i++) {
                sem_post(&reindeer_wait);
                sem_post(&reindeer_barrier);
            }
            
            sem_post(&mutex);
            break;
        }
        sem_wait(&santa_sleep);
        sem_wait(&mutex);
        
        if(reindeer_num == 9) {
            santa_state = 1;
            printf("Santa: preparing sleigh\n");
            
    
            for(int i = 0; i < 9; i++) {
                sem_post(&reindeer_wait);
            }
            sem_post(&mutex); 
            for(int i = 0; i < 9; i++) {
                sem_wait(&reindeer_done);
            }
            
            sem_wait(&mutex);
            reindeer_num = 0;
            santa_state = 0;
            count++;
            for(int i = 0; i < 9; i++) {
                sem_post(&reindeer_barrier);
            }
            sem_post(&mutex);
        }
        else if(elf_num == 3 && elves_waiting) {
            santa_state = 2;
            printf("Santa: help elves\n");
            
            for(int i = 0; i < 3; i++) {
                sem_post(&elves_wait);
            }
            sem_post(&mutex);  
            for(int i = 0; i < 3; i++) {
                sem_wait(&elves_done);
            }
            
            sem_wait(&mutex);

            santa_state = 0;
            sem_post(&mutex);
        }
    }
    return NULL;
}

void *func_elf() {
    while(!program_finished) {
        sleep(rand() % WORK_TIME);  
        
        sem_wait(&elf_queue);       
        sem_wait(&mutex);
        
        if(program_finished) {
            sem_post(&mutex);
            sem_post(&elf_queue);
            break;
        }
        
        if(santa_state == 0 && reindeer_num < 9) {
            elf_num++;
            printf("Elf: waiting (%d)\n", elf_num);
            
            if(elf_num == 3) {
                elves_waiting = 1;
                printf("Elves call Santa\n");
                sem_post(&santa_sleep);
            }
            sem_post(&mutex);
            
            sem_wait(&elves_wait);
            if(program_finished) {
                sem_post(&elves_wait);  
                break;
            }
            
            printf("Elf: get help\n");
            sleep(rand() % num);
            sem_post(&elves_done);      
            
            sem_wait(&mutex);
            elf_num--;                  
            if(elf_num == 0) {         
                elves_waiting = 0;
            }
            sem_post(&mutex);
            sem_post(&elf_queue);       
        } else {
            sem_post(&mutex);
            sem_post(&elf_queue);      
            sleep(rand() % WORK_TIME);
        }
    }
    return NULL;
}

void *func_reindeer() {
    while(!program_finished) {
        sem_wait(&reindeer_barrier);
        
        if(program_finished) {
            sem_post(&reindeer_barrier);  
            break;
        }
        
        sleep(rand() % VACATION_TIME);  
        
        sem_wait(&mutex);
        reindeer_num++;
        printf("Reindeer: waiting (%d)\n", reindeer_num);
        if(reindeer_num == 9) {
            printf("Reindeer 9 calls Santa.\n");
            sem_post(&santa_sleep);
        }
        sem_post(&mutex);
        
        sem_wait(&reindeer_wait);
        printf("Reindeer: getting hitched\n");
        sleep(rand() % num);
        sem_post(&reindeer_done);  
    }
    return NULL;
}

pthread_t *reindeer_threads;
pthread_t *elf_threads;

void create_threads() {
    
    reindeer_threads = malloc(REINDEER_COUNT * sizeof(pthread_t));
    for(int i = 0; i < REINDEER_COUNT; i++) {
        pthread_create(&reindeer_threads[i], NULL, func_reindeer, NULL);
        sleep(rand() % 2);  
    }

    elf_threads = malloc(ELF_COUNT * sizeof(pthread_t));
    for(int i = 0; i < ELF_COUNT; i++) {
        pthread_create(&elf_threads[i], NULL, func_elf, NULL);
        sleep(rand() % 2);  
    }
}

int main() {
   
    srand(time(NULL));
    
    sem_init(&santa_sleep, 0, 0);//wake up Santa
    sem_init(&reindeer_wait, 0, 0);//getHitchedv in queue
    sem_init(&elves_wait, 0, 0);//get help in queue
    sem_init(&mutex, 0, 1);//proction
    sem_init(&elf_queue, 0, 3);  
    elves_waiting = 0;            
    sem_init(&reindeer_done, 0, 0);//concurrently
    sem_init(&elves_done, 0, 0);//concurrently
    sem_init(&reindeer_barrier, 0, REINDEER_COUNT); //prevent reindeer waiting while some haven't been getHitched

    pthread_t santa_thread;
    pthread_create(&santa_thread, NULL, func_santa, NULL);
    
    create_threads();
    
    pthread_join(santa_thread, NULL);
    
    for(int i = 0; i < REINDEER_COUNT; i++) {
        pthread_join(reindeer_threads[i], NULL);
    }
    
    for(int i = 0; i < ELF_COUNT; i++) {
        pthread_join(elf_threads[i], NULL);
    }
    sem_destroy(&santa_sleep);
    sem_destroy(&reindeer_wait);
    sem_destroy(&elves_wait);
    sem_destroy(&mutex);
    sem_destroy(&elf_queue);
    sem_destroy(&reindeer_done);
    sem_destroy(&elves_done);
    sem_destroy(&reindeer_barrier);
    
    free(reindeer_threads);
    free(elf_threads);
    
    
    return 0;
}