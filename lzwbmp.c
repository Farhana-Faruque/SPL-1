#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "image.h"

#define MAX_DICT_SIZE 4096
#define INITIAL_DICT_SIZE 256

typedef struct {
    unsigned short prefix;
    unsigned char append;
} DictionEn2;

unsigned char* lzw_compress(unsigned char* input, unsigned int input_size, unsigned int* output_size) {
    DictionEn2 dict[MAX_DICT_SIZE];
    unsigned short dict_size = INITIAL_DICT_SIZE;
    unsigned int i;

    for (i = 0; i < INITIAL_DICT_SIZE; i++) {
        dict[i].prefix = 0xFFFF;
        dict[i].append = (uint8_t)i;
    }

    unsigned char* output = malloc(input_size * 2); 
    if (!output) {
        printf("Error: Memory allocation failed for compression\n");
        return NULL;
    }
    unsigned int out_pos = 0;
    unsigned short prefix = input[0];
    unsigned int in_pos = 1;

    while (in_pos < input_size) {
        unsigned char next_char = input[in_pos++];
        unsigned short current = 0;

        for (current = 0; current < dict_size; current++) {
            if (dict[current].prefix == prefix && dict[current].append == next_char) {
                prefix = current;
                break;
            }
        }

        if (current == dict_size) {
            output[out_pos++] = prefix & 0xFF;
            output[out_pos++] = (prefix >> 8) & 0xFF;
            if (dict_size < MAX_DICT_SIZE) {
                dict[dict_size].prefix = prefix;
                dict[dict_size].append = next_char;
                dict_size++;
            }
            prefix = next_char;
        }
    }

    output[out_pos++] = prefix & 0xFF;
    output[out_pos++] = (prefix >> 8) & 0xFF;

    *output_size = out_pos;
    return output;
}

unsigned char* lzw_decompress(unsigned char* input, unsigned int input_size, unsigned int original_size) {
    DictionEn2 dict[MAX_DICT_SIZE];
    unsigned short dict_size = INITIAL_DICT_SIZE;
    unsigned int i;

    for (i = 0; i < INITIAL_DICT_SIZE; i++) {
        dict[i].prefix = 0xFFFF;
        dict[i].append = (unsigned char)i;
    }

    unsigned char* output = malloc(original_size);
    if (!output) {
        printf("Error: Memory allocation failed for decompression\n");
        return NULL;
    }
    unsigned int out_pos = 0;
    unsigned int in_pos = 0;

    unsigned short old = input[in_pos++] | (input[in_pos++] << 8);
    output[out_pos++] = (unsigned char)old;

    while (in_pos < input_size) {
        unsigned short new = input[in_pos++] | (input[in_pos++] << 8);
        unsigned char* temp = malloc(original_size);
        unsigned int temp_pos = 0;

        if (new >= dict_size) {
            temp[temp_pos++] = dict[old].append;
            unsigned short temp_code = old;
            while (temp_code != 0xFFFF) {
                temp[temp_pos++] = dict[temp_code].append;
                temp_code = dict[temp_code].prefix;
            }
        } else {
            unsigned short temp_code = new;
            while (temp_code != 0xFFFF) {
                temp[temp_pos++] = dict[temp_code].append;
                temp_code = dict[temp_code].prefix;
            }
        }

        for (i = temp_pos; i > 0; i--) {
            output[out_pos++] = temp[i - 1];
        }

        if (dict_size < MAX_DICT_SIZE) {
            dict[dict_size].prefix = old;
            dict[dict_size].append = temp[temp_pos - 1];
            dict_size++;
        }
        old = new;
        free(temp);
    }

    printf("Decompressed %u bytes to %u bytes\n", input_size, original_size);
    return output;
}

