#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include "Interfaces.h"
#include "LogRecord.h"

class FileSink : public Sink<LogRecord> {
private:
    std::string filePath;
    std::ofstream outFile;

public:
    explicit FileSink(const std::string& path) : filePath(path) {}

    // Pipeline tarafindan yasam dongusu kancasi olarak cagirilir
    void open() override {
        std::filesystem::path p(filePath);
        
        // Eger ust klasor mevcut degilse (ornek: out/errors.txt icin 'out' klasoru) olustur
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }

        outFile.open(filePath);
        if (!outFile.is_open()) {
            throw std::runtime_error("FileSink error: Cannot open output file " + filePath);
        }
    }

    void consume(const LogRecord& item) override {
        if (outFile.is_open()) {
            // ProjectionStage'den gelen formatlanmis 'raw' metnini dosyaya yazar
            outFile << item.raw << "\n";
        }
    }

    // Pipeline bitisinde dosyayi guvenli sekilde kapatir
    void close() override {
        if (outFile.is_open()) {
            outFile.close();
        }
    }
};