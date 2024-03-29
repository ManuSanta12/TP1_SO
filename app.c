#include "include/lib.h"

#define FILES_PER_SLAVE 1
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

  // Abre el archivo para escribir los resultados MD5
  int results_fd = open("results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (results_fd == -1) {
    perror("open");
    return EXIT_FAILURE;
  }

  // Fork procesos hijos y crea pipes para la comunicación
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
    } else if (pid == 0) {            // Proceso hijo
      close(masterFds[i][WRITE_END]); // Cierra el extremo de escritura no
                                      // utilizado en el hijo

      // Redirecciona stdin para leer desde el pipe maestro
      dup2(masterFds[i][READ_END], STDIN_FILENO);
      close(masterFds[i][READ_END]); // Cierra el extremo de lectura original

      // Ejecuta las tareas del proceso hijo aquí
      execv("./slave", (char *[]){"./slave", NULL});
      perror("execv");
      exit(EXIT_FAILURE);
    } else {
      pids[i] = pid;
      close(masterFds[i][READ_END]); // Cierra el extremo de lectura no
                                     // utilizado en el padre
    }
  }

  // Proceso padre
  for (int i = 1; i <= files; i++) {
    int current_slave = (i - 1) / FILES_PER_SLAVE;
    write(masterFds[current_slave][WRITE_END], argv[i], strlen(argv[i]));
    write(masterFds[current_slave][WRITE_END], "\n", 1);
  }
  closePipes(masterFds, slaves);

  // Espera a que todos los procesos hijos terminen
  for (int i = 0; i < slaves; i++) {
    waitpid(pids[i], NULL, 0);
  }

  // Cierra el archivo de resultados
  close(results_fd);

  return EXIT_SUCCESS;
}
