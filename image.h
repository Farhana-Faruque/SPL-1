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
    unsigned short sign;      // "BM"
    unsigned int Size;      // File size
    unsigned short Rs1;
    unsigned short Rs2;
    unsigned int Offbits;    // Offset to pixel data
} BmpFile;

typedef struct {
    unsigned int biSize;
    int Width;
    int Height;
    unsigned short Planes;
    unsigned short BitCount;
    unsigned int Compression;
    unsigned int SizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int ClrUsed;
    unsigned int ClrImportant;
} BmpInfo;

#pragma pack(pop) 

// Dictionary entry structure
typedef struct {
    int prefix;
    unsigned char value;
    int code;
} DictionaryEntry;

// pgm structure for header

typedef struct {
    char sign[3];
    int width, height, maxIntensity;
} PGMHeader;

int readLine(FILE* file, char* buffer, int maxLen) {
    while (fgets(buffer, maxLen, file)) {
        if (buffer[0] != '#' && buffer[0] != '\n' && buffer[0] != '\0') {
            return 1; 
        }
    }
    return 0;
}

#endif // COMPRESSION_H