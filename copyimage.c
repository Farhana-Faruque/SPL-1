#include <stdio.h>

void copy_image(const char *source, const char *destination) {
    FILE *src, *dest;
    char buffer[1024];
    size_t bytes;

    // Open source file in binary read mode
    src = fopen(source, "rb");
    if (src == NULL) {
        perror("Error opening source file");
        return;
    }

    // Open destination file in binary write mode
    dest = fopen(destination, "wb");
    if (dest == NULL) {
        perror("Error opening destination file");
        fclose(src);
        return;
    }

    // Copy the contents from source to destination
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }

    // Close the files
    fclose(src);
    fclose(dest);
}

int main() {
    const char *source_file = "spiderman.png"; // Replace with your source image file
    const char *destination_file = "copied_image.png"; // Replace with your destination image file

    copy_image(source_file, destination_file);

    printf("Image copied successfully!\n");

    return 0;
}
