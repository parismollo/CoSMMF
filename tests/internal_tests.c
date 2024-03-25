#include "internal.h"

int main(int argc, char * argv[]) {
    create_files();
    setupSignalHandler();
    add_content_files();
    create_processes();
    return 0;
}
