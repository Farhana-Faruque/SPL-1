#ifndef IMAGE_H
#define IMAGE_H

// your code, including DictionaryEntry

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dictionary entry structure
typedef struct {
    int prefix;
    unsigned char value;
    int code;
} DictionaryEntry;

typedef struct {
    char magic[3];
    int width, height, maxval;
} PGMHeader;

// Function to read a line and skip comments
int readLine(FILE* file, char* buffer, int maxLen) {
    while (fgets(buffer, maxLen, file)) {
        if (buffer[0] != '#' && buffer[0] != '\n' && buffer[0] != '\0') {
            return 1;  // Successfully read a non-comment line
        }
    }
    return 0;  // EOF or error
}

#endif // COMPRESSION_H

