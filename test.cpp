#include <iostream>
#include <string>

int main() {
    std::string line = "hello\nworld!\naa";
    std::string test;

    size_t newline_idx;
    // Find the newline, process it, and remove it from the source string
    while ((newline_idx = line.find("\n")) != std::string::npos) {
        
        // 1. Extract the word (from index 0, length of newline_idx)
        test = line.substr(0, newline_idx);
        std::cout << "Extracted: " << test << std::endl;

        // 2. Erase the word AND the newline character (length + 1)
        line.erase(0, newline_idx + 1);
    }

    // 3. Handle the very last part (the "aa" which has no newline)
    if (!line.empty()) {
        std::cout << "Final part: " << line << std::endl;
    }

    return 0;
}