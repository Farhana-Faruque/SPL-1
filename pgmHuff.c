#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "Huff.h"

#define MAX_SIZE 256  // For 8-bit grayscale values (0-255)

int HuffP2() {
    char inputFile[256];
    printf("Enter the PGM file name: ");
    scanf("%255s", inputFile);

    FILE *input = fopen(inputFile, "r");
  // Changed from "rb" to "r" for ASCII
    if (!input) {
        printf("Cannot open input file\n");
        return 1;
    }

    // Read PGM header
    char magic[3];
    int width, height, maxval;
    fscanf(input, "%2s\n%d %d\n%d\n", magic, &width, &height, &maxval);
    
    if ((strcmp(magic, "P2") != 0 && strcmp(magic, "P5") != 0) || maxval > 255) {
        printf("Unsupported PGM format\n");
        fclose(input);
        return 1;
    }
    
    int size = width * height;
    unsigned char* imageData = (unsigned char*)malloc(size);
    
    // Read image data (ASCII format)
    if (strcmp(magic, "P2") != 0 || maxval > 255)
    {
        for (int i = 0; i < size; i++) {
            int pixel;
            fscanf(input, "%d", &pixel);
            imageData[i] = (unsigned char)pixel;
        }
        fclose(input);
    }
    else{
        fread(imageData, 1, size, input);
        fclose(input);
    }

    // Calculate frequency of each pixel value
    int freq[MAX_SIZE] = {0};
    for (int i = 0; i < size; i++) {
        freq[imageData[i]]++;
    }

    // Build Huffman tree
    Node* root = buildHuffmanTree(imageData, freq, MAX_SIZE);

    // Generate Huffman codes
    HuffmanCode codes[MAX_SIZE] = {0};
    char code[256] = {0};
    generateCodes(root, code, 0, codes);

    // Compress and write to binary file
    char outputFile[] = "compressed.pgm";
    FILE *output = fopen(outputFile, "wb");
    if (!output) {
        printf("Cannot create output file\n");
        return 1;
    }

    // Write header information
    fwrite(&width, sizeof(int), 1, output);
    fwrite(&height, sizeof(int), 1, output);

    // Write Huffman codes table
    for (int i = 0; i < MAX_SIZE; i++) {
        if (freq[i] > 0) {
            fwrite(&i, sizeof(unsigned char), 1, output);
            fwrite(&codes[i].length, sizeof(int), 1, output);
            int bytes = (codes[i].length + 7) / 8;
            unsigned char* codeBytes = (unsigned char*)calloc(bytes, 1);
            for (int j = 0; j < codes[i].length; j++) {
                if (codes[i].code[j] == '1') {
                    codeBytes[j / 8] |= (1 << (7 - (j % 8)));
                }
            }
            fwrite(codeBytes, 1, bytes, output);
            free(codeBytes);
        }
    }
    unsigned char zero = 0;
    fwrite(&zero, 1, 1, output);  // End of code table marker

    // Compress image data
    unsigned char buffer = 0;
    int bits = 0;
    for (int i = 0; i < size; i++) {
        unsigned char pixel = imageData[i];
        for (int j = 0; j < codes[pixel].length; j++) {
            buffer = (buffer << 1) | (codes[pixel].code[j] - '0');
            bits++;
            if (bits == 8) {
                fwrite(&buffer, 1, 1, output);
                buffer = 0;
                bits = 0;
            }
        }
    }
    // Write remaining bits
    if (bits > 0) {
        buffer <<= (8 - bits);
        fwrite(&buffer, 1, 1, output);
    }

    // Cleanup
    fclose(output);
    free(imageData);
    freeHuffmanTree(root);
    printf("Compression complete: %s -> %s \n", inputFile, outputFile);

    //printf("Compression completed successfully\n");
    return 0;
}
