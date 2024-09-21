#ifndef BF_IR_H
#define BF_IR_H

#include "parser.h"
#include <assert.h>

#define MAX_OPS 1024 * 8
#define _JUMP_STACK_SIZE 1024

enum BF_OPERATOR
{
    OP_LFT = '<',
    OP_RGT = '>',
    OP_INC = '+',
    OP_DEC = '-',
    OP_PUT = '.',
    OP_GET = ',',
    OP_JIZ = '[',
    OP_JNZ = ']',
};

typedef struct
{
    enum BF_OPERATOR op;

    // For most operators represents the number of time it is repeated in a row.
    // For [ and ], represents the spot to go to
    unsigned long operand;
} bf_operation;

typedef struct
{
    bf_operation *ops;
    unsigned long ops_len;
} bf_program;

bool generate_bf_ops(bf_program *prog, FileParser *parser, size_t ops_capacity)
{
    reset_parser(parser);

    prog->ops = (bf_operation *)malloc(sizeof(bf_operation) * ops_capacity);

    static const char *bf_tokens = "<>+-.,[]";

    unsigned long jump_stack[_JUMP_STACK_SIZE];
    size_t stack_ptr = 0;

    char op_token = next_token(parser, bf_tokens);
    while (!end_of_file(parser))
    {

        bf_operation op;
        op.op = (enum BF_OPERATOR)op_token;

        switch (op_token)
        {
        case OP_LFT: // <
        case OP_RGT: // >
        case OP_INC: // +
        case OP_DEC: // -
        case OP_PUT: // .
        case OP_GET: // ,
        {
            op.operand = 1;
            char repeat_token = next_token(parser, bf_tokens);
            while (repeat_token == op_token && !end_of_file(parser))
            {
                op.operand += 1;
                repeat_token = next_token(parser, bf_tokens);
            }
            op_token = repeat_token; // When a new token is found, put it as the next token to check for repeats.
            break;
        }

        case OP_JIZ: // [
        {
            assert(stack_ptr < _JUMP_STACK_SIZE);
            jump_stack[stack_ptr++] = prog->ops_len;
            op_token = next_token(parser, bf_tokens);
            break;
        }

        case OP_JNZ: // ]
        {
            assert(stack_ptr > 0);
            unsigned long pair_idx = jump_stack[--stack_ptr];
            prog->ops[pair_idx].operand = prog->ops_len; // Make '[' op point to the ']' operator
            op.operand = pair_idx;                       // Make this (']') op point to the '[' operator
            op_token = next_token(parser, bf_tokens);
            break;
        }
        default:
            return true;
        }
        prog->ops[prog->ops_len++] = op;
    }
    return true;
}

bf_program create_bf_prog_from_file(char *file_path, size_t ops_capacity)
{
    FileParser file = {0};

    bf_program ret = {0};

    if (!from_file(file_path, &file))
    {
        fputs("Error opening file", stderr);
        return ret;
    }

    if (!generate_bf_ops(&ret, &file, ops_capacity))
        fputs("Error parsing brainf file", stderr);

    return ret;
}

#endif // BF_IR_H