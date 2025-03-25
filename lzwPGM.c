#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256
#define MAX_DICT_SIZE 4096 
#define MAX_LINE 1024

int compressLZW(const char* inputFile, const char* outputFile) {
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
        printf("Failed to read dimensions\n");
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
        printf("Unsupported PGM format: %s, maxval: %d\n", pgm.sign, pgm.maxIntensity);
        fclose(input);
        fclose(output);
        return 1;
    }

    long tP = (long)pgm.width * pgm.height; // totalPixels
    unsigned char* iD = (unsigned char*)malloc(tP); // imageData
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
            printf("Error reading P5 data\n");
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

    DictionaryEntry* dict = (DictionaryEntry*)malloc(MAX_DICT_SIZE * sizeof(DictionaryEntry));
    int dictSize = MAX_SIZE;
    for (int i = 0; i < MAX_SIZE; i++) {
        dict[i].prefix = -1;
        dict[i].value = (unsigned char)i;
        dict[i].code = i;
    }

    fwrite(&pgm.width, sizeof(int), 1, output);
    fwrite(&pgm.height, sizeof(int), 1, output);
    fwrite(pgm.sign, sizeof(char), 2, output);

    int code = iD[0];
    int nextCode = MAX_SIZE;
    unsigned int bitBuffer = 0;
    int bits = 0;

    for (long i = 1; i < tP; i++) {
        unsigned char nextChar = iD[i];
        int j;
        for (j = 0; j < dictSize; j++) {
            if (dict[j].prefix == code && dict[j].value == nextChar) {
                code = dict[j].code;
                break;
            }
        }
        if (j == dictSize) { 
            bitBuffer = (bitBuffer << 12) | code;
            bits += 12;
            while (bits >= 8) {
                unsigned char byte = (bitBuffer >> (bits - 8)) & 0xFF;
                fwrite(&byte, 1, 1, output);
                bits -= 8;
            }
            bitBuffer &= (1 << bits) - 1; 

            if (dictSize < MAX_DICT_SIZE) {
                dict[dictSize].prefix = code;
                dict[dictSize].value = nextChar;
                dict[dictSize].code = nextCode++;
                dictSize++;
            }
            code = nextChar;
        }
    }
    bitBuffer = (bitBuffer << 12) | code;
    bits += 12;
    while (bits > 0) {
        unsigned char byte = (bitBuffer >> (bits - 8)) & 0xFF;
        fwrite(&byte, 1, 1, output);
        bits -= 8;
        if (bits < 0) break;
    }

    fclose(output);
    free(iD);
    free(dict);

    printf("Original size: %ld bytes\n", size);
    
    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - ((float)compressed_size / size)) * 100); 

    return 0;
}

int decompressLZW(const char* inputFile, const char* outputFile) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }

    PGMHeader pgm;
    if (fread(&pgm.width, sizeof(int), 1, input) != 1 ||
        fread(&pgm.height, sizeof(int), 1, input) != 1 ||
        fread(pgm.sign, sizeof(char), 2, input) != 2) {
        printf("Failed to read header\n");
        fclose(input);
        return 1;
    }
    pgm.sign[2] = '\0';
    pgm.maxIntensity = 255;

    long tP = (long)pgm.width * pgm.height; // totalPixels
    unsigned char* dD = (unsigned char*)malloc(tP); // decompressedData
    if (!dD) {
        printf("Memory allocation failed\n");
        fclose(input);
        return 1;
    }

    DictionaryEntry* dict = (DictionaryEntry*)malloc(MAX_DICT_SIZE * sizeof(DictionaryEntry));
    int dictSize = MAX_SIZE;
    for (int i = 0; i < MAX_SIZE; i++) {
        dict[i].prefix = -1;
        dict[i].value = (unsigned char)i;
        dict[i].code = i;
    }

    unsigned int bitBuffer = 0;
    int bits = 0;
    long pW = 0; //pixelsWritten
    int next = MAX_SIZE;

    unsigned char byte;
    if (fread(&byte, 1, 1, input) != 1) {
        printf("Failed to read initial byte\n");
        free(dD);
        free(dict);
        fclose(input);
        return 1;
    }
    bitBuffer = byte;
    bits = 8;
    if (fread(&byte, 1, 1, input) != 1) {
        printf("Failed to read second byte\n");
        free(dD);
        free(dict);
        fclose(input);
        return 1;
    }
    bitBuffer = (bitBuffer << 8) | byte;
    bits += 8;

    int code = (bitBuffer >> (bits - 12)) & 0xFFF;
    bits -= 12;
    dD[pW++] = (unsigned char)code;
    int prev = code;

    while (pW < tP) {
        while (bits < 12) {
            if (fread(&byte, 1, 1, input) != 1) {
                printf("Unexpected end of file\n");
                break;
            }
            bitBuffer = (bitBuffer << 8) | byte;
            bits += 8;
        }
        code = (bitBuffer >> (bits - 12)) & 0xFFF;
        bits -= 12;

        unsigned char temp[MAX_SIZE];
        int tempLen = 0;
        int currentCode = code;

        if (code >= dictSize) {
            currentCode = prev;
            temp[tempLen++] = dict[prev].value;
        }

        while (currentCode >= 0 && tempLen < MAX_SIZE) {
            temp[tempLen++] = dict[currentCode].value;
            currentCode = dict[currentCode].prefix;
        }

        for (int i = tempLen - 1; i >= 0 && pW < tP; i--) {
            dD[pW++] = temp[i];
        }

        if (dictSize < MAX_DICT_SIZE) {
            dict[dictSize].prefix = prev;
            dict[dictSize].value = temp[tempLen - 1];
            dict[dictSize].code = next++;
            dictSize++;
        }
        prev = code;
    }

    fclose(input);

    if (pW != tP) {
        printf("Error: Decompressed pixel count (%ld) doesn't match expected (%ld)\n",
               pW, tP);
        free(dD);
        free(dict);
        return 1;
    }

    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file: %s\n", outputFile);
        free(dD);
        free(dict);
        return 1;
    }

    fprintf(output, "%s\n%d %d\n%d\n", pgm.sign, pgm.width, pgm.height, pgm.maxIntensity);
    if (strcmp(pgm.sign, "P2") == 0) {
        for (long i = 0; i < tP; i++) {
            if (fprintf(output, "%d", dD[i]) < 0) {
                printf("Error writing P2 data\n");
                fclose(output);
                free(dD);
                free(dict);
                return 1;
            }
            if ((i + 1) % pgm.width == 0) fprintf(output, "\n");
            else fprintf(output, " ");
        }
    } else {
        if (fwrite(dD, 1, tP, output) != tP) {
            printf("Error writing P5 data\n");
            fclose(output);
            free(dD);
            free(dict);
            return 1;
        }
    }

    fclose(output);
    free(dD);
    free(dict);
    return 0;
}

int lzw() {
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
        if (compressLZW(inputFile, compressedFile) == 0) {
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
        
        if (decompressLZW(compressedFile, decompressedFile) == 0) {
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
