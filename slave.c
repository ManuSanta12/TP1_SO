// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./include/lib.h"
#define MAX_COMMAND_LENGTH 33
int main(void) {
  // Disable STDOUT buffering.
  setvbuf(stdout, NULL, _IONBF, 0);

  char *filePath = NULL;
  size_t lineCapacity = 0;
  ssize_t bytesRead;

  // Read file paths from stdin until EOF
  while ((bytesRead = getline(&filePath, &lineCapacity, stdin)) > 0) {
    // Remove newline character from file path
    if (filePath[bytesRead - 1] == '\n') {
      filePath[bytesRead - 1] = '\0';
    }

    // Construct the complete MD5 command
    char md5Command[strlen(MD5_COMMAND) + 1 + strlen(filePath) + 1];
    sprintf(md5Command, "%s %s", MD5_COMMAND, filePath);

    // Verificar que la longitud del comando no exceda un límite seguro
    if (strlen(md5Command) >= MAX_COMMAND_LENGTH) {
      fprintf(stderr,
              "Error: Longitud del comando MD5 excede el límite permitido\n");
      exit(EXIT_FAILURE);
    }

    // Verify that md5Command is correctly terminated with a null character
    if (md5Command[sizeof(md5Command) - 1] != '\0') {
      fprintf(stderr, "Error: Comando MD5 no está correctamente terminado\n");
      exit(EXIT_FAILURE);
    }

    // Execute MD5 command using popen
    FILE *pipe = popen(md5Command, "r");
    if (pipe == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }

    char md5Sum[MD5_LENGTH + 1];
    // Read MD5 sum from pipe
    if (fscanf(pipe, "%32s", md5Sum) != 1) {
      fprintf(stderr, "Error reading MD5 sum\n");
      exit(EXIT_FAILURE);
    }

    // Print the result in the specified format
    printf("%d %s %s\n", getpid(), md5Sum, filePath);

    // Close the pipe
    pclose(pipe);
  }

  // Free allocated memory for file path
  free(filePath);
  return 0;
}
