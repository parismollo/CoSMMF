#include "internal.h"

int main(int argc, char * argv[]) {
    setup();
    log_message(LOG_INFO, "Project Setup - Completed\n");
    launch_processes();
    log_message(LOG_INFO, "Processes Launch - Completed\n");
    return 0;
}
