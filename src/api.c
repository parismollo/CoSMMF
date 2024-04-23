#include "api.h"

/*
This function will create NUMBER OF PROCESSES and each
child process will attempt to write on a read only file
*/
bool start_file_write_processes() {
    configure_signal_handlers();
    ptedit_init();
    int pids[NUMBER_OF_PROCESSES];
    int num_started = 0;
    bool all_success = true;
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            log_message(LOG_ERROR, "fork failed: %s", strerror(errno));
            all_success = false; 
            break;
        } else if (pids[i] == 0) {
            // log_message(LOG_UPDATE, "Process %d created", getpid());
            if(!perform_file_modifications()){
                log_message(LOG_ERROR, "write failed: %s", strerror(errno)); 
                exit(EXIT_FAILURE);

            }
            exit(EXIT_SUCCESS);
        }else {
            num_started++;
        }
    }

    if(!all_success) {
        for(int j = 0; j < num_started; j++) {
            kill(pids[j], SIGTERM);
            waitpid(pids[j], NULL, 0);
        }
    } else {
        int status;
        for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
            waitpid(pids[i], &status, 0);
            if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
                log_message(LOG_ERROR, "Child process %d did not exit successfully", pids[i]);
                all_success = false;
            }
        }
    }

    ptedit_cleanup();
    return all_success;
}

bool ensure_directory_exists(const char* dir_path) {
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) == -1) {
            perror("Failed to create directory");
            return false;
        }
    }
    return true;
}

/*
This function will write to a log file the modification to the file. The write will occur on a 
new write authorized mapped memory region.
*/
bool log_and_write_memory_region(char *mapped_region, off_t offset, const char *data, size_t len, size_t region_size, char * file_name) {
    if (offset + len > region_size) {
        log_message(LOG_ERROR, "Write operation exceeds mapped region bounds.");
        return false;
    }

    char log_dir_path[256];
    snprintf(log_dir_path, sizeof(log_dir_path), "logs/logs_%d", getpid());

    
    if (!ensure_directory_exists(log_dir_path)) {
        return false;
    }

    const char* original_file_name = strrchr(file_name, '/');
    if (!original_file_name) original_file_name = file_name;
    else original_file_name++;


    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
    char log_file_path[512];
    snprintf(log_file_path, sizeof(log_file_path), "%s/log_%s_%s.log", log_dir_path, original_file_name, timestamp);
    int log_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd == -1) {
        log_message(LOG_ERROR, "Failed to open log file: %s", strerror(errno));
        return false;
    }

    dprintf(log_fd, "Offset: %ld, Length: %zu, Data: ", (long)offset, len);
    write(log_fd, data, len);
    write(log_fd, "\n", 1);

    close(log_fd);

    memcpy(mapped_region + offset, data, len);
    // log_message(LOG_UPDATE, "Process %d logged %s", getpid(), log_file_path);
    return true;
}

/*
Function will copy original file contents, and write a new updated 
version else where based off the log information
*/
bool merge(const char* original_file_path, const char* log_file_path) {
    int original_fd = open(original_file_path, O_RDONLY);
    if (original_fd == -1) {
        log_message(LOG_ERROR, "Failed to open original file: %s", strerror(errno));
        return false;
    }

    int log_fd = open(log_file_path, O_RDONLY);
    if (log_fd == -1) {
        log_message(LOG_ERROR, "Failed to open log file: %s", strerror(errno));
        close(original_fd);
        return false;
    }

    char merged_file_path[512], merge_dir_path[256];
    const char* original_file_name = strrchr(original_file_path, '/') ? strrchr(original_file_path, '/') + 1 : original_file_path;
    const char* log_file_name = strrchr(log_file_path, '/') ? strrchr(log_file_path, '/') + 1 : log_file_path;

    snprintf(merge_dir_path, sizeof(merge_dir_path), "merge/merge_%s", original_file_name);
    if (!ensure_directory_exists(merge_dir_path)) {
        close(original_fd);
        close(log_fd);
        return false;
    }
    
    snprintf(merged_file_path, sizeof(merged_file_path), "%s/%s_%s", merge_dir_path, original_file_name, log_file_name);

    int merged_fd = open(merged_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (merged_fd == -1) {
        log_message(LOG_ERROR, "Failed to create merged file: %s", strerror(errno));
        close(original_fd);
        close(log_fd);
        return false;
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(original_fd, buffer, sizeof(buffer))) > 0) {
        if (write(merged_fd, buffer, bytes_read) != bytes_read) {
            log_message(LOG_ERROR, "Failed to write all bytes to merged file");
            close(original_fd);
            close(log_fd);
            close(merged_fd);
            return false;
        }
    }

    apply_merge(merged_fd, log_fd);

    close(original_fd);
    close(log_fd);
    close(merged_fd);
    // log_message(LOG_UPDATE, "merge created for file %s", original_file_name);
    return true;
}

