#include "api.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "init") == 0) {
        initialize_project_environment();
    } else if (strcmp(command, "test") == 0) {
        start_file_write_processes();
    } else if (strcmp(command, "merge") == 0) {
        if (argc != 6) {
            fprintf(stderr, "Usage: %s merge -s [source_file] -l [log_file]\n", argv[0]);
            return 1;
        }
        char *source_file = NULL;
        char *log_file = NULL;
        for (int i = 2; i < argc; i += 2) {
            if (strcmp(argv[i], "-s") == 0) {
                source_file = argv[i + 1];
            } else if (strcmp(argv[i], "-l") == 0) {
                log_file = argv[i + 1];
            }
        }
        if (source_file && log_file) {
            merge(source_file, log_file);
        } else {
            fprintf(stderr, "Missing arguments for merge\n");
            return 1;
        }
    } else if (strcmp(command, "merge_all") == 0) {
        if (argc != 4 || strcmp(argv[2], "-s") != 0) {
            fprintf(stderr, "Usage: %s merge_all -s [source_file]\n", argv[0]);
            return 1;
        }
        merge_all(argv[3]);
    } else {
        fprintf(stderr, "Unknown command '%s'\n", command);
        return 1;
    }

    return 0;
}
