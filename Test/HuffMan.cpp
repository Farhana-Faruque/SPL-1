#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <bitset>
#include <fstream>
using namespace std;

struct Node {
    char ch;
    int freq;
    Node* left;
    Node* right;
    Node(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(Node* left, Node* right) {
        return left->freq > right->freq;
    }
};

void generateCodes(Node* root, const string& str, unordered_map<char, string>& huffmanCodes) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = str;
    }
    generateCodes(root->left, str + "0", huffmanCodes);
    generateCodes(root->right, str + "1", huffmanCodes);
}

Node* buildHuffmanTree(const unordered_map<char, int>& freqMap) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (const auto& pair : freqMap) {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();

        Node* newNode = new Node('$', left->freq + right->freq);
        newNode->left = left;
        newNode->right = right;

        pq.push(newNode);
    }
    return pq.top();
}

string encode(const string& str, const unordered_map<char, string>& huffmanCodes) {
    string encodedStr = "";
    for (char c : str) {
        encodedStr += huffmanCodes.at(c);
    }
    return encodedStr;
}

string decode(Node* root, const string& encodedStr) {
    string decodedStr = "";
    Node* currentNode = root;

    for (char bit : encodedStr) {
        if (bit == '0') {
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }

        if (!currentNode->left && !currentNode->right) {
            decodedStr += currentNode->ch;
            currentNode = root;
        }
    }
    return decodedStr;
}

void saveCodesToFile(const unordered_map<char, string>& huffmanCodes, const string& filename) {
    ofstream file(filename);
    for (const auto& pair : huffmanCodes) {
        file << pair.first << " " << pair.second << "\n";
    }
    file.close();
}

void compressFile(const string& inputFilename, const string& outputFilename, const unordered_map<char, string>& huffmanCodes) {
    ifstream inputFile(inputFilename);
    ofstream outputFile(outputFilename, ios::binary);

    string text, encodedStr = "";
    getline(inputFile, text, '\0');

    for (char c : text) {
        encodedStr += huffmanCodes.at(c);
    }

    while (encodedStr.size() % 8 != 0) {
        encodedStr += "0"; 
    }

    for (size_t i = 0; i < encodedStr.size(); i += 8) {
        bitset<8> bits(encodedStr.substr(i, 8));
        outputFile.put(static_cast<unsigned char>(bits.to_ulong()));
    }

    inputFile.close();
    outputFile.close();
}
void convertCompressedToText(const std::string& compressedFilename, const std::string& outputFilename) {
    std::ifstream inputFile(compressedFilename, std::ios::binary);
    std::ofstream outputFile(outputFilename);

    std::string binaryString = "";
    char byte;

    while (inputFile.get(byte)) {
        std::bitset<8> bits(static_cast<unsigned char>(byte));
        binaryString += bits.to_string();
    }

    outputFile << binaryString;

    inputFile.close();
    outputFile.close();
}

// Decompress a file using Huffman Tree
void decompressFile(const string& compressedFilename, const string& outputFilename, Node* root) {
    ifstream inputFile(compressedFilename, ios::binary);
    ofstream outputFile(outputFilename);

    string encodedStr = "";
    char byte;

    while (inputFile.get(byte)) {
        bitset<8> bits(static_cast<unsigned char>(byte));
        encodedStr += bits.to_string();
    }
    string decodedStr = decode(root, encodedStr);
    outputFile << decodedStr;

    inputFile.close();
    outputFile.close();
}

int main(void){
    string inputStr = "farhana";

    unordered_map<char, int> freqMap;
    for (char c : inputStr) {
        freqMap[c]++;
    }

    Node* root = buildHuffmanTree(freqMap);
    unordered_map<char, string> huffmanCodes;
    generateCodes(root, "", huffmanCodes);

    saveCodesToFile(huffmanCodes, "huffman_codes.txt");

    ofstream sampleFile("sample.txt");
    sampleFile << inputStr;
    sampleFile.close();
    compressFile("sample.txt", "compressed.bin", huffmanCodes);

    convertCompressedToText("compressed.bin", "compressed.txt");

    decompressFile("compressed.bin", "decompressed.txt", root);

    cout << "Compression and Decompression completed successfully!" << endl;

    return 0;
}