// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
#include "./include/lib.h"
#include <stdio.h>

int amountToProcess(int fileQuantity, int deliveredFiles);
void closePipes(int appToSlaveFD[][NUMBER_OF_PIPE_ENDS],
                int slaveToAppFD[][NUMBER_OF_PIPE_ENDS], int maxSlaves);

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    perror("Invalid arguments quantity");
  }

  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("Shared memory error in app process");
    exit(0);
  }
  ftruncate(shm_fd, BUFFER_SIZE);
  char *map_result =
      mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

  sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
  printf("%s", SHM_NAME);
  fflush(stdout);

  FileDeliveryInfo fileDeliveryInfo;
  fileDeliveryInfo.deliveredFiles = 0;
  fileDeliveryInfo.receivedFiles = 0;
  fileDeliveryInfo.fileQuantity = 0;

  char **paths = filterFilePaths(argc, argv, &fileDeliveryInfo.fileQuantity);

  int maxSlaves = (SLAVES_QTY < ((fileDeliveryInfo.fileQuantity + 1) / 2))
                      ? SLAVES_QTY
                      : ((fileDeliveryInfo.fileQuantity + 1) / 2);

  int appToSlaveFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int slaveToAppFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int pids[maxSlaves];

  for (int nSlave = 0; nSlave < maxSlaves; nSlave++) {
    if (pipe(appToSlaveFD[nSlave]) != 0 || pipe(slaveToAppFD[nSlave]) != 0) {
      perror("Failed to create pipes");
    }
    pids[nSlave] = fork();
    if (pids[nSlave] == 0) {
      close(appToSlaveFD[nSlave][WRITE]);
      dup2(appToSlaveFD[nSlave][READ], STDIN_FILENO);

      close(slaveToAppFD[nSlave][READ]);
      dup2(slaveToAppFD[nSlave][WRITE], STDOUT_FILENO);

      execv("slave", (char *[]){"./slave", NULL});
    } else if (pids[nSlave] > 0) {
      close(slaveToAppFD[nSlave][WRITE]);
      close(appToSlaveFD[nSlave][READ]);
    } else {
      perror("Error in fork");
    }
  }

  fd_set readFDs;

  for (int i = 0; i < maxSlaves; i++) {
    int quantity = amountToProcess(fileDeliveryInfo.fileQuantity,
                                   fileDeliveryInfo.deliveredFiles);
    for (int j = 0; j < quantity; j++) {
      if (write(appToSlaveFD[i][WRITE], paths[fileDeliveryInfo.deliveredFiles],
                strlen(paths[fileDeliveryInfo.deliveredFiles])) ==
          WRITE_ERROR) {
        perror("Failed to send paths to slave process");
      }

      if (write(appToSlaveFD[i][WRITE], "\n", 1) == WRITE_ERROR) {
        perror("Failed to send paths to slave process");
      }
      fileDeliveryInfo.deliveredFiles++;
    }
  }

  FILE *resultFile = fopen("result.txt", "w");
  if (resultFile == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  while (fileDeliveryInfo.receivedFiles < fileDeliveryInfo.fileQuantity) {
    int maxFD = 0;
    FD_ZERO(&readFDs);
    for (int i = 0; i < maxSlaves; i++) {
      FD_SET(slaveToAppFD[i][READ], &readFDs);
      if (slaveToAppFD[i][READ] > maxFD)
        maxFD = slaveToAppFD[i][READ];
    }
    if (select(maxFD + 1, &readFDs, NULL, NULL, NULL) == SELECT_ERROR) {
      perror("Error in select");
    }
    char buffer[SLAVE_BUFFER_SIZE];
    char md5[SLAVE_BUFFER_SIZE];
    for (int i = 0; i < maxSlaves; i++) {
      if (FD_ISSET(slaveToAppFD[i][READ], &readFDs)) {
        ssize_t readAnswer =
            read(slaveToAppFD[i][READ], buffer, SLAVE_BUFFER_SIZE - 1);
        if (readAnswer == -1) {
          perror("Error while reading slave output.");
        } else {
          buffer[readAnswer] = '\0';
        }

        int md5Index = 0;
        for (int j = 0; j < readAnswer; j++) {
          md5[md5Index++] = buffer[j];
          if (buffer[j] == '\n') {
            md5[md5Index] = '\0';
            write(shm_fd, md5, READ_BUFFER_SIZE);
            fprintf(resultFile, "%s", md5);
            fileDeliveryInfo.receivedFiles++;
            sem_post(sem);
          }
        }
        if (fileDeliveryInfo.deliveredFiles < fileDeliveryInfo.fileQuantity) {
          if (write(appToSlaveFD[i][WRITE],
                    paths[fileDeliveryInfo.deliveredFiles],
                    strlen(paths[fileDeliveryInfo.deliveredFiles])) ==
              WRITE_ERROR) {
            perror("Failed to send paths to slave process");
          }
          if (write(appToSlaveFD[i][WRITE], "\n", 1) == WRITE_ERROR) {
            perror("Failed to send paths to slave process");
          }
          fileDeliveryInfo.deliveredFiles++;
        }
      }
    }
  }

  char end[sizeof(END_MSG)] = END_MSG;
  write(shm_fd, end, sizeof(END_MSG));
  sem_post(sem);

  closePipes(appToSlaveFD, slaveToAppFD, maxSlaves);

  munmap(map_result, BUFFER_SIZE);
  close(shm_fd);
  shm_unlink(SHM_NAME);
  sem_close(sem);

  sem_unlink(SEM_NAME);
  for (int i = 0; i < fileDeliveryInfo.fileQuantity; i++) {
    free(paths[i]);
  }
  free(paths);
  return 0;
}

