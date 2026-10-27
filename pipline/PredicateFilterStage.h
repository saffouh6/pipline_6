#pragma once
#include "Interfaces.h"
#include "LogRecord.h"
#include <functional>

class PredicateFilterStage : public Stage<LogRecord, LogRecord> {
private:
    std::function<bool(const LogRecord&)> predicate;

public:
    explicit PredicateFilterStage(std::function<bool(const LogRecord&)> pred)
        : predicate(pred) {}

    void process(const LogRecord& input, Emitter<LogRecord>& out) override {
        if (predicate(input)) {
            out.emit(input); // Koşulu sağlayanlar bir sonraki aşamaya iletilir
        }
    }
};