#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256  // For 8-bit grayscale values (0-255)

int RunLengthPGM() {
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
    PGMHeader pgm;// Read PGM header
    fscanf(input, "%2s", pgm.magic);
    fscanf(input, "%d %d", &pgm.width, &pgm.height);
    fscanf(input, "%d", &pgm.maxval);
    fgetc(input); // Consume newline
    
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

    // Write header information
    fprintf(output, "%2s\n%d %d\n%d\n",pgm.magic, pgm.width, pgm.height, pgm.maxval);

    // Compress using Run-Length Encoding
    unsigned char currentValue = imageData[0];
    unsigned char count = 1;
    
    for (int i = 1; i < size2; i++) {
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
    printf("Compression complete: %s -> %s\n", inputFile, outputFile);

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
