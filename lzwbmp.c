#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "image.h"

#define MAX_DICT_SIZE 4096
#define INITIAL_DICT_SIZE 256

typedef struct {
    uint16_t prefix;
    uint8_t append;
} DictionaryEntry2;

// LZW Compression
unsigned char* lzw_compress(unsigned char* input, uint32_t input_size, uint32_t* output_size) {
    DictionaryEntry2 dict[MAX_DICT_SIZE];
    uint16_t dict_size = INITIAL_DICT_SIZE;
    uint32_t i;

    for (i = 0; i < INITIAL_DICT_SIZE; i++) {
        dict[i].prefix = 0xFFFF;
        dict[i].append = (uint8_t)i;
    }

    unsigned char* output = malloc(input_size * 2); // Worst case
    if (!output) {
        printf("Error: Memory allocation failed for compression\n");
        return NULL;
    }
    uint32_t output_pos = 0;
    uint16_t prefix = input[0];
    uint32_t input_pos = 1;

    while (input_pos < input_size) {
        uint8_t next_char = input[input_pos++];
        uint16_t current_code = 0;

        for (current_code = 0; current_code < dict_size; current_code++) {
            if (dict[current_code].prefix == prefix && dict[current_code].append == next_char) {
                prefix = current_code;
                break;
            }
        }

        if (current_code == dict_size) {
            output[output_pos++] = prefix & 0xFF;
            output[output_pos++] = (prefix >> 8) & 0xFF;
            if (dict_size < MAX_DICT_SIZE) {
                dict[dict_size].prefix = prefix;
                dict[dict_size].append = next_char;
                dict_size++;
            }
            prefix = next_char;
        }
    }

    output[output_pos++] = prefix & 0xFF;
    output[output_pos++] = (prefix >> 8) & 0xFF;

    *output_size = output_pos;
    // printf("Compressed %u bytes to %u bytes (ratio: %.2f%%)\n", input_size, output_pos, (float)output_pos / input_size * 100);
    return output;
}

// LZW Decompression
unsigned char* lzw_decompress(unsigned char* input, uint32_t input_size, uint32_t original_size) {
    DictionaryEntry2 dict[MAX_DICT_SIZE];
    uint16_t dict_size = INITIAL_DICT_SIZE;
    uint32_t i;

    for (i = 0; i < INITIAL_DICT_SIZE; i++) {
        dict[i].prefix = 0xFFFF;
        dict[i].append = (uint8_t)i;
    }

    unsigned char* output = malloc(original_size);
    if (!output) {
        printf("Error: Memory allocation failed for decompression\n");
        return NULL;
    }
    uint32_t output_pos = 0;
    uint32_t input_pos = 0;

    uint16_t old_code = input[input_pos++] | (input[input_pos++] << 8);
    output[output_pos++] = (uint8_t)old_code;

    while (input_pos < input_size) {
        uint16_t new_code = input[input_pos++] | (input[input_pos++] << 8);
        uint8_t* temp = malloc(original_size); // Temporary buffer for string
        uint32_t temp_pos = 0;

        if (new_code >= dict_size) {
            temp[temp_pos++] = dict[old_code].append;
            uint16_t temp_code = old_code;
            while (temp_code != 0xFFFF) {
                temp[temp_pos++] = dict[temp_code].append;
                temp_code = dict[temp_code].prefix;
            }
        } else {
            uint16_t temp_code = new_code;
            while (temp_code != 0xFFFF) {
                temp[temp_pos++] = dict[temp_code].append;
                temp_code = dict[temp_code].prefix;
            }
        }

        // Reverse and copy to output
        for (i = temp_pos; i > 0; i--) {
            output[output_pos++] = temp[i - 1];
        }

        if (dict_size < MAX_DICT_SIZE) {
            dict[dict_size].prefix = old_code;
            dict[dict_size].append = temp[temp_pos - 1];
            dict_size++;
        }
        old_code = new_code;
        free(temp);
    }

    printf("Decompressed %u bytes to %u bytes\n", input_size, original_size);
    return output;
}

int compressBMP2(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    if (!fin) {
        printf("Error: Cannot open input file %s\n", input_file);
        return -1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    if (fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fin) != 1 ||
        fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fin) != 1) {
        printf("Error: Failed to read BMP headers\n");
        fclose(fin);
        return -1;
    }

    if (infoHeader.biCompression != 0) {
        printf("Error: Input BMP is already compressed\n");
        fclose(fin);
        return -1;
    }

    printf("Input BMP: %dx%d, %d bpp, size: %u bytes\n", 
           infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount, fileHeader.bfSize);

    fseek(fin, fileHeader.bfOffbits, SEEK_SET);
    uint32_t dataSize = infoHeader.biWidth * abs(infoHeader.biHeight) * (infoHeader.biBitCount / 8);
    unsigned char* pixelData = malloc(dataSize);
    if (!pixelData) {
        printf("Error: Memory allocation failed\n");
        fclose(fin);
        return -1;
    }

    int padding = (4 - (infoHeader.biWidth * (infoHeader.biBitCount / 8)) % 4) % 4;
    for (int i = 0; i < abs(infoHeader.biHeight); i++) {
        if (fread(pixelData + (i * infoHeader.biWidth * (infoHeader.biBitCount / 8)), 
                 infoHeader.biWidth * (infoHeader.biBitCount / 8), 1, fin) != 1) {
            printf("Error: Failed to read pixel data\n");
            free(pixelData);
            fclose(fin);
            return -1;
        }
        fseek(fin, padding, SEEK_CUR);
    }

    uint32_t compressed_size;
    unsigned char* compressed = lzw_compress(pixelData, dataSize, &compressed_size);
    if (!compressed) {
        free(pixelData);
        fclose(fin);
        return -1;
    }

    FILE* fout = fopen(output_file, "wb");
    if (!fout) {
        printf("Error: Cannot open output file %s\n", output_file);
        free(pixelData);
        free(compressed);
        fclose(fin);
        return -1;
    }

    fileHeader.bfOffbits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(uint32_t);
    fileHeader.bfSize = fileHeader.bfOffbits + compressed_size;
    infoHeader.biCompression = 1; // Custom LZW flag
    infoHeader.biSizeImage = compressed_size;

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fout);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fout);
    fwrite(&dataSize, sizeof(uint32_t), 1, fout); // Store original size
    fwrite(compressed, compressed_size, 1, fout);

    //printf("Compressed file size: %u bytes\n", fileHeader.bfSize);
    
    printf("\n");

    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, sizeof(BITMAPFILEHEADER), SEEK_SET);

    // Get original file size
    printf("Original size: %ld bytes\n", size);
    
    // Get compressed file size
    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size2 = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size2);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - ((float)compressed_size2 / size)) * 100); 


    free(pixelData);
    free(compressed);
    fclose(fin);
    fclose(fout);
    return 0;
}

int decompressBMP2(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    if (!fin) {
        printf("Error: Cannot open input file %s\n", input_file);
        return -1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    if (fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fin) != 1 ||
        fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fin) != 1) {
        printf("Error: Failed to read BMP headers\n");
        fclose(fin);
        return -1;
    }

    uint32_t original_size;
    if (fread(&original_size, sizeof(uint32_t), 1, fin) != 1) {
        printf("Error: Failed to read original size\n");
        fclose(fin);
        return -1;
    }

    unsigned char* compressed = malloc(infoHeader.biSizeImage);
    if (!compressed || fread(compressed, infoHeader.biSizeImage, 1, fin) != 1) {
        printf("Error: Failed to read compressed data\n");
        free(compressed);
        fclose(fin);
        return -1;
    }

    unsigned char* pixelData = lzw_decompress(compressed, infoHeader.biSizeImage, original_size);
    if (!pixelData) {
        free(compressed);
        fclose(fin);
        return -1;
    }

    FILE* fout = fopen(output_file, "wb");
    if (!fout) {
        printf("Error: Cannot open output file %s\n", output_file);
        free(compressed);
        free(pixelData);
        fclose(fin);
        return -1;
    }

    fileHeader.bfOffbits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffbits + original_size + 
                     (abs(infoHeader.biHeight) * ((4 - (infoHeader.biWidth * 3) % 4) % 4));
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = 0; // Uncompressed BMP typically sets this to 0

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fout);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fout);

    int padding = (4 - (infoHeader.biWidth * (infoHeader.biBitCount / 8)) % 4) % 4;
    int row_size = infoHeader.biWidth * (infoHeader.biBitCount / 8);
    unsigned char pad[4] = {0};

    for (int i = 0; i < abs(infoHeader.biHeight); i++) {
        fwrite(pixelData + (i * row_size), row_size, 1, fout);
        fwrite(pad, padding, 1, fout);
    }

    free(compressed);
    free(pixelData);
    fclose(fin);
    fclose(fout);
    printf("Decompressed file written to %s\n", output_file);
    return 0;
}

int lzwBMP() {
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
        if (compressBMP2(inputFile, compressedFile) == 0) {
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
        if (decompressBMP2(compressedFile, decompressedFile) == 0) {
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