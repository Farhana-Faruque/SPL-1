#include <stdio.h>
#include <stdlib.h>

// BMP File Header structure (14 bytes)
#pragma pack(push, 1) // Ensure no padding
typedef struct {
    unsigned short bfType;      // "BM" signature (0x4D42)
    unsigned int bfSize;        // File size in bytes
    unsigned short bfReserved1; // Reserved, must be 0
    unsigned short bfReserved2; // Reserved, must be 0
    unsigned int bfOffBits;     // Offset to pixel data
} BITMAPFILEHEADER;

// BMP Info Header structure (40 bytes for BITMAPINFOHEADER)
typedef struct {
    unsigned int biSize;         // Size of this header
    int biWidth;                // Width in pixels
    int biHeight;               // Height in pixels
    unsigned short biPlanes;     // Number of color planes (must be 1)
    unsigned short biBitCount;   // Bits per pixel (1, 4, 8, 24, 32)
    unsigned int biCompression;  // Compression type
    unsigned int biSizeImage;    // Image data size
    int biXPelsPerMeter;        // Horizontal resolution
    int biYPelsPerMeter;        // Vertical resolution
    unsigned int biClrUsed;      // Number of colors in palette
    unsigned int biClrImportant; // Important colors
} BITMAPINFOHEADER;
#pragma pack(pop)

int main() {
    FILE *file;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Open BMP file in binary read mode
    file = fopen("bd.bmp", "rb");
    if (file == NULL) {
        printf("Error: Cannot open file\n");
        return 1;
    }

    // Read BMP File Header
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    
    // Check if it's a valid BMP file
    if (fileHeader.bfType != 0x4D42) { // 'BM' in little-endian
        printf("Error: Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    // Read BMP Info Header
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    // Print basic information
    printf("File Size: %u bytes\n", fileHeader.bfSize);
    printf("Pixel Data Offset: %u bytes\n", fileHeader.bfOffBits);
    printf("Image Width: %d pixels\n", infoHeader.biWidth);
    printf("Image Height: %d pixels\n", infoHeader.biHeight);
    printf("Bits Per Pixel: %d\n", infoHeader.biBitCount);
    printf("Compression: %u\n", infoHeader.biCompression);

    // Calculate size of pixel data
    int pixelDataSize = infoHeader.biWidth * abs(infoHeader.biHeight) * (infoHeader.biBitCount / 8);
    
    // Move to pixel data
    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    // Allocate memory for pixel data
    unsigned char *pixelData = (unsigned char *)malloc(pixelDataSize);
    if (pixelData == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    // Read pixel data
    fread(pixelData, 1, pixelDataSize, file);

    // Example: Print first pixel's RGB values (assuming 24-bit BMP)
    if (infoHeader.biBitCount == 24) {
        printf("First pixel - Blue: %d, Green: %d, Red: %d\n",
               pixelData[0], pixelData[1], pixelData[2]);
    }

    // Clean up
    free(pixelData);
    fclose(file);

    return 0;
}