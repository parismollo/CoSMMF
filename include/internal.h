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
#include <time.h>
#include <stdarg.h>
#include <dirent.h>
#include "ptedit_header.h"


#define FILE_NAME_SIZE 128
#define FILE_PERMISSIONS 0644
#define TEST_FILE_FOLDER "files"
#define NUMBER_OF_FILES 1
#define NUMBER_OF_PROCESSES 1
#define PAGE_SIZE 4096
#define DATA_DEMO "------------ Hello World! ------------"
#define WRITE_DEMO "xxx"
#define WRITE_OFFSET 15

typedef enum { LOG_INFO, LOG_ERROR, LOG_DEBUG, LOG_UPDATE } LogLevel;


bool create_initial_project_files();
bool start_file_write_processes();
bool write_initial_data_to_files();
bool perform_file_modifications();
void signalHandler(int sig, siginfo_t * si, void * unused);
bool configure_signal_handlers();
void log_message(LogLevel level, const char* format, ...);
void log_virtual_to_physical(void* address);
void* align_to_page_boundary(void* address);
bool log_and_write_memory_region(char *mapped_region, off_t offset, const char *data, size_t len, size_t region_size, char * file_name);
bool ensure_directory_exists(const char* dir_path);
bool merge(const char* original_file_path, const char* log_file_path);
void initialize_project_environment();
void create_required_directories();
void show_diff(const char *file1, const char *file2);
bool merge_all(char * source_file_path);
bool isLogFile(const char *filename, const char *target);
void applyMerge(int to_fd, int from_fd);

#endif