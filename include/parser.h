#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    char *contents;
    size_t file_size;
    long char_idx;
} FileParser;

bool reset_parser(FileParser *parser)
{
    parser->char_idx = 0;
    return true;
}

bool end_of_file(const FileParser *parser)
{
    return (parser->char_idx >= parser->file_size);
}

// Returns a null terminated string from the current character to the next newline (not including the newline)
// This is malloced to a new string, not just a pointer so the user has to free the string when done.
char *next_line(FileParser *parser)
{
    long start_idx = parser->char_idx;
    while (!end_of_file(parser) && parser->contents[parser->char_idx] != '\n')
    {
        ++parser->char_idx;
    }
    if (start_idx > parser->char_idx)
        return NULL;
    size_t line_size = (parser->char_idx - start_idx);

    char *result = (char *)malloc(line_size + 1); // +1 for null termination
    memcpy(result, &(parser->contents[start_idx]), line_size);

    ++parser->char_idx; // Don't put newline char in output, but skip over it in parser.

    result[line_size] = '\0'; // Add null byte at the end
    return result;
}

// Returns the next character in the file that is part of the valid tokens string.
// Returns null character if reached end of file before finding valid token
char next_token(FileParser *parser, const char *valid_tokens)
{
    char c;
    char *idx;

    if (end_of_file(parser))
        return EOF;

    do
    {
        c = parser->contents[parser->char_idx++];
        idx = strchr(valid_tokens, c);
    } while (idx == NULL && !end_of_file(parser));

    if (end_of_file(parser))
        return EOF;

    return c;
}

// Reads a file's contents from given file path and returns the size of malloced buffer
// The user must free the buffer themselves!
long read_file(const char *file_path, char **buf, const bool add_nul)
{
    FILE *fp;
    long off_end;
    size_t file_size;
    int rc;

    fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        return -1L;
    }

    rc = fseek(fp, 0L, SEEK_END);
    if (rc != 0)
    {
        return -1L;
    }

    off_end = ftell(fp);
    if (off_end < 0)
    {
        return -1L;
    }

    file_size = (size_t)off_end;

    *buf = (char *)malloc(file_size + (int)add_nul);
    if (*buf == NULL)
    {
        return -1L;
    }

    rewind(fp);

    if (fread(*buf, 1, file_size, fp) != file_size)
    {
        free(*buf);
        return -1L;
    }

    if (fclose(fp) == EOF)
    {
        free(*buf);
        return -1L;
    }

    if (add_nul)
    {
        (*buf)[file_size] = '\0';
    }

    return (long)file_size;
}

// Creates a file parser from given file path, optionally null-terminated if specified
bool from_file(const char *file_path, FileParser *parser)
{
    long file_size = read_file(file_path, &(parser->contents), true);
    if (file_size < 0)
    {
        return false;
    }
    parser->file_size = file_size;
    parser->char_idx = 0;
    return true;
}

#endif // PARSER_H