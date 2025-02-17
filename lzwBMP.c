#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "image.h"

#define MAX_DICTIONARY_SIZE 4096
#define MAX_CODE_SIZE 12

// Initialize the LZW dictionary
void initLZWDictionary(int dictionary[MAX_DICTIONARY_SIZE][2]) {
    for (int i = 0; i < 256; i++) {
        dictionary[i][0] = i;  // The dictionary stores the RGB value (or a code) and its index
        dictionary[i][1] = -1;  // Indicates that there is no entry following it for now
    }
}

// Perform LZW encoding on the image pixels
void lzwCompress(RGB *pixels, int width, int height, FILE *output) {
    int dictionary[MAX_DICTIONARY_SIZE][2];
    initLZWDictionary(dictionary);

    int nextCode = 256;  // Codes 0-255 are already assigned in the dictionary
    int prevCode = -1;
    int currentCode = 0;
    int dictSize = 256;
    int bitsRequired = 9;  // LZW typically starts with 9-bit codes

    for (int i = 0; i < width * height; i++) {
        currentCode = (pixels[i].red << 16) | (pixels[i].green << 8) | pixels[i].blue;

        // Check if currentCode exists in the dictionary
        int found = 0;
        for (int j = 0; j < dictSize; j++) {
            if (dictionary[j][0] == currentCode) {
                found = 1;
                break;
            }
        }

        if (found) {
            // Continue with the current sequence
            prevCode = currentCode;
        } else {
            // Output the previous code
            fwrite(&prevCode, sizeof(int), 1, output);

            // Add currentCode to the dictionary
            if (dictSize < MAX_DICTIONARY_SIZE) {
                dictionary[dictSize][0] = currentCode;
                dictionary[dictSize][1] = prevCode;
                dictSize++;
                if (dictSize == (1 << bitsRequired)) {
                    bitsRequired++;
                }
            }

            // Set prevCode to current
            prevCode = currentCode;
        }
    }

    // Output the final code
    if (prevCode != -1) {
        fwrite(&prevCode, sizeof(int), 1, output);
    }
}

int LZW() {
    char inputFile[256];
    printf("Enter the BMP file name: ");
    scanf("%255s", inputFile);

    FILE *input = fopen(inputFile, "rb");

    if (!input) {
        printf("Error: Unable to open input BMP file.\n");
        return 1;
    }

    fseek(input, 18, SEEK_SET);  // Skip header to find width and height (assuming 24-bit image)
    int width, height;
    fread(&width, sizeof(int), 1, input);
    fread(&height, sizeof(int), 1, input);

    fseek(input, 54, SEEK_SET);  // Skip to the pixel data (assuming no palette)
    int rowSize = (width * 3 + 3) & ~3;  // Rows are padded to 4 bytes
    int pixelDataSize = rowSize * height;

    // Read the pixel data
    RGB *pixels = (RGB *)malloc(pixelDataSize);
    fread(pixels, pixelDataSize, 1, input);

    // Open the output file for the compressed data
    FILE *output = fopen("compressed.bin", "wb");
    if (!output) {
        printf("Error: Unable to open output file.\n");
        free(pixels);
        fclose(input);
        return 1;
    }

    // Perform LZW compression
    lzwCompress(pixels, width, height, output);

    // Cleanup
    fclose(input);
    fclose(output);
    free(pixels);

    printf("Image compression complete.\n");
    return 0;
}
