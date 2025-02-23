#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For BMP image compression
#include "combined_bmp_huffman.c"
#include "RunLengthForBMP.c"
#include "lzwBMP.c"

// For PGM image compression

// P2 Header
#include "p2Huff.c"
#include "p2Run.c"
#include "p2LZW.c"

// P5 Header
#include "p5Huff.c"
#include "p5Run.c"
#include "p5LZW.c"

// For JPEG image compression

