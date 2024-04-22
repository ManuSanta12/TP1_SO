#include "./include/lib.h"

void readAndPrint(int shm_fd);

int main(){
    int fifo_fd = open("/temp_pipename", O_RDONLY);
    readAndPrint(fifo_fd);
}

void readAndPrint(int shm_fd) {
  char readFromShm[READ_BUFFER_SIZE] = "";
  char toPrint[BUFFER_SIZE] = "";
  while (strcmp(toPrint, END_MSG) != 0) {
    int j;
    for (j = 0; read(shm_fd, readFromShm, 1) > 0 && *readFromShm != '\n'; j++) {
      toPrint[j] = readFromShm[0];
    }
    toPrint[j] = '\0';
    printf("%s\n", toPrint);
  }
}

