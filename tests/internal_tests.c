#include "internal.h"

int main(int argc, char * argv[]) {
    
    if(ptedit_init()) {
        printf("Failed to initialize PTEditor. Please ensure the kernel module is loaded.\n");
        return -1;
    }

    create_files(5);
    add_content_files();
    setupSignalHandler();
    create_processes();

    ptedit_cleanup();

    return 0;
}
