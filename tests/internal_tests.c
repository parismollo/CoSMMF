#include "internal.h"

int main(int argc, char * argv[]) {
    create_files(5);
    add_content_files();
    setupSignalHandler();
    create_processes();
}