#ifndef LIB_H
#define LIB_H

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define SLAVES_QTY 4
#define MAX_FILES_SLAVE 2
#define NUMBER_OF_PIPE_ENDS 2
#define SELECT_ERROR -1
#define WRITE_ERROR -1
#define SLAVE_BUFFER_SIZE 256
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

void close_pipes(int pipefd_w[][2], int pipefd_r[][2], int max_slaves);
#endif
