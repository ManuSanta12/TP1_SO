// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./include/lib.h"

int main(void) {
  // Disable STDOUT buffering.
  setvbuf(stdout, NULL, _IONBF, 0);

  char *filePath = NULL;
  size_t lineCapacity = 0;
  ssize_t bytesRead;

  int fifo_fd = open("/temp_pipename", O_WRONLY);


  // Read file paths from stdin until EOF
  while ((bytesRead = getline(&filePath, &lineCapacity, stdin)) > 0) {
    if (filePath[bytesRead - 1] == '\n') {
      filePath[bytesRead - 1] = '\0';
    }

    char md5Command[strlen(MD5_COMMAND) + 1 + strlen(filePath) + 1];
    sprintf(md5Command, "%s %s", MD5_COMMAND, filePath);

    if (strlen(md5Command) >= MD5_LENGTH) {
      fprintf(stderr,
              "Error: Longitud del comando MD5 excede el límite permitido\n");
      exit(EXIT_FAILURE);
    }

    if (md5Command[sizeof(md5Command) - 1] != '\0') {
      fprintf(stderr, "Error: Comando MD5 no está correctamente terminado\n");
      exit(EXIT_FAILURE);
    }

    FILE *pipe = popen(md5Command, "r");
    if (pipe == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }

    char md5Sum[MD5_LENGTH + 1];

    if (fscanf(pipe, "%32s", md5Sum) != 1) {
      fprintf(stderr, "Error reading MD5 sum\n");
      exit(EXIT_FAILURE);
    }

    printf("%d %s %s\n", getpid(), md5Sum, filePath);
    dprintf(fifo_fd, "%d %s %s\n", getpid(), md5Sum, filePath);
    pclose(pipe);
  }

  free(filePath);
  return 0;
}
