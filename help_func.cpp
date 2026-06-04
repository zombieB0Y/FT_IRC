#include "help_func.hpp"
#include <algorithm>

void ltrim(std::string &s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
}

void rtrim(std::string &s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
}

bool    isspecial(char c) {
    std::string allowed_specials = "[]\\`_^{|}";
    return (allowed_specials.find(c) != std::string::npos);
}

std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}