// gcc main.c -o main.o

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
// #include "test_framework.h"

#define MAX_PATH_LENGTH 1024

// void run_tests_for_file(const char* file_path) {
//     printf("Running tests for file: %s\n", file_path);

//     // Compile the C file into a shared library
//     char command[MAX_PATH_LENGTH * 2];
//     snprintf(command, MAX_PATH_LENGTH * 2, "gcc -shared -fPIC %s -o %s.so", file_path, file_path);
//     system(command);

//     // Load the shared library
//     void* library = dlopen(file_path, RTLD_LAZY);
//     if (!library) {
//         printf("Error: Failed to load shared library for file: %s\n", file_path);
//         printf("%s\n", dlerror());
//         return;
//     }

//     // Find the test functions in the shared library
//     size_t num_tests = 0;
//     const char** test_names = NULL;
//     find_test_functions(library, &num_tests, &test_names);
//     printf("Found %zu test(s) in file.\n", num_tests);

//     // Run each test function
//     for (size_t i = 0; i < num_tests; i++) {
//         printf("Running test: %s\n", test_names[i]);
//         run_test(library, test_names[i]);
//     }

//     // Unload the shared library
//     dlclose(library);

//     // Delete the shared library file
//     snprintf(command, MAX_PATH_LENGTH * 2, "rm -f %s.so", file_path);
//     system(command);
// }

void find_files_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        printf("Error: Failed to open directory '%s'.\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1)
        {
            printf("Error: Failed to get information for file '%s'.\n", full_path);
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            // Recurse into subdirectory
            find_files_recursive(full_path);
        }
        else if (S_ISREG(st.st_mode))
        {
            // Found a regular file, process it
            printf("%s\n", full_path);
        }
    }

    closedir(dir);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    find_files_recursive(argv[1]);
    return 0;
}
