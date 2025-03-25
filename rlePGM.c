#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256
#define MAX_LINE 1024

int compressRLE(const char* inputFile, const char* outputFile) {
    FILE* input = fopen(inputFile, "rb");
    FILE* output = fopen(outputFile, "wb");
    if (!input || !output) {
        printf("Cannot open the file");
        return 1;
    }

    PGMHeader pgm;
    char line[MAX_LINE];
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%2s", pgm.sign) != 1) {
        printf("Failed to read magic number\n");
        fclose(input);
        fclose(output);
        return 1;
    }
    
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%d %d", &pgm.width, &pgm.height) != 2) {
        printf("Failed to read dimensions from line: %s", line);
        fclose(input);
        fclose(output);
        return 1;
    }
    
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%d", &pgm.maxIntensity) != 1) {
        printf("Failed to read maxval\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    if ((strcmp(pgm.sign, "P2") != 0 && strcmp(pgm.sign, "P5") != 0) || pgm.maxIntensity > 255) {
        printf("Unsupported PGM format: %s, maxvalue: %d\n", pgm.sign, pgm.maxIntensity);
        fclose(input);
        fclose(output);
        return 1;
    }

    long tP = (long)pgm.width * pgm.height; //totalPixels
    unsigned char* iD = (unsigned char*)malloc(tP); //imageData
    if (!iD) {
        printf("Memory allocation failed\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    if (strcmp(pgm.sign, "P2") == 0) {
        for (long i = 0; i < tP; i++) {
            int pixel;
            if (fscanf(input, "%d", &pixel) != 1) {
                printf("Error reading P2 data at pixel %ld\n", i);
                free(iD);
                fclose(input);
                fclose(output);
                return 1;
            }
            iD[i] = (unsigned char)pixel;
        }
    } else {
        if (fread(iD, 1, tP, input) != tP) {
            printf("Error reading P5 data: expected %ld bytes\n", tP);
            free(iD);
            fclose(input);
            fclose(output);
            return 1;
        }
    }
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, sizeof(pgm), SEEK_SET);
    fclose(input);

    fwrite(&pgm.width, sizeof(int), 1, output);
    fwrite(&pgm.height, sizeof(int), 1, output);
    fwrite(pgm.sign, sizeof(char), 2, output);

    unsigned char current = iD[0];
    unsigned char count = 1;
    
    for (long i = 1; i < tP; i++) {
        if (iD[i] == current && count < 255) {
            count++;
        } else {
            if (fwrite(&current, sizeof(unsigned char), 1, output) != 1 ||
                fwrite(&count, sizeof(unsigned char), 1, output) != 1) {
                printf("Error writing RLE pair\n");
                free(iD);
                fclose(output);
                return 1;
            }
            current = iD[i];
            count = 1;
        }
    }
    if (fwrite(&current, sizeof(unsigned char), 1, output) != 1 ||
        fwrite(&count, sizeof(unsigned char), 1, output) != 1) {
        printf("Error writing final RLE pair\n");
        free(iD);
        fclose(output);
        return 1;
    }
    fclose(output);
    free(iD);

    printf("Original size: %ld bytes\n", size);

    FILE *check_size = fopen(outputFile, "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n",  (1.0 - ((float)compressed_size / size)) * 100); 
    return 0;
}

int decompressRLE(const char* inputFile, const char* outputFile) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }

    PGMHeader pgm;
    if (fread(&pgm.width, sizeof(int), 1, input) != 1 ||
        fread(&pgm.height, sizeof(int), 1, input) != 1 ||
        fread(pgm.sign, sizeof(char), 2, input) != 2) {
        printf("Failed to read header from %s\n", inputFile);
        fclose(input);
        return 1;
    }
    pgm.sign[2] = '\0';
    pgm.maxIntensity = 255;

    long tP = (long)pgm.width * pgm.height;
    unsigned char* dD = (unsigned char*)malloc(tP);
    if (!dD) {
        printf("Memory allocation failed\n");
        fclose(input);
        return 1;
    }

    unsigned char value;
    unsigned char count;
    long pixelsWritten = 0;
    
    while (pixelsWritten < tP) {
        if (fread(&value, sizeof(unsigned char), 1, input) != 1 ||
            fread(&count, sizeof(unsigned char), 1, input) != 1) {
            printf("Error reading RLE pair at pixel %ld\n", pixelsWritten);
            free(dD);
            fclose(input);
            return 1;
        }
        for (int i = 0; i < count && pixelsWritten < tP; i++) {
            dD[pixelsWritten++] = value;
        }
    }
    fclose(input);

    if (pixelsWritten != tP) {
        printf("Error: Decompressed pixel count (%ld) doesn't match expected (%ld)\n",
               pixelsWritten, tP);
        free(dD);
        return 1;
    }

    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file: %s\n", outputFile);
        free(dD);
        return 1;
    }

    fprintf(output, "%s\n%d %d\n%d\n", pgm.sign, pgm.width, pgm.height, pgm.maxIntensity);

    if (strcmp(pgm.sign, "P2") == 0) {
        for (long i = 0; i < tP; i++) {
            if (fprintf(output, "%d", dD[i]) < 0) {
                printf("Error writing P2 data at pixel %ld\n", i);
                fclose(output);
                free(dD);
                return 1;
            }
            if ((i + 1) % pgm.width == 0) {
                fprintf(output, "\n");
            } else {
                fprintf(output, " ");
            }
        }
    } else if (strcmp(pgm.sign, "P5") == 0) {
        if (fwrite(dD, 1, tP, output) != tP) {
            printf("Error writing P5 data\n");
            fclose(output);
            free(dD);
            return 1;
        }
    } else {
        printf("Invalid format in compressed file: %s\n", pgm.sign);
        fclose(output);
        free(dD);
        return 1;
    }

    fclose(output);
    free(dD);
    return 0;
}

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
