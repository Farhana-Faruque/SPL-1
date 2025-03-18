#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256
#define MAX_LINE 1024

// Compression function
int compressRLE(const char* inputFile, const char* outputFile) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }

    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file: %s\n", outputFile);
        fclose(input);
        return 1;
    }

    // Read PGM header with improved parsing
    PGMHeader pgm;
    char line[MAX_LINE];
    
    // Read magic number
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%2s", pgm.magic) != 1) {
        printf("Failed to read magic number\n");
        fclose(input);
        fclose(output);
        return 1;
    }
    
    // Read dimensions
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%d %d", &pgm.width, &pgm.height) != 2) {
        printf("Failed to read dimensions from line: %s", line);
        fclose(input);
        fclose(output);
        return 1;
    }
    
    // Read maxval
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%d", &pgm.maxval) != 1) {
        printf("Failed to read maxval\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    if ((strcmp(pgm.magic, "P2") != 0 && strcmp(pgm.magic, "P5") != 0) || pgm.maxval > 255) {
        printf("Unsupported PGM format: %s, maxval: %d\n", pgm.magic, pgm.maxval);
        fclose(input);
        fclose(output);
        return 1;
    }

    long totalPixels = (long)pgm.width * pgm.height;
    unsigned char* imageData = (unsigned char*)malloc(totalPixels);
    if (!imageData) {
        printf("Memory allocation failed\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    // Read image data
    if (strcmp(pgm.magic, "P2") == 0) {
        for (long i = 0; i < totalPixels; i++) {
            int pixel;
            if (fscanf(input, "%d", &pixel) != 1) {
                printf("Error reading P2 data at pixel %ld\n", i);
                free(imageData);
                fclose(input);
                fclose(output);
                return 1;
            }
            imageData[i] = (unsigned char)pixel;
        }
    } else {
        if (fread(imageData, 1, totalPixels, input) != totalPixels) {
            printf("Error reading P5 data: expected %ld bytes\n", totalPixels);
            free(imageData);
            fclose(input);
            fclose(output);
            return 1;
        }
    }
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, sizeof(pgm), SEEK_SET);

    fclose(input);

    // Write header
    fwrite(&pgm.width, sizeof(int), 1, output);
    fwrite(&pgm.height, sizeof(int), 1, output);
    fwrite(pgm.magic, sizeof(char), 2, output);

    // Compress using RLE
    unsigned char currentValue = imageData[0];
    unsigned char count = 1;
    
    for (long i = 1; i < totalPixels; i++) {
        if (imageData[i] == currentValue && count < 255) {
            count++;
        } else {
            if (fwrite(&currentValue, sizeof(unsigned char), 1, output) != 1 ||
                fwrite(&count, sizeof(unsigned char), 1, output) != 1) {
                printf("Error writing RLE pair\n");
                free(imageData);
                fclose(output);
                return 1;
            }
            currentValue = imageData[i];
            count = 1;
        }
    }
    if (fwrite(&currentValue, sizeof(unsigned char), 1, output) != 1 ||
        fwrite(&count, sizeof(unsigned char), 1, output) != 1) {
        printf("Error writing final RLE pair\n");
        free(imageData);
        fclose(output);
        return 1;
    }

    fclose(output);
    free(imageData);

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

// Decompression function
int decompressRLE(const char* inputFile, const char* outputFile) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }

    // Read header
    PGMHeader pgm;
    if (fread(&pgm.width, sizeof(int), 1, input) != 1 ||
        fread(&pgm.height, sizeof(int), 1, input) != 1 ||
        fread(pgm.magic, sizeof(char), 2, input) != 2) {
        printf("Failed to read header from %s\n", inputFile);
        fclose(input);
        return 1;
    }
    pgm.magic[2] = '\0';
    pgm.maxval = 255;

    long totalPixels = (long)pgm.width * pgm.height;
    unsigned char* decompressedData = (unsigned char*)malloc(totalPixels);
    if (!decompressedData) {
        printf("Memory allocation failed\n");
        fclose(input);
        return 1;
    }

    // Decompress using RLE
    unsigned char value;
    unsigned char count;
    long pixelsWritten = 0;
    
    while (pixelsWritten < totalPixels) {
        if (fread(&value, sizeof(unsigned char), 1, input) != 1 ||
            fread(&count, sizeof(unsigned char), 1, input) != 1) {
            printf("Error reading RLE pair at pixel %ld\n", pixelsWritten);
            free(decompressedData);
            fclose(input);
            return 1;
        }
        for (int i = 0; i < count && pixelsWritten < totalPixels; i++) {
            decompressedData[pixelsWritten++] = value;
        }
    }

    fclose(input);

    if (pixelsWritten != totalPixels) {
        printf("Error: Decompressed pixel count (%ld) doesn't match expected (%ld)\n",
               pixelsWritten, totalPixels);
        free(decompressedData);
        return 1;
    }

    // Write decompressed PGM file
    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file: %s\n", outputFile);
        free(decompressedData);
        return 1;
    }

    fprintf(output, "%s\n%d %d\n%d\n", pgm.magic, pgm.width, pgm.height, pgm.maxval);

    if (strcmp(pgm.magic, "P2") == 0) {
        for (long i = 0; i < totalPixels; i++) {
            if (fprintf(output, "%d", decompressedData[i]) < 0) {
                printf("Error writing P2 data at pixel %ld\n", i);
                fclose(output);
                free(decompressedData);
                return 1;
            }
            if ((i + 1) % pgm.width == 0) {
                fprintf(output, "\n");
            } else {
                fprintf(output, " ");
            }
        }
    } else if (strcmp(pgm.magic, "P5") == 0) {
        if (fwrite(decompressedData, 1, totalPixels, output) != totalPixels) {
            printf("Error writing P5 data\n");
            fclose(output);
            free(decompressedData);
            return 1;
        }
    } else {
        printf("Invalid format in compressed file: %s\n", pgm.magic);
        fclose(output);
        free(decompressedData);
        return 1;
    }

    fclose(output);
    free(decompressedData);
    return 0;
}