void apply_merge(int to_fd, int from_fd) {
    char log_line[1024];
    ssize_t read_size;
    lseek(from_fd, 0, SEEK_SET);
    while ((read_size = read(from_fd, log_line, sizeof(log_line) - 1)) > 0) {
        log_line[read_size] = '\0';
        char* ptr = log_line;
        while (ptr < log_line + read_size) {
            char* end_ptr = strchr(ptr, '\n');
            if (!end_ptr) break;
            *end_ptr = '\0';
            off_t offset;
            size_t len;
            char data[512];
            if (sscanf(ptr, "Offset: %ld, Length: %zu, Data: %[^\n]", &offset, &len, data) == 3) {
                lseek(to_fd, offset, SEEK_SET);
                write(to_fd, data, strlen(data));
            }
            ptr = end_ptr + 1;
        }
    }
}

/*
This function will check if filename
is a log type of file and if it is from target
*/
bool is_log_file(const char *filename, const char *target) {
    const char *dot = strrchr(filename, '.');
    if (!dot || strcmp(dot, ".log") != 0) {
        return false;
    }

    const char *underscore = strchr(filename, '_');
    if (!underscore) {
        return false;
    }


    const char *subString = underscore + 1;
    if (strstr(subString, target) == subString) {
        return true;
    }

    return false;
}
/*
Function loop over every log folder inside logs and apply merge
for all log files corresponding to that source file
- incremental file updating
*/
bool merge_all(char * source_file_path) {
    int source_file_fd = open(source_file_path, O_RDONLY);
    if (source_file_fd == -1) {
        perror("Failed to open original file");
        return false;
    }

    const char* original_file_name = strrchr(source_file_path, '/');
    if (!original_file_name) original_file_name = source_file_path;
    else original_file_name++;

    char merged_file_path[512];
    snprintf(merged_file_path, sizeof(merged_file_path), "merge/merge_all_%s", original_file_name);
    int merged_all_fd = open(merged_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (merged_all_fd == -1) {
        perror("Failed to create merged file");
        close(source_file_fd);
        return false;
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(source_file_fd, buffer, sizeof(buffer))) > 0) {
        write(merged_all_fd, buffer, bytes_read);
    }

    char log_path[1024];
    DIR * d;
    struct dirent *dir;
    d = opendir("logs");
    if(d) {
        while((dir = readdir(d)) != NULL) {
            if(dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..")!=0) {
                char path[1024];
                snprintf(path, sizeof(path), "logs/%s", dir->d_name);
                DIR * subdir = opendir(path);
                if(subdir) {
                    struct dirent * subDirEntry;
                    while((subDirEntry = readdir(subdir)) !=  NULL) {
                        if(is_log_file(subDirEntry->d_name, original_file_name)) {
                            snprintf(log_path, sizeof(log_path), "logs/%s/%s", dir->d_name, subDirEntry->d_name);
                            printf("log_path: %s\n",log_path);
                            int log_fd = open(log_path, O_RDONLY);
                            if (log_fd == -1) {
                                perror("Failed to open log file");
                                return false;
                            }
                            apply_merge(merged_all_fd, log_fd);
                            close(log_fd);
                        }
                    }
                }
                closedir(subdir);
            }
        }
    }
    closedir(d);
    return true;
}

/*

This function will attempt a non authorized write to every read only file available at files folder
after mapping the file to read only memory.

*/
bool perform_file_modifications() {
    // log_message(LOG_UPDATE, "Process %d started reading and write routine", getpid());
    int fd[NUMBER_OF_FILES];
    char file_name[FILE_NAME_SIZE];
    int i = 0;
    for(; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file%d", i);
        fd[i] = open(file_name, O_RDONLY);
        
        if(fd[i] == -1) {
            log_message(LOG_ERROR, "file descriptor failed: %s", strerror(errno));
            return false;
        }

        struct stat st;
        if(fstat(fd[i], &st) == -1) {
            log_message(LOG_ERROR, "fstat failed: %s", strerror(errno));
            close(fd[i]);
            return false;
        }
        char * mapped_region = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd[i], 0);
        if(mapped_region == MAP_FAILED) {
            log_message(LOG_ERROR, "mmap failed: %s", strerror(errno));
            close(fd[i]);
            return false;
        }
        // log_message(LOG_DEBUG, "File size: %zu, Write offset: %d, Data length: %zu", st.st_size, WRITE_OFFSET, strlen(WRITE_DEMO));
        log_and_write_memory_region(mapped_region, WRITE_OFFSET, WRITE_DEMO, strlen(WRITE_DEMO), st.st_size, file_name);
        munmap(mapped_region, st.st_size);
        close(fd[i]);
    }
    // log_message(LOG_UPDATE, "Process %d modified %d files", getpid(), i);
    return true;
}

void initialize_project_environment() {
    /*This function will set up the project folder so that we can test the tool pteditor*/
    create_required_directories();
    create_initial_project_files();
    write_initial_data_to_files();
    configure_signal_handlers();
}

void create_required_directories() {
    const char* directories[] = {"logs", "merge", "files"};
    size_t num_directories = sizeof(directories) / sizeof(directories[0]);
    for (size_t i = 0; i < num_directories; ++i) {
        struct stat st = {0};
        if (stat(directories[i], &st) == -1) {
            if (mkdir(directories[i], 0755) == -1) {
                perror("Failed to create directory");
            }
        }
    }
    // log_message(LOG_UPDATE, "Folders log, merge and files created");
}

bool create_initial_project_files() {
    char file_name[FILE_NAME_SIZE];
    int fd;
    int i = 0;
    for(; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file%d", i);
        fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
        if(fd == -1) {
            log_message(LOG_ERROR, "open failed: %s", strerror(errno));
            return false;
        }
        close(fd);
    }
    // log_message(LOG_UPDATE, "%d files were created", i);
    return true;
}

bool write_initial_data_to_files() {
    char file_name[FILE_NAME_SIZE];
    int fd;
    for(int i=0; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file%d", i);
        fd = open(file_name, O_WRONLY, FILE_PERMISSIONS);
        if(fd == -1) {
            log_message(LOG_ERROR, "open failed: %s", strerror(errno));
            return false;
        }
        ssize_t bytes = write(fd, DATA_DEMO, strlen(DATA_DEMO));        
        if(bytes == -1) {
            log_message(LOG_ERROR, "write failed: %s", strerror(errno));
            return false;
        }
        close(fd);
    }
    // log_message(LOG_UPDATE, "Data written to files");
    return true;
}

// void log_virtual_to_physical(void* address) {
//     ptedit_entry_t entry = ptedit_resolve(address, 0);
//     size_t pfn = ptedit_get_pfn(entry.pte);
//     //log_message(LOG_INFO, "Virtual address %p -> Physical Frame Number (PFN): %zu\n", address, pfn);
// }

void* align_to_page_boundary(void* address) {
    return (void*)((size_t)address & ~(PAGE_SIZE - 1));
}

/*
This function will update the PFN of the virtual address of where the segmentation fault occured
It will then point to a valid write/read mapped memory region where we will write the modifications.
Reminder: Page Frame Number (PFN) is an index into the physical memory of a computer
*/
void signal_handler(int sig, siginfo_t * si, void * unused) {
    // log_message(LOG_INFO, "Handler caught SIGSEGV - write attempt by process %d\n", getpid());
    void * fault_addr = si->si_addr;
    fault_addr = align_to_page_boundary(fault_addr);
    //log_message(LOG_INFO, "Faulting address (aligned): %p\n", fault_addr);

    void *new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_page == MAP_FAILED) {
        log_message(LOG_ERROR, "mmap failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    // log_message(LOG_INFO, "New page mapped at: %p\n", new_page);

    memcpy(new_page, fault_addr, PAGE_SIZE);
    //log_message(LOG_INFO, "Content copied to new page by process %d\n", getpid());

    if (mprotect(new_page, PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        log_message(LOG_ERROR, "mprotect failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    ptedit_entry_t fault_entry = ptedit_resolve(fault_addr, 0);
    ptedit_entry_t new_page_entry = ptedit_resolve(new_page, 0);

    size_t pt_pfn = ptedit_cast(fault_entry.pmd, ptedit_pmd_t).pfn;
    char* pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    size_t entry_index = (((size_t)fault_addr) >> 12) & 0x1FF;
    size_t *mapped_entry = ((size_t *)pt) + entry_index;

    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(new_page_entry.pte));

    ptedit_update(fault_addr, 0, &new_page_entry);

    ptedit_invalidate_tlb(fault_addr);

    // log_message(LOG_UPDATE, "Process %d updated virtual address %p to new physical address %zu", getpid(), fault_addr, (new_page_entry.pte));
}

bool configure_signal_handlers() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask); 
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGSEGV, &sa, NULL) == -1) {
        log_message(LOG_ERROR, "sigaction setup for SIGSEGV failed: %s", strerror(errno));
        return false;
    }
    // log_message(LOG_UPDATE, "SIGSEGV Signal Handler updated");
    return true;
}

void show_diff(const char *file1, const char *file2) {
    char command[256];
    snprintf(command, sizeof(command), "diff %s %s", file1, file2);
    system(command);
}

void log_message(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';

    const char* color_red = "\033[1;31m";
    const char* color_blue = "\033[1;34m";
    const char* color_yellow = "\033[1;33m";
    const char* color_green = "\033[1;32m";
    const char* color_reset = "\033[0m";

    const char* level_strs[] = {"INFO", "END", "DEBUG", "UPDATE"};
    const char* level_colors[] = {color_blue, color_red, color_yellow, color_green};

    printf("%s[%s] [%s]%s ", level_colors[level], time_str, level_strs[level], color_reset);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}