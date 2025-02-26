#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to convert a binary string to an integer
int binary_to_int(const char *binary) {
    int value = 0;
    while (*binary != '\0') {
        value = (value << 1) + (*binary - '0');
        binary++;
    }
    return value;
}

// Function to convert signed integer (-127 to 127) to unsigned (0 to 255)
unsigned char signed_to_unsigned(int value) {
    return (unsigned char)(value + 128);
}

void convert_binary_to_rgb(const char *source, const char *destination) {
    FILE *src, *dest;
    char buffer[9]; // Buffer to hold binary string (8 bits + null terminator)
    int r, g, b;
    int counter = 0;
    
    // Open source file in read mode
    src = fopen(source, "r");
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

    // Read binary strings and convert to RGB values
    while (fgets(buffer, sizeof(buffer), src) != NULL) {
        int value = binary_to_int(buffer);
        unsigned char unsigned_value = signed_to_unsigned(value);

        if (counter == 0) {
            r = unsigned_value;
        } else if (counter == 1) {
            g = unsigned_value;
        } else if (counter == 2) {
            b = unsigned_value;
            fprintf(dest, "R: %d, G: %d, B: %d\n", r, g, b);
            counter = -1;
        }
        counter++;
    }

    // Close the files
    fclose(src);
    fclose(dest);
}

int main() {
    const char *source_file = "image_bits.txt"; // Replace with your binary text file
    const char *destination_file = "image_rgb2.txt"; // Replace with your RGB text file

    convert_binary_to_rgb(source_file, destination_file);

    printf("RGB values stored successfully in %s\n", destination_file);

    return 0;
}
