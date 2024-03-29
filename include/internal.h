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
#include <stdint.h>
#include <semaphore.h>
#include <time.h>
#include <stdarg.h>
#include "ptedit_header.h"


#define FILE_NAME_SIZE 128
#define FILE_PERMISSIONS 0644
#define ERROR_MESSAGE "\e[0;34m[PSAR LOG]\e[0m - Error"
#define TEST_FILE_FOLDER "files"
#define NUMBER_OF_FILES 1
#define NUMBER_OF_PROCESSES 1
#define PAGE_SIZE 4096 /* Todo: Add function to get this dynamically */
#define SEM_NAME "/pteditor_semaphore"
#define DATA_DEMO "Hello PSAR Project!"
#define WRITE_DEMO "WOW"
#define WRITE_OFFSET 10

typedef enum { LOG_INFO, LOG_ERROR, LOG_DEBUG, LOG_UPDATE } LogLevel;


bool create_files();
bool launch_processes();
bool check_directory();
bool add_content_files();
bool demo_read_write();
void signalHandler(int sig, siginfo_t * si, void * unused);
bool setupSignalHandler();
void log_message(LogLevel level, const char* format, ...);
void log_virtual_to_physical(void* address);
void* align_to_page_boundary(void* address);
bool psar_write(char *mapped_region, off_t offset, const char *data, size_t len, size_t region_size, int file_number);
bool ensure_directory_exists(const char* dir_path);
bool merge(const char* original_file_path, const char* log_file_path);
void setup();
void ensure_directories_exist();
void show_diff(const char *file1, const char *file2);

#endif