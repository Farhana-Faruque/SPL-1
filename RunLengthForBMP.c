#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "image.h"

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPHeader;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;

#pragma pack(pop)

void applyRLE(RGB *pixels, int width, int height, FILE *output) {
    int i = 0;
    while (i < width * height) {
        RGB currentPixel = pixels[i];
        int count = 1;

        // Count the consecutive pixels with the same color
        while (i + count < width * height &&
               pixels[i + count].red == currentPixel.red &&
               pixels[i + count].green == currentPixel.green &&
               pixels[i + count].blue == currentPixel.blue) {
            count++;
        }

        // Write the pixel and its count to the output file
        fwrite(&count, sizeof(int), 1, output);
        fwrite(&currentPixel, sizeof(RGB), 1, output);

        // Move the pointer forward by the count
        i += count;
    }
}

int RLE() {
    char inputFile[256];
    printf("Enter the BMP file name: ");
    scanf("%255s", inputFile);

    FILE *input = fopen(inputFile, "rb");

    if (!input) {
        printf("Error: Unable to open input BMP file.\n");
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    // Read BMP header and info header
    fread(&header, sizeof(BMPHeader), 1, input);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, input);

    if (header.bfType != 0x4D42) { // Check if the file is a BMP
        printf("Error: Not a BMP file.\n");
        fclose(input);
        return 1;
    }

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;

    // Calculate padding for each row
    int rowSize = (3 * width + 3) & ~3; // Each row is padded to the nearest 4-byte boundary
    int dataSize = rowSize * height;

    // Read the pixel data
    RGB *pixels = (RGB *)malloc(dataSize);
    fseek(input, header.bfOffBits, SEEK_SET);
    fread(pixels, dataSize, 1, input);

    // Open output file for compressed data
    FILE *output = fopen("compressed.bin", "wb");
    if (!output) {
        printf("Error: Unable to open output file.\n");
        free(pixels);
        fclose(input);
        return 1;
    }

    // Apply Run-Length Encoding to the pixel data
    applyRLE(pixels, width, height, output);

    // Clean up and close files
    fclose(input);
    fclose(output);
    free(pixels);

    printf("Image compression complete.\n");
    return 0;
}
