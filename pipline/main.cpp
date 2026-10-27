#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdexcept>

#include "Interfaces.h"
#include "Pipeline.h"
#include "PipelineBuilder.h"
#include "PipelineLoader.h"
#include "StageRegistry.h"
#include "PredicateFilterStage.h"
#include "ProjectionStage.h"

inline std::vector<std::string> parseFields(const std::string& fieldStr) {
    std::vector<std::string> fields;
    std::stringstream ss(fieldStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
        fields.push_back(item);
    }
    return fields;
}

void setupRegistry() {
    StageRegistry::registerStage("filter", [](const StageParams& params) {
        return std::make_shared<PredicateFilterStage>([params](const LogRecord& r) {
            std::string field = params.count("field") ? params.at("field") : "";
            std::string op = params.count("op") ? params.at("op") : "==";
            std::string val = params.count("value") ? params.at("value") : "";

            // Tirnak isaretlerini temizle
            val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
            val.erase(std::remove(val.begin(), val.end(), '\''), val.end());
            val.erase(std::remove(val.begin(), val.end(), ' '), val.end());

            // 1. Status Filtresi (op: ">=", op: "==" vb.)
            if (field == "status") {
                if (val.empty()) return true;
                int targetStatus = std::stoi(val);
                if (op == ">=") return r.status >= targetStatus;
                if (op == ">")  return r.status > targetStatus;
                if (op == "<=") return r.status <= targetStatus;
                if (op == "<")  return r.status < targetStatus;
                return r.status == targetStatus;
            }

            // 2. Path Filtresi (Prefix / Eslenim)
            if (field == "path") {
                if (val.empty()) return true;
                return (r.path.rfind(val, 0) == 0) || (r.path.find(val) != std::string::npos);
            }

            // 3. Method Filtresi
            if (field == "method") {
                if (val.empty()) return true;
                return r.method == val;
            }

            return true; 
        });
    });

    StageRegistry::registerStage("project", [](const StageParams& params) {
        std::vector<std::string> fields = {"timestamp", "clientIp", "status", "path"};
        if (params.count("fields")) {
            std::string cleanFields = params.at("fields");
            cleanFields.erase(std::remove(cleanFields.begin(), cleanFields.end(), '['), cleanFields.end());
            cleanFields.erase(std::remove(cleanFields.begin(), cleanFields.end(), ']'), cleanFields.end());
            fields = parseFields(cleanFields);
        }
        return std::make_shared<ProjectionStage>(fields);
    });
}

int main(int argc, char* argv[]) {
    try {
        setupRegistry();

        std::string configPath = "";
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc) {
                configPath = argv[++i];
            }
        }

        if (!configPath.empty()) {
            std::cout << "Loading configuration from: " << configPath << std::endl;
            
            Pipeline pipeline = PipelineLoader::load(configPath);
            pipeline.run();

            std::cout << "Execution completed successfully." << std::endl;
            return 0;
        } else {
            std::cerr << "Usage: " << argv[0] << " --config configs/error-report.yaml" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
}