#include "header.h" 
#include <time.h>
int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Error: Invalid number of arguments.\n");
        return 1;
    }
    int bufferSize = atoi(argv[3]);  

    FILE *src;
	src = fopen (argv[1],"r");
        if (src == NULL) {
        printf ("Error: Could not open file '%s'.\n", argv[1]);
        exit(-1);
        }

    FILE *dest;
	dest = fopen(argv[2], "w+");	
	if (dest == NULL) {
	printf("Error: Could not open file '%s'.\n", argv[2]);
	fclose(src);
	exit(-1);
	}

    int mypipe[2];
    if (pipe(mypipe) == -1) {
        fprintf (stderr, "Pipe failed.\n");
        return -1;
    }

    char *buffer = (char *)calloc(bufferSize, sizeof(char)); 
    clock_t start, end;
    size_t reader;
    double elapsed;

    pid_t ForkPID = fork();
    switch (ForkPID) {
        case -1:
            printf("Error: Failed to fork.\n");
            fclose(src);
            fclose(dest);
            break;
        case 0: 
            close(mypipe[1]);
            while ((reader=read(mypipe[0], buffer, bufferSize))>0) {
                
                fwrite(buffer,1, reader, dest);
            }
            fclose(dest);
            close(mypipe[0]);
            break;
        default: 
	        start=clock();
            close(mypipe[0]);
            while ((reader=fread(buffer,1, bufferSize, src))>0) {
                write(mypipe[1], buffer, reader);
            }
            fclose(src);
            close(mypipe[1]);
            wait(NULL);
            end = clock();
            elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
            printf("Time used: %f milliseconds\n", elapsed);
   	break; }
       
	   
    
    return 0;
}
