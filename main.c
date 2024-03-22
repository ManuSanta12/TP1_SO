#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char * argv[]){
    return 0;
}

int father(char * path){
    if(fork() == 0){
        //child code
        char *argvChild[] = {"slave",path};
        char *env [] = {NULL};
        execve("slave", argvChild, env);
    }
    
}

