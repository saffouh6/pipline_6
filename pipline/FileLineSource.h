#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "Interfaces.h"

class FileLineSource : public Source<std::string> {
private:
    std::string filePath;
    std::ifstream file;

public:
    explicit FileLineSource(const std::string& path) : filePath(path) {}

    void open() override {
        // Eger YAML icinde 'data/access.log' yaziyor ama dosya yoksa, 'data/access-small.log' dene
        if (!std::filesystem::exists(filePath)) {
            if (filePath == "data/access.log" && std::filesystem::exists("data/access-small.log")) {
                filePath = "data/access-small.log";
            } else if (!std::filesystem::exists(filePath)) {
                throw PipelineException("FileLineSource Error: Input file not found: " + filePath);
            }
        }

        file.open(filePath);
        if (!file.is_open()) {
            throw PipelineException("FileLineSource Error: Cannot open input file: " + filePath);
        }
    }

    void produce(Emitter<std::string>& out) override {
        if (!file.is_open()) {
            open();
        }
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                out.emit(line);
            }
        }
    }

    void close() override {
        if (file.is_open()) {
            file.close();
        }
    }
};