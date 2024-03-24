#ifndef INTERNAL_H // prevent double inclusion
#define INTERNAL_H

#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include "ptedit_header.h"


#define FILE_NAME_SIZE 128
#define FILE_PERMISSIONS 0644
#define ERROR_MESSAGE "\e[0;34m[PSAR LOG]\e[0m - Error"
#define TEST_FILE_FOLDER "files"
#define NUMBER_OF_FILES 3
#define NUMBER_OF_PROCESSES 2
#define PAGE_SIZE 4096 /* Todo: Add function to get this dynamically */


bool create_files();
bool create_processes();
bool check_directory();
bool add_content_files();
bool tmp_process_read_write();
void signalHandler(int sig, siginfo_t * si, void * unused);
bool setupSignalHandler();

#endif