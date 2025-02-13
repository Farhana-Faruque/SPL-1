#include <stdio.h>
#include <stdlib.h>
#include "readfile.c"
#include "HuffmanCod2.c"

void write_rgb_binary(unsigned char r, unsigned char g, unsigned char b, FILE *output_file) {
    //printf("Debugging: r = %d, g = %d, b = %d\n", r, g, b);
    // fprintf(output_file, "R: ");
    // for (int i = 7; i >= 0; i--) 
    //     fprintf(output_file, "%d", (r >> i) & 1);
    // fprintf(output_file, " G: ");
    // for (int i = 7; i >= 0; i--) 
    //     fprintf(output_file, "%d", (g >> i) & 1);
    // fprintf(output_file, " B: ");
    // for (int i = 7; i >= 0; i--) 
    //     fprintf(output_file, "%d", (b >> i) & 1);
    fprintf(output_file, "R: %02x G: %02x B: %02x ", r, g, b);
}

void compressRGBData(unsigned char* rgb_data, int data_size, FILE* output_file, FILE* binary_file, BMPInfo bmpInfo) {
    unsigned int freq[256] = {0};
    for (int i = 0; i < data_size; i++) {
        freq[rgb_data[i]]++;
    }

    unsigned char* codes[256] = {NULL};
    int code_lengths[256] = {0};
    unsigned char data[256];
    int unique_chars = 0;

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            data[unique_chars++] = (unsigned char)i;
        }
    }

    buildHuffmanCodes(data, freq, unique_chars, codes, code_lengths);

    fwrite(freq, sizeof(unsigned), 256, binary_file);

    fprintf(output_file, "\nHuffman Compressed Data:\n");
    unsigned char buffer = 0;
    int bits_in_buffer = 0;

    for (int i = 0; i < data_size; i+=3) {
        unsigned char r = rgb_data[i];
        unsigned char g = rgb_data[i+1];
        unsigned char b = rgb_data[i+2];

        // printf("%02x ", current_byte);
        fprintf(output_file, "Original byte {%d,%d,%d} -> ", r,g,b);
        write_rgb_binary(r, g, b, output_file);
        fprintf(output_file, " -> Huffman code: ");

        // huffman code for RED 
        for (int j = 0; j < code_lengths[r]; j++) {
            fprintf(output_file, "%d", codes[r][j]);

            buffer = (buffer << 1) | codes[r][j];
            bits_in_buffer++;

            if (bits_in_buffer == 8) {
                fwrite(&buffer, 1, 1, binary_file);
                buffer = 0;
                bits_in_buffer = 0;
            }
        }

        // huffman code for GREEN
        for (int j = 0; j < code_lengths[g]; j++) {
            fprintf(output_file, "%d", codes[g][j]);

            buffer = (buffer << 1) | codes[g][j];
            bits_in_buffer++;

            if (bits_in_buffer == 8) {
                fwrite(&buffer, 1, 1, binary_file);
                buffer = 0;
                bits_in_buffer = 0;
            }
        }

        // huffman code for BLUE
        for (int j = 0; j < code_lengths[b]; j++) {
            fprintf(output_file, "%d", codes[b][j]);

            buffer = (buffer << 1) | codes[b][j];
            bits_in_buffer++;

            if (bits_in_buffer == 8) {
                fwrite(&buffer, 1, 1, binary_file);
                buffer = 0;
                bits_in_buffer = 0;
            }
        }
        fprintf(output_file, "\n");
    }

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
    char inputFileName[256];
    printf("Enter the BMP file name: ");
    scanf("%255s", inputFileName);

    FILE *input_file = fopen(inputFileName, "rb");
    FILE *output_file = fopen("output_bits.txt", "w");
    FILE *compressed_file = fopen("compressed.bin", "wb");
    
    if (!input_file || !output_file || !compressed_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    BMPInfo bmpInfo = readBMPHeader(input_file);
    
    // Calculate total RGB data size
    int total_size = bmpInfo.width * bmpInfo.height * 3;
    unsigned char* rgb_data = (unsigned char*)malloc(total_size);

    fseek(input_file, bmpInfo.dataOffset, SEEK_SET);
    int padding = (4 - ((bmpInfo.width * 3) % 4)) % 4;

    for(int y = 0; y < bmpInfo.height; y++){
      for(int x = 0; x < bmpInfo.width; x++){
        unsigned char blue, green, red;
        fread(&blue, 1, 1, input_file);
        fread(&green, 1, 1, input_file);
        fread(&red, 1, 1, input_file);

        rgb_data[(y * bmpInfo.width + x) * 3] = red;
        rgb_data[(y * bmpInfo.width + x) * 3 + 1] = green;
        rgb_data[(y * bmpInfo.width + x) * 3 + 2] = blue;
        
      }
      // Skip padding bytes at the end of each row
      fseek(input_file, padding, SEEK_CUR);
    }


    fread(rgb_data, 1, total_size, input_file);

    fprintf(output_file, "Original BMP Information:\n");
    fprintf(output_file, "Width: %d, Height: %d\n", bmpInfo.width, bmpInfo.height);
    fprintf(output_file, "Total RGB bytes: %d\n", total_size);

    compressRGBData(rgb_data, total_size, output_file, compressed_file, bmpInfo);

    free(rgb_data);
    fclose(input_file);
    fclose(output_file);
    fclose(compressed_file);

    printf("Compression completed. Check output_bits.txt for binary representation\n");
    printf("and compressed.bin for compressed data.\n");

    return EXIT_SUCCESS;
}
