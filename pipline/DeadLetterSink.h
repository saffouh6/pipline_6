#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "Interfaces.h"
#include "LogRecord.h"

class DeadLetterSink : public Sink<FailedRecord> {
private:
    std::string filePath;
    std::ofstream outFile;

    std::string formatTime(const std::chrono::system_clock::time_point& tp) const {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        return std::string(buf);
    }

public:
    explicit DeadLetterSink(const std::string& path = "out/dead-letter.log") 
        : filePath(path) {}

    void open() override {
        std::filesystem::path p(filePath);
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }
        outFile.open(filePath);
        if (!outFile.is_open()) {
            throw PipelineException("DeadLetterSink error: Cannot open file " + filePath);
        }
    }

    void consume(const FailedRecord& item) override {
        if (outFile.is_open()) {
            outFile << "[" << formatTime(item.timestamp) << "] "
                    << "STAGE: " << item.stageName << " | "
                    << "ERROR: " << item.exceptionMessage << " | "
                    << "RAW: " << item.rawInput << "\n";
        }
    }

    void close() override {
        if (outFile.is_open()) {
            outFile.close();
        }
    }
};