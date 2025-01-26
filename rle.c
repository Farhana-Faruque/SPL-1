#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Function to safely write to file with error checking
int safeWrite(const void* data, size_t size, size_t count, FILE* file) {
    if (fwrite(data, size, count, file) != count) {
        perror("Write error");
        return 0;
    }
    return 1;
}

// Function to perform Run Length Encoding and write to a binary file
int compressRLE(unsigned char* input, size_t size, FILE* output) {
    if (!input || !output || size == 0) {
        return 0;
    }

    // Write original file size for decompression
    if (!safeWrite(&size, sizeof(size_t), 1, output)) {
        return 0;
    }

    size_t count;
    unsigned char current;

    for (size_t i = 0; i < size; i++) {
        count = 1;
        current = input[i];

        while (i + 1 < size && input[i + 1] == current && count < 255) {
            count++;
            i++;
        }

        // Write the count and value to the binary file
        if (!safeWrite(&count, sizeof(unsigned char), 1, output) ||
            !safeWrite(&current, sizeof(unsigned char), 1, output)) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input image file> <output binary file>\n", argv[0]);
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
        perror("Error opening input file");
        return 1;
    }

    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    // Check for reasonable file size
    if (fileSize <= 0 || fileSize > (1024 * 1024 * 1024)) { // 1GB limit
        fprintf(stderr, "Invalid file size or file too large\n");
        fclose(inputFile);
        return 1;
    }

    unsigned char *imageData = (unsigned char *)malloc(fileSize);
    if (!imageData) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(inputFile);
        return 1;
    }

    fread(imageData, 1, fileSize, inputFile);
    fclose(inputFile);

    FILE *outputFile = fopen(argv[2], "wb");
    if (!outputFile) {
        perror("Error opening output file");
        free(imageData);
        return 1;
    }

    if (!compressRLE(imageData, fileSize, outputFile)) {
        fprintf(stderr, "Compression failed\n");
        free(imageData);
        fclose(outputFile);
        return 1;
    }

    free(imageData);
    fclose(outputFile);

    printf("Compression completed successfully.\n");
    return 0;
}
