#include <stdio.h>
#include <stdlib.h>

#define BMP_HEADER_SIZE 54
#define BYTES_PER_PIXEL 3

typedef struct {
    int width;
    int height;
    int dataOffset;
} BMPInfo;

// Function to read BMP header and get image info
BMPInfo readBMPHeader(FILE *file) {
    BMPInfo info = {0};
    unsigned char header[BMP_HEADER_SIZE];
    
    if (fread(header, 1, BMP_HEADER_SIZE, file) != BMP_HEADER_SIZE) {
        fprintf(stderr, "Error: Cannot read BMP header\n");
        exit(1);
    }

    // Check BMP signature
    if (header[0] != 'B' || header[1] != 'M') {
        fprintf(stderr, "Error: Not a valid BMP file\n");
        exit(1);
    }

    info.dataOffset = *(int*)&header[10];
    info.width = *(int*)&header[18];
    info.height = *(int*)&header[22];
    
    return info;
}

void write_binary(unsigned char byte, FILE *output_file) {
    for (int i = 7; i >= 0; i--) {
        fprintf(output_file, "%d", (byte >> i) & 1);
    }
}

int main() {
    FILE *input_file = fopen("bd.bmp", "rb");
    FILE *output_file = fopen("output_bits.txt", "w");
    if (!input_file || !output_file) {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    BMPInfo bmpInfo = readBMPHeader(input_file);
    printf("Image size: %dx%d pixels\n", bmpInfo.width, bmpInfo.height);

    // Print header in hex format
    fprintf(output_file, "BMP Header:\n");
    fseek(input_file, 0, SEEK_SET);
    for (int i = 0; i < BMP_HEADER_SIZE; i++) {
        unsigned char byte;
        fread(&byte, 1, 1, input_file);
        fprintf(output_file, "%02X ", byte);
        if ((i + 1) % 16 == 0) fprintf(output_file, "\n");
    }

    // Seek to pixel data
    fseek(input_file, bmpInfo.dataOffset, SEEK_SET);

    // Print pixel data
    fprintf(output_file, "\nPixel Data (RGB in Binary):\n");
    fprintf(output_file, "Each pixel uses 24 bits (3 bytes) of memory\n\n");
    unsigned char pixel[3];
    int padding = (4 - (bmpInfo.width * BYTES_PER_PIXEL) % 4) % 4;
    int total_pixels = 0;

    for (int row = 0; row < bmpInfo.height; row++) {
        for (int col = 0; col < bmpInfo.width; col++) {
            if (fread(pixel, 1, BYTES_PER_PIXEL, input_file) != BYTES_PER_PIXEL) {
                fprintf(stderr, "Error reading pixel data at (%d,%d)\n", col, row);
                goto cleanup;
            }
            
            fprintf(output_file, "Pixel(%d,%d) Memory: 24 bits | ", row, col);
            fprintf(output_file, "R: ");
            write_binary(pixel[2], output_file);
            fprintf(output_file, " G: ");
            write_binary(pixel[1], output_file);
            fprintf(output_file, " B: ");
            write_binary(pixel[0], output_file);
            fprintf(output_file, "\n");
            total_pixels++;
        }
        
        // Skip padding bytes
        fseek(input_file, padding, SEEK_CUR);
        fprintf(output_file, "\n");
    }

    // Print total memory usage
    printf("\nTotal Memory Usage:\n");
    printf("Total Pixels: %d\n", total_pixels);
    printf("Bytes per Pixel: %d\n", BYTES_PER_PIXEL);
    printf("Total Memory: %d bytes (%d bits)\n", total_pixels * BYTES_PER_PIXEL, total_pixels * BYTES_PER_PIXEL * 8);

cleanup:
    fclose(input_file);
    fclose(output_file);
    return EXIT_SUCCESS;
}
