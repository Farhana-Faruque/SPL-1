#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_DICT_SIZE 4096  // 12-bit codes
#define BITS_PER_CODE 12
#define MAX_CODE ((1 << BITS_PER_CODE) - 1)

int LZWP2() {
    char inputFile[256];
    printf("Enter the PGM file name: ");
    scanf("%255s", inputFile);

    char outputFile[] = "compressed.bin";

    FILE *input = fopen(inputFile, "r");
    FILE *output = fopen(outputFile, "wb");
    if (!input || !output) {
        printf("Cannot create output file\n");
        return 1;
    }

    // Read PGM header
    PGMHeader pgm;
    fscanf(input, "%2s\n%d %d\n%d\n", pgm.magic, &pgm.width, &pgm.height, &pgm.maxval);
    
    if ((strcmp(pgm.magic, "P2") != 0 && strcmp(pgm.magic, "P5") != 0) || pgm.maxval > 255) {
        printf("Unsupported PGM format\n");
        fclose(input);
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, sizeof(pgm), SEEK_SET);

    int size2 = pgm.width * pgm.height;  // Correct pixel count
    unsigned char* imageData = (unsigned char*)malloc(size2);
    
    // Read image data (ASCII format)
    if (strcmp(pgm.magic, "P2") != 0 || pgm.maxval > 255)
    {
        for (int i = 0; i < size2; i++) {
            int pixel;
            fscanf(input, "%d", &pixel);
            imageData[i] = (unsigned char)pixel;
        }
        fclose(input);
    }
    else{
        fread(imageData, 1, size2, input);
        fclose(input);
    }
    
    // Initialize dictionary
    DictionaryEntry* dictionary = (DictionaryEntry*)malloc(MAX_DICT_SIZE * sizeof(DictionaryEntry));
    int dictSize = 256;
    for (int i = 0; i < 256; i++) {
        dictionary[i].prefix = -1;
        dictionary[i].character = (unsigned char)i;
    }

    // Write header information
    fwrite(&pgm.width, sizeof(int), 1, output);
    fwrite(&pgm.height, sizeof(int), 1, output);

    // LZW compression
    unsigned int buffer = 0;
    int bitsInBuffer = 0;
    int currentCode = imageData[0];
    int nextCode = 256;

    for (int i = 1; i < size2; i++) {
        unsigned char nextChar = imageData[i];
        int tempCode = currentCode;

        // Check if currentCode + nextChar exists in dictionary
        int found = 0;
        for (int j = 0; j < dictSize; j++) {
            if (dictionary[j].prefix == currentCode && dictionary[j].character == nextChar) {
                currentCode = j;
                found = 1;
                break;
            }
        }

        if (!found) {
            // Output current code
            buffer = (buffer << BITS_PER_CODE) | currentCode;
            bitsInBuffer += BITS_PER_CODE;

            while (bitsInBuffer >= 8) {
                unsigned char byte = (buffer >> (bitsInBuffer - 8)) & 0xFF;
                fwrite(&byte, 1, 1, output);
                bitsInBuffer -= 8;
                buffer &= (1 << bitsInBuffer) - 1;
            }

            // Add new code to dictionary if not full
            if (dictSize < MAX_DICT_SIZE) {
                dictionary[dictSize].prefix = currentCode;
                dictionary[dictSize].character = nextChar;
                dictSize++;
            }

            currentCode = nextChar;
        }
    }

    // Output final code
    buffer = (buffer << BITS_PER_CODE) | currentCode;
    bitsInBuffer += BITS_PER_CODE;
    while (bitsInBuffer > 0) {
        int shift = bitsInBuffer - 8;
        if (shift < 0) shift = 0;
        unsigned char byte = (buffer >> shift) & 0xFF;
        fwrite(&byte, 1, 1, output);
        bitsInBuffer -= 8;
        if (bitsInBuffer < 0) bitsInBuffer = 0;
        buffer &= (1 << bitsInBuffer) - 1;
    }

    // Cleanup
    fclose(output);
    free(imageData);
    free(dictionary);
    printf("Compression complete: %s -> %s \n", inputFile, outputFile);

    // Get original file size
    printf("Original size: %ld bytes\n", size);
    
    // Get compressed file size
    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - ((float)compressed_size / size)) * 100);

    return 0;
}
