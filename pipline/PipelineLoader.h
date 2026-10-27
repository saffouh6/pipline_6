#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Pipeline.h"
#include "PipelineBuilder.h"
#include "StageRegistry.h"
#include "FileLineSource.h"
#include "ParserStage.h"
#include "FileSink.h"
#include "DeadLetterSink.h"
#include "ValidationStage.h"

class PipelineLoader {
private:
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    static StageParams parseParams(const std::string& line) {
        StageParams params;
        size_t start = line.find('{');
        size_t end = line.rfind('}');
        if (start == std::string::npos || end == std::string::npos || start >= end) {
            return params;
        }

        std::string content = line.substr(start + 1, end - start - 1);
        
        std::string currentKey = "";
        std::string currentValue = "";
        bool inKey = true;
        int bracketDepth = 0;

        for (size_t i = 0; i < content.length(); ++i) {
            char c = content[i];
            if (c == '[' || c == '{') bracketDepth++;
            else if (c == ']' || c == '}') bracketDepth--;

            if (inKey) {
                if (c == ':') {
                    inKey = false;
                } else {
                    currentKey += c;
                }
            } else {
                if (c == ',' && bracketDepth == 0) {
                    params[trim(currentKey)] = trim(currentValue);
                    currentKey = "";
                    currentValue = "";
                    inKey = true;
                } else {
                    currentValue += c;
                }
            }
        }
        if (!currentKey.empty()) {
            params[trim(currentKey)] = trim(currentValue);
        }

        return params;
    }

public:
    static Pipeline load(const std::string& configPath) {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            throw PipelineException("Configuration error: Cannot open file " + configPath);
        }

        std::string line;
        std::string inputPath = "data/access.log";
        std::string outputPath = "out/output.txt";
        
        auto parser = std::make_shared<ParserStage>();
        // Artim 5: Validation Stage (Dogrulama) her pipeline'in basinda otomatik calisir
        auto validator = std::make_shared<ValidationStage>();
        
        std::vector<std::shared_ptr<Stage<LogRecord, LogRecord>>> filters;

        std::string currentSection = "";

        while (std::getline(file, line)) {
            std::string tLine = trim(line);
            if (tLine.empty() || tLine[0] == '#') continue;

            if (tLine.find("source:") == 0) { currentSection = "source"; continue; }
            if (tLine.find("sink:") == 0) { currentSection = "sink"; continue; }
            if (tLine.find("stages:") == 0) { currentSection = "stages"; continue; }

            if (currentSection == "source" && tLine.find("params:") != std::string::npos) {
                StageParams p = parseParams(tLine);
                if (p.count("path")) inputPath = p["path"];
            }
            else if (currentSection == "sink" && tLine.find("params:") != std::string::npos) {
                StageParams p = parseParams(tLine);
                if (p.count("path")) outputPath = p["path"];
            }
            else if (currentSection == "stages" && tLine.find("- type:") != std::string::npos) {
                std::string type = trim(tLine.substr(tLine.find(":") + 1));
                
                std::string paramLine;
                if (std::getline(file, paramLine)) {
                    StageParams params = parseParams(trim(paramLine));
                    
                    if (type != "parse") {
                        auto stage = StageRegistry::createStage(type, params);
                        filters.push_back(stage);
                    }
                }
            }
        }

        auto source = std::make_shared<FileLineSource>(inputPath);
        auto sink = std::make_shared<FileSink>(outputPath);
        
        // Artim 5: Hata yonetimi icin DeadLetter hedefini ekle
        auto deadLetterSink = std::make_shared<DeadLetterSink>("out/dead-letter.log");
        
        auto builder = PipelineBuilder::from(source)
            .withDeadLetterSink(deadLetterSink)
            .via(parser)
            .via(validator);
            
        for (auto& filter : filters) {
            builder.via(filter);
        }
        
        return builder.to(sink);
    }
};