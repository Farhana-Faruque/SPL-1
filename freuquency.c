#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to count frequencies of RGB values
void count_rgb_frequencies(const char *source, const char *destination) {
    FILE *src, *dest;
    int r, g, b;
    int rgb_frequencies[256] = {0};

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

    // Read RGB values and count frequencies
    while (fscanf(src, "R: %d, G: %d, B: %d\n", &r, &g, &b) == 3) {
        rgb_frequencies[r]++;
        rgb_frequencies[g]++;
        rgb_frequencies[b]++;
    }

    // Write frequencies to destination file
    fprintf(dest, "Value, Frequency\n");
    for (int i = 0; i < 256; i++) {
        fprintf(dest, "%d, %d\n", i, rgb_frequencies[i]);
    }

    // Close the files
    fclose(src);
    fclose(dest);
}

int main() {
    const char *source_file = "image_rgb2.txt"; // Replace with your RGB text file
    const char *destination_file = "rgb_frequencies.txt"; // Replace with your frequency text file

    count_rgb_frequencies(source_file, destination_file);

    printf("RGB frequencies stored successfully in %s\n", destination_file);

    return 0;
}
