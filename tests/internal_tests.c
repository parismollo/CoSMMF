#include "internal.h"

int main(int argc, char * argv[]) {
    setup();
    log_message(LOG_INFO, "Project Setup - Completed\n");
    launch_processes();
    log_message(LOG_INFO, "Processes Launch - Completed\n");
    merge_all("files/file0");
    // merge
    // merge_all
    // show_diff
    return 0;
}