int compressBMP2(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    FILE* fout = fopen(output_file, "wb");
    if (!fin || !fout) {
        printf("Error: Cannot open input/output files\n");
        return -1;
    }

    BmpFile file;
    BmpInfo info;
    if (fread(&file, sizeof(BmpFile), 1, fin) != 1 ||
        fread(&info, sizeof(BmpInfo), 1, fin) != 1) {
        printf("Error: Failed to read BMP headers\n");
        fclose(fin);
        return -1;
    }

    if (info.Compression != 0) {
        printf("Error: Input BMP is already compressed\n");
        fclose(fin);
        return -1;
    }

    printf("Input BMP: %dx%d, %d bpp, size: %u bytes\n", info.Width, info.Height, info.BitCount, file.Size);

    fseek(fin, file.Offbits, SEEK_SET);
    unsigned int dS = info.Width * abs(info.Height) * (info.BitCount / 8); // Data size
    unsigned char* pD = malloc(dS); // Pixel data
    if (!pD) {
        printf("Error: Memory allocation failed\n");
        fclose(fin);
        return -1;
    }

    int padding = (4 - (info.Width * (info.BitCount / 8)) % 4) % 4;
    for (int i = 0; i < abs(info.Height); i++) {
        if (fread(pD + (i * info.Width * (info.BitCount / 8)), info.Width * (info.BitCount / 8), 1, fin) != 1) {
            printf("Error: Failed to read pixel data\n");
            free(pD);
            fclose(fin);
            return -1;
        }
        fseek(fin, padding, SEEK_CUR);
    }

    unsigned int com_size;
    unsigned char* compressed = lzw_compress(pD, dS, &com_size);
    if (!compressed) {
        free(pD);
        fclose(fin);
        return -1;
    }

    file.Offbits = sizeof(BmpFile) + sizeof(BmpInfo) + sizeof(unsigned int);
    file.Size = file.Offbits + com_size;
    info.Compression = 1; 
    info.SizeImage = com_size;

    fwrite(&file, sizeof(BmpFile), 1, fout);
    fwrite(&info, sizeof(BmpInfo), 1, fout);
    fwrite(&dS, sizeof(unsigned int), 1, fout); 
    fwrite(compressed, com_size, 1, fout);
    printf("\n");

    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, sizeof(BmpFile), SEEK_SET);

    printf("Original size: %ld bytes\n", size);

    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size2 = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size2);
    printf("Compression ratio: %.2f%%\n", (1.0 - ((float)compressed_size2 / size)) * 100); 

    free(pD);
    free(compressed);
    fclose(fin);
    fclose(fout);
    return 0;
}

int decompressBMP2(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    FILE* fout = fopen(output_file, "wb");
    if (!fin || !fout){
        printf("Error: Cannot open input/output files\n");
        return -1;
    }

    BmpFile file;
    BmpInfo info;
    if (fread(&file, sizeof(BmpFile), 1, fin) != 1 ||
        fread(&info, sizeof(BmpInfo), 1, fin) != 1) {
        printf("Error: Failed to read BMP headers\n");
        fclose(fin);
        return -1;
    }

    unsigned int og_size;
    if (fread(&og_size, sizeof(unsigned int), 1, fin) != 1) {
        printf("Error: Failed to read original size\n");
        fclose(fin);
        return -1;
    }

    unsigned char* compressed = malloc(info.SizeImage);
    if (!compressed || fread(compressed, info.SizeImage, 1, fin) != 1) {
        printf("Error: Failed to read compressed data\n");
        free(compressed);
        fclose(fin);
        return -1;
    }

    unsigned char* pD = lzw_decompress(compressed, info.SizeImage, og_size); // pixel data
    if (!pD) {
        free(compressed);
        fclose(fin);
        return -1;
    }

    file.Offbits = sizeof(BmpFile) + sizeof(BmpInfo);
    file.Size = file.Offbits + og_size + (abs(info.Height) * ((4 - (info.Width * 3) % 4) % 4));
    info.Compression = 0;
    info.SizeImage = 0;

    fwrite(&file, sizeof(BmpFile), 1, fout);
    fwrite(&info, sizeof(BmpInfo), 1, fout);

    int padding = (4 - (info.Width * (info.BitCount / 8)) % 4) % 4;
    int row_size = info.Width * (info.BitCount / 8);
    unsigned char pad[4] = {0};

    for (int i = 0; i < abs(info.Height); i++) {
        fwrite(pD + (i * row_size), row_size, 1, fout);
        fwrite(pad, padding, 1, fout);
    }

    free(compressed);
    free(pD);
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
