#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffpgm.h"

#define MAX_SIZE 256
#define MAX_LINE 1024


// Compression function
int compressHuffman(const char* inputFile, const char* outputFile) {
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

    // Read PGM header
    PGMHeader pgm;
    char line[MAX_LINE];
    if (!readLine(input, line, MAX_LINE) || sscanf(line, "%2s", pgm.magic) != 1) {
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
            printf("Error reading P5 data\n");
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

    // Calculate frequencies
    unsigned int freq[MAX_SIZE] = {0};
    for (long i = 0; i < totalPixels; i++) {
        freq[imageData[i]]++;
    }

    // Build Huffman tree
    Node* root = buildHuffmanTree(freq);
    if (!root) {
        printf("Failed to build Huffman tree during compression\n");
        free(imageData);
        fclose(output);
        return 1;
    }

    // Generate Huffman codes
    char codes[MAX_SIZE][MAX_SIZE] = {0};
    int lengths[MAX_SIZE] = {0};
    char code[MAX_SIZE];
    generateCodes(root, code, 0, codes, lengths);

    // Write header
    if (fwrite(&pgm.width, sizeof(int), 1, output) != 1 ||
        fwrite(&pgm.height, sizeof(int), 1, output) != 1 ||
        fwrite(pgm.magic, sizeof(char), 2, output) != 2) {
        printf("Failed to write header\n");
        free(imageData);
        freeHuffmanTree(root);
        fclose(output);
        return 1;
    }

    // Write frequency table
    for (int i = 0; i < MAX_SIZE; i++) {
        if (freq[i] > 0) {
            if (fwrite(&i, sizeof(unsigned char), 1, output) != 1 ||
                fwrite(&freq[i], sizeof(unsigned int), 1, output) != 1) {
                printf("Failed to write frequency table\n");
                free(imageData);
                freeHuffmanTree(root);
                fclose(output);
                return 1;
            }
        }
    }
    unsigned char zero = 0;
    if (fwrite(&zero, 1, 1, output) != 1) {
        printf("Failed to write frequency table end marker\n");
        free(imageData);
        freeHuffmanTree(root);
        fclose(output);
        return 1;
    }

    // Compress data
    unsigned int bitBuffer = 0;
    int bits = 0;
    for (long i = 0; i < totalPixels; i++) {
        unsigned char pixel = imageData[i];
        for (int j = 0; j < lengths[pixel]; j++) {
            bitBuffer = (bitBuffer << 1) | (codes[pixel][j] - '0');
            bits++;
            if (bits == 8) {
                unsigned char byte = (unsigned char)bitBuffer;
                if (fwrite(&byte, 1, 1, output) != 1) {
                    printf("Failed to write compressed data\n");
                    free(imageData);
                    freeHuffmanTree(root);
                    fclose(output);
                    return 1;
                }
                bitBuffer = 0;
                bits = 0;
            }
        }
    }
    if (bits > 0) {
        bitBuffer <<= (8 - bits);
        unsigned char byte = (unsigned char)bitBuffer;
        if (fwrite(&byte, 1, 1, output) != 1) {
            printf("Failed to write final compressed byte\n");
            free(imageData);
            freeHuffmanTree(root);
            fclose(output);
            return 1;
        }
    }

    fclose(output);
    free(imageData);
    freeHuffmanTree(root);

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
int decompressHuffman(const char* inputFile, const char* outputFile) {
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

    // Read header
    PGMHeader pgm;
    if (fread(&pgm.width, sizeof(int), 1, input) != 1 ||
        fread(&pgm.height, sizeof(int), 1, input) != 1 ||
        fread(pgm.magic, sizeof(char), 2, input) != 2) {
        printf("Failed to read header\n");
        fclose(input);
        fclose(output);
        return 1;
    }
    pgm.magic[2] = '\0';
    pgm.maxval = 255;

    long totalPixels = (long)pgm.width * pgm.height;
    unsigned char* decompressedData = (unsigned char*)malloc(totalPixels);
    if (!decompressedData) {
        printf("Memory allocation failed\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    // Read frequency table and rebuild Huffman tree
    unsigned int freq[MAX_SIZE] = {0};
    unsigned char value;
    int freqCount = 0;
    while (fread(&value, 1, 1, input) == 1) {
        if (value == 0) break;  // End of table marker
        unsigned int f;
        if (fread(&f, sizeof(unsigned int), 1, input) != 1) {
            printf("Error reading frequency value for byte %d\n", value);
            free(decompressedData);
            fclose(input);
            fclose(output);
            return 1;
        }
        freq[value] = f;
        freqCount++;
    }
    if (freqCount == 0) {
        printf("No frequency data found in compressed file\n");
        free(decompressedData);
        fclose(input);
        fclose(output);
        return 1;
    }

    Node* root = buildHuffmanTree(freq);
    if (!root) {
        printf("Failed to rebuild Huffman tree\n");
        free(decompressedData);
        fclose(input);
        fclose(output);
        return 1;
    }

    // Decompress data
    Node* current = root;
    unsigned char byte;
    long pixelsWritten = 0;
    int bits = 0;

    while (pixelsWritten < totalPixels) {
        if (bits == 0) {
            if (fread(&byte, 1, 1, input) != 1) {
                printf("Unexpected end of file at pixel %ld\n", pixelsWritten);
                break;
            }
            bits = 8;
        }
        int bit = (byte >> (bits - 1)) & 1;
        bits--;

        if (bit) {
            if (!current->right) {
                printf("Invalid Huffman code at pixel %ld\n", pixelsWritten);
                free(decompressedData);
                freeHuffmanTree(root);
                fclose(input);
                fclose(output);
                return 1;
            }
            current = current->right;
        } else {
            if (!current->left) {
                printf("Invalid Huffman code at pixel %ld\n", pixelsWritten);
                free(decompressedData);
                freeHuffmanTree(root);
                fclose(input);
                fclose(output);
                return 1;
            }
            current = current->left;
        }

        if (!current->left && !current->right) {
            decompressedData[pixelsWritten++] = current->data;
            current = root;
        }
    }

    fclose(input);

    if (pixelsWritten != totalPixels) {
        printf("Error: Decompressed pixel count (%ld) doesn't match expected (%ld)\n",
               pixelsWritten, totalPixels);
        free(decompressedData);
        freeHuffmanTree(root);
        fclose(output);
        return 1;
    }

    // Write PGM file
    fprintf(output, "%s\n%d %d\n%d\n", pgm.magic, pgm.width, pgm.height, pgm.maxval);
    if (strcmp(pgm.magic, "P2") == 0) {
        for (long i = 0; i < totalPixels; i++) {
            if (fprintf(output, "%d", decompressedData[i]) < 0) {
                printf("Error writing P2 data at pixel %ld\n", i);
                fclose(output);
                free(decompressedData);
                freeHuffmanTree(root);
                return 1;
            }
            if ((i + 1) % pgm.width == 0) fprintf(output, "\n");
            else fprintf(output, " ");
        }
    } else {
        if (fwrite(decompressedData, 1, totalPixels, output) != totalPixels) {
            printf("Error writing P5 data\n");
            fclose(output);
            free(decompressedData);
            freeHuffmanTree(root);
            return 1;
        }
    }

    fclose(output);
    free(decompressedData);
    freeHuffmanTree(root);
    return 0;
}

// Main function with validation
int huffman() {
    char inputFile[256];
    char compressedFile[256] = "compressed.bin";
    char decompressedFile[256];
    int yn;

    printf("\nWhat do you want to do??\n1.Compress an image.\n2.Decompress an image.\n");
    printf("Enter your choice in number: ");  
    scanf("%d", &yn);
    if(yn == 1){
        printf("Enter the input PGM file name:");
        scanf("%255s", inputFile);


        printf("Attempting to compress %s...\n", inputFile);
        if (compressHuffman(inputFile, compressedFile) == 0) {
            printf("Compression successful: %s -> %s\n", inputFile, compressedFile);

        } else {
            printf("Compression failed\n");
        }
    }
    else if(yn == 2){ 
        printf("Enter decompressed PGM file name: ");
        scanf("%255s", decompressedFile);
        printf("Attempting to decompress %s...\n", compressedFile);
        if (decompressHuffman(compressedFile, decompressedFile) == 0) {
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