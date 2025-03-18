#ifndef HUFFMAN_PGM_H
#define HUFFMAN_PGM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

#define MAX_SIZE 256
#define MAX_LINE 1024

// Huffman tree node
typedef struct Node {
    unsigned char data;
    unsigned int freq;
    struct Node *left, *right;
} Node;

// Min Heap structure for Huffman tree building
typedef struct {
    Node** array;
    int size;
    int capacity;
} MinHeap;

// Helper functions for Huffman coding
Node* newNode(unsigned char data, unsigned int freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        printf("Failed to allocate memory for new node\n");
        return NULL;
    }
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

MinHeap* createMinHeap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!minHeap) {
        printf("Failed to allocate memory for min heap\n");
        return NULL;
    }
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (Node**)malloc(capacity * sizeof(Node*));
    if (!minHeap->array) {
        printf("Failed to allocate memory for min heap array\n");
        free(minHeap);
        return NULL;
    }
    return minHeap;
}

void freeHuffmanTree(Node* root) {
    if (root) {
        freeHuffmanTree(root->left);
        freeHuffmanTree(root->right);
        free(root);
    }
}

void swapNode(Node** a, Node** b) {
    Node* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq)
        smallest = left;
    if (right < minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq)
        smallest = right;
    if (smallest != idx) {
        swapNode(&minHeap->array[idx], &minHeap->array[smallest]);
        minHeapify(minHeap, smallest);
    }
}

Node* extractMin(MinHeap* minHeap) {
    if (minHeap->size == 0) return NULL;
    Node* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    minHeap->size--;
    minHeapify(minHeap, 0);
    return temp;
}

void insertMinHeap(MinHeap* minHeap, Node* node) {
    minHeap->size++;
    int i = minHeap->size - 1;
    while (i && node->freq < minHeap->array[(i - 1) / 2]->freq) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    minHeap->array[i] = node;
}

void buildMinHeap(MinHeap* minHeap) {
    int n = minHeap->size - 1;
    for (int i = (n - 1) / 2; i >= 0; i--)
        minHeapify(minHeap, i);
}

MinHeap* createAndBuildMinHeap(unsigned int* freq) {
    MinHeap* minHeap = createMinHeap(MAX_SIZE);
    if (!minHeap) return NULL;
    for (int i = 0; i < MAX_SIZE; i++) {
        if (freq[i] > 0) {
            Node* node = newNode((unsigned char)i, freq[i]);
            if (!node) {
                free(minHeap->array);
                free(minHeap);
                return NULL;
            }
            minHeap->array[minHeap->size++] = node;
        }
    }
    if (minHeap->size == 0) {
        printf("No frequencies found to build heap\n");
        free(minHeap->array);
        free(minHeap);
        return NULL;
    }
    buildMinHeap(minHeap);
    return minHeap;
}

Node* buildHuffmanTree(unsigned int* freq) {
    MinHeap* minHeap = createAndBuildMinHeap(freq);
    if (!minHeap) {
        printf("Failed to create and build min heap\n");
        return NULL;
    }
    
    while (minHeap->size > 1) {
        Node* left = extractMin(minHeap);
        Node* right = extractMin(minHeap);
        Node* top = newNode('$', left->freq + right->freq);
        if (!top) {
            freeHuffmanTree(left);
            freeHuffmanTree(right);
            free(minHeap->array);
            free(minHeap);
            return NULL;
        }
        top->left = left;
        top->right = right;
        insertMinHeap(minHeap, top);
    }
    
    Node* root = extractMin(minHeap);
    if (!root) {
        printf("Failed to extract root from min heap\n");
    }
    free(minHeap->array);
    free(minHeap);
    return root;
}

void generateCodes(Node* root, char* code, int top, char codes[MAX_SIZE][MAX_SIZE], int* lengths) {
    if (!root) return;
    if (root->left) {
        code[top] = '0';
        generateCodes(root->left, code, top + 1, codes, lengths);
    }
    if (root->right) {
        code[top] = '1';
        generateCodes(root->right, code, top + 1, codes, lengths);
    }
    if (!root->left && !root->right) {
        code[top] = '\0';
        strcpy(codes[root->data], code);
        lengths[root->data] = top;
    }
}

#endif 