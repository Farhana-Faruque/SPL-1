#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>

// For PGM image compression

#include "pgmHuff.c"
#include "rle.c"
#include "pgmLZW.c"

// For BMP image compression
#include "combined_bmp_huffman.c"
#include "RunLengthForBMP.c"
#include "lzwBMP.c"

// For JPEG image compression
#include "JpegDCT.c"

