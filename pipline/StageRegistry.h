#pragma once
#include <string>
#include <map>
#include <functional>
#include <memory>
#include "Interfaces.h"
#include "LogRecord.h"

// Eksik olan StageParams tanimi buradadir
using StageParams = std::map<std::string, std::string>;

using StageFactoryFunc = std::function<std::shared_ptr<Stage<LogRecord, LogRecord>>(const StageParams&)>;

class StageRegistry {
private:
    std::map<std::string, StageFactoryFunc> registry;

    static StageRegistry& getInstance() {
        static StageRegistry instance;
        return instance;
    }

public:
    // Eksik olan 'static' anahtar kelimesi eklendi
    static void registerStage(const std::string& type, StageFactoryFunc factory) {
        getInstance().registry[type] = factory;
    }

    static std::shared_ptr<Stage<LogRecord, LogRecord>> createStage(const std::string& type, const StageParams& params) {
        auto it = getInstance().registry.find(type);
        if (it == getInstance().registry.end()) {
            throw PipelineException("Configuration error: Unknown stage type '" + type + "'");
        }
        return it->second(params);
    }
};