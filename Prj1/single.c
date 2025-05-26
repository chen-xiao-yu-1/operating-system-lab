#include <pthread.h>
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    struct timeval start_time, end_time; 
    double time;
    int num;
    int **A, **B, **C;

    // 打开文件读取数据
    fin = fopen("data.in", "r");
    if (fin == NULL) {
        printf("Failed to open data.in\n");
        return 0;
    }

    // 读取矩阵大小
    fscanf(fin, "%d", &num);

    // 分配内存
    A = (int **)malloc(num * sizeof(int*));
    B = (int **)malloc(num * sizeof(int*));
    C = (int **)malloc(num * sizeof(int*)); 
    for(int i = 0; i < num; i++) {
        A[i] = (int *)malloc(num * sizeof(int));
        B[i] = (int *)malloc(num * sizeof(int));
        C[i] = (int *)malloc(num * sizeof(int));
    }

    // 读取矩阵A
    for(int i = 0; i < num; i++) {
        for(int j = 0; j < num; j++) {
            fscanf(fin, "%d", &A[i][j]);
        }
    }

    // 读取矩阵B
    for(int i = 0; i < num; i++) {
        for(int j = 0; j < num; j++) {
            fscanf(fin, "%d", &B[i][j]);
        }
    }

    // 初始化矩阵C
    for(int i = 0; i < num; i++) {
        for(int j = 0; j < num; j++) {
            C[i][j] = 0;
        }
    }

    fclose(fin);

    gettimeofday(&start_time, NULL);
    
    for(int i=0;i<num;i++)
        for(int j=0;j<num;j++)
            for(int k=0;k<num;k++)
                C[i][j] += A[i][k] * B[k][j];
                
   
    gettimeofday(&end_time, NULL);
    
    time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;   
    time += (end_time.tv_usec - start_time.tv_usec) / 1000.0; 
    printf("Time used: %.2f milliseconds\n", time);

    fout = fopen("data.out", "w");
	if (fout == NULL)
	{
		printf("Filed to create data.out\n");
		return 0;
	}
	fprintf(fout,"%d\n",num);
	for(int i = 0; i < num; i++)
	{
		for (int j = 0; j < num; ++j)
		{
			fprintf(fout, "%d ",C[i][j]);
		}
		fprintf(fout, "\n");
	}
	fclose(fout);
	for (int i = 0; i < num; i++) 
	{
        	free(A[i]);
            free(B[i]);
            free(C[i]);
        }
   	free(A);
    free(B);
    free(C);
    return 0;
}


