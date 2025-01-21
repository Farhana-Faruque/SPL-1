#include <stdio.h>
#include <stdlib.h>

// A Huffman tree node
typedef struct Node {
    unsigned char data;
    unsigned int freq;
    struct Node *left, *right;
} Node;

// A structure to represent a Min Heap
typedef struct MinHeap {
    unsigned int size;
    unsigned int capacity;
    Node **array;
} MinHeap;

// Function to create a new node
Node* newNode(unsigned char data, unsigned int freq) {
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;
    return temp;
}

// Function to create a Min Heap of given capacity
MinHeap* createMinHeap(unsigned int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (Node**)malloc(minHeap->capacity * sizeof(Node*));
    return minHeap;
}

// Function to swap two min heap nodes
void swapMinHeapNode(Node** a, Node** b) {
    Node* t = *a;
    *a = *b;
    *b = t;
}

// Standard minHeapify function
void minHeapify(MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

// Function to check if size of heap is 1
int isSizeOne(MinHeap* minHeap) {
    return (minHeap->size == 1);
}

// Function to extract minimum value node from heap
Node* extractMin(MinHeap* minHeap) {
    Node* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    minHeapify(minHeap, 0);
    return temp;
}

// Function to insert a new node to Min Heap
void insertMinHeap(MinHeap* minHeap, Node* minHeapNode) {
    ++minHeap->size;
    int i = minHeap->size - 1;

    while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    minHeap->array[i] = minHeapNode;
}

// Function to build min heap
void buildMinHeap(MinHeap* minHeap) {
    int n = minHeap->size - 1;
    int i;

    for (i = (n - 1) / 2; i >= 0; --i)
        minHeapify(minHeap, i);
}

// Function to check if node is leaf
int isLeaf(Node* root) {
    return !(root->left) && !(root->right);
}

// Function to create and build a min heap
MinHeap* createAndBuildMinHeap(unsigned char data[], unsigned int freq[], int size) {
    MinHeap* minHeap = createMinHeap(size);

    for (int i = 0; i < size; ++i)
        minHeap->array[i] = newNode(data[i], freq[i]);

    minHeap->size = size;
    buildMinHeap(minHeap);
    return minHeap;
}

// Function to build the Huffman tree
Node* buildHuffmanTree(unsigned char data[], unsigned int freq[], int size) {
    Node *left, *right, *top;

    MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);

    while (!isSizeOne(minHeap)) {
        left = extractMin(minHeap);
        right = extractMin(minHeap);

        top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;

        insertMinHeap(minHeap, top);
    }

    return extractMin(minHeap);
}

// Function to print an array of size n
void printArr(int arr[], int n) {
    for (int i = 0; i < n; ++i)
        printf("%d", arr[i]);

    printf("\n");
}

// Function to store codes from the root of Huffman Tree
void storeCodes(Node* root, int arr[], int top, int codes[256][256], int codeLengths[256]) {
    if (root->left) {
        arr[top] = 0;
        storeCodes(root->left, arr, top + 1, codes, codeLengths);
    }

    if (root->right) {
        arr[top] = 1;
        storeCodes(root->right, arr, top + 1, codes, codeLengths);
    }

    if (isLeaf(root)) {
        for (int i = 0; i < top; i++) {
            codes[root->data][i] = arr[i];
        }
        codeLengths[root->data] = top;
    }
}

// Function to build Huffman codes for the given input data
void HuffmanCodes(unsigned char data[], unsigned int freq[], int size, int codes[256][256], int codeLengths[256]) {
    Node* root = buildHuffmanTree(data, freq, size);
    int arr[256], top = 0;
    storeCodes(root, arr, top, codes, codeLengths);
}

void compressImage(const char *inputFilePath, const char *outputFilePath) {
    FILE *inputFile = fopen(inputFilePath, "rb");
    FILE *outputFile = fopen(outputFilePath, "wb");

    if (!inputFile || !outputFile) {
        fprintf(stderr, "Error opening files.\n");
        return;
    }

    // Read image data (simplified, does not handle PNG format)
    unsigned char buffer[1024 * 1024]; // 1 MB buffer
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), inputFile);
    if (bytesRead <= 0) {
        fprintf(stderr, "Error reading input file.\n");
        return;
    }

    // Calculate frequency of each byte value
    unsigned int freq[256] = {0};
    for (size_t i = 0; i < bytesRead; i++) {
        freq[buffer[i]]++;
    }

    // Create Huffman codes
    unsigned char data[256];
    int size = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            data[size++] = (unsigned char)i;
        }
    }

    int codes[256][256] = {0};
    int codeLengths[256] = {0};
    HuffmanCodes(data, freq, size, codes, codeLengths);

    // Write the Huffman table (for simplicity, not a proper format)
    fwrite(freq, sizeof(unsigned int), 256, outputFile);

    // Encode the input file
    for (size_t i = 0; i < bytesRead; i++) {
        for (int j = 0; j < codeLengths[buffer[i]]; j++) {
            fputc(codes[buffer[i]][j] ? '1' : '0', outputFile);
        }
    }

    fclose(inputFile);
    fclose(outputFile);
}

int main() {
    char inputFilePath[256];
    char outputFilePath[256];

    printf("Enter input PNG file path: ");
    scanf("%255s", inputFilePath);

    printf("Enter output compressed file path: ");
    scanf("%255s", outputFilePath);

    compressImage(inputFilePath, outputFilePath);
    printf("Compression completed\n");

    return 0;
}
