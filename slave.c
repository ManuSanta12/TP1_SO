#include "include/lib.h"
#define MAX_PATH_LENGTH 256
#define MD5_SUM_LENGTH 33
#define BUFFER_SIZE 4096
#define MD5_COMMAND "md5sum "

void calculateMd5(const char *filePath) {
  char md5Command[MAX_PATH_LENGTH + strlen(MD5_COMMAND)];
  printf("fielpath: %s\n \n", filePath);
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

  printf("MD5 sum of %s: %s\n", filePath, md5Sum);
  printf("%-34s%-34s%-8d\n", filePath, md5Sum, getpid());
  pclose(pipe);
}

int main() {
  printf("Buenas, estoy en slave\n");

  ssize_t nbytes;
  char inputBuffer[BUFFER_SIZE];

  while ((nbytes = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer))) > 0) {
    // Reemplazamos el salto de línea final con un carácter nulo
    if (inputBuffer[nbytes - 1] == '\n') {
      inputBuffer[nbytes - 1] = '\0';
    }
    calculateMd5(inputBuffer);
  }

  return 0;
}
