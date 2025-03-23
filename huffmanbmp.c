#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "image.h"

#define MAX_TREE_NODES 511 // 256 leaf nodes + 255 internal nodes

typedef struct HuffmanNode {
    unsigned int freq;
    unsigned char symbol;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
} HuffmanNode;

typedef struct {
    FILE* file;
    unsigned char buffer;
    int bit_count;
} BitBuffer;

HuffmanNode* create_node(unsigned char symbol, unsigned int freq) {
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

void build_freq_table(unsigned char* data, unsigned int size, unsigned int* freq) {
    memset(freq, 0, 256 * sizeof(unsigned int));
    for (unsigned int i = 0; i < size; i++) {
        freq[data[i]]++;
    }
}

HuffmanNode* build_huffman_tree(unsigned int* freq) {
    HuffmanNode* nodes[MAX_TREE_NODES];
    int node_count = 0;

    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            nodes[node_count++] = create_node((unsigned char)i, freq[i]);
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

void generate_codes(HuffmanNode* root, unsigned int code, int length, unsigned int* codes, int* lengths) {
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
    FILE* fout = fopen(output_file, "wb");
    if (!fin || !fout) {
        printf("Error: Cannot open input/output files\n");
        return -1;
    }

    BmpFile file;
    BmpInfo info;
    fread(&file, sizeof(BmpFile), 1, fin);
    fread(&info, sizeof(BmpInfo), 1, fin);

    if (info.BitCount != 24 || info.Compression != 0) {
        printf("Error: Only 24-bit uncompressed BMP supported\n");
        fclose(fin);
        return -1;
    }

    unsigned int dS = info.Width * abs(info.Height) * 3; // data size
    unsigned char* pD = malloc(dS); // pixel data
    fseek(fin, file.Offbits, SEEK_SET);
    int padding = (4 - (info.Width * 3) % 4) % 4;
    for (int i = 0; i < abs(info.Height); i++) {
        fread(pD + (i * info.Width * 3), info.Width * 3, 1, fin);
        fseek(fin, padding, SEEK_CUR);
    }

    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, sizeof(BmpFile), SEEK_SET);
    fclose(fin);

    unsigned int freq[256];
    build_freq_table(pD, dS, freq);
    HuffmanNode* root = build_huffman_tree(freq);

    unsigned int codes[256] = {0};
    int lengths[256] = {0};
    generate_codes(root, 0, 0, codes, lengths);

    file.Offbits = sizeof(BmpFile) + sizeof(BmpInfo) + sizeof(unsigned int) + 256 * sizeof(unsigned int);
    info.Compression = 2;
    BitBuffer bb;
    init_bit_buffer(&bb, fout);

    fwrite(&file, sizeof(BmpFile), 1, fout);
    fwrite(&info, sizeof(BmpInfo), 1, fout);
    fwrite(&dS, sizeof(unsigned int), 1, fout);
    fwrite(freq, sizeof(unsigned int), 256, fout);

    for (unsigned int i = 0; i < dS; i++) {
        unsigned int code = codes[pD[i]];
        int len = lengths[pD[i]];
        for (int j = len - 1; j >= 0; j--) {
            write_bit(&bb, (code >> j) & 1);
        }
    }
    flush_bit_buffer(&bb);

    unsigned int com_size = ftell(fout) - file.Offbits;
    info.SizeImage = com_size;
    file.Size = file.Offbits + com_size;
    fseek(fout, 0, SEEK_SET);
    fwrite(&file, sizeof(BmpFile), 1, fout);
    fwrite(&info, sizeof(BmpInfo), 1, fout);

    printf("\nOriginal size: %ld bytes\n", size);

    FILE *check_size = fopen("compressed.bin", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size2 = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size2);
    printf("Compression ratio: %.2f%%\n", (1.0 - ((float)compressed_size2 / size)) * 100); 

    free(pD);
    free_tree(root);
    fclose(fout);

    return 0;
}

int decompressBMP3(const char* input_file, const char* output_file) {
    FILE* fin = fopen(input_file, "rb");
    FILE* fout = fopen(output_file, "wb");
    if (!fin || !fout) {
        printf("Error: Cannot open input/output files\n");
        return -1;
    }

    BmpFile file;
    BmpInfo info;
    fread(&file, sizeof(BmpFile), 1, fin);
    fread(&info, sizeof(BmpInfo), 1, fin);

    if (info.Compression != 2) {
        printf("Error: Not a Huffman-compressed BMP\n");
        fclose(fin);
        return -1;
    }

    unsigned int og_size;
    fread(&og_size, sizeof(unsigned int), 1, fin);

    unsigned int freq[256];
    fread(freq, sizeof(unsigned int), 256, fin);

    HuffmanNode* root = build_huffman_tree(freq);
    unsigned char* pD = malloc(og_size); // pixel data
    BitBuffer bb;
    init_bit_buffer(&bb, fin);

    unsigned int pos = 0;
    HuffmanNode* current = root;
    while (pos < og_size) {
        int bit = read_bit(&bb);
        current = bit ? current->right : current->left;
        if (!current->left && !current->right) {
            pD[pos++] = current->symbol;
            current = root;
        }
    }

    file.Offbits = sizeof(BmpFile) + sizeof(BmpInfo);
    file.Size = file.Offbits + og_size + (abs(info.Height) * ((4 - (info.Width * 3) % 4) % 4));
    info.Compression = 0;
    info.SizeImage = 0;

    fwrite(&file, sizeof(BmpFile), 1, fout);
    fwrite(&info, sizeof(BmpInfo), 1, fout);

    int padding = (4 - (info.Width * 3) % 4) % 4;
    int row_size = info.Width * 3;
    unsigned char pad[4] = {0};
    for (int i = 0; i < abs(info.Height); i++) {
        fwrite(pD + (i * row_size), row_size, 1, fout);
        fwrite(pad, padding, 1, fout);
    }

    free(pD);
    free_tree(root);
    fclose(fin);
    fclose(fout);
    printf("Decompressed to %u bytes\n", og_size);
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
