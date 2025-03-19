#ifndef IMAGE_H
#define IMAGE_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// BMP image structures

#pragma pack(push, 1)
typedef struct {
    unsigned short bftype;      // "BM"
    unsigned int bfSize;      // File size
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffbits;    // Offset to pixel data
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;

#pragma pack(pop) 


// Dictionary entry structure
typedef struct {
    int prefix;
    unsigned char value;
    int code;
} DictionaryEntry;

// pgm structure for header

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

