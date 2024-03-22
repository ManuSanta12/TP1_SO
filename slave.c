#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char * argv[]){
   
}

void md5(char * path){
     char command[100];
    sprintf(command, "md5sum %s", path);

    // Execute the command
    printf("Executing command: %s\n", command);
    int status = system(command);

    // Check if the command executed successfully
    if (status == -1) {
        printf("Error executing md5sum.\n");
        return 1;
    }

    return 0;
}
