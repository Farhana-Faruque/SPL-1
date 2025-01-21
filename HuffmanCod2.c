#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Huffman tree node structure
typedef struct Node {
    unsigned char data;
    unsigned freq;
    struct Node *left, *right;
} Node;

// Priority queue node
typedef struct QueueNode {
    Node *node;
    struct QueueNode *next;
} QueueNode;

// Create a new Huffman tree node
Node* createNode(unsigned char data, unsigned freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

// Add node to priority queue (sorted by frequency)
void enqueue(QueueNode** head, Node* tree_node) {
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->node = tree_node;
    
    // Empty queue or new frequency is less than head
    if (*head == NULL || tree_node->freq < (*head)->node->freq) {
        new_node->next = *head;
        *head = new_node;
        return;
    }
    
    // Find position to insert
    QueueNode* current = *head;
    while (current->next != NULL && current->next->node->freq <= tree_node->freq) {
        current = current->next;
    }
    
    new_node->next = current->next;
    current->next = new_node;
}

// Remove and return the node with lowest frequency
Node* dequeue(QueueNode** head) {
    if (*head == NULL) return NULL;
    
    QueueNode* temp = *head;
    Node* node = temp->node;
    *head = temp->next;
    free(temp);
    return node;
}

// Store Huffman codes in arrays
void storeCode(Node* root, int* code_array, int top, unsigned char character, unsigned char* codes[], int* code_lengths) {
    if (root->left) {
        code_array[top] = 0;
        storeCode(root->left, code_array, top + 1, character, codes, code_lengths);
    }
    
    if (root->right) {
        code_array[top] = 1;
        storeCode(root->right, code_array, top + 1, character, codes, code_lengths);
    }
    
    // If leaf node, store the code
    if (!root->left && !root->right && root->data == character) {
        codes[character] = (unsigned char*)malloc(top);
        for (int i = 0; i < top; i++) {
            codes[character][i] = code_array[i];
        }
        code_lengths[character] = top;
    }
}

// Build Huffman tree and generate codes
void buildHuffmanCodes(unsigned char* data, unsigned* freq, int size, unsigned char* codes[], int* code_lengths) {
    QueueNode* queue = NULL;
    
    // Create initial queue of leaf nodes
    for (int i = 0; i < size; i++) {
        if (freq[i] > 0) {
            Node* node = createNode(data[i], freq[i]);
            enqueue(&queue, node);
        }
    }
    
    // Build Huffman tree
    while (queue != NULL && queue->next != NULL) {
        Node* left = dequeue(&queue);
        Node* right = dequeue(&queue);
        
        Node* parent = createNode('$', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        
        enqueue(&queue, parent);
    }
    
    // Get root of Huffman tree
    Node* root = queue->node;
    
    // Generate codes for each character
    int code_array[256];
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            storeCode(root, code_array, 0, (unsigned char)i, codes, code_lengths);
        }
    }
}

void compressFile(const char* input_path, const char* output_path) {
    // Open input file
    FILE* input = fopen(input_path, "rb");
    if (!input) {
        printf("Error opening input file\n");
        return;
    }
    
    // Count frequencies
    unsigned freq[256] = {0};
    unsigned char ch;
    while (fread(&ch, 1, 1, input) == 1) {
        freq[ch]++;
    }
    
    // Create arrays for Huffman codes
    unsigned char* codes[256] = {NULL};
    int code_lengths[256] = {0};
    unsigned char data[256];
    int size = 0;
    
    // Prepare data array
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            data[size++] = (unsigned char)i;
        }
    }
    
    // Generate Huffman codes
    buildHuffmanCodes(data, freq, size, codes, code_lengths);
    
    // Open output file
    FILE* output = fopen(output_path, "wb");
    if (!output) {
        printf("Error opening output file\n");
        fclose(input);
        return;
    }
    
    // Write header (frequencies)
    fwrite(freq, sizeof(unsigned), 256, output);
    
    // Reset input file for compression
    fseek(input, 0, SEEK_SET);
    
    // Write compressed data
    unsigned char buffer = 0;
    int bits_in_buffer = 0;
    
    while (fread(&ch, 1, 1, input) == 1) {
        for (int i = 0; i < code_lengths[ch]; i++) {
            buffer = (buffer << 1) | codes[ch][i];
            bits_in_buffer++;
            
            if (bits_in_buffer == 8) {
                fwrite(&buffer, 1, 1, output);
                buffer = 0;
                bits_in_buffer = 0;
            }
        }
    }
    
    // Write remaining bits if any
    if (bits_in_buffer > 0) {
        buffer = buffer << (8 - bits_in_buffer);
        fwrite(&buffer, 1, 1, output);
    }
    
    // Cleanup
    for (int i = 0; i < 256; i++) {
        if (codes[i] != NULL) {
            free(codes[i]);
        }
    }
    
    fclose(input);
    fclose(output);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    compressFile(argv[1], argv[2]);
    printf("Compression completed\n");
    return 0;
}
