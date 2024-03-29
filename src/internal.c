#include "internal.h"

void log_message(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';


    const char* color_red = "\033[1;31m";
    const char* color_blue = "\033[1;34m";
    const char* color_yellow = "\033[1;33m";
    const char* color_reset = "\033[0m";

    const char* level_strs[] = {"INFO", "ERROR", "DEBUG"};
    const char* level_colors[] = {color_blue, color_red, color_yellow};

    printf("%s[%s] [%s]%s ", level_colors[level], time_str, level_strs[level], color_reset);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

bool create_processes() {
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        log_message(LOG_ERROR, "sem_open failed: %s", strerror(errno));
        return false;
    }
    int pids[NUMBER_OF_PROCESSES];
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            sem_close(sem);
            sem_unlink(SEM_NAME);
            log_message(LOG_ERROR, "fork failed: %s", strerror(errno));
            return false;
        } else if (pids[i] == 0) {
            sem_wait(sem);
            if (ptedit_init()) {
                log_message(LOG_ERROR, "error ptedit init");
                sem_post(sem);
                return 1;
            }
            log_message(LOG_DEBUG, "Starting writing tests...\n");
            if(!process_read_write()){
                log_message(LOG_ERROR, "write failed: %s", strerror(errno)); 
                ptedit_cleanup();
                sem_post(sem);
                exit(EXIT_FAILURE);

            }
            ptedit_cleanup();
            sem_post(sem);
            exit(EXIT_SUCCESS);
        }
    }

    int status;
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        waitpid(pids[i], &status, 0);
        if(WIFEXITED(status)) {
            log_message(LOG_INFO, "Child process %d returned with status %d\n", pids[i], WEXITSTATUS(status));
        } else if(WIFSIGNALED(status)) {
            log_message(LOG_INFO,"Child process %d was terminated by a signal %d - ", pids[i], WTERMSIG(status));
            if(WTERMSIG(status) == SIGSEGV) {
                log_message(LOG_INFO, "That was due a segmentation fault\n");
            }
        }
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    log_message(LOG_INFO, "All child processes have finished\n");
    return true;
}

bool ensure_log_directory_exists(const char* dir_path) {
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) == -1) {
            log_message(LOG_ERROR, "Failed to create log directory: %s", strerror(errno));
            return false;
        }
    }
    return true;
}

bool psar_write(char *mapped_region, off_t offset, const char *data, size_t len, size_t region_size) {
    if (offset + len > region_size) {
        log_message(LOG_ERROR, "Write operation exceeds mapped region bounds.");
        return false;
    }

    char log_dir_path[256];
    snprintf(log_dir_path, sizeof(log_dir_path), "logs/logs_%d", getpid());

    
    if (!ensure_log_directory_exists(log_dir_path)) {
        return false;
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
    char log_file_path[512];
    snprintf(log_file_path, sizeof(log_file_path), "%s/log_%s.log", log_dir_path, timestamp);

    
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

    return true;
}

bool process_read_write() {
    int fd[NUMBER_OF_FILES];
    char file_name[FILE_NAME_SIZE];
    for(int i=0; i < NUMBER_OF_FILES; i++) {
        snprintf(file_name, FILE_NAME_SIZE, "files/file_%d", i);
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
        
        log_message(LOG_INFO, "Process %d reads %s before write: [%s]\n", getpid(), file_name, mapped_region);
        const char * testing = "SUPER COOL";
        psar_write(mapped_region, 2, testing, strlen(testing), st.st_size);
        const char * testing2 = "!!!!";
        psar_write(mapped_region, 12, testing2, strlen(testing2), st.st_size);
        log_message(LOG_INFO, "Process %d reading new mapped region [%s]\n", getpid(), mapped_region);
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
            log_message(LOG_ERROR, "open failed: %s", strerror(errno));
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
            log_message(LOG_ERROR, "open failed: %s", strerror(errno));
            return false;
        }
        
        const char * data = "Hello PSAR Project";
        ssize_t bytes = write(fd, data, strlen(data));
        
        if(bytes == -1) {
            log_message(LOG_ERROR, "write failed: %s", strerror(errno));
            return false;
        }
        
        close(fd);
    }
    return true;
}

bool check_directory() {
    if(mkdir(TEST_FILE_FOLDER, 0755)) {
        if(errno != EEXIST) {
            log_message(LOG_ERROR, "mkdir failed: %s", strerror(errno));
            return false;
        }
    }
    return true;
}

void log_virtual_to_physical(void* address) {
    ptedit_entry_t entry = ptedit_resolve(address, 0);
    size_t pfn = ptedit_get_pfn(entry.pte);
    log_message(LOG_INFO, "Virtual address %p -> Physical Frame Number (PFN): %zu\n", address, pfn);
}

void* align_to_page_boundary(void* address) {
    return (void*)((size_t)address & ~(PAGE_SIZE - 1));
}

void signalHandler(int sig, siginfo_t * si, void * unused) {
    log_message(LOG_INFO, "Handler caught SIGSEGV - write attempt by process %d\n", getpid());
    void * fault_addr = si->si_addr;
    fault_addr = align_to_page_boundary(fault_addr);
    log_message(LOG_INFO, "Faulting address (aligned): %p\n", fault_addr);

    log_virtual_to_physical(fault_addr);

    void *new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_page == MAP_FAILED) {
        log_message(LOG_ERROR, "mmap failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    log_message(LOG_INFO, "New page mapped at: %p\n", new_page);

    memcpy(new_page, fault_addr, PAGE_SIZE);
    log_message(LOG_INFO, "Content copied to new page by process %d\n", getpid());

    if (mprotect(new_page, PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        log_message(LOG_ERROR, "mprotect failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    ptedit_entry_t fault_entry = ptedit_resolve(fault_addr, 0); //
    ptedit_entry_t new_page_entry = ptedit_resolve(new_page, 0);

    size_t pt_pfn = ptedit_cast(fault_entry.pmd, ptedit_pmd_t).pfn;
    char* pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    size_t entry_index = (((size_t)fault_addr) >> 12) & 0x1FF;
    size_t *mapped_entry = ((size_t *)pt) + entry_index;

    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(new_page_entry.pte));

    ptedit_update(fault_addr, 0, &new_page_entry);

    ptedit_invalidate_tlb(fault_addr);

    log_virtual_to_physical(fault_addr);
}

bool setupSignalHandler() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask); 
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGSEGV, &sa, NULL) == -1) {
        log_message(LOG_ERROR, "sigaction setup for SIGSEGV failed: %s", strerror(errno));
        return false;
    }
    return true;
}