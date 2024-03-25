#include "internal.h"

bool create_processes() {
    int pids[NUMBER_OF_PROCESSES];
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            perror(ERROR_MESSAGE);
            return false;
        } else if (pids[i] == 0) {
            if (ptedit_init()) {
                printf("error ptedit init");
                return 1;
            }
            if(!tmp_process_read_write()){
                perror(ERROR_MESSAGE); 
                exit(EXIT_FAILURE);
            }
            ptedit_cleanup();
            exit(EXIT_SUCCESS);
        }
    }

    int status;
    for(int i=0; i < NUMBER_OF_PROCESSES; i++) {
        waitpid(pids[i], &status, 0);
        if(WIFEXITED(status)) {
            printf("Child process %d returned with status %d\n", pids[i], WEXITSTATUS(status));
        } else if(WIFSIGNALED(status)) {
            printf("Child process %d was terminated by a signal %d - ", pids[i], WTERMSIG(status));
            if(WTERMSIG(status) == SIGSEGV) {
                printf("That was due a segmentation fault\n");
            }
        }
    }

    printf("All child processes have finished\n");
    return true;
}

bool tmp_process_read_write() {
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
        // Let's trigger a sigsev...
        memset(mapped_region, 'Z', 1);
        printf("Reading new mapped region %c\n", mapped_region[0]);
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

// Function to log the virtual to physical address translation
void log_virtual_to_physical(void* address) {
    ptedit_entry_t entry = ptedit_resolve(address, 0);
    size_t pfn = ptedit_get_pfn(entry.pte);
    printf("Virtual address %p -> Physical Frame Number (PFN): %zu\n", address, pfn);
}

// Function to align an address to the nearest page boundary
void* align_to_page_boundary(void* address) {
    return (void*)((size_t)address & ~(PAGE_SIZE - 1));
}

void signalHandler(int sig, siginfo_t * si, void * unused) {
    printf("Handler caught SIGSEGV - write attempt by process %d\n", getpid());
    void * fault_addr = si->si_addr;
    fault_addr = align_to_page_boundary(fault_addr); // Ensure fault_addr is page-aligned
    printf("Faulting address (aligned): %p\n", fault_addr);

    // Log virtual to physical translation before changes
    printf("Before Update: ");
    log_virtual_to_physical(fault_addr);

    // Allocate a new page with read-write permissions
    void *new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_page == MAP_FAILED) {
        perror("Failed to map new page");
        _exit(EXIT_FAILURE);
    }
    printf("New page mapped at: %p\n", new_page);

    // Copy the content from the faulting page to the new page
    memcpy(new_page, fault_addr, PAGE_SIZE);
    printf("Content copied to new page by process %d\n", getpid());

    // Ensure the new page has read-write permissions
    if (mprotect(new_page, PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        perror("mprotect failed for new page");
        _exit(EXIT_FAILURE);
    }

    // Resolve the page table entry for both the faulting and new page addresses
    ptedit_entry_t fault_entry = ptedit_resolve(fault_addr, 0);
    ptedit_entry_t new_page_entry = ptedit_resolve(new_page, 0);

    // Map the page table for the faulting address into user space
    size_t pt_pfn = ptedit_cast(fault_entry.pmd, ptedit_pmd_t).pfn;
    char* pt = ptedit_pmap(pt_pfn * ptedit_get_pagesize(), ptedit_get_pagesize());

    // Calculate the index into the page table for the faulting address
    size_t entry_index = (((size_t)fault_addr) >> 12) & 0x1FF;
    size_t *mapped_entry = ((size_t *)pt) + entry_index;

    // Set the faulting address's page table entry to point to the new page's PFN
    *mapped_entry = ptedit_set_pfn(*mapped_entry, ptedit_get_pfn(new_page_entry.pte));

    // Invalidate the TLB for the faulting address to ensure the changes are applied
    ptedit_invalidate_tlb(fault_addr);

    // Log virtual to physical translation after changes
    printf("After Update: ");
    log_virtual_to_physical(fault_addr);

    sleep(2);
}

bool setupSignalHandler() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask); 
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction setup for SIGSEGV failed");
        return false;
    }
    return true;
}

/*parismollo@parismollo:~/Github/Master/S2/PSAR/PSAR$ getconf PAGESIZE 4096*/