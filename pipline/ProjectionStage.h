#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include "Interfaces.h"
#include "LogRecord.h"

class ProjectionStage : public Stage<LogRecord, LogRecord> {
private:
    std::vector<std::string> fields;

    std::string cleanString(std::string s) const {
        s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    std::string formatTime(const std::chrono::system_clock::time_point& tp) const {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        return std::string(buf);
    }

public:
    explicit ProjectionStage(const std::vector<std::string>& selectedFields) {
        for (const auto& f : selectedFields) {
            fields.push_back(cleanString(f));
        }
    }

    void process(const LogRecord& input, Emitter<LogRecord>& out) override {
        std::ostringstream oss;
        
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];
            
            if (field == "timestamp") oss << formatTime(input.timestamp);
            else if (field == "clientip" || field == "client_ip") oss << input.clientIp;
            else if (field == "method") oss << input.method;
            else if (field == "path") oss << input.path;
            else if (field == "status") oss << input.status;
            else if (field == "bytes") oss << input.bytes;
            else if (field == "useragent" || field == "user_agent") oss << input.userAgent;
            
            if (i < fields.size() - 1) oss << " ";
        }

        LogRecord projected(
            input.timestamp, input.clientIp, input.method, 
            input.path, input.status, input.bytes, 
            input.userAgent, input.attributes, oss.str()
        );

        out.emit(projected);
    }
};