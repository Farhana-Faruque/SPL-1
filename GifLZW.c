#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DICT_SIZE 4096
#define INITIAL_DICT_SIZE 256
#define GIF_SIGNATURE "GIF89a"
#define MIN_CODE_SIZE 8
#define BLOCK_SIZE 255
#define MAX_CODE_SIZE 12
#define CLEAR_CODE(min_size) (1 << (min_size))
#define END_CODE(min_size) ((1 << (min_size)) + 1)

typedef struct {
    char signature[6];
    unsigned short width;
    unsigned short height;
    unsigned char flags;
    unsigned char background;
    unsigned char aspect;
} GIFHeader;

typedef struct {
    unsigned char *pattern;
    int length;
} DictEntry;

// Dictionary structure
DictEntry dictionary[MAX_DICT_SIZE];
int dict_size;

// Initialize dictionary with single pixel values
void init_dictionary() {
    dict_size = 0;
    for (int i = 0; i < INITIAL_DICT_SIZE; i++) {
        dictionary[i].pattern = (unsigned char*)malloc(1);
        dictionary[i].pattern[0] = i;
        dictionary[i].length = 1;
        dict_size++;
    }
}

// Find pattern in dictionary
int find_pattern(unsigned char *pattern, int length) {
    for (int i = 0; i < dict_size; i++) {
        if (dictionary[i].length == length) {
            if (memcmp(dictionary[i].pattern, pattern, length) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// Add pattern to dictionary
void add_to_dictionary(unsigned char *pattern, int length) {
    if (dict_size < MAX_DICT_SIZE) {
        dictionary[dict_size].pattern = (unsigned char*)malloc(length);
        memcpy(dictionary[dict_size].pattern, pattern, length);
        dictionary[dict_size].length = length;
        dict_size++;
    }
}

// Read GIF header
GIFHeader read_gif_header(FILE *file) {
    GIFHeader header;
    fread(header.signature, 1, 6, file);
    fread(&header.width, 2, 1, file);
    fread(&header.height, 2, 1, file);
    fread(&header.flags, 1, 1, file);
    fread(&header.background, 1, 1, file);
    fread(&header.aspect, 1, 1, file);
    return header;
}

// Verify if file is valid GIF
int is_valid_gif(const GIFHeader *header) {
    return memcmp(header->signature, GIF_SIGNATURE, 6) == 0;
}

// Bit buffer for output
typedef struct {
    unsigned char *data;
    unsigned int current;
    int remainingBits;
    int bytePos;
    int totalBytes;
} BitBuffer;

// Initialize bit buffer
BitBuffer* create_bit_buffer(int size) {
    BitBuffer* buffer = (BitBuffer*)malloc(sizeof(BitBuffer));
    buffer->data = (unsigned char*)calloc(size, 1);
    buffer->current = 0;
    buffer->remainingBits = 8;
    buffer->bytePos = 0;
    buffer->totalBytes = size;
    return buffer;
}

// Write bits to buffer
void write_bits(BitBuffer *buffer, int code, int bits) {
    while (bits > 0) {
        if (buffer->remainingBits == 0) {
            buffer->data[buffer->bytePos++] = buffer->current;
            buffer->current = 0;
            buffer->remainingBits = 8;
        }

        int writeBits = bits < buffer->remainingBits ? bits : buffer->remainingBits;
        int mask = (1 << writeBits) - 1;
        int shift = bits - writeBits;
        int fragment = (code >> shift) & mask;

        buffer->current |= fragment << (buffer->remainingBits - writeBits);
        buffer->remainingBits -= writeBits;
        bits -= writeBits;
    }
}

void flush_bit_buffer(BitBuffer *buffer) {
    if (buffer->remainingBits < 8) {
        buffer->data[buffer->bytePos++] = buffer->current;
    }
}

// Write compressed data blocks
void write_compressed_blocks(FILE *output, BitBuffer *buffer) {
    int remaining = buffer->bytePos;
    int pos = 0;
    
    while (remaining > 0) {
        int blockSize = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
        fputc(blockSize, output);
        fwrite(buffer->data + pos, 1, blockSize, output);
        remaining -= blockSize;
        pos += blockSize;
    }
    
    // Write block terminator
    fputc(0x00, output);
}

// Improved compress_image function
void compress_image(unsigned char *input, int size, FILE *output, GIFHeader *header) {
    // Write GIF header
    fwrite(header->signature, 1, 6, output);
    fwrite(&header->width, 2, 1, output);
    fwrite(&header->height, 2, 1, output);
    fwrite(&header->flags, 1, 1, output);
    fwrite(&header->background, 1, 1, output);
    fwrite(&header->aspect, 1, 1, output);
    
    // Write Global Color Table if present
    if (header->flags & 0x80) {
        int gct_size = 2 << (header->flags & 0x07);
        unsigned char gct[768];  // Max size for global color table
        fwrite(gct, 1, gct_size * 3, output);
    }
    
    // Write Image Descriptor
    fputc(0x2C, output);  // Image Separator
    int left = 0, top = 0;
    fwrite(&left, 2, 1, output);
    fwrite(&top, 2, 1, output);
    fwrite(&header->width, 2, 1, output);
    fwrite(&header->height, 2, 1, output);
    fputc(0x00, output);  // Local Image Descriptor
    
    // Initialize LZW
    init_dictionary();
    int min_code_size = 8;  // Start with 8 bits for 256 colors
    int code_size = min_code_size + 1;
    int clear_code = CLEAR_CODE(min_code_size);
    int end_code = END_CODE(min_code_size);
    int next_code = end_code + 1;
    
    // Write minimum code size
    fputc(min_code_size, output);
    
    // Initialize bit buffer with reasonable size
    BitBuffer *buffer = create_bit_buffer(size);
    
    // Write initial clear code
    write_bits(buffer, clear_code, code_size);
    
    // Initialize compression dictionary
    init_dictionary();
    dict_size = clear_code + 2;  // Start after clear and end codes
    
    int prefix = input[0];
    int current_length = 1;
    
    for (int i = 1; i < size; i++) {
        int character = input[i];
        
        // Build current sequence
        unsigned char *sequence = (unsigned char*)malloc(current_length + 1);
        memcpy(sequence, dictionary[prefix].pattern, dictionary[prefix].length);
        sequence[current_length] = character;
        
        int code = find_pattern(sequence, current_length + 1);
        
        if (code != -1) {
            // Sequence exists in dictionary
            prefix = code;
            current_length++;
        } else {
            // Output code for prefix
            write_bits(buffer, prefix, code_size);
            
            // Add new sequence to dictionary if space allows
            if (dict_size < MAX_DICT_SIZE) {
                add_to_dictionary(sequence, current_length + 1);
                dict_size++;
                
                // Increase code size if needed
                if (dict_size == (1 << code_size) && code_size < MAX_CODE_SIZE) {
                    code_size++;
                }
            } else {
                // Dictionary full - reset
                write_bits(buffer, clear_code, code_size);
                init_dictionary();
                dict_size = clear_code + 2;
                code_size = min_code_size + 1;
            }
            
            prefix = character;
            current_length = 1;
        }
        
        free(sequence);
    }
    
    // Output final code
    write_bits(buffer, prefix, code_size);
    write_bits(buffer, end_code, code_size);
    
    // Flush remaining bits
    flush_bit_buffer(buffer);
    
    // Write compressed data blocks
    write_compressed_blocks(output, buffer);
    
    // Write GIF trailer
    fputc(0x3B, output);
    
    // Cleanup
    free(buffer->data);
    free(buffer);
}

int main() {
    FILE *input_file = fopen("input.gif", "rb");
    FILE *output_file = fopen("compressed.gif", "wb");
    
    if (!input_file || !output_file) {
        printf("Error opening files\n");
        return 1;
    }
    
    // Read GIF header
    GIFHeader header = read_gif_header(input_file);
    
    if (!is_valid_gif(&header)) {
        printf("Invalid GIF file!\n");
        fclose(input_file);
        fclose(output_file);
        return 1;
    }
    
    printf("GIF Image Details:\n");
    printf("Width: %d\n", header.width);
    printf("Height: %d\n", header.height);
    
    // Get image data size
    fseek(input_file, 0, SEEK_END);
    long size = ftell(input_file);
    fseek(input_file, sizeof(GIFHeader), SEEK_SET);
    
    // Modify the image data reading to handle color table
    unsigned char *image_data = (unsigned char*)malloc(size);
    fseek(input_file, sizeof(GIFHeader), SEEK_SET);
    
    // Skip color table if present
    if (header.flags & 0x80) {
        int color_table_size = 3 * (1 << ((header.flags & 7) + 1));
        fseek(input_file, color_table_size, SEEK_CUR);
    }
    
    // Skip other blocks until image data
    unsigned char block_type;
    while ((block_type = fgetc(input_file)) != 0x2C) {
        if (block_type == 0x21) {  // Extension block
            fgetc(input_file);  // Skip extension type
            unsigned char block_size;
            while ((block_size = fgetc(input_file)) != 0) {
                fseek(input_file, block_size, SEEK_CUR);
            }
        }
    }
    
    // Read actual image data
    fseek(input_file, 9, SEEK_CUR);  // Skip image descriptor
    int lzw_min_size = fgetc(input_file);
    
    // Read image data blocks
    int data_size = 0;
    unsigned char block_size;
    while ((block_size = fgetc(input_file)) != 0) {
        fread(image_data + data_size, 1, block_size, input_file);
        data_size += block_size;
    }
    
    // Compress image
    compress_image(image_data, data_size, output_file, &header);
    
    // Cleanup
    free(image_data);
    fclose(input_file);
    fclose(output_file);
    
    printf("Compression complete!\n");
    printf("Original size: %ld bytes\n", size);
    
    // Get compressed file size
    FILE *check_size = fopen("compressed.gif", "rb");
    fseek(check_size, 0, SEEK_END);
    long compressed_size = ftell(check_size);
    fclose(check_size);
    
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - ((float)compressed_size / size)) * 100);
    
    // Free dictionary memory
    for (int i = 0; i < dict_size; i++) {
        free(dictionary[i].pattern);
    }
    
    return 0;
}
