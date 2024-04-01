// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "include/lib.h"

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

  printf("%-34s%-34s%-8d\n", filePath, md5Sum,
         getpid()); // Envía el resultado en el formato especificado
  fflush(stdout);   // Asegura que el buffer de salida esté vaciado
  pclose(pipe);
}

int main() {
  ssize_t nbytes;
  char inputBuffer[BUFFER_SIZE];
  // Lee la ruta del archivo del proceso padre a través del STDIN
  if ((nbytes = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer))) > 0) {
    // Reemplaza el salto de línea final con un carácter nulo
    if (inputBuffer[nbytes - 1] == '\n') {
      inputBuffer[nbytes - 1] = '\0';
    }
    // Tokeniza la entrada por espacio en blanco
    char *token = strtok(inputBuffer, " ");
    while (token != NULL) {
      calculateMd5(token); // Calcula el MD5 y lo envía al STDOUT (pipe)
      token = strtok(NULL, " ");
    }
  }

  return 0;
}