int amountToProcess(int fileQuantity, int deliveredFiles) {
  if (deliveredFiles > fileQuantity) {
    perror("Error in processing of files");
  }
  if (deliveredFiles + MAX_FILES_SLAVE <= fileQuantity) {
    return MAX_FILES_SLAVE;
  }
  return fileQuantity - deliveredFiles;
}

void closePipes(int appToSlaveFD[][NUMBER_OF_PIPE_ENDS],
                int slaveToAppFD[][NUMBER_OF_PIPE_ENDS], int maxSlaves) {
  for (int i = 0; i < maxSlaves; i++) {
    close(slaveToAppFD[i][READ]);
    close(appToSlaveFD[i][WRITE]);
  }
}

char **filterFilePaths(int argc, char *argv[], int *fileQuantity) {
  const int BLOCK_QTY = 10; // Initial block size and additional blocks to add

  if (argc < 2) {
    fprintf(stderr, "No files to process.\n");
    exit(EXIT_FAILURE);
  }

  struct stat pathStat;
  char **validPaths = malloc(BLOCK_QTY * sizeof(char *));
  if (validPaths == NULL) {
    perror("Memory allocation failed");
    exit(EXIT_FAILURE);
  }

  int validPathCount = 0;

  for (int i = 1; i < argc; i++) {
    // Check if the file is a regular file
    if (stat(argv[i], &pathStat) == 0 && S_ISREG(pathStat.st_mode)) {
      // Perform reallocation if necessary
      if (validPathCount % BLOCK_QTY == 0) {
        char **tmp_ptr =
            realloc(validPaths, (validPathCount + BLOCK_QTY) * sizeof(char *));
        if (tmp_ptr == NULL) {
          perror("Memory reallocation failed");
          exit(EXIT_FAILURE);
        } else {
          validPaths = tmp_ptr;
        }
      }

      char *str = malloc(strlen(argv[i]) + 1);
      if (str == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
      }
      validPaths[validPathCount++] = strcpy(str, argv[i]);
    } else {
      fprintf(stderr, "Invalid file or file type: %s\n", argv[i]);
    }
  }

  *fileQuantity = validPathCount;

  return validPaths;
}
