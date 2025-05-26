#include "header.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>   // 添加头文件

int **A, **B;
int Numofthread = 16;
int num;
struct thread_arguments
{
int first_paramter;//foot size
int second_paramter;//id
int **return_value;
};
void *my_function (void *args) {
    struct thread_arguments *a;
    a = (struct thread_arguments *)args;
    int a1 = a->first_paramter;
    int b1 = a->second_paramter;
    int **Return_value=a->return_value;

	for (int i = 0; i < a1; ++i)
	{
		for (int j = 0; j < num; ++j)
		{
			int sum = 0;
			for (int k = 0; k < num; ++k)
			{
				sum += A[ b1*a1+i][k]* B[k][j];
			}
			Return_value[i][j] = sum;
		}
	}

    pthread_exit(NULL); 
    
    }
int main(int argc, char *argv[]) {
    struct timeval start_time, end_time;  
    double time;
    bool flag = false;
    pthread_t t1[Numofthread];
    pthread_attr_t attr1[Numofthread];
    struct thread_arguments args[Numofthread]; 
    for (int i = 0; i< Numofthread; ++i)
	{
		pthread_attr_init(&attr1[i]);
		pthread_attr_setdetachstate(&attr1[i], PTHREAD_CREATE_JOINABLE);	
		pthread_attr_setscope(&attr1[i], PTHREAD_SCOPE_SYSTEM);
	}
    if(argc != 2) {
        flag=true;
        char file[128] = {0};  
        strcpy(file, "data.in");  
        FILE *fin = fopen(file, "r");  
        char *line=NULL;
        ssize_t len=0;
        if (fin == NULL) {
            printf("Error: Cannot open data.in.\n");
            return 0;
        }
        if(getline(&line, &len, fin) == -1) {
            printf("Error: Cannot read data.in.\n");
            fclose(fin);
            return 0;
        }
        num = atoi(line);
        free(line);
        fclose(fin);
    }
    else{
        num=atoi(argv[1]);
    }
    A=(int **)malloc(num*sizeof(int*));
    B=(int **)malloc(num*sizeof(int*));
        for(int i=0;i<num;i++){
            A[i]=(int *)malloc(num*sizeof(int));
            B[i]=(int *)malloc(num*sizeof(int));
           }
        for(int i=0;i<num;i++)
            for(int j=0;j<num;j++){
                A[i][j]=rand()%11;
                B[i][j]=rand()%11;
             }
    gettimeofday(&start_time, NULL);  
    for (int i = 0; i< Numofthread; ++i)
    	{   args[i].first_paramter=num/Numofthread;
            args[i].second_paramter=i;
            args[i].return_value = (int **)malloc(args[i].first_paramter * sizeof(int *));
            for (int j = 0; j <args[i].first_paramter; j++) {
                args[i].return_value[j] = (int *)malloc(num * sizeof(int));
            }
    		int rc;
    		rc = pthread_create(&t1[i], &attr1[i], my_function, &args[i]);
    		if (rc)
    		{
    			printf("ERROR:return code error\n");
    			exit(-1);
    		}
    	}
    
    for (int i = 0; i< Numofthread; ++i)
    {
        void *status;
        int rc = pthread_join(t1[i], &status);
        if (rc)
        {
            printf("ERROR: Join error\n");
            exit(-1);
        }
    }
    gettimeofday(&end_time, NULL);  

    time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;    
    time += (end_time.tv_usec - start_time.tv_usec) / 1000.0; 
    printf("time used: %.2f milliseconds\n", time);

    FILE *fout;
    if(flag==true){
        fout = fopen("data.out", "w");
        if (fout == NULL)
        {
            printf("Filed to create data.out\n");
            return 0;
        }
    }
    else{
        fout = fopen("random.out", "w");
        if (fout == NULL)
        {
            printf("Filed to create random.out\n");
            return 0;
        }
    }
        
        fprintf(fout,"%d\n",num);
        for (int k = 0; k < Numofthread; k++)
        {
        for (int i =0; i< args[k].first_paramter; ++i)
        {
            for (int j = 0; j < num; ++j) 
            {
                fprintf(fout,"%d ", args[k].return_value[i][j]);
            }
            fprintf(fout,"\n");
        }	
    }
    fclose(fout);
    
    
    for(int i = 0; i < num; i++) {
        free(A[i]);
        free(B[i]);
    }
    free(A);
    free(B);
    for (int i = 0; i < Numofthread; i++) 
	{
        	for(int j = 0; j < num/Numofthread; j++)
        	{
        		free(args[i].return_value[j]);
        	}
        	free(args[i].return_value);
    	}
    
    return 0;
}