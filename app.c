#include "include/lib.h"

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

  AppInfo info;
  info.delivered_files = 0;
  info.file_count = argc - 1;
  info.received_files = 0;

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
      // close(appToSlave[READ_END]);

      dup2(slaveToApp[WRITE_END], STDOUT_FILENO);
      // close(slaveToApp[WRITE_END]);

      execv("./slave", (char *[]){"./slave", NULL});
      perror("execv");
      return EXIT_FAILURE;
    } else if (pid > 0) {
      close(appToSlave[READ_END]);
      close(slaveToApp[WRITE_END]);

      info.appFds[i][WRITE_END] = appToSlave[WRITE_END];
      info.appFds[i][READ_END] = slaveToApp[READ_END];

      write(info.appFds[i][WRITE_END], argv[info.delivered_files + 1],
            strlen(argv[info.delivered_files + 1]));
      write(info.appFds[i][WRITE_END], " ", 1);
      info.delivered_files++;
      if (info.delivered_files != info.file_count) {
        write(info.appFds[i][1], argv[info.delivered_files + 1],
              strlen(argv[info.delivered_files + 1]));
        write(info.appFds[i][1], "\n", 1);
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
  char slave_output[BUFFER_SIZE];
  while (info.received_files < info.file_count) {
    int maxFd = 0;
    fd_set read_fds = {};
    // FD_ZERO(&read_fds);
    // FD_SET(info.appFds[current_slave][READ_END], &read_fds);
    // FD_ZERO(&read_fds);
    for (int i = 0; i < MAX_SLAVES; i++) {
      FD_SET(info.appFds[i][READ_END], &read_fds);
      if (info.appFds[i][READ_END] > maxFd) {
        maxFd = info.appFds[i][READ_END];
      }
    }

    int selectResult = select(maxFd + 1, &read_fds, NULL, NULL, NULL);
    if (selectResult == -1) {
      perror("select");
      break;
    }

    for (int i = 0; info.received_files < info.file_count && i < MAX_SLAVES;
         i++) {
      if (FD_ISSET(info.appFds[i][READ_END], &read_fds)) {
        // printf("Reading from slave %d\n", i);
        ssize_t bytes_read =
            read(info.appFds[i][READ_END], slave_output, BUFFER_SIZE);
        if (bytes_read > 0) {
          printf("slave_output: %s\n", slave_output);
          printf("Read %ld bytes\n", bytes_read);
          fprintf(outputFile, "%s", slave_output);
          info.received_files++;
        }

        if (info.delivered_files < info.file_count) {
          write(info.appFds[i][WRITE_END], argv[info.delivered_files + 1],
                strlen(argv[info.delivered_files + 1]));
          write(info.appFds[i][WRITE_END], " ", 1);
          info.delivered_files++;
        }
      }
    }
  }

  fclose(outputFile);

  // Print appInfo struct
  printf("Delivered files: %u\n", info.delivered_files);
  printf("File count: %u\n", info.file_count);
  printf("Received files: %u\n", info.received_files);
  return EXIT_SUCCESS;
}
