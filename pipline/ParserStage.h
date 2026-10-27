#pragma once
#include <string>
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <vector>
#include <map>
#include "Interfaces.h"
#include "LogRecord.h"

class ParserStage : public Stage<std::string, LogRecord> {
private:
    int errorCount = 0;
    int successCount = 0;

    // Standart C++ string yapisi (Raw string R"(...)" kullanilmadan R-parenthesis hatasi engellendi)
    static const std::regex& getLogRegex() {
        static const std::regex logRegex(
            "^(\\S+) \\S+ \\S+ \\[(.*?)\\] \"(\\S+) ([^\"\\s]+)(?: \\S+)?\" (\\d+) (\\d+|-)(?: \"([^\"]*)\")?(?: \"([^\"]*)\")?.*"
        );
        return logRegex;
    }

    std::chrono::system_clock::time_point parseTimestamp(const std::string& timeStr) {
        int day = 1, month = 1, year = 2000, hour = 0, min = 0, sec = 0;
        char monthStr[4] = {0};
        if (sscanf(timeStr.c_str(), "%d/%3s/%d:%d:%d:%d", &day, monthStr, &year, &hour, &min, &sec) >= 6) {
            std::string m(monthStr);
            static const std::vector<std::string> months = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            for (int i = 0; i < 12; ++i) {
                if (m == months[i]) { month = i + 1; break; }
            }
            std::tm tm = {};
            tm.tm_mday = day;
            tm.tm_mon = month - 1;
            tm.tm_year = year - 1900;
            tm.tm_hour = hour;
            tm.tm_min = min;
            tm.tm_sec = sec;
            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
        return std::chrono::system_clock::now();
    }

public:
    void process(const std::string& input, Emitter<LogRecord>& out) override {
        if (input.empty()) return;

        std::smatch matches;
        if (std::regex_match(input, matches, getLogRegex())) {
            try {
                std::string ip = matches[1].str();
                std::string timeStr = matches[2].str();
                std::string method = matches[3].str();
                std::string path = matches[4].str();
                int status = std::stoi(matches[5].str());
                
                std::string byteStr = matches[6].str();
                long bytes = (byteStr == "-") ? 0 : std::stol(byteStr);
                
                std::string userAgent = (matches.size() > 8 && matches[8].matched) ? matches[8].str() : "-";
                std::map<std::string, std::string> attributes;
                
                size_t qMark = path.find('?');
                if (qMark != std::string::npos) {
                    attributes["query"] = path.substr(qMark + 1);
                    path = path.substr(0, qMark);
                }

                auto timestamp = parseTimestamp(timeStr);
                LogRecord record(timestamp, ip, method, path, status, bytes, userAgent, attributes, input);
                successCount++;
                out.emit(record);

            } catch (...) {
                errorCount++;
            }
        } else {
            errorCount++;
        }
    }

    void close() override {
        if (errorCount > 0) {
            std::cout << "Parser summary: " << successCount << " parsed successfully, " 
                      << errorCount << " skipped lines." << std::endl;
        }
    }
};