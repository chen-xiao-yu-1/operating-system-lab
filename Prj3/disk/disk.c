#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/mman.h>
#include  <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>  
int main(int argc, char *argv[])
{   int c,n,b,delay;
    char *filename;
    if(argc != 5) {
        fprintf(stderr,"Usage: %s <diskfile>\n",argv[0]);
        exit(1);
    }
    b=256;
    c=atoi(argv[1]);
    n=atoi(argv[2]);
    delay=atoi(argv[3]);
    filename=argv[4];
    
    FILE *log_fp;
    log_fp = fopen("disk.log", "w");
    if (!log_fp) {
        perror("Cannot open log file");
        exit(1);
    }

    int fd = open (filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
    printf("Error: Could not open disk file '%s'.\n", filename);
    perror("Error opening disk file"); 
    fprintf(stderr, "Errno: %d\n", errno);  
    exit(-1);
    }

    long FILESIZE = b * n * c;
    int result = lseek (fd, FILESIZE-1, SEEK_SET);
    int last_track = 0;
    if (result == -1) {
    perror ("Error calling lseek() to 'stretch' the file");
    close (fd);
    exit(-1); 
    }
    result = write (fd, "", 1);
        if (result != 1) {
        perror("Error writing last byte of the file");
        close(fd);
        exit(-1);
        }
    char* diskfile;

    diskfile = (char *) mmap (NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (diskfile == MAP_FAILED){
        close(fd);
        printf("Error: Could not map file.\n");
        exit(-1);
        }

    char command[1024];
    while(1){
         if(fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = 0;
        char *op=strtok(command, " ");
        switch(op[0]){
            case 'I':
                     {
                        fprintf(log_fp, "%d %d\n", c, n);
                        fflush(log_fp);
                        break;
                     }
            case 'R':{
               int track=atoi(strtok(NULL, " "));
               int sector=atoi(strtok(NULL, " "));
               if(track<0 || track>=c || sector<0 || sector>=n){
                printf("Insructions error!\n");
                fprintf(log_fp, "NO\n");
                   break;
               }
                if (last_track != -1 && last_track != track) {
                    usleep(delay * 1000*abs(track - last_track)); 
                }
                last_track = track;
                long offset=track*n*b+sector*b;
                char buffer[b];
                memcpy(buffer, diskfile + offset, b);
                fprintf(log_fp, "YES %.*s\n", b, buffer);
                break;
            }
            case 'W':{
                int track=atoi(strtok(NULL, " "));
                int sector=atoi(strtok(NULL, " "));
                char *data=strtok(NULL, "");
                if(track<0 || track>=c || sector<0 || sector>=n){
                    printf("Insructions error!\n");
                    fprintf(log_fp, "NO\n");
                    break;
                }
                if (last_track != -1 && last_track != track) {
                    usleep(delay * 1000*abs(track - last_track)); 
                }
                last_track = track;
                long offset=track*n*b+sector*b;
                char buffer[b];
                memset(buffer, 0, b);  
                memcpy(buffer, data, b);  
                memcpy( &diskfile[offset],buffer, b);
                fprintf(log_fp, "YES\n");
                break;
            }
            case 'E': {
               
                fprintf(log_fp, "Goodbye\n");
                fflush(log_fp);
                fclose(log_fp);
                munmap(diskfile, FILESIZE);
                close(fd);
                exit(0);
            }

        }

        

    }
    return 0;
}