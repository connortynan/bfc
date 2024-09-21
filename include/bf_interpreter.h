#ifndef BF_INTER_H
#define BF_INTER_H

#include "bf_ir.h"
#include "iomode.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_MEMORY_SIZE 30000

enum MEMORY_OVERFLOW_BEHAVIOR
{
    OF_UNDEFINED,
    OF_WRAP,
    OF_ABORT
};

void bf_print_ops(const bf_program *prog)
{
    for (size_t i = 0; i < prog->ops_len; i++)
    {
        bf_operation op = prog->ops[i];
        printf(" %ld: op %c, operand %ld\n", i, op.op, op.operand);
    }
}

bool bf_run(const bf_program *prog, size_t memory_size)
{
    struct termios orig_termios;

    // Enable raw mode
    enableRawMode(&orig_termios);

    unsigned char *prog_memory = (unsigned char *)malloc(memory_size);
    memset(prog_memory, 0, memory_size);

    size_t ptr = 0;

    for (int i = 0; i < prog->ops_len; i++)
    {
        bf_operation op = prog->ops[i];

        switch (op.op)
        {
        case OP_LFT:
        {
            if (ptr < op.operand)
            {
                fputs("  ERROR IN BF: memory underflow", stderr);
                disableRawMode(&orig_termios);
                return false;
            }
            ptr -= op.operand;
            break;
        }
        case OP_RGT:
        {
            if (ptr >= memory_size - op.operand)
            {
                fputs("  ERROR IN BF: memory overflow", stderr);
                disableRawMode(&orig_termios);
                return false;
            }
            ptr += op.operand;
            break;
        }
        case OP_INC:
        {
            prog_memory[ptr] = ((int)prog_memory[ptr] + op.operand) & 0xFF;
            break;
        }
        case OP_DEC:
        {
            prog_memory[ptr] = ((int)prog_memory[ptr] - op.operand) & 0xFF;
            break;
        }
        case OP_PUT:
        {
            for (int j = 0; j < op.operand; j++)
            {
                printf("%c", prog_memory[ptr]);
            }
            break;
        }
        case OP_GET:
        {
            for (int j = 0; j < op.operand; j++)
            {
                prog_memory[ptr] = getchar();
            }
            break;
        }
        case OP_JIZ:
        {
            if (prog_memory[ptr] == 0)
                i = op.operand;
            break;
        }
        case OP_JNZ:
        {
            if (prog_memory[ptr] != 0)
                i = op.operand;
            break;
        }
        }
    }
    disableRawMode(&orig_termios);
    return true;
}

#endif // BF_INTER_H