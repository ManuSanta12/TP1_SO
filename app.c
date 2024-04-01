// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "include/lib.h"
#include <stdio.h>

#define MAX_SLAVES 4
#define BUFFER_SIZE 4096
#define READ_END 0
#define WRITE_END 1

typedef struct {
  unsigned int delivered_files;
  unsigned int file_count;
  unsigned int received_files;
  int appFds[MAX_SLAVES][2];
} AppInfo;

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
    return EXIT_FAILURE;
  }

  AppInfo info;
  info.delivered_files = 0;
  info.file_count = argc - 1;
  info.received_files = 0;

  for (int i = 0; i < MAX_SLAVES && info.delivered_files < info.file_count;
       i++) {
    int appToSlave[2];
    int slaveToApp[2];

    if (pipe(appToSlave) == -1 || pipe(slaveToApp) == -1) {
      perror("pipe");
      return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == 0) {
      close(appToSlave[WRITE_END]);
      close(slaveToApp[READ_END]);

      dup2(appToSlave[READ_END], STDIN_FILENO);
      close(appToSlave[READ_END]);

      dup2(slaveToApp[WRITE_END], STDOUT_FILENO);
      close(slaveToApp[WRITE_END]);

      execv("slave", (char *[]){"./slave", NULL});
      perror("execv");
      return EXIT_FAILURE;
    } else if (pid > 0) {
      close(appToSlave[READ_END]);
      close(slaveToApp[WRITE_END]);

      info.appFds[i][WRITE_END] = appToSlave[WRITE_END];
      info.appFds[i][READ_END] = slaveToApp[READ_END];

      if (info.delivered_files != info.file_count) {
        printf("father path: %s\n", argv[info.delivered_files + 1]);
        write(info.appFds[i][WRITE_END], argv[info.delivered_files + 1],
              strlen(argv[info.delivered_files + 1]));
        write(info.appFds[i][WRITE_END], " ", 1);
        info.delivered_files++;
      }
    } else {
      perror("fork");
      return EXIT_FAILURE;
    }
  }

  FILE *outputFile = fopen("md5Result.txt", "a");
  if (outputFile == NULL) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  while (info.received_files < info.file_count) {
    fd_set read_fds = {};
    int maxFd = 0;
    FD_ZERO(&read_fds);
    for (int j = 0; j < MAX_SLAVES; j++) {
      FD_SET(info.appFds[j][READ_END], &read_fds);
      if (info.appFds[j][READ_END] > maxFd) {
        maxFd = info.appFds[j][READ_END];
      }
    }

    int selectResult = select(maxFd + 1, &read_fds, NULL, NULL, NULL);
    if (selectResult == -1) {
      perror("select");
      break;
    }

    char slave_output[BUFFER_SIZE];
    for (int i = 0; i < MAX_SLAVES && info.delivered_files < info.file_count;
         i++) {
      // printf("[[[]]]\n");
      if (FD_ISSET(info.appFds[i][READ_END], &read_fds)) {
        // printf("entre aca\n");
        ssize_t bytes_read =
            read(info.appFds[i][READ_END], slave_output, BUFFER_SIZE);
        if (bytes_read > 0) {
          slave_output[bytes_read] = '\0';
          printf("slave_output: %s from slave: %d\n", slave_output, i);
          fprintf(outputFile, "%s\n", slave_output);
          info.received_files++;
          printf("recieved file: %d\n", info.received_files);
          if (info.delivered_files < info.file_count) {
            printf("child path: %s from slave: %d\n",
                   argv[info.delivered_files + 1], i);
            write(info.appFds[i][WRITE_END], argv[info.delivered_files + 1],
                  strlen(argv[info.delivered_files + 1]));
            write(info.appFds[i][WRITE_END], " ", 1);
            info.delivered_files++;
          }
        }
      }
    }
  }

  fclose(outputFile);

  return EXIT_SUCCESS;
}
