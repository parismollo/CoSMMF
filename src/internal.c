#include "internal.h"

typedef enum { LOG_INFO, LOG_ERROR, LOG_DEBUG } LogLevel;

void log_message(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    // Get current time
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove the newline at the end

    // ANSI color codes
    const char* color_red = "\033[1;31m";
    const char* color_blue = "\033[1;34m";
    const char* color_yellow = "\033[1;33m";
    const char* color_reset = "\033[0m";

    // Log level strings and colors
    const char* level_strs[] = {"INFO", "ERROR", "DEBUG"};
    const char* level_colors[] = {color_blue, color_red, color_yellow};

    // Print the time stamp, level, and message with appropriate color
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
            if(!tmp_process_read_write()){
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

bool tmp_process_read_write() {
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
        
        log_message(LOG_INFO, "Process %d reads first letter of %s : [%c]\n", getpid(), file_name, mapped_region[0]);
        // Let's trigger a sigsev...
        memset(mapped_region, 'Z', 1);
        log_message(LOG_INFO, "Process %d reading new mapped region [%c]\n", getpid(), mapped_region[0]);
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

// Function to log the virtual to physical address translation
void log_virtual_to_physical(void* address) {
    ptedit_entry_t entry = ptedit_resolve(address, 0);
    size_t pfn = ptedit_get_pfn(entry.pte);
    log_message(LOG_INFO, "Virtual address %p -> Physical Frame Number (PFN): %zu\n", address, pfn);
}

// Function to align an address to the nearest page boundary
void* align_to_page_boundary(void* address) {
    return (void*)((size_t)address & ~(PAGE_SIZE - 1));
}

void signalHandler(int sig, siginfo_t * si, void * unused) {
    log_message(LOG_INFO, "Handler caught SIGSEGV - write attempt by process %d\n", getpid());
    void * fault_addr = si->si_addr;
    fault_addr = align_to_page_boundary(fault_addr); // Ensure fault_addr is page-aligned
    log_message(LOG_INFO, "Faulting address (aligned): %p\n", fault_addr);

    // Log virtual to physical translation before changes
    log_message(LOG_INFO, "Before Update->");
    log_virtual_to_physical(fault_addr);

    // Allocate a new page with read-write permissions
    void *new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_page == MAP_FAILED) {
        log_message(LOG_ERROR, "mmap failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    log_message(LOG_INFO, "New page mapped at: %p\n", new_page);

    // Copy the content from the faulting page to the new page
    memcpy(new_page, fault_addr, PAGE_SIZE);
    log_message(LOG_INFO, "Content copied to new page by process %d\n", getpid());

    // Ensure the new page has read-write permissions
    if (mprotect(new_page, PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        log_message(LOG_ERROR, "mprotect failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    // Resolve the page table entry for both the faulting and new page addresses
    ptedit_entry_t fault_entry = ptedit_resolve(fault_addr, 0); //
    ptedit_entry_t new_page_entry = ptedit_resolve(new_page, 0);

    // Map the page table for the faulting address into user space
    size_t pt_pfn = ptedit_cast(fault_entry.pmd, ptedit_pmd_t).pfn;
    char* pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    // Calculate the index into the page table for the faulting address
    size_t entry_index = (((size_t)fault_addr) >> 12) & 0x1FF;
    size_t *mapped_entry = ((size_t *)pt) + entry_index;

    // Set the faulting address's page table entry to point to the new page's PFN
    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(new_page_entry.pte));

    ptedit_update(fault_addr, 0, &new_page_entry);

    // Invalidate the TLB for the faulting address to ensure the changes are applied
    ptedit_invalidate_tlb(fault_addr);

    // Log virtual to physical translation after changes
    log_message(LOG_INFO, "After Update->");
    log_virtual_to_physical(fault_addr);

    // sleep(2);
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
/*parismollo@parismollo:~/Github/Master/S2/PSAR/PSAR$ getconf PAGESIZE 4096*/