#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 256  // For 8-bit grayscale values (0-255)

int RunP5() {
    char inputFile[256];
    printf("Enter the PGM file name: ");
    scanf("%255s", inputFile);

    FILE *input = fopen(inputFile, "rb");
    
    // Open input PGM file
    if (!input) {
        printf("Cannot open input file\n");
        return 1;
    }

    // Read PGM header
    char magic[3];
    int width, height, maxval;
    fscanf(input, "%2s\n%d %d\n%d\n", magic, &width, &height, &maxval);
    
    if (strcmp(magic, "P5") != 0 || maxval > 255) {
        printf("Unsupported PGM format\n");
        fclose(input);
        return 1;
    }

    // Read image data (binary format)
    int size = width * height;
    unsigned char* imageData = (unsigned char*)malloc(size);
    fread(imageData, 1, size, input);
    fclose(input);

    // Open output file
    FILE *output = fopen("compressed.bin", "wb");
    if (!output) {
        printf("Cannot create output file\n");
        free(imageData);
        return 1;
    }

    // Write header information
    fwrite(&width, sizeof(int), 1, output);
    fwrite(&height, sizeof(int), 1, output);

    // Compress using Run-Length Encoding
    unsigned char currentValue = imageData[0];
    unsigned char count = 1;
    
    for (int i = 1; i < size; i++) {
        if (imageData[i] == currentValue && count < 255) {
            count++;
        } else {
            // Write the run
            fwrite(&currentValue, sizeof(unsigned char), 1, output);
            fwrite(&count, sizeof(unsigned char), 1, output);
            
            // Start new run
            currentValue = imageData[i];
            count = 1;
        }
    }
    
    // Write the final run
    fwrite(&currentValue, sizeof(unsigned char), 1, output);
    fwrite(&count, sizeof(unsigned char), 1, output);

    // Cleanup
    fclose(output);
    free(imageData);

    printf("Compression completed successfully\n");
    return 0;
}