// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./include/lib.h"
#include <stdio.h>
// Function to determine the number of files to be processed
int amountToProcess(int fileQuantity, int deliveredFiles);
// Function to close pipes
void closePipes(int appToSlaveFD[][NUMBER_OF_PIPE_ENDS],
                int slaveToAppFD[][NUMBER_OF_PIPE_ENDS], int maxSlaves);
int main(int argc, char *argv[]) {
  // Check if the number of arguments is valid
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
  fflush(stdout); // Vaciar el buffer de salida

  // Struct to keep track of file delivery information
  FileDeliveryInfo fileDeliveryInfo;
  fileDeliveryInfo.deliveredFiles = 0;
  fileDeliveryInfo.receivedFiles = 0;
  fileDeliveryInfo.fileQuantity = 0;

  // Allocate memory for paths
  char **paths = filterFilePaths(argc, argv, &fileDeliveryInfo.fileQuantity);

  // Calculate the maximum number of slaves to be used
  int maxSlaves = (SLAVES_QTY < ((fileDeliveryInfo.fileQuantity + 1) / 2))
                      ? SLAVES_QTY
                      : ((fileDeliveryInfo.fileQuantity + 1) / 2);

  // Arrays to hold file descriptors for pipes
  int appToSlaveFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int slaveToAppFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int pids[maxSlaves];

  // Create pipes for communication between parent process and child processes
  for (int nSlave = 0; nSlave < maxSlaves; nSlave++) {
    if (pipe(appToSlaveFD[nSlave]) != 0 || pipe(slaveToAppFD[nSlave]) != 0) {
      perror("Failed to create pipes");
    }
    pids[nSlave] = fork();
    if (pids[nSlave] == 0) {
      close(appToSlaveFD[nSlave][WRITE]);
      dup2(appToSlaveFD[nSlave][READ], STDIN_FILENO);
      // close(appToSlaveFD[nSlave][READ]);

      close(slaveToAppFD[nSlave][READ]);
      dup2(slaveToAppFD[nSlave][WRITE], STDOUT_FILENO);
      // close(slaveToAppFD[nSlave][WRITE]);

      // for (int i = 0; i < nSlave; i++) {
      //   close(appToSlaveFD[i][WRITE]);
      //   close(slaveToAppFD[i][READ]);
      // }
      execv("slave", (char *[]){"./slave", NULL});
    } else if (pids[nSlave] > 0) {
      close(slaveToAppFD[nSlave][WRITE]);
      close(appToSlaveFD[nSlave][READ]);
    } else {
      perror("Error in fork");
    }
  }

  // Set up file descriptor set for reading
  fd_set readFDs;
  int quantity = amountToProcess(fileDeliveryInfo.fileQuantity,
                                 fileDeliveryInfo.deliveredFiles);

  // Send files to slaves
  for (int i = 0; i < maxSlaves; i++) {
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

  // Open result file
  FILE *resultFile = fopen("result.txt", "w");
  if (resultFile == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  // Main loop to process results from slaves
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
    char buffer[SLAVE_BUFFER_SIZE * 2];
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
            write(shm_fd, buffer, READ_BUFFER_SIZE);
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
  // sending message to end the view process.
  char end[sizeof(END_MSG)] = END_MSG;
  write(shm_fd, end, sizeof(END_MSG));
  sem_post(sem);

  // Close pipes
  closePipes(appToSlaveFD, slaveToAppFD, maxSlaves);

  munmap(map_result, BUFFER_SIZE);
  close(shm_fd);
  shm_unlink(SHM_NAME);
  sem_close(sem);

  sem_unlink(SEM_NAME);
  // Free allocated memory
  for (int i = 0; i < fileDeliveryInfo.fileQuantity; i++) {
    free(paths[i]);
  }
  free(paths);
  return 0;
}
// Function to calculate the number of files to process
int amountToProcess(int fileQuantity, int deliveredFiles) {
  if (deliveredFiles > fileQuantity) {
    perror("Error in processing of files");
  }
  if (deliveredFiles + MAX_FILES_SLAVE <= fileQuantity) {
    return MAX_FILES_SLAVE;
  }
  return fileQuantity - deliveredFiles;
}
// Function to close pipes
void closePipes(int appToSlaveFD[][NUMBER_OF_PIPE_ENDS],
                int slaveToAppFD[][NUMBER_OF_PIPE_ENDS], int maxSlaves) {
  for (int i = 0; i < maxSlaves; i++) {
    close(slaveToAppFD[i][READ]);
    close(appToSlaveFD[i][WRITE]);
  }
}

char **filterFilePaths(int argc, char *argv[], int *fileQuantity) {
  const int BLOCK_QTY = 10; // Initial block size and additional blocks to add

  // Check if there are enough arguments
  if (argc < 2) {
    perror("No files to process.");
    exit(EXIT_FAILURE);
  }

  struct stat pathStat;
  char **validPaths = malloc(BLOCK_QTY * sizeof(char *));
  if (validPaths == NULL) {
    perror("Memory allocation failed");
    exit(EXIT_FAILURE);
  }

  int validPathCount = 0;

  // Iterate through the arguments
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

      // Allocate memory for the path string
      char *str = malloc(strlen(argv[i]) + 1); // +1 for the null terminator
      if (str == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
      }
      validPaths[validPathCount++] = strcpy(str, argv[i]);
    } else {
      // Print invalid file paths
      // printf("Invalid file or file type: %s\n", argv[i]);
    }
  }

  // Update the file quantity
  *fileQuantity = validPathCount;

  return validPaths;
}
