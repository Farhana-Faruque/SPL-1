#include <stdio.h>
#include <stdlib.h>

// Structure to hold BMP file information
typedef struct {
    long fileSize;
    long dataOffset;
    int headerBytes;
    int width;
    int height;
    long bitsPerPixel;
} BMPInfo;

// Function to open BMP file
FILE* openBMPFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error Opening File!!\n");
        exit(1);
    }
    return file;
}

// Function to read BMP file signature
void readBMPSignature(FILE* file) {
    printf("Getting file ID...\n\n");
    for (int i = 0; i < 2; i++) {
        char data;
        fread(&data, 1, 1, file);
        printf("%c", data);
    }
    printf("\n\n");
}

// Function to read BMP file information
BMPInfo readBMPInfo(FILE* file) {
    BMPInfo info;
    
    printf("Getting size of BMP File...\n\n");
    fread(&info.fileSize, 4, 1, file);
    printf("Size of the BMP File:: %ld bytes\n\n", info.fileSize);
    
    printf("Getting offset where the pixel array starts...\n\n");
    fseek(file, 10, SEEK_SET);
    fread(&info.dataOffset, 4, 1, file);
    printf("Bitmap data offset:: %ld\n\n", info.dataOffset);
    
    printf("DIB Header\n\n");
    fread(&info.headerBytes, 4, 1, file);
    printf("Number of bytes in header:: %d\n\n", info.headerBytes);
    
    fread(&info.width, 4, 1, file);
    fread(&info.height, 4, 1, file);
    printf("Width of Image: %d\n", info.width);
    printf("Height of image: %d\n\n", info.height);
    
    fseek(file, 2, SEEK_CUR);
    fread(&info.bitsPerPixel, 2, 1, file);
    printf("Number of bits per pixel: %ld\n\n", info.bitsPerPixel);
    
    return info;
}

// Function to allocate memory for image array
int** createImageArray(int height, int width) {
    printf("Creating Image array...\n\n");
    int** image = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++) {
        image[i] = (int*)malloc(width * sizeof(int));
    }
    return image;
}

// Function to read pixel data into image array
void readPixelData(FILE* file, int** image, BMPInfo info) {
    int numbytes = (info.fileSize - info.dataOffset) / 3;
    printf("Number of bytes: %d \n\n", numbytes);
    
    printf("Reading the BMP File into Image Array...\n\n");
    fseek(file, info.dataOffset, SEEK_SET);
    
    for (int r = 0; r < info.height; r++) {
        for (int c = 0; c < info.width; c++) {
            int temp;
            fread(&temp, 3, 1, file);
            temp = temp & 0x0000FF;
            image[r][c] = temp;
        }
    }
    printf("Reading the BMP File into Image Array Completed...\n\n");
}

// Function to free allocated memory
void freeImageArray(int** image, int height) {
    for (int i = 0; i < height; i++) {
        free(image[i]);
    }
    free(image);
}

int main() {
    // Open BMP file
    FILE* image_file = openBMPFile("bd.bmp");
    
    // Read file header
    readBMPSignature(image_file);
    
    // Read BMP information
    BMPInfo bmpInfo = readBMPInfo(image_file);
    
    // Create image array
    int** image = createImageArray(bmpInfo.height, bmpInfo.width);
    
    // Read pixel data
    readPixelData(image_file, image, bmpInfo);
    
    // Clean up
    freeImageArray(image, bmpInfo.height);
    fclose(image_file);
    
    return EXIT_SUCCESS;
}