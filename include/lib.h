#ifndef LIB_H
#define LIB_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

#define SLAVES_QTY 4
#define MAX_FILES_SLAVE 2
#define NUMBER_OF_PIPE_ENDS 2

#define SELECT_ERROR -1
#define WRITE_ERROR -1
#define READ_BUFFER_SIZE 50
#define SLAVE_BUFFER_SIZE 256
#define MAX_PATH_LENGTH 256
#define BUFFER_SIZE  1024

#define MAX_NAME_SIZE 255
#define SHM_NAME "/shared_memory"
#define SEM_NAME "/sempahore"
#define END_MSG "Done"

#define READ 0
#define WRITE 1

#define MD5_LENGTH 33
#define MD5_COMMAND "md5sum"

typedef struct {
  int deliveredFiles;
  int fileQuantity;
  int receivedFiles;
} FileDeliveryInfo;

int amount_to_process(int file_qty, int files_processed);
char **filterFilePaths(int argc, char *argv[], int *fileQuantity);
void close_pipes(int pipefd_w[][2], int pipefd_r[][2], int max_slaves);
#endif
