#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

typedef struct {
    unsigned char count; 
    unsigned char b, g, r; 
} RLEEntry; 

int compressBMP(const char* inputFile, const char* outputFile) {
    FILE *in = fopen(inputFile, "rb");
    FILE *out = fopen(outputFile, "wb");
    if (!in || !out) {
        printf("Error opening files\n");
        return 1;
    }

    BmpFile file;
    BmpInfo info;
    fread(&file, sizeof(BmpFile), 1, in);
    fread(&info, sizeof(BmpInfo), 1, in);
    
    info.Compression = 1;
    
    fwrite(&file, sizeof(BmpFile), 1, out);
    fwrite(&info, sizeof(BmpInfo), 1, out);

    int padding = (4 - ((info.Width * 3) % 4)) % 4;
    int rowSize = info.Width * 3 + padding;
    
    fseek(in, file.Offbits, SEEK_SET);
    unsigned char* pD = malloc(rowSize * info.Height); // pixel data
    fread(pD, 1, rowSize * info.Height, in);

    RLEEntry entry = {0, 0, 0, 0};
    unsigned long cS = 0; // compressed size
    
    for (int i = 0; i < info.Height * rowSize; i += 3) {
        if (i % rowSize >= info.Width * 3) continue;
        unsigned char b = pD[i];
        unsigned char g = pD[i + 1];
        unsigned char r = pD[i + 2];
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
            cS += sizeof(RLEEntry);
            entry.count = 1;
            entry.b = b;
            entry.g = g;
            entry.r = r;
        }
    }
    if (entry.count > 0) {
        fwrite(&entry, sizeof(RLEEntry), 1, out);
        cS += sizeof(RLEEntry);
    }
    fseek(out, 0, SEEK_SET);
    file.Size = sizeof(BmpFile) + sizeof(BmpInfo) + cS;
    fwrite(&file, sizeof(BmpFile), 1, out);

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, sizeof(BmpFile), SEEK_SET);

    free(pD);
    fclose(in);
    fclose(out);

    printf("\nOriginal size: %ld bytes\n", size);
    
    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", (1.0 - ((float)compressed_size / size)) * 100); 

    return 0;
}

int decompressBMP(const char* inputFile, const char* outputFile) {
    FILE *in = fopen(inputFile, "rb");
    FILE *out = fopen(outputFile, "wb");
    if (!in || !out) {
        printf("Error opening files\n");
        return 1;
    }

    BmpFile file;
    BmpInfo info;
    fread(&file, sizeof(BmpFile), 1, in);
    fread(&info, sizeof(BmpInfo), 1, in);
    
    info.Compression = 0;
    info.SizeImage = info.Width * info.Height * 3;
    
    fwrite(&file, sizeof(BmpFile), 1, out);
    fwrite(&info, sizeof(BmpInfo), 1, out);

    int padding = (4 - ((info.Width * 3) % 4)) % 4;
    int rowSize = info.Width * 3 + padding;
    unsigned char* row = calloc(rowSize, 1);

    int Count = 0;
    RLEEntry entry;
    while (fread(&entry, sizeof(RLEEntry), 1, in) == 1) {
        for (int i = 0; i < entry.count; i++) {
            int pos = (Count % info.Width) * 3;
            row[pos] = entry.b;
            row[pos + 1] = entry.g;
            row[pos + 2] = entry.r;
            
            Count++;
            if (Count % info.Width == 0) {
                fwrite(row, 1, rowSize, out);
                memset(row, 0, rowSize);
            }
        }
    }
    fseek(out, 0, SEEK_SET);
    file.Size = sizeof(BmpFile) + sizeof(BmpInfo) + (rowSize * info.Height);
    file.Offbits = 54;
    fwrite(&file, sizeof(BmpFile), 1, out);

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