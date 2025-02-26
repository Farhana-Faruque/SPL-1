#include <stdio.h>

void read_image_and_store_bits(const char *source, const char *destination) {
    FILE *src, *dest;
    int byte;

    // Open source file in binary read mode
    src = fopen(source, "rb");
    if (src == NULL) {
        perror("Error opening source file");
        return;
    }

    // Open destination file in write mode
    dest = fopen(destination, "w");
    if (dest == NULL) {
        perror("Error opening destination file");
        fclose(src);
        return;
    }

    // Read each byte from the source file and write its binary representation to the destination file
    while ((byte = fgetc(src)) != EOF) {
        for (int i = 7; i >= 0; i--) {
            fprintf(dest, "%d", (byte >> i) & 1);
        }
        fprintf(dest, "\n"); // Add a new line after each byte for readability
    }

    // Close the files
    fclose(src);
    fclose(dest);
}

int main() {
    const char *source_file = "bd.bmp"; // Replace with your source image file
    const char *destination_file = "image_bits.txt"; // Replace with your destination text file

    read_image_and_store_bits(source_file, destination_file);

    printf("Image bits stored successfully in %s\n", destination_file);

    return 0;
}
