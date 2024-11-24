#pragma once
#include <filesystem>
#include <fstream>
#include "pch.h"

inline void loadEnv() {
    // Get the directory of the current source file
    std::filesystem::path sourceDir = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();  // at the root of the project
    std::filesystem::path envFilePath = sourceDir / ".env";

    std::ifstream envFile(envFilePath);
    if (!envFile.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to open .env file at " << envFilePath;
        return;
    }

    std::string line;
    while (std::getline(envFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            setenv(key.c_str(), value.c_str(), 1);
        }
    }
    envFile.close();
}