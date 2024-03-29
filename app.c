#include "include/lib.h"

#define FILES_PER_SLAVE 5
#define READ_END 0
#define WRITE_END 1

void closePipes(int fds[][2], int count) {
  for (int i = 0; i < count; i++) {
    close(fds[i][READ_END]);
    close(fds[i][WRITE_END]);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int files = argc - 1;
  int slaves = (files / FILES_PER_SLAVE) + 1;

  int masterFds[slaves][2];
  pid_t pids[slaves];

  // Fork child processes and create pipes for communication
  for (int i = 0; i < slaves; i++) {
    if (pipe(masterFds[i]) == -1) {
      perror("pipe");
      closePipes(masterFds, i);
      return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      closePipes(masterFds, i);
      return EXIT_FAILURE;
    } else if (pid == 0) {            // Child process
      close(masterFds[i][WRITE_END]); // Close unused write end in child

      // Redirect stdin to read from the master pipe
      if (dup2(masterFds[i][READ_END], STDIN_FILENO) == -1) {
        perror("dup2");
        return EXIT_FAILURE;
      }
      close(masterFds[i][READ_END]); // Close original read end

      // Perform child process tasks here
      execv("./slave", (char *[]){"./slave", NULL});
      perror("execv");
      exit(EXIT_FAILURE);
    } else {
      pids[i] = pid;
      close(masterFds[i][READ_END]); // Close unused read end in parent
    }
  }

  // Parent process
  for (int i = 1; i <= files; i++) {
    printf("hola\n");
    int current_slave = (i - 1) / FILES_PER_SLAVE;
    write(masterFds[current_slave][WRITE_END], argv[i], strlen(argv[i]));
    write(masterFds[current_slave][WRITE_END], " ", 1);
  }
  closePipes(masterFds, slaves);

  // Wait for all child processes to finish
  for (int i = 0; i < slaves; i++) {
    waitpid(pids[i], NULL, 0);
  }

  return EXIT_SUCCESS;
}
