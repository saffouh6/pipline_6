#pragma once
#include "Interfaces.h"
#include "LogRecord.h"
#include <iostream>

class ConsoleSink : public Sink<LogRecord> {
public:
    void consume(const LogRecord& item) override {
        std::cout << "[IP: " << item.clientIp << "] "
                  << "Method: " << item.method << " "
                  << "Path: " << item.path << " "
                  << "Status: " << item.status << " "
                  << "Bytes: " << item.bytes << std::endl;
    }
};