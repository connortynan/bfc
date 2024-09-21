#ifndef BF_TO_BIN_H
#define BF_TO_BIN_H

#include "bf_ir.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>

#define DEFAULT_MEMORY_SIZE 30000

size_t bf_program_size(bf_program *prog)
{
    size_t size = 10 + 16; // init and exit code

    for (size_t i = 0; i < prog->ops_len; i++)
    {
        bf_operation op = prog->ops[i];
        switch (op.op)
        {
        case OP_LFT:
        case OP_RGT:
        {
            size += 7;
            break;
        }

        case OP_INC:
        case OP_DEC:
        {
            size += 3;
            break;
        }

        case OP_PUT:
        case OP_GET:
        {
            size += 19 + (9 * op.operand);
            break;
        }

        case OP_JIZ:
        case OP_JNZ:
        {
            size += 10;
            break;
        }
        }
    }

    return size;
}

void *allocate_memory(size_t size)
{
    void *ptr = mmap(0, size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        printf("mmap error\n");
        return NULL;
    }
    return ptr;
}

void add_nbyte_to_bin(char **program_ptr, const uint64_t bytes, const int n)
{
    for (int i = 0; i < n; i++)
    {
        *(*program_ptr)++ = (bytes >> (i * 8)) & 0xFF;
    }
}

void run(void *program_data)
{
    // Cast to function pointer and execute
    void (*program_func)() = (void (*)())program_data;
    program_func();
}

