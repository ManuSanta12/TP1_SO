#include "./include/lib.h"

int main(int argc, char* argv[]){
    char shm_name[MAX_NAME_SIZE];
    if(argc == 2){
        strcpy(shm_name, argv[1]);
    } else {
        //get shared memory name form stdin, sent by app process
        int a = read(STDIN_FILENO,shm_name,MAX_NAME_SIZE);
        shm_name[a] = '\0';
    }

    int shm_fd = shm_open(shm_name, O_RDONLY, S_IRUSR | S_IWUSR);
    if (shm_fd == -1){
        perror("Shared memory error in view process");
        return EXIT_FAILURE;
    }
    char* buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    sem_t* sem = sem_open(SEM_NAME, 0);
    if(sem == SEM_FAILED){
        perror("Semaphore error");
        exit(0);
    }
    char re[READ_BUFFER_SIZE];
    while(strcmp(re, END_MSG)!=0){
        sem_wait(sem);
        read(shm_fd, re, READ_BUFFER_SIZE);
        printf("%s\n\n\n",re);    
    }
    munmap(buffer, BUFFER_SIZE);
    sem_close(sem);
    close(shm_fd);
    return 0;
}