#pragma once
#include <string>
#include <regex>
#include <chrono>
#include "Interfaces.h"
#include "LogRecord.h"

class ValidationStage : public Stage<LogRecord, LogRecord> {
private:
    static bool isValidIp(const std::string& ip) {
        if (ip.empty()) return false;
        static const std::regex ipRegex(
            "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
        );
        return std::regex_match(ip, ipRegex);
    }

public:
    void process(const LogRecord& input, Emitter<LogRecord>& out) override {
        // 1. IP Adresi Dogrulamasi
        if (!isValidIp(input.clientIp)) {
            throw StageException("ValidationStage", "Invalid IP format: " + input.clientIp);
        }

        // 2. HTTP Durum Kodu Aralik Denetimi (100 - 599)
        if (input.status < 100 || input.status > 599) {
            throw StageException("ValidationStage", "Status code out of range (100-599): " + std::to_string(input.status));
        }

        // 3. Bayt Sayisi Denetimi (Negatif olamaz)
        if (input.bytes < 0) {
            throw StageException("ValidationStage", "Negative byte count: " + std::to_string(input.bytes));
        }

        // 4. Zaman Damgasi Mantik Denetimi (1980 - 2050 yillari arasi)
        std::time_t t = std::chrono::system_clock::to_time_t(input.timestamp);
        std::tm* tmPtr = std::gmtime(&t);
        if (tmPtr) {
            int year = tmPtr->tm_year + 1900;
            if (year < 1980 || year > 2050) {
                throw StageException("ValidationStage", "Timestamp year out of valid range: " + std::to_string(year));
            }
        }

        // 5. Zorunlu Alan Kontrolu
        if (input.method.empty() || input.path.empty()) {
            throw StageException("ValidationStage", "Empty method or path field");
        }

        out.emit(input);
    }
};