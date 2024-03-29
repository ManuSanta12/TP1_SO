#include "include/lib.h"
#include <stdio.h>
#define MAX_PATH_LENGTH 256
#define MD5_SUM_LENGTH 33
#define BUFFER_SIZE 4096
#define MD5_COMMAND "md5sum "

void calculateMd5(const char *filePath) {
  char md5Command[MAX_PATH_LENGTH + strlen(MD5_COMMAND)];
  snprintf(md5Command, sizeof(md5Command), "%s%s", MD5_COMMAND, filePath);

  FILE *pipe = popen(md5Command, "r");
  if (pipe == NULL) {
    perror("popen");
    exit(EXIT_FAILURE);
  }

  char md5Sum[MD5_SUM_LENGTH];
  if (fscanf(pipe, "%32s", md5Sum) != 1) {
    fprintf(stderr, "Error reading MD5 sum\n");
    exit(EXIT_FAILURE);
  }

  printf("%-34s%-34s%-8d\n", filePath, md5Sum, getpid());
  pclose(pipe);
}

int main() {
  ssize_t nbytes;
  char inputBuffer[BUFFER_SIZE];

  if ((nbytes = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer))) > 0) {
    // Reemplazamos el salto de línea final con un carácter nulo
    if (inputBuffer[nbytes - 1] == '\n') {
      inputBuffer[nbytes - 1] = '\0';
    }
    printf("path recieved in slave: %s\n\n\n", inputBuffer);
    // Tokenizar la entrada por espacio en blanco
    char *token = strtok(inputBuffer, " ");
    while (token != NULL) {
      calculateMd5(token);
      token = strtok(NULL, " ");
    }
  }

  return 0;
}
