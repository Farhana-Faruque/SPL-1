#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256

int decompressRunLength2(const char* inputFile, const char* outputFile) {
    // Open input file
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }

    // Read header information
    PGMHeader pgm;
    // Read RLE header
    fscanf(input, "%2s", pgm.magic);
    fscanf(input, "%d %d", &pgm.width, &pgm.height);
    fscanf(input, "%d", &pgm.maxval);
    fgetc(input); // Consume newline

    // Calculate total pixels
    int totalPixels = pgm.width * pgm.height;
    unsigned char* decompressedData = (unsigned char*)malloc(totalPixels);
    if (!decompressedData) {
        printf("Memory allocation failed\n");
        fclose(input);
        return 1;
    }

    // Decompress using Run-Length Decoding
    unsigned char value;
    unsigned char count;
    int pixelsWritten = 0;
    
    while (pixelsWritten < totalPixels && 
           fread(&value, sizeof(unsigned char), 1, input) == 1 &&
           fread(&count, sizeof(unsigned char), 1, input) == 1) {
        for (int i = 0; i < count && pixelsWritten < totalPixels; i++) {
            decompressedData[pixelsWritten] = value;
            pixelsWritten++;
        }
    }

    // Close input file
    fclose(input);

    // Verify we got all pixels
    if (pixelsWritten != totalPixels) {
        printf("Error: Decompressed pixel count (%d) doesn't match expected (%d)\n",
               pixelsWritten, totalPixels);
        free(decompressedData);
        return 1;
    }

    // Open output file
    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file: %s\n", outputFile);
        free(decompressedData);
        return 1;
    }

    // Write PGM header and data
    if (strcmp(pgm.magic, "P2") == 0) {
        // ASCII format
        fprintf(output, "P2\n%d %d\n%d\n", pgm.width, pgm.height, pgm.maxval);
        for (int i = 0; i < totalPixels; i++) {
            fprintf(output, "%d", decompressedData[i]);
            if ((i + 1) % pgm.width == 0) {
                fprintf(output, "\n");
            } else {
                fprintf(output, " ");
            }
        }
    } else if (strcmp(pgm.magic, "P5") == 0) {
        // Binary format
        fprintf(output, "P5\n%d %d\n%d\n", pgm.width, pgm.height, pgm.maxval);
        fwrite(decompressedData, sizeof(unsigned char), totalPixels, output);
    } else {
        printf("Unsupported format: %s\n", pgm.magic);
        fclose(output);
        free(decompressedData);
        return 1;
    }

    // Cleanup
    fclose(output);
    free(decompressedData);

    return 0;  // Success
}

// Example usage
int decompressRunLength() {
    // Example call - replace with your actual file names and format
    int result = decompressRunLength2("compressed.bin", "output.pgm");
    if (result == 0) {
        printf("Decompression successful\n");
    } else {
        printf("Decompression failed\n");
    }
    return result;
}