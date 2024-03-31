#include "include/lib.h"
#include <stdio.h>

#define MAX_SLAVES 2
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

  int files = argc - 1;

  AppInfo info = {0, files, 0};

  for (int i = 0; i < MAX_SLAVES; i++) {
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

      execv("./slave", (char *[]){"./slave", NULL});
      perror("execv");
      return EXIT_FAILURE;
    } else if (pid > 0) {
      close(appToSlave[READ_END]);
      close(slaveToApp[WRITE_END]);
      info.appFds[i][WRITE_END] = appToSlave[WRITE_END];
      info.appFds[i][READ_END] = slaveToApp[READ_END];
      write(info.appFds[i][WRITE_END], argv[i + 1], strlen(argv[i + 1]));
      write(info.appFds[i][WRITE_END], " ", 1);
      info.delivered_files++;
    } else {
      perror("fork");
      return EXIT_FAILURE;
    }
  }

  int current_slave = 0;
  int file_index = 1;
  int files_sent = 0;

  FILE *outputFile = fopen("md5Result.txt", "a");
  if (outputFile == NULL) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  char slave_output[BUFFER_SIZE];
  while (files_sent < files) {
    printf("hola\n");
    if (file_index <= argc - 1) {
      write(info.appFds[current_slave][WRITE_END], argv[file_index],
            strlen(argv[file_index]));
      write(info.appFds[current_slave][WRITE_END], " ", 1);
      file_index++;
      files_sent++;
      info.delivered_files++;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(info.appFds[current_slave][READ_END], &read_fds);

    struct timeval timeout = {0, 0}; // No timeout, block indefinitely
    int ready_fds = select(info.appFds[current_slave][READ_END] + 1, &read_fds,
                           NULL, NULL, &timeout);
    if (ready_fds == -1) {
      perror("select");
      break;
    }

    if (FD_ISSET(info.appFds[current_slave][READ_END], &read_fds)) {
      ssize_t bytes_read =
          read(info.appFds[current_slave][READ_END], slave_output, BUFFER_SIZE);
      printf("slave output: %s", slave_output);
      if (bytes_read > 0) {
        fwrite(slave_output, 1, bytes_read, outputFile);
      }
    }

    current_slave = (current_slave + 1) % MAX_SLAVES;
  }

  fclose(outputFile);

  // Print appInfo struct
  printf("Delivered files: %u\n", info.delivered_files);
  printf("File count: %u\n", info.file_count);
  printf("Received files: %u\n", info.received_files);

  return EXIT_SUCCESS;
}
