
#ifndef __IMAGE_H_
#define __IMAGE_H_


#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for storing the image data
typedef struct PGMImage {
	char pgmType[3];
	unsigned char** data;
	unsigned int width;
	unsigned int height;
	unsigned int maxColorValue;
} PGMImage;

#define COLOR_RANGE 255
#define IMAGE_CREATOR "Sabbir"


#endif