#include "internal.h"

int main(int argc, char * argv[]) {
    initialize_project_environment();
    log_message(LOG_INFO, "initialize_project_environment() - Successful\n");
    start_file_write_processes();
    log_message(LOG_INFO, "start_file_write_processes() - Successful\n");
    // merge_all("files/file0");
    // merge
    // merge_all
    // show_diff
    return 0;
}
