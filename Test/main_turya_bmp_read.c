#include<stdio.h>
#include<malloc.h>

char * hexToBin_8bit(unsigned char x) {
  char *bin = (char *)malloc(9);
  bin[8] = '\0';

  for(int i = 0; i < 8; i++) {
    bin[7 - i] = (x & (1 << i)) ? '1' : '0';
  }
  return bin;
}

int main(int argc, char const *argv[])
{
  FILE *img_file;
  unsigned char header[2];
  unsigned int file_size;
  if (argc != 2)
  {
    printf("Usage: %s <image file>\n", argv[0]);
    return 1;
  }
  img_file = fopen(argv[1], "rb");

  // read header and check if the file is BMP type or not
  fseek(img_file, 0, SEEK_SET);
  fread(header, 1, 2, img_file);
  if (header[0] != 'B' || header[1] != 'M')
  {
    printf("Not a BMP file\n");
    return 1;
  }

  // else print all the information in header
  fseek(img_file, 0, SEEK_SET);
  unsigned char *header_data = (unsigned char *)malloc(54);
  fread(header_data, 1, 54, img_file);
  printf("Header data:");
  for (int i = 0; i < 54; i++)
  {
    break;
    printf("%d = %02x\n", *(unsigned int *)(header_data + i), header_data[i]);
  }
  printf("\n");
  file_size = *(unsigned int *)(header_data + 2);
  printf("\nFile size: %d kB\n", file_size / 1024);

  // print height and width in pixel
  unsigned int width = *(unsigned int *)(header_data + 18);
  unsigned int height = *(unsigned int *)(header_data + 22);

  printf("Width : %dpx  (%0x)\n", width, *(header_data + 18));
  printf("Height: %dpx  (%0x)\n", height, *(header_data + 22));

  // Print image size, horizontal resolution, vertical resolution, and colors in color table
  unsigned int image_size = *(unsigned int *)(header_data + 34);
  unsigned int horizontal_resolution = *(unsigned int *)(header_data + 38);
  unsigned int vertical_resolution = *(unsigned int *)(header_data + 42);
  unsigned int colors_in_color_table = *(unsigned int *)(header_data + 46);

  // printf("Image size: %d kB\n", image_size / 1024);
  // printf("Header size: %d kB\n", (file_size - image_size) / 1024);

  // printf("Horizontal resolution: %d pixels/meter\n", horizontal_resolution);
  // printf("Vertical resolution: %d pixels/meter\n", vertical_resolution);
  // printf("Colors in color table: %d\n", colors_in_color_table);

  /* Now read color bit/pixels */

  printf("Colors in color table: %d\n", colors_in_color_table);

  // Get to pixel data array
  fseek(img_file, *(unsigned int *)(header_data + 10), SEEK_SET);  // Seek to pixel array start

  // Allocate memory for pixel data
  unsigned char *pixel_data = (unsigned char *)malloc(3 * width * height);
  
  // Calculate row padding (rows must be aligned to 4 bytes)
  int padding = (4 - ((width * 3) % 4)) % 4;

  // Read and print RGB values for each pixel
  printf("\nPixel Data (RGB):\n");
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      unsigned char blue, green, red;
      fread(&blue, 1, 1, img_file);
      fread(&green, 1, 1, img_file);
      fread(&red, 1, 1, img_file);
      
      // Print RGB values in hexadecimal
      printf("Pixel[%d,%d]: R: %02x, G: %02x, B: %02x\n", x, y,  (red),  (green),  (blue));
    }
    // Skip padding bytes at the end of each row
    fseek(img_file, padding, SEEK_CUR);
  }

  // Free allocated memory
  free(pixel_data);
  free(header_data);
  fclose(img_file);

  return 0;
}
