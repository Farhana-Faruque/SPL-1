#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "image.h"

#define MAX_TREE_NODES 511 // 256 leaf nodes + 255 internal nodes

// Huffman Node (using typedef consistently)
typedef struct HuffmanNode {
    uint32_t freq;
    uint8_t symbol;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
} HuffmanNode;

// Bit buffer for writing/reading compressed data
typedef struct {
    FILE* file;
    uint8_t buffer;
    int bit_count;
} BitBuffer;

HuffmanNode* create_node(uint8_t symbol, uint32_t freq) {
    HuffmanNode* node = malloc(sizeof(HuffmanNode));
    node->symbol = symbol;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

void free_tree(HuffmanNode* node) {
    if (!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

// Build frequency table
void build_freq_table(unsigned char* data, uint32_t size, uint32_t* freq) {
    memset(freq, 0, 256 * sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        freq[data[i]]++;
    }
}

// Build Huffman tree
HuffmanNode* build_huffman_tree(uint32_t* freq) {
    HuffmanNode* nodes[MAX_TREE_NODES];
    int node_count = 0;

    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            nodes[node_count++] = create_node((uint8_t)i, freq[i]);
        }
    }

    while (node_count > 1) {
        int min1 = 0, min2 = 1;
        if (nodes[min2]->freq < nodes[min1]->freq) {
            int temp = min1;
            min1 = min2;
            min2 = temp;
        }
        for (int i = 2; i < node_count; i++) {
            if (nodes[i]->freq < nodes[min1]->freq) {
                min2 = min1;
                min1 = i;
            } else if (nodes[i]->freq < nodes[min2]->freq) {
                min2 = i;
            }
        }

        HuffmanNode* parent = create_node(0, nodes[min1]->freq + nodes[min2]->freq);
        parent->left = nodes[min1];
        parent->right = nodes[min2];

        nodes[min1] = parent;
        nodes[min2] = nodes[node_count - 1];
        node_count--;
    }

    return nodes[0];
}

// Generate Huffman codes
void generate_codes(HuffmanNode* root, uint32_t code, int length, uint32_t* codes, int* lengths) {
    if (!root->left && !root->right) {
        codes[root->symbol] = code;
        lengths[root->symbol] = length;
        return;
    }
    if (root->left) {
        generate_codes(root->left, code << 1, length + 1, codes, lengths);
    }
    if (root->right) {
        generate_codes(root->right, (code << 1) | 1, length + 1, codes, lengths);
    }
}

// Bit buffer functions
void init_bit_buffer(BitBuffer* bb, FILE* file) {
    bb->file = file;
    bb->buffer = 0;
    bb->bit_count = 0;
}

void write_bit(BitBuffer* bb, int bit) {
    bb->buffer = (bb->buffer << 1) | bit;
    bb->bit_count++;
    if (bb->bit_count == 8) {
        fwrite(&bb->buffer, 1, 1, bb->file);
        bb->buffer = 0;
        bb->bit_count = 0;
    }
}

void flush_bit_buffer(BitBuffer* bb) {
    if (bb->bit_count > 0) {
        bb->buffer <<= (8 - bb->bit_count);
        fwrite(&bb->buffer, 1, 1, bb->file);
        bb->buffer = 0;
        bb->bit_count = 0;
    }
}

int read_bit(BitBuffer* bb) {
    if (bb->bit_count == 0) {
        fread(&bb->buffer, 1, 1, bb->file);
        bb->bit_count = 8;
    }
    bb->bit_count--;
    return (bb->buffer >> bb->bit_count) & 1;
}

int compressBMP3(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    if (!fin) {
        printf("Error: Cannot open input file %s\n", input_file);
        return -1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fin);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fin);

    if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) {
        printf("Error: Only 24-bit uncompressed BMP supported\n");
        fclose(fin);
        return -1;
    }

    uint32_t dataSize = infoHeader.biWidth * abs(infoHeader.biHeight) * 3;
    unsigned char* pixelData = malloc(dataSize);
    fseek(fin, fileHeader.bfOffbits, SEEK_SET);
    int padding = (4 - (infoHeader.biWidth * 3) % 4) % 4;
    for (int i = 0; i < abs(infoHeader.biHeight); i++) {
        fread(pixelData + (i * infoHeader.biWidth * 3), infoHeader.biWidth * 3, 1, fin);
        fseek(fin, padding, SEEK_CUR);
    }

    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, sizeof(BITMAPFILEHEADER), SEEK_SET);

    fclose(fin);

    uint32_t freq[256];
    build_freq_table(pixelData, dataSize, freq);
    HuffmanNode* root = build_huffman_tree(freq);

    uint32_t codes[256] = {0};
    int lengths[256] = {0};
    generate_codes(root, 0, 0, codes, lengths);

    FILE* fout = fopen(output_file, "wb");
    if (!fout) {
        printf("Error: Cannot open output file %s\n", output_file);
        free(pixelData);
        free_tree(root);
        return -1;
    }

    fileHeader.bfOffbits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(uint32_t) + 256 * sizeof(uint32_t);
    infoHeader.biCompression = 2;
    BitBuffer bb;
    init_bit_buffer(&bb, fout);

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fout);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fout);
    fwrite(&dataSize, sizeof(uint32_t), 1, fout);
    fwrite(freq, sizeof(uint32_t), 256, fout);

    for (uint32_t i = 0; i < dataSize; i++) {
        uint32_t code = codes[pixelData[i]];
        int len = lengths[pixelData[i]];
        for (int j = len - 1; j >= 0; j--) {
            write_bit(&bb, (code >> j) & 1);
        }
    }
    flush_bit_buffer(&bb);

    uint32_t compressed_size = ftell(fout) - fileHeader.bfOffbits;
    infoHeader.biSizeImage = compressed_size;
    fileHeader.bfSize = fileHeader.bfOffbits + compressed_size;
    fseek(fout, 0, SEEK_SET);
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fout);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fout);

    // Get original file size
    printf("\nOriginal size: %ld bytes\n", size);
    
    // Get compressed file size
    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size2 = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size2);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - ((float)compressed_size2 / size)) * 100); 


    free(pixelData);
    free_tree(root);
    fclose(fout);

    // printf("Compressed %u bytes to %u bytes\n", dataSize, compressed_size);
    return 0;
}

int decompressBMP3(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    if (!fin) {
        printf("Error: Cannot open input file %s\n", input_file);
        return -1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fin);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fin);

    if (infoHeader.biCompression != 2) {
        printf("Error: Not a Huffman-compressed BMP\n");
        fclose(fin);
        return -1;
    }

    uint32_t original_size;
    fread(&original_size, sizeof(uint32_t), 1, fin);

    uint32_t freq[256];
    fread(freq, sizeof(uint32_t), 256, fin);

    HuffmanNode* root = build_huffman_tree(freq);
    unsigned char* pixelData = malloc(original_size);
    BitBuffer bb;
    init_bit_buffer(&bb, fin);

    uint32_t pos = 0;
    HuffmanNode* current = root;
    while (pos < original_size) {
        int bit = read_bit(&bb);
        current = bit ? current->right : current->left;
        if (!current->left && !current->right) {
            pixelData[pos++] = current->symbol;
            current = root;
        }
    }

    FILE* fout = fopen(output_file, "wb");
    if (!fout) {
        printf("Error: Cannot open output file %s\n", output_file);
        free(pixelData);
        free_tree(root);
        fclose(fin);
        return -1;
    }

    fileHeader.bfOffbits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffbits + original_size + 
                     (abs(infoHeader.biHeight) * ((4 - (infoHeader.biWidth * 3) % 4) % 4));
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = 0;

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fout);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fout);

    int padding = (4 - (infoHeader.biWidth * 3) % 4) % 4;
    int row_size = infoHeader.biWidth * 3;
    unsigned char pad[4] = {0};
    for (int i = 0; i < abs(infoHeader.biHeight); i++) {
        fwrite(pixelData + (i * row_size), row_size, 1, fout);
        fwrite(pad, padding, 1, fout);
    }

    free(pixelData);
    free_tree(root);
    fclose(fin);
    fclose(fout);
    printf("Decompressed to %u bytes\n", original_size);
    return 0;
}

int huffmanBMP() {
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
        if (compressBMP3(inputFile, compressedFile) == 0) {
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
        if (decompressBMP3(compressedFile, decompressedFile) == 0) {
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