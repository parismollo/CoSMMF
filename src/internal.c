#include "internal.h"


bool create_processes() {
    int pid;
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        pid = fork();
        if(pid < 0) {
            perror(ERROR_MESSAGE);
            return false;
        } else if (pid == 0) {
            printf("Child %d do something.\n",getpid());
            if(!tmp_process_read()){perror(ERROR_MESSAGE);}
            return true;
        }
    }

    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        wait(NULL);
    }

    printf("All child processes have finished\n");
    return true;
}

bool tmp_process_read() {
    int fd[NUMBER_OF_FILES];
    char file_name[FILE_NAME_SIZE];
    for(int i=0; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file_%d", i);
        fd[i] = open(file_name, O_RDONLY);
        
        if(fd[i] == -1) {
            perror(ERROR_MESSAGE);
            return false;
        }
        struct stat st;
        if(fstat(fd[i], &st) == -1) {
            perror(ERROR_MESSAGE);
            close(fd[i]);
            return false;
        }
        char * mapped_region = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd[i], 0);
        if(mapped_region == MAP_FAILED) {
            perror(ERROR_MESSAGE);
            close(fd[i]);
            return false;
        }

        printf("Process %d reads first letter of %s : [%c]\n", getpid(), file_name, mapped_region[0]);
        munmap(mapped_region, st.st_size);
        close(fd[i]);
    }
    return true;
}

bool create_files() {
    char file_name[FILE_NAME_SIZE];
    int fd;

    if(!check_directory()) return false;

    for(int i=0; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file_%d", i);
        fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
        if(fd == -1) {
            perror(ERROR_MESSAGE);
            return false;
        }
        close(fd);
    }
    return true;
}

bool add_content_files() {
    char file_name[FILE_NAME_SIZE];
    int fd;
    if(!check_directory()) return false;
    for(int i=0; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file_%d", i);
        fd = open(file_name, O_WRONLY, FILE_PERMISSIONS);
        if(fd == -1) {
            perror(ERROR_MESSAGE);
            return false;
        }
        
        const char * data = "Hello PSAR Project";
        ssize_t bytes = write(fd, data, strlen(data));
        
        if(bytes == -1) {
            perror(ERROR_MESSAGE);
            return false;
        }
        
        close(fd);
    }
    return true;
}

bool check_directory() {
    if(mkdir(TEST_FILE_FOLDER, 0755)) {
        if(errno != EEXIST) {
            perror(ERROR_MESSAGE);
            return false;
        }
    }
    return true;
}