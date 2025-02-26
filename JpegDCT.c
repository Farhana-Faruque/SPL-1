#include <stdio.h>
#include <stdlib.h>

#include <jpeglib.h>

void compress_jpeg_file(const char *input_filename, const char *output_filename, int quality) {
    // JPEG structures for decompression and compression
    struct jpeg_decompress_struct cinfo;
    struct jpeg_compress_struct cinfo_out;
    struct jpeg_error_mgr jerr;

    // Open the input file
    FILE *infile = fopen(input_filename, "rb");
    if (!infile) {
        fprintf(stderr, "Cannot open input file: %s\n", input_filename);
        return;
    }

    // Open the output file
    FILE *outfile = fopen(output_filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Cannot open output file: %s\n", output_filename);
        fclose(infile);
        return;
    }

    // Set up error handling
    cinfo.err = jpeg_std_error(&jerr);
    cinfo_out.err = jpeg_std_error(&jerr);

    // Initialize decompression
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    // Initialize compression
    jpeg_create_compress(&cinfo_out);
    jpeg_stdio_dest(&cinfo_out, outfile);

    // Set compression parameters
    cinfo_out.image_width = cinfo.output_width;
    cinfo_out.image_height = cinfo.output_height;
    cinfo_out.input_components = cinfo.output_components;
    cinfo_out.in_color_space = cinfo.out_color_space;
    jpeg_set_defaults(&cinfo_out);
    jpeg_set_quality(&cinfo_out, quality, TRUE);

    // Start compression
    jpeg_start_compress(&cinfo_out, TRUE);

    // Allocate memory for row buffer
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);

    // Process scanlines
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        jpeg_write_scanlines(&cinfo_out, buffer, 1);
    }

    // Finish compression
    jpeg_finish_compress(&cinfo_out);
    jpeg_destroy_compress(&cinfo_out);

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    // Close files
    fclose(infile);
    fclose(outfile);

    printf("Compression complete: %s -> %s (Quality: %d)\n", input_filename, output_filename, quality);
}

int DCT() {
    char inputFile[256];
    printf("Enter the JPEG file name: ");
    scanf("%255s", inputFile);

    // Check if the input file exists
    FILE *input = fopen(inputFile, "rb");
    if (!input) {
        printf("Cannot open input file: %s\n", inputFile);
        return 1;
    }
    fclose(input); // Close after checking, it will be opened again in `compress_jpeg_file()`

    int quality;
    printf("0 = Maximum compression (Lowest quality, highest compression)\n");
    printf("100 = Minimum compression (Highest quality, lowest compression)\n");
    printf("Enter the Quality (0-100): ");
    scanf("%d", &quality);

    if (quality < 0 || quality > 100) {
        fprintf(stderr, "Quality must be between 0 and 100.\n");
        return 1;
    }

    // Output file name
    char outputFile[] = "compressed.jpeg";

    // Call the function with filenames, not file pointers
    compress_jpeg_file(inputFile, outputFile, quality);

    return 0;
}
