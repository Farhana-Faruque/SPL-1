#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// BMP File Header (14 bytes)
typedef struct {
    uint16_t type;      // Magic number "BM" (0x4D42)
    uint32_t size;      // File size in bytes
    uint16_t reserved1; // Reserved
    uint16_t reserved2; // Reserved
    uint32_t offset;    // Offset to image data
} BMPFileHeader;

// BMP Info Header (40 bytes - Windows BITMAPINFOHEADER)
typedef struct {
    uint32_t size;          // Header size
    int32_t width;         // Image width in pixels
    int32_t height;        // Image height in pixels
    uint16_t planes;       // Number of color planes (must be 1)
    uint16_t bitsPerPixel; // Bits per pixel (1, 4, 8, 16, 24, 32)
    uint32_t compression;  // Compression type
    uint32_t imageSize;    // Image data size
    int32_t xPixelsPerM;   // Horizontal resolution
    int32_t yPixelsPerM;   // Vertical resolution
    uint32_t colorsUsed;   // Number of colors in palette
    uint32_t colorsImportant; // Important colors
} BMPInfoHeader;

int main() {
    FILE *file;
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    
    // Open BMP file in binary read mode
    file = fopen("bd.bmp", "rb");
    if (file == NULL) {
        printf("Error: Cannot open file\n");
        return 1;
    }

    // Read File Header
    fread(&fileHeader.type, sizeof(uint16_t), 1, file);
    fread(&fileHeader.size, sizeof(uint32_t), 1, file);
    fread(&fileHeader.reserved1, sizeof(uint16_t), 1, file);
    fread(&fileHeader.reserved2, sizeof(uint16_t), 1, file);
    fread(&fileHeader.offset, sizeof(uint32_t), 1, file);

    // Check if it's a valid BMP file
    if (fileHeader.type != 0x4D42) {  // "BM" in little-endian
        printf("Error: Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    // Read Info Header
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, file);

    // Print basic information
    printf("BMP File Information:\n");
    printf("File Size: %u bytes\n", fileHeader.size);
    printf("Image Width: %d pixels\n", infoHeader.width);
    printf("Image Height: %d pixels\n", infoHeader.height);
    printf("Bits Per Pixel: %u\n", infoHeader.bitsPerPixel);
    printf("Compression: %u\n", infoHeader.compression);
    printf("Data Offset: %u bytes\n", fileHeader.offset);

    // Calculate image data size if not compressed
    if (infoHeader.compression == 0) {
        int rowSize = ((infoHeader.width * infoHeader.bitsPerPixel + 31) / 32) * 4; // Row size padded to 4-byte boundary
        int dataSize = rowSize * abs(infoHeader.height);
        
        // Move to image data
        fseek(file, fileHeader.offset, SEEK_SET);
        
        // Allocate memory for image data
        unsigned char *imageData = (unsigned char *)malloc(dataSize);
        if (imageData == NULL) {
            printf("Error: Memory allocation failed\n");
            fclose(file);
            return 1;
        }

        // Read image data
        fread(imageData, 1, dataSize, file);
        
        printf("Successfully read image data\n");
        // Here you can process the image data as needed
        
        free(imageData);
    } else {
        printf("Note: This code only handles uncompressed BMP files\n");
    }

    fclose(file);
    return 0;
}