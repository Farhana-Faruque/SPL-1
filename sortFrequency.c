#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int value;
    int frequency;
} Frequency;

// Function to compare two frequencies (used in qsort)
int compare(const void *a, const void *b) {
    Frequency *freqA = (Frequency *)a;
    Frequency *freqB = (Frequency *)b;
    return freqB->frequency - freqA->frequency;
}

// Function to sort frequencies and store them in a new file
void sort_and_store_frequencies(const char *source, const char *destination) {
    FILE *src, *dest;
    int value, frequency;
    Frequency freq_array[256];
    int size = 0;

    // Open source file in read mode
    src = fopen(source, "r");
    if (src == NULL) {
        perror("Error opening source file");
        return;
    }

    // Skip the header line
    fscanf(src, "Value, Frequency\n");

    // Read the values and frequencies
    while (fscanf(src, "%d, %d\n", &value, &frequency) == 2) {
        freq_array[size].value = value;
        freq_array[size].frequency = frequency;
        size++;
    }

    // Close the source file
    fclose(src);

    // Sort the frequencies in descending order
    qsort(freq_array, size, sizeof(Frequency), compare);

    // Open destination file in write mode
    dest = fopen(destination, "w");
    if (dest == NULL) {
        perror("Error opening destination file");
        return;
    }

    // Write sorted frequencies to the destination file
    fprintf(dest, "Value, Frequency\n");
    for (int i = 0; i < size; i++) {
        fprintf(dest, "%d, %d\n", freq_array[i].value, freq_array[i].frequency);
    }

    // Close the destination file
    fclose(dest);
}

int main() {
    const char *source_file = "rgb_frequencies.txt"; // Replace with your source file
    const char *destination_file = "sorted_rgb_frequencies.txt"; // Replace with your destination file

    sort_and_store_frequencies(source_file, destination_file);

    printf("Frequencies sorted and stored successfully in %s\n", destination_file);

    return 0;
}