void *bf_create_bin(bf_program *prog, size_t memory_size, size_t *program_size, void *program_memory_ptr)
{
    *program_size = bf_program_size(prog);
    char *program_data = (char *)allocate_memory(*program_size);
    if (program_data == NULL)
    {
        return NULL;
    }

    // Keep track of current spot in program data
    char *ptr = program_data;

    program_memory_ptr = allocate_memory(memory_size);
    if (program_memory_ptr == NULL)
    {
        munmap(program_data, *program_size);
        return NULL;
    }

    char *jump_stack[_JUMP_STACK_SIZE];
    size_t stack_ptr = 0;

    // set rdi to beginning of tape memory
    // 10 bytes
    *ptr++ = 0x48; // REX
    *ptr++ = 0xbf; // movabs rdi
    add_nbyte_to_bin(&ptr, (uint64_t)program_memory_ptr & 0xFFFFFFFFFFFF, 8);

    for (int i = 0; i < prog->ops_len; i++)
    {
        bf_operation op = prog->ops[i];

        switch (op.op)
        {
        case OP_LFT:
        {
            // 7 bytes
            *ptr++ = 0x48;                                   // REX
            *ptr++ = 0x81;                                   // add/sub
            *ptr++ = 0xef;                                   // sub rdi, imm64
            add_nbyte_to_bin(&ptr, (uint32_t)op.operand, 4); // op.operand in imm64
            break;
        }
        case OP_RGT:
        {
            // 7 bytes
            *ptr++ = 0x48;                                   // REX
            *ptr++ = 0x81;                                   // add/sub
            *ptr++ = 0xc7;                                   // add rdi, imm64
            add_nbyte_to_bin(&ptr, (uint32_t)op.operand, 4); // op.operand in imm64
            break;
        }
        case OP_INC:
        {
            // 3 bytes
            *ptr++ = 0x80;                         // imm8
            *ptr++ = 0x07;                         // add
            *ptr++ = (uint8_t)(op.operand & 0xFF); // op.operand in imm8
            break;
        }
        case OP_DEC:
        {
            // 3 bytes
            *ptr++ = 0x80;                         // imm8
            *ptr++ = 0x2f;                         // sub
            *ptr++ = (uint8_t)(op.operand & 0xFF); // op.operand in imm8
            break;
        }
        case OP_PUT:
        {
            // 19 + 9*operand bytes

            *ptr++ = 0x57; // push rdi

            // mov rsi, rdi (set buf to tape ptr)
            *ptr++ = 0x48; // REX
            *ptr++ = 0x89; // mov
            *ptr++ = 0xfe; // rsi, rdi

            // mov rdi 1 (file = stdout)
            *ptr++ = 0x48;                          // REX
            *ptr++ = 0xc7;                          // mov imm
            *ptr++ = 0xc7;                          // rdi
            add_nbyte_to_bin(&ptr, (uint32_t)1, 4); // 1 in imm64

            // mov rdx 1 (buf size = 1 byte)
            *ptr++ = 0x48;                          // REX
            *ptr++ = 0xc7;                          // mov imm
            *ptr++ = 0xc2;                          // rdx
            add_nbyte_to_bin(&ptr, (uint32_t)1, 4); // 1 in imm64

            for (int j = 0; j < op.operand; j++)
            {
                // mov rax 1 (write syscall)
                // *gets overwritten during syscall, must call for repeat syscalls
                *ptr++ = 0x48;                          // REX
                *ptr++ = 0xc7;                          // mov imm
                *ptr++ = 0xc0;                          // rax
                add_nbyte_to_bin(&ptr, (uint32_t)1, 4); // 1 in imm64

                // syscall
                *ptr++ = 0x0f;
                *ptr++ = 0x05;
            }

            *ptr++ = 0x5f; // pop rdi
            break;
        }
        case OP_GET:
        {
            // 19 + 9*operand bytes

            *ptr++ = 0x57; // push rdi

            // mov rsi, rdi (set buf to tape ptr)
            *ptr++ = 0x48; // REX
            *ptr++ = 0x89; // mov
            *ptr++ = 0xfe; // rsi, rdi

            // mov rdi 0 (file = stdin)
            *ptr++ = 0x48;                          // REX
            *ptr++ = 0xc7;                          // mov imm
            *ptr++ = 0xc7;                          // rdi
            add_nbyte_to_bin(&ptr, (uint32_t)0, 4); // 0 in imm64

            // mov rdx 1 (buf size = 1 byte)
            *ptr++ = 0x48;                          // REX
            *ptr++ = 0xc7;                          // mov imm
            *ptr++ = 0xc2;                          // rdx
            add_nbyte_to_bin(&ptr, (uint32_t)1, 4); // 1 in imm64

            for (int j = 0; j < op.operand; j++)
            {
                // mov rax 0 (read syscall)
                // *gets overwritten during syscall, must call for repeat syscalls
                *ptr++ = 0x48;                          // REX
                *ptr++ = 0xc7;                          // mov imm
                *ptr++ = 0xc0;                          // rax
                add_nbyte_to_bin(&ptr, (uint32_t)0, 4); // 0 in imm64

                // syscall
                *ptr++ = 0x0f;
                *ptr++ = 0x05;
            }

            *ptr++ = 0x5f; // pop rdi
            break;
        }
        case OP_JIZ:
        {
            // 10 bytes
            // je offset is at (<stack stored addr> + 6)

            // mov al, byte [rdi]
            *ptr++ = 0x8a; // mov
            *ptr++ = 0x07; // al, [rdi]

            // cmp al, 0 (sets zero flag)
            *ptr++ = 0x3c; // cmp al
            *ptr++ = 0x00; // 0 in imm8

            // je <offset> (we leave blank for now as we don't know where ']' is)
            *ptr++ = 0x0f;
            *ptr++ = 0x84;                          // je (jump if zero)
            add_nbyte_to_bin(&ptr, (uint32_t)0, 4); // dummy offset

            jump_stack[stack_ptr++] = ptr;

            break;
        }
        case OP_JNZ:
        {
            // 10 bytes
            // desired jump for '[' is at (<current ptr> + 10)

            char *back_jump_addr = jump_stack[--stack_ptr]; // The first byte of the instruction following '['
            char *front_jump_addr = ptr + 10;               // The first byte of the instruction following ']'

            // int32_t je_offset = (int32_t)(???); // should be positive
            // int32_t jne_offset = (int32_t)(???); // should be negative
            int32_t offset = (int32_t)(front_jump_addr - back_jump_addr);

            // mov al, byte [rdi]
            *ptr++ = 0x8a; // mov
            *ptr++ = 0x07; // al, [rdi]

            // cmp al, 0 (sets zero flag)
            *ptr++ = 0x3c; // cmp al
            *ptr++ = 0x00; // 0 in imm8

            // je <offset>
            *ptr++ = 0x0f;
            *ptr++ = 0x85;                      // jne (jump if not zero)
            add_nbyte_to_bin(&ptr, -offset, 4); // offset to instruction after paired '['

            // We also have to change the dummy addr for the '[' instruction to the instruction after this ']'
            // (which happens to be ptr right now)
            char *save_ptr = ptr;
            ptr = back_jump_addr - 4;          // The byte offset begins
            add_nbyte_to_bin(&ptr, offset, 4); // update offset
            ptr = save_ptr;                    // Return to current ptr

            break;
        }
        }
    }

    // Exit cleanly
    // 16 bytes

    // mov rax 60 (exit syscall)
    *ptr++ = 0x48;                           // REX
    *ptr++ = 0xc7;                           // mov imm
    *ptr++ = 0xc0;                           // rax
    add_nbyte_to_bin(&ptr, (uint32_t)60, 4); // 60 in imm64

    // mov rdi 0 (exit code)
    *ptr++ = 0x48;                          // REX
    *ptr++ = 0xc7;                          // mov imm
    *ptr++ = 0xc7;                          // rdi
    add_nbyte_to_bin(&ptr, (uint32_t)0, 4); // 0 in imm64

    // syscall
    *ptr++ = 0x0f;
    *ptr++ = 0x05;

    return program_data;
}

#endif // BF_TO_BIN_H