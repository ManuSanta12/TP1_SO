#include "./include/lib.h"

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

  // Allocate memory for paths
  char **paths = calloc(argc - 1, sizeof(char *));
  if (paths == NULL) {
    perror("Failed to allocate memory for paths");
  }

  // Struct to keep track of file delivery information
  FileDeliveryInfo fileDeliveryInfo;
  fileDeliveryInfo.deliveredFiles = 0;
  fileDeliveryInfo.receivedFiles = 0;

  // Store file paths in an array
  for (fileDeliveryInfo.fileQuantity = 1; fileDeliveryInfo.fileQuantity <= argc - 1; fileDeliveryInfo.fileQuantity++) {
    paths[fileDeliveryInfo.fileQuantity - 1] = argv[fileDeliveryInfo.fileQuantity];
  }
  fileDeliveryInfo.fileQuantity--;

  // Calculate the maximum number of slaves to be used
  int maxSlaves = (SLAVES_QTY < ((fileDeliveryInfo.fileQuantity + 1) / 2)) ? SLAVES_QTY : ((fileDeliveryInfo.fileQuantity + 1) / 2);

  // Arrays to hold file descriptors for pipes
  int appToSlaveFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int slaveToAppFD[maxSlaves][NUMBER_OF_PIPE_ENDS];
  int pids[maxSlaves];

  // Create pipes for communication between parent process and child processes
  for (int nSlave = 0; nSlave < maxSlaves; nSlave++) {
    if (pipe(appToSlaveFD[nSlave]) != 0 || pipe(slaveToAppFD[nSlave]) != 0) {
      perror("Failed to create pipes");
    }
    if ((pids[nSlave] = fork()) == 0) {
      close(appToSlaveFD[nSlave][WRITE]);
      dup2(appToSlaveFD[nSlave][READ], STDIN_FILENO);
      close(appToSlaveFD[nSlave][READ]);

      close(slaveToAppFD[nSlave][READ]);
      dup2(slaveToAppFD[nSlave][WRITE], STDOUT_FILENO);
      close(slaveToAppFD[nSlave][WRITE]);

      for (int i = 0; i < nSlave; i++) {
        close(appToSlaveFD[i][WRITE]);
        close(slaveToAppFD[i][READ]);
      }
      execv("slave", (char *[]){"./slave", NULL});
    }
    close(slaveToAppFD[nSlave][WRITE]);
    close(appToSlaveFD[nSlave][READ]);
  }

  // Set up file descriptor set for reading
  fd_set readFDs;
  int quantity = amountToProcess(fileDeliveryInfo.fileQuantity, fileDeliveryInfo.deliveredFiles);

  // Send files to slaves
  for (int i = 0; i < maxSlaves; i++) {
    for (int j = 0; j < quantity; j++) {
      if (write(appToSlaveFD[i][WRITE], paths[fileDeliveryInfo.deliveredFiles], strlen(paths[fileDeliveryInfo.deliveredFiles])) == WRITE_ERROR) {
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
        ssize_t readAnswer = read(slaveToAppFD[i][READ], buffer, SLAVE_BUFFER_SIZE - 1);
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
            printf("output: %s", md5);
            fprintf(resultFile, "%s", md5);
            fileDeliveryInfo.receivedFiles++;
          }
        }

        if (fileDeliveryInfo.deliveredFiles < fileDeliveryInfo.fileQuantity) {
          if (write(appToSlaveFD[i][WRITE], paths[fileDeliveryInfo.deliveredFiles], strlen(paths[fileDeliveryInfo.deliveredFiles])) == WRITE_ERROR) {
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

  // Close pipes
  closePipes(appToSlaveFD, slaveToAppFD, maxSlaves);

  // Free allocated memory
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