// Main function with validation
int rle() {
    char inputFile[256];
    char compressedFile[256] = "compressed.bin";
    char decompressedFile[256];
    int yn;

    printf("What do you want to do??\n1.Compress an image.\n2.Decompress an image.\n");
    printf("Enter your choice in number: ");  
    scanf("%d", &yn);
    printf("\n");
    if(yn == 1){
        printf("Enter the input PGM file name:");
        scanf("%255s", inputFile);
        printf("\n");

        printf("Attempting to compress %s...\n", inputFile);
        printf("\n");
        if (compressRLE(inputFile, compressedFile) == 0) {
            printf("Compression successful: %s -> %s\n", inputFile, compressedFile);

        } else {
            printf("Compression failed\n");
        }
    }
    else if(yn == 2){ 
        printf("Enter decompressed PGM file name: ");
        scanf("%255s", decompressedFile);
        printf("\n");

        printf("Attempting to decompress %s...\n", compressedFile);
        printf("\n");
        
        if (decompressRLE(compressedFile, decompressedFile) == 0) {
            printf("Decompression successful: %s -> %s\n", compressedFile, decompressedFile);

            FILE* comp = fopen(compressedFile, "rb");
            fseek(comp, 0, SEEK_END);
            long compSize = ftell(comp);
            fclose(comp);
        
            FILE* decomp = fopen(decompressedFile, "rb");
            fseek(decomp, 0, SEEK_END);
            long decompSize = ftell(decomp);
            fclose(decomp);

            printf("Compressed size: %ld bytes\n", compSize);
            printf("Decompressed size: %ld bytes\n", decompSize);
        } else {
            printf("Decompression failed\n");
        }
    }
    else{
        printf("Invalid choice.\n");
    }

    return 0;
}