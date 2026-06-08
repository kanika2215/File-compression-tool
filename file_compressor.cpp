
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <cstring>

using namespace std;

const string RESET  = "\033[0m";
const string BOLD   = "\033[1m";
const string CYAN   = "\033[36m";
const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string RED    = "\033[31m";
const string DIM    = "\033[2m";

struct HuffNode {
    unsigned char ch;
    int           freq;
    HuffNode*     left;
    HuffNode*     right;

    HuffNode(unsigned char c, int f)
        : ch(c), freq(f), left(nullptr), right(nullptr) {}

    HuffNode(int f, HuffNode* l, HuffNode* r)
        : ch(0), freq(f), left(l), right(r) {}

    bool isLeaf() const { return !left && !right; }
};

struct NodeCmp {
    bool operator()(HuffNode* a, HuffNode* b) {
        return a->freq > b->freq;
    }
};


void freeTree(HuffNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

// Count how many times each byte appears in the file.
// These frequencies will be used to build the Huffman tree.

unordered_map<unsigned char, int>
buildFreqTable(const vector<unsigned char>& data) {
    unordered_map<unsigned char, int> freq;
    for (int i = 0; i < (int)data.size(); i++)
        freq[data[i]]++;
    return freq;
}

//  BUILD HUFFMAN TREE

HuffNode* buildTree(const unordered_map<unsigned char, int>& freq) {
    priority_queue<HuffNode*, vector<HuffNode*>, NodeCmp> pq;

    for (auto it = freq.begin(); it != freq.end(); ++it)
        pq.push(new HuffNode(it->first, it->second));

    // Edge case
    if (pq.size() == 1) {
        HuffNode* only = pq.top(); pq.pop();
        HuffNode* root = new HuffNode(only->freq, only, nullptr);
        return root;
    }

    while (pq.size() > 1) {
        HuffNode* left  = pq.top(); pq.pop();
        HuffNode* right = pq.top(); pq.pop();
        HuffNode* merge = new HuffNode(left->freq + right->freq, left, right);
        pq.push(merge);
    }
    return pq.top();
}

void generateCodes(HuffNode* node, const string& prefix,
                   unordered_map<unsigned char, string>& codes) {
    if (!node) return;
    if (node->isLeaf()) {
        codes[node->ch] = prefix.empty() ? "0" : prefix;
        return;
    }
    generateCodes(node->left,  prefix + "0", codes);
    generateCodes(node->right, prefix + "1", codes);
}


void serializeTree(HuffNode* node, string& bits) {
    if (!node) return;
    if (node->isLeaf()) {
        bits += '1';
        for (int i = 7; i >= 0; i--)
            bits += ((node->ch >> i) & 1) ? '1' : '0';
        return;
    }
    bits += '0';
    serializeTree(node->left,  bits);
    serializeTree(node->right, bits);
}

HuffNode* deserializeTree(const string& bits, int& pos) {
    if (pos >= (int)bits.size()) return nullptr;
    if (bits[pos] == '1') {
        pos++;
        unsigned char ch = 0;
        for (int i = 7; i >= 0; i--) {
            if (bits[pos] == '1') ch |= (1 << i);
            pos++;
        }
        return new HuffNode(ch, 0);
    }
    pos++;
    HuffNode* left  = deserializeTree(bits, pos);
    HuffNode* right = deserializeTree(bits, pos);
    return new HuffNode(0, left, right);
}

vector<unsigned char> bitsToBytes(const string& bits) {
    vector<unsigned char> bytes;
    for (int i = 0; i < (int)bits.size(); i += 8) {
        unsigned char byte = 0;
        for (int j = 0; j < 8 && i+j < (int)bits.size(); j++)
            if (bits[i+j] == '1') byte |= (1 << (7 - j));
        bytes.push_back(byte);
    }
    return bytes;
}

string bytesToBits(const vector<unsigned char>& bytes) {
    string bits;
    for (int i = 0; i < (int)bytes.size(); i++)
        for (int j = 7; j >= 0; j--)
            bits += ((bytes[i] >> j) & 1) ? '1' : '0';
    return bits;
}

bool readFile(const string& path, vector<unsigned char>& data) {
    ifstream f(path, ios::binary);
    if (!f.is_open()) {
        cout << RED << "  [X] Cannot open file: " << path << RESET << "\n";
        return false;
    }
    data.assign((istreambuf_iterator<char>(f)),
                 istreambuf_iterator<char>());
    f.close();
    return true;
}

bool writeFile(const string& path, const vector<unsigned char>& data) {
    ofstream f(path, ios::binary);
    if (!f.is_open()) {
        cout << RED << "  [X] Cannot write file: " << path << RESET << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    f.close();
    return true;
}

//  COMPRESS

bool compress(const string& inputPath, const string& outputPath) {
    cout << BOLD << "\n  Compressing: " << inputPath << RESET << "\n";

    // Read input
    vector<unsigned char> data;
    if (!readFile(inputPath, data)) return false;
    if (data.empty()) {
        cout << RED << "  [X] File is empty.\n" << RESET;
        return false;
    }

    cout << "  Original size  : " << BOLD << data.size()
         << " bytes" << RESET << "\n";

    unordered_map<unsigned char, int> freq = buildFreqTable(data);
    cout << "  Unique symbols : " << BOLD << freq.size() << RESET << "\n";

    // Build Huffman tree
    HuffNode* root = buildTree(freq);

    unordered_map<unsigned char, string> codes;
    generateCodes(root, "", codes);

    string encoded;
    encoded.reserve(data.size() * 4);
    for (int i = 0; i < (int)data.size(); i++)
        encoded += codes[data[i]];

    int padding = (8 - (int)(encoded.size() % 8)) % 8;
    for (int i = 0; i < padding; i++) encoded += '0';

    // Serialize tree
    string treeBits;
    serializeTree(root, treeBits);
    int treePad = (8 - (int)(treeBits.size() % 8)) % 8;
    for (int i = 0; i < treePad; i++) treeBits += '0';

    // Convert bits to bytes
    vector<unsigned char> treeBytes  = bitsToBytes(treeBits);
    vector<unsigned char> dataBytes  = bitsToBytes(encoded);

    // Build output buffer
    vector<unsigned char> output;

    // 4 bytes: original data size
    unsigned int origSize = (unsigned int)data.size();
    output.push_back((origSize >> 24) & 0xFF);
    output.push_back((origSize >> 16) & 0xFF);
    output.push_back((origSize >>  8) & 0xFF);
    output.push_back((origSize      ) & 0xFF);

    // 4 bytes: tree bits length (before padding)
    unsigned int treeBitsLen = (unsigned int)(treeBits.size() - treePad);
    output.push_back((treeBitsLen >> 24) & 0xFF);
    output.push_back((treeBitsLen >> 16) & 0xFF);
    output.push_back((treeBitsLen >>  8) & 0xFF);
    output.push_back((treeBitsLen      ) & 0xFF);

    // Tree bytes
    for (int i = 0; i < (int)treeBytes.size(); i++)
        output.push_back(treeBytes[i]);

    // 1 byte: padding count for encoded data
    output.push_back((unsigned char)padding);

    // Encoded data bytes
    for (int i = 0; i < (int)dataBytes.size(); i++)
        output.push_back(dataBytes[i]);

    if (!writeFile(outputPath, output)) {
        freeTree(root);
        return false;
    }

    double ratio = 100.0 * (1.0 - (double)output.size() / data.size());
    cout << "  Compressed size: " << BOLD << output.size()
         << " bytes" << RESET << "\n"
         << "  Space saved    : " << BOLD << GREEN
         << fixed << setprecision(2) << ratio << "%" << RESET << "\n"
         << "  Output file    : " << BOLD << outputPath << RESET << "\n";

    cout << "\n" << DIM
         << "  Char   Freq       Code\n"
         << "  ─────────────────────────────────────\n" << RESET;

    vector<pair<unsigned char, int>> freqVec(freq.begin(), freq.end());
    sort(freqVec.begin(), freqVec.end(),
         [](const pair<unsigned char,int>& a,
            const pair<unsigned char,int>& b){
             return a.second > b.second;
         });

    int shown = 0;
    for (int i = 0; i < (int)freqVec.size() && shown < 15; i++, shown++) {
        unsigned char c = freqVec[i].first;
        string display;
        if      (c == '\n') display = "\\n";
        else if (c == '\t') display = "\\t";
        else if (c == ' ')  display = "SP";
        else if (c < 32)    display = "0x" + to_string((int)c);
        else                display = string(1, (char)c);

        cout << "  " << CYAN << left << setw(7) << display << RESET
             << DIM  << setw(10) << freqVec[i].second << RESET
             << YELLOW << codes[c] << RESET << "\n";
    }
    if ((int)freqVec.size() > 15)
        cout << DIM << "  ...and " << freqVec.size()-15
             << " more symbols\n" << RESET;

    freeTree(root);
    cout << GREEN
     << "\nCompression completed successfully.\n"
     << RESET;
    return true;
}

bool decompress(const string& inputPath, const string& outputPath) {
    cout << BOLD << "\n  Decompressing: " << inputPath << RESET << "\n";

    vector<unsigned char> input;
    if (!readFile(inputPath, input)) return false;
    if (input.size() < 9) {
        cout << RED << "  [X] File too small to be a valid .huff file.\n"
             << RESET;
        return false;
    }

    int pos = 0;

    unsigned int origSize = 0;
    origSize |= ((unsigned int)input[pos++] << 24);
    origSize |= ((unsigned int)input[pos++] << 16);
    origSize |= ((unsigned int)input[pos++] <<  8);
    origSize |= ((unsigned int)input[pos++]      );

    unsigned int treeBitsLen = 0;
    treeBitsLen |= ((unsigned int)input[pos++] << 24);
    treeBitsLen |= ((unsigned int)input[pos++] << 16);
    treeBitsLen |= ((unsigned int)input[pos++] <<  8);
    treeBitsLen |= ((unsigned int)input[pos++]      );

    int treeByteCount = (treeBitsLen + 7) / 8;
    if (pos + treeByteCount > (int)input.size()) {
        cout << RED << "  [X] Corrupt file: tree data missing.\n" << RESET;
        return false;
    }
    vector<unsigned char> treeBytes(input.begin() + pos,
                                    input.begin() + pos + treeByteCount);
    pos += treeByteCount;

    int padding = (int)input[pos++];

    vector<unsigned char> dataBytes(input.begin() + pos, input.end());

    string treeBits = bytesToBits(treeBytes);
    treeBits        = treeBits.substr(0, treeBitsLen);

    // Deserialize tree
    int treePos = 0;
    HuffNode* root = deserializeTree(treeBits, treePos);
    if (!root) {
        cout << RED << "  [X] Failed to reconstruct Huffman tree.\n" << RESET;
        return false;
    }

    string encoded = bytesToBits(dataBytes);
    if (padding > 0 && (int)encoded.size() >= padding)
        encoded = encoded.substr(0, encoded.size() - padding);

    // Decode
    vector<unsigned char> decoded;
    decoded.reserve(origSize);
    HuffNode* cur = root;

    for (int i = 0; i < (int)encoded.size() && (int)decoded.size() < (int)origSize; i++) {
        cur = (encoded[i] == '0') ? cur->left : cur->right;
        if (!cur) { cur = root; continue; }
        if (cur->isLeaf()) {
            decoded.push_back(cur->ch);
            cur = root;
        }
    }

    if ((int)decoded.size() != (int)origSize) {
        cout << YELLOW << "  [!] Warning: decoded "
             << decoded.size() << " bytes, expected "
             << origSize << "\n" << RESET;
    }

    if (!writeFile(outputPath, decoded)) {
        freeTree(root);
        return false;
    }

    cout << "  Compressed size  : " << BOLD << input.size()
         << " bytes" << RESET << "\n"
         << "  Decompressed size: " << BOLD << decoded.size()
         << " bytes" << RESET << "\n"
         << "  Output file      : " << BOLD << outputPath << RESET << "\n";

    freeTree(root);
    cout << GREEN << "\n  [OK] Decompression complete.\n" << RESET;
    return true;
}

void inspectFile(const string& path) {
    vector<unsigned char> input;
    if (!readFile(path, input)) return;
    if (input.size() < 9) {
        cout << RED << "  [X] Not a valid .huff file.\n" << RESET;
        return;
    }

    unsigned int origSize = 0;
    origSize |= ((unsigned int)input[0] << 24);
    origSize |= ((unsigned int)input[1] << 16);
    origSize |= ((unsigned int)input[2] <<  8);
    origSize |= ((unsigned int)input[3]      );

    unsigned int treeBitsLen = 0;
    treeBitsLen |= ((unsigned int)input[4] << 24);
    treeBitsLen |= ((unsigned int)input[5] << 16);
    treeBitsLen |= ((unsigned int)input[6] <<  8);
    treeBitsLen |= ((unsigned int)input[7]      );

    int treeByteCount = (treeBitsLen + 7) / 8;
    int padding       = (int)input[8 + treeByteCount];
    int dataBytes     = (int)input.size() - 9 - treeByteCount;

    double ratio = 100.0 * (1.0 - (double)input.size() / origSize);

    cout << BOLD << "\n  File Inspector: " << path << RESET << "\n"
         << DIM  << "  ─────────────────────────────────────\n" << RESET
         << "  Compressed size  : " << BOLD << input.size()
         << " bytes" << RESET << "\n"
         << "  Original size    : " << BOLD << origSize
         << " bytes" << RESET << "\n"
         << "  Compression ratio: " << BOLD << GREEN
         << fixed << setprecision(2) << ratio << "%" << RESET << "\n"
         << "  Tree size        : " << BOLD << treeByteCount
         << " bytes (" << treeBitsLen << " bits)" << RESET << "\n"
         << "  Data size        : " << BOLD << dataBytes
         << " bytes" << RESET << "\n"
         << "  Padding bits     : " << BOLD << padding << RESET << "\n";
}

void createSampleFile(const string& path = "sample.txt") {
    ofstream f(path);
    if (!f.is_open()) {
        cout << RED << "  [X] Cannot create sample file.\n" << RESET;
        return;
    }
    f << "Huffman coding is a lossless data compression algorithm.\n"
      << "It assigns shorter binary codes to more frequent characters\n"
      << "and longer codes to less frequent ones.\n\n"
      << "This tool demonstrates:\n"
      << "  - Frequency analysis of input bytes\n"
      << "  - Building a min-heap priority queue\n"
      << "  - Constructing an optimal prefix-free binary tree\n"
      << "  - Encoding data into a compact bit stream\n"
      << "  - Serializing the tree for storage in the output file\n"
      << "  - Decoding the bit stream back to original data\n\n"
      << "The quick brown fox jumps over the lazy dog.\n"
      << "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
      << "0123456789 !@#$%^&*()-_=+[]{}|;:',.<>?/`~\n\n"
      << "Repeated text compresses well:\n"
      << "aaaaaaaabbbbbbbbccccccccddddddddeeeeeeee\n"
      << "aaaaaaaabbbbbbbbccccccccddddddddeeeeeeee\n"
      << "aaaaaaaabbbbbbbbccccccccddddddddeeeeeeee\n";
    f.close();
    cout << GREEN << "  [OK] Created " << path << RESET << "\n";
}

void banner() {
    cout << CYAN << BOLD << "\n"
         << "  +==================================================+\n"
         << "  |   File Compression Tool  v1.0                   |\n"
         << "  |   Huffman Coding | C++ | DSA | File I/O         |\n"
         << "  +==================================================+\n"
         << RESET;
}

void menu() {
    cout << BOLD
         << "\n  +-- MENU ------------------------------------------+\n"
         << RESET
         << "  |  1.  Compress a file                            |\n"
         << "  |  2.  Decompress a .huff file                    |\n"
         << "  |  3.  Inspect a .huff file                       |\n"
         << "  |  4.  Create sample text file for testing        |\n"
         << "  |  0.  Exit                                       |\n"
         << BOLD
         << "  +-------------------------------------------------+\n"
         << RESET
         << "  Choice: ";
}

void runCLI() {
    string choice, input, output;

    while (true) {
        menu();
        getline(cin, choice);
        int cmd = -1;
        try { cmd = stoi(choice); } catch (...) {}
        cout << "\n";

        if (cmd == 0) {
            cout << GREEN << "  Goodbye!\n" << RESET;
            break;
        }

        else if (cmd == 1) {
            cout << "  Input file path  : ";
            getline(cin, input);
            cout << "  Output file path : ";
            getline(cin, output);
            if (output.empty())
                output = input + ".huff";
            compress(input, output);
        }

        else if (cmd == 2) {
            cout << "  Input .huff file : ";
            getline(cin, input);
            cout << "  Output file path : ";
            getline(cin, output);
            if (output.empty()) {
                output = input;
                if (output.size() > 5 &&
                    output.substr(output.size()-5) == ".huff")
                    output = output.substr(0, output.size()-5);
                else
                    output += ".decoded";
            }
            decompress(input, output);
        }

        else if (cmd == 3) {
            cout << "  .huff file path  : ";
            getline(cin, input);
            inspectFile(input);
        }

        else if (cmd == 4) {
            cout << "  Output filename (default: sample.txt): ";
            getline(cin, input);
            if (input.empty()) input = "sample.txt";
            createSampleFile(input);
            cout << "  Now use option 1 to compress it.\n";
        }

        else {
            cout << RED << "  Invalid option.\n" << RESET;
        }

        cout << DIM << "\n  Press Enter to continue..." << RESET;
        getline(cin, input);
    }
}

int main(int argc, char* argv[]) {
    banner();

    if (argc == 4) {
        string mode = argv[1];
        string in   = argv[2];
        string out  = argv[3];
        if (mode == "compress")
            compress(in, out);
        else if (mode == "decompress")
            decompress(in, out);
        else
            cout << RED << "  Usage: huffman compress <in> <out>\n"
                        << "         huffman decompress <in> <out>\n"
                 << RESET;
        return 0;
    }

    runCLI();
    return 0;
}