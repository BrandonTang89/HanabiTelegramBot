#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <stdexcept>
inline int parseInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (std::invalid_argument& e) {
        return -1;
    }
}

#endif // HELPER_H