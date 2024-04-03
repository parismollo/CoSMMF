#include "internal.h"

int main(int argc, char * argv[]) {
    setup();
    log_message(LOG_INFO, "Project Setup - Completed\n");
    launch_processes();
    log_message(LOG_INFO, "Processes Launch - Completed\n");
    merge("files/file0", "logs/logs_5105/log_file0_20240403_141513.log");
    log_message(LOG_INFO, "Merge requested - Completed");
    // show_diff("files/file_0", "merge/merge_file_0/file_0_log_f0_20240329_160124.log");
    // log_message(LOG_INFO, "Differences displayed - Completed");
    // log_message(LOG_ERROR, "Finished");
    return 0;
}
