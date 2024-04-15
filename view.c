// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./include/lib.h"

int connectToshm(char * shm_name, char ** buffer);
void readAndPrint(int shm_fd, sem_t *sem);

int main(int argc, char *argv[]) {

  //geting shm name
  char shm_name[MAX_NAME_SIZE];
  if (argc == 2) {
    strncpy(shm_name, argv[1], MAX_NAME_SIZE - 1);
    shm_name[MAX_NAME_SIZE - 1] =
        '\0'; 
  } else {
    // get shared memory name from stdin, sent by app process
    int amountRead = read(STDIN_FILENO, shm_name, MAX_NAME_SIZE - 1);
    if (amountRead == -1) {
      perror("Error reading from stdin");
      return EXIT_FAILURE;
    }
    shm_name[amountRead] = '\0'; 
  }
  //connecting to shm and semaphore
  char * buffer;
  int shm_fd = connectToshm(shm_name, &buffer);

  
  sem_t *sem = sem_open(SEM_NAME, 0);
  if (sem == SEM_FAILED) {
    perror("Semaphore open error");
    exit(0);
  }

  //reading and printing
  readAndPrint(shm_fd, sem);
  

  munmap(buffer, BUFFER_SIZE);
  sem_close(sem);
  close(shm_fd);
  return 0;
}


int connectToshm(char * shm_name, char ** buffer){
  int shm_fd = shm_open(shm_name, O_RDONLY, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("Shared memory error in view process");
    return EXIT_FAILURE;
  }
  *buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
  return shm_fd;
}

void readAndPrint(int shm_fd, sem_t *sem){
  char readFromShm[READ_BUFFER_SIZE] = "";
  char toPrint[BUFFER_SIZE];
  while (strcmp(toPrint, END_MSG) != 0) {
    sem_wait(sem);
    int j;
    for (j = 0; read(shm_fd, readFromShm, 1) > 0 && *readFromShm != '\n'; j++) {
      toPrint[j] = readFromShm[0];
    }
    toPrint[j] = '\0';
    printf("%s\n", toPrint);
  }
}