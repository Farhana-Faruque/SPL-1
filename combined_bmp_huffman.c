#include <stdio.h>
#include <stdlib.h>
#include "readfile.c"
#include "HuffmanCod2.c"

// Function to write RGB values in binary format
void write_rgb_binary(unsigned char r, unsigned char g, unsigned char b, FILE *output_file) {
    fprintf(output_file, "R: ");
    for (int i = 7; i >= 0; i--) 
        fprintf(output_file, "%d", (r >> i) & 1);
    fprintf(output_file, " G: ");
    for (int i = 7; i >= 0; i--) 
        fprintf(output_file, "%d", (g >> i) & 1);
    fprintf(output_file, " B: ");
    for (int i = 7; i >= 0; i--) 
        fprintf(output_file, "%d", (b >> i) & 1);
}

void compressRGBData(unsigned char* rgb_data, int data_size, FILE* output_file, FILE* binary_file) {
    // Count frequencies for each RGB value
    unsigned freq[256] = {0};
    for (int i = 0; i < data_size; i++) {
        freq[rgb_data[i]]++;
    }

    // Create Huffman codes
    unsigned char* codes[256] = {NULL};
    int code_lengths[256] = {0};
    unsigned char data[256];
    int unique_chars = 0;

    // Prepare data array for Huffman coding
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            data[unique_chars++] = (unsigned char)i;
        }
    }

    // Build Huffman tree and generate codes
    buildHuffmanCodes(data, freq, unique_chars, codes, code_lengths);

    // Write frequency table to binary file
    fwrite(freq, sizeof(unsigned), 256, binary_file);

    // Write compressed data and show binary representation
    fprintf(output_file, "\nHuffman Compressed Data:\n");
    unsigned char buffer = 0;
    int bits_in_buffer = 0;

    for (int i = 0; i < data_size; i++) {
        unsigned char current_byte = rgb_data[i];
        fprintf(output_file, "Original byte %d: ", current_byte);
        write_rgb_binary(current_byte, 0, 0, output_file);
        fprintf(output_file, " -> Huffman code: ");

        // Write Huffman code to output file
        for (int j = 0; j < code_lengths[current_byte]; j++) {
            fprintf(output_file, "%d", codes[current_byte][j]);
            
            // Add bit to buffer for binary file
            buffer = (buffer << 1) | codes[current_byte][j];
            bits_in_buffer++;

            if (bits_in_buffer == 8) {
                fwrite(&buffer, 1, 1, binary_file);
                buffer = 0;
                bits_in_buffer = 0;
            }
        }
        fprintf(output_file, "\n");
    }

    // Write remaining bits if any
    if (bits_in_buffer > 0) {
        buffer = buffer << (8 - bits_in_buffer);
        fwrite(&buffer, 1, 1, binary_file);
    }

    for (int i = 0; i < 256; i++) {
        if (codes[i] != NULL) {
            free(codes[i]);
        }
    }
}

int main() {
    // Open BMP file
    FILE *input_file = fopen("bd.bmp", "rb");
    FILE *output_file = fopen("output_bits.txt", "w");
    FILE *compressed_file = fopen("compressed.bin", "wb");
    
    if (!input_file || !output_file || !compressed_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    // Read BMP header and info
    BMPInfo bmpInfo = readBMPHeader(input_file);
    
    // Calculate total RGB data size
    int total_size = bmpInfo.width * bmpInfo.height * 3;
    unsigned char* rgb_data = (unsigned char*)malloc(total_size);
    
    fseek(input_file, bmpInfo.dataOffset, SEEK_SET);
    fread(rgb_data, 1, total_size, input_file);

    // Write original BMP info
    fprintf(output_file, "Original BMP Information:\n");
    fprintf(output_file, "Width: %d, Height: %d\n", bmpInfo.width, bmpInfo.height);
    fprintf(output_file, "Total RGB bytes: %d\n\n", total_size);

    // Compress and write data
    compressRGBData(rgb_data, total_size, output_file, compressed_file);

    free(rgb_data);
    fclose(input_file);
    fclose(output_file);
    fclose(compressed_file);

    printf("Compression completed. Check output_bits.txt for binary representation\n");
    printf("and compressed.bin for compressed data.\n");

    return EXIT_SUCCESS;
}
