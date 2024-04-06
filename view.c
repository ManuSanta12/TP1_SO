#include "./include/lib.h"

int main(){
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, S_IRUSR | S_IWUSR);
    if (shm_fd == -1){
        perror("Shared memory error");
        return EXIT_FAILURE;
    }
    char* buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    sem_t* sem = sem_open(SEM_NAME, 0);
    sem_wait(sem);
    printf("%s\n", buffer);
    sem_post(sem);
    printf("hola\n");
    munmap(buffer, BUFFER_SIZE);
    sem_close(sem);
    close(shm_fd);
    return 0;
}