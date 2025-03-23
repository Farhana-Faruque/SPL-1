#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

// Structure for RLE compressed data
typedef struct {
    unsigned char count;    // Number of repetitions (1-255)
    unsigned char b, g, r;  // Pixel values
} RLEEntry; 

int compressBMP(const char* inputFile, const char* outputFile) {
    FILE *in = fopen(inputFile, "rb");
    FILE *out = fopen(outputFile, "wb");
    if (!in || !out) {
        printf("Error opening files\n");
        return 1;
    }

    // Read and write headers
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, in);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, in);
    
    // Update compression field
    infoHeader.biCompression = 1; // Custom RLE
    
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, out);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, out);

    // Calculate padding
    int padding = (4 - ((infoHeader.biWidth * 3) % 4)) % 4;
    int rowSize = infoHeader.biWidth * 3 + padding;
    
    // Read pixel data
    fseek(in, fileHeader.bfOffbits, SEEK_SET);
    unsigned char* pixelData = malloc(rowSize * infoHeader.biHeight);
    fread(pixelData, 1, rowSize * infoHeader.biHeight, in);

    // RLE compression
    RLEEntry entry = {0, 0, 0, 0};
    unsigned long compressedSize = 0;
    
    for (int i = 0; i < infoHeader.biHeight * rowSize; i += 3) {
        if (i % rowSize >= infoHeader.biWidth * 3) continue; // Skip padding
        
        unsigned char b = pixelData[i];
        unsigned char g = pixelData[i + 1];
        unsigned char r = pixelData[i + 2];

        if (entry.count == 0) {
            entry.count = 1;
            entry.b = b;
            entry.g = g;
            entry.r = r;
        }
        else if (entry.b == b && entry.g == g && entry.r == r && entry.count < 255) {
            entry.count++;
        }
        else {
            fwrite(&entry, sizeof(RLEEntry), 1, out);
            compressedSize += sizeof(RLEEntry);
            entry.count = 1;
            entry.b = b;
            entry.g = g;
            entry.r = r;
        }
    }
    // Write last entry
    if (entry.count > 0) {
        fwrite(&entry, sizeof(RLEEntry), 1, out);
        compressedSize += sizeof(RLEEntry);
    }

    // Update file size
    fseek(out, 0, SEEK_SET);
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + compressedSize;
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, out);

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, sizeof(BITMAPFILEHEADER), SEEK_SET);

    free(pixelData);
    fclose(in);
    fclose(out);

    // Get original file size
    printf("\nOriginal size: %ld bytes\n", size);
    
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

int decompressBMP(const char* inputFile, const char* outputFile) {
    FILE *in = fopen(inputFile, "rb");
    FILE *out = fopen(outputFile, "wb");
    if (!in || !out) {
        printf("Error opening files\n");
        return 1;
    }

    // Read headers
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, in);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, in);
    
    // Reset compression to uncompressed BMP
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = infoHeader.biWidth * infoHeader.biHeight * 3;
    
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, out);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, out);

    // Calculate padding and row size
    int padding = (4 - ((infoHeader.biWidth * 3) % 4)) % 4;
    int rowSize = infoHeader.biWidth * 3 + padding;
    unsigned char* row = calloc(rowSize, 1);

    // Decompress RLE
    int pixelCount = 0;
    RLEEntry entry;
    while (fread(&entry, sizeof(RLEEntry), 1, in) == 1) {
        for (int i = 0; i < entry.count; i++) {
            int pos = (pixelCount % infoHeader.biWidth) * 3;
            row[pos] = entry.b;
            row[pos + 1] = entry.g;
            row[pos + 2] = entry.r;
            
            pixelCount++;
            if (pixelCount % infoHeader.biWidth == 0) {
                fwrite(row, 1, rowSize, out);
                memset(row, 0, rowSize);
            }
        }
    }

    // Update file size
    fseek(out, 0, SEEK_SET);
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 
                       (rowSize * infoHeader.biHeight);
    fileHeader.bfOffbits = 54;
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, out);

    free(row);
    fclose(in);
    fclose(out);

    return 0;
}

int runlengthBmp() {
    char inputFile[256];
    char compressedFile[256] = "compressed.bin";
    char decompressedFile[256];
    int yn;

    printf("What do you want to do??\n1.Compress an image.\n2.Decompress an image.\n");
    printf("Enter your choice in number: ");  
    scanf("%d", &yn);
    printf("\n");
    if(yn == 1){
        printf("Enter the input BMP file name: ");
        scanf("%255s", inputFile);
        printf("\n");

        printf("Attempting to compress %s...\n", inputFile);
        if (compressBMP(inputFile, compressedFile) == 0) {
            printf("Compression successful: %s -> %s\n", inputFile, compressedFile);

        } else {
            printf("Compression failed\n");
        }
    }
    else if(yn == 2){ 
        printf("Enter decompressed BMP file name: ");
        scanf("%255s", decompressedFile);
        printf("\n");
        printf("Attempting to decompress %s...\n", compressedFile);
        printf("\n");
        if (decompressBMP(compressedFile, decompressedFile) == 0) {
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