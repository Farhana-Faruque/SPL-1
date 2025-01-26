#include <stdio.h>
#include <stdlib.h>

// Function to perform Run Length Encoding and write to a binary file
void compressRLE(unsigned char* input, int size, FILE* output) {
    int count;
    unsigned char current;

    for (int i = 0; i < size; i++) {
        count = 1;
        current = input[i];

        while (i + 1 < size && input[i + 1] == current) {
            count++;
            i++;
        }

        // Write the count and value to the binary file
        fwrite(&count, sizeof(int), 1, output);
        fwrite(&current, sizeof(unsigned char), 1, output);
    }
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

    compressRLE(imageData, fileSize, outputFile);

    free(imageData);
    fclose(outputFile);

    printf("Compression completed successfully.\n");
    return 0;
}
