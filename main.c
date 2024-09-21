#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/bf_ir.h"
#include "include/bf_to_bin.h"

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        fputs("ERROR: program not given\n", stderr);
        return 1;
    }
    char *p = argv[0];
    if (argc < 2)
    {
        fputs("ERROR: file not given\n", stderr);
        printf("Usage: %s <filename.bf>\n", p);
        return 1;
    }

    char *file_path = argv[1];
    bf_program program = create_bf_prog_from_file(file_path, MAX_OPS);

    size_t program_bin_size;
    void *program_memory_ptr;

    void *program_bin = bf_create_bin(&program, DEFAULT_MEMORY_SIZE, &program_bin_size, program_memory_ptr);
    if (program_bin == NULL)
    {
        fputs("error creating bin", stderr);
        return 1;
    }

    run(program_bin);
    munmap(program_bin, program_bin_size);
    munmap(program_memory_ptr, DEFAULT_MEMORY_SIZE);

    return 0;
}
