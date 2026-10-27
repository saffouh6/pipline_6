#pragma once
#include <string>
#include <map>
#include <chrono>

struct LogRecord {
    const std::chrono::system_clock::time_point timestamp;
    const std::string clientIp;
    const std::string method;
    const std::string path;
    const int status;
    const long bytes;
    const std::string userAgent;
    const std::map<std::string, std::string> attributes;
    const std::string raw;

    LogRecord(
        std::chrono::system_clock::time_point ts,
        std::string ip,
        std::string m,
        std::string p,
        int s,
        long b,
        std::string ua,
        std::map<std::string, std::string> attr,
        std::string r
    ) : timestamp(ts), clientIp(std::move(ip)), method(std::move(m)), 
        path(std::move(p)), status(s), bytes(b), userAgent(std::move(ua)), 
        attributes(std::move(attr)), raw(std::move(r)) {}
};

// Dead-Letter kanali icin basarisiz kayit modeli
struct FailedRecord {
    const std::string rawInput;
    const std::string stageName;
    const std::string exceptionMessage;
    const std::chrono::system_clock::time_point timestamp;

    FailedRecord(
        std::string raw,
        std::string stage,
        std::string msg,
        std::chrono::system_clock::time_point ts = std::chrono::system_clock::now()
    ) : rawInput(std::move(raw)), stageName(std::move(stage)), 
        exceptionMessage(std::move(msg)), timestamp(ts) {}
};