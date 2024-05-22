#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define FILE_SIZE 10485700
#define FILE_NAME "testfile.dat"
#define NUM_ITERATIONS 10

void generate_test_file(const char *filename, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    char *data = malloc(size);
    if (data == NULL) {
        perror("Error allocating memory");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < size; ++i) {
        data[i] = (char)(i % 256);
    }
    fwrite(data, 1, size, file);
    free(data);
    fclose(file);
}

double timed_io_operations(const char *filename) {
    clock_t start, end;
    char *buffer = malloc(FILE_SIZE);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        exit(EXIT_FAILURE);
    }

    start = clock();
    // Reading
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file for reading");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    fread(buffer, 1, FILE_SIZE, file);
    fclose(file);

    // Writing
    file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    fwrite(buffer, 1, FILE_SIZE, file);
    fclose(file);
    end = clock();

    free(buffer);
    return (double)(end - start) / CLOCKS_PER_SEC;
}

double timed_mmap_operations(const char *filename) {
    clock_t start, end;

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    start = clock();
    // Memory mapping
    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Simulate read and write
    for (size_t i = 0; i < FILE_SIZE; ++i) {
        char c = map[i]; // Read
        map[i] = c;      // Write
    }

    // Unmap and close
    if (munmap(map, FILE_SIZE) == -1) {
        perror("Error unmapping file");
    }
    close(fd);
    end = clock();

    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main() {
    generate_test_file(FILE_NAME, FILE_SIZE);

    double io_total_time = 0.0;
    double mmap_total_time = 0.0;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        io_total_time += timed_io_operations(FILE_NAME);
        mmap_total_time += timed_mmap_operations(FILE_NAME);
    }

    double io_avg_time = io_total_time / NUM_ITERATIONS;
    double mmap_avg_time = mmap_total_time / NUM_ITERATIONS;

    printf("Average I/O Operations Time: %f seconds\n", io_avg_time);
    printf("Average Memory Mapped File Operations Time: %f seconds\n", mmap_avg_time);

    return 0;
}
