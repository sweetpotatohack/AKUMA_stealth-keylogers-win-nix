#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// XOR decryption function
std::string decrypt(const std::string& encrypted) {
    std::string decrypted = encrypted;
    int key = 0x42;  // Same key used in both keyloggers
    
    for (size_t i = 0; i < decrypted.length(); i++) {
        decrypted[i] ^= key;
        key = (key + 1) % 256;
    }
    
    return decrypted;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <encrypted_log_file>" << std::endl;
        return 1;
    }
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Decrypt and output
    std::string decrypted = decrypt(content);
    std::cout << decrypted << std::endl;
    
    return 0;
}