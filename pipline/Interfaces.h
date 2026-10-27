#pragma once
#include <string>
#include <stdexcept>
#include <chrono>
#include <map>
#include <vector>

// Kurtarilabilir asama hatasi (Recoverable stage exception)
class StageException : public std::runtime_error {
private:
    std::string stageName;
public:
    StageException(const std::string& stage, const std::string& msg)
        : std::runtime_error(msg), stageName(stage) {}
    
    std::string getStageName() const { return stageName; }
};

// Durdurucu sistem hatasi (Unrecoverable pipeline exception)
class PipelineException : public std::runtime_error {
public:
    explicit PipelineException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// Hata Politikasi Secenekleri
enum class ErrorPolicy {
    FAIL_FAST,
    SKIP,
    DEAD_LETTER,
    RETRY
};

// Calistirma Ozeti (RunSummary)
struct RunSummary {
    size_t inputRecords = 0;
    size_t outputRecords = 0;
    size_t deadLetterRecords = 0;
    std::map<std::string, size_t> skippedByStage;
    double elapsedTimeMs = 0.0;
};

// Emitter arayuzu
template <typename T>
class Emitter {
public:
    virtual ~Emitter() = default;
    virtual void emit(const T& item) = 0;
};

// Asama (Stage) arayuzu
template <typename I, typename O>
class Stage {
public:
    virtual ~Stage() = default;
    virtual void process(const I& input, Emitter<O>& out) = 0;
    virtual void open() {}
    virtual void close() {}
};

// Kaynak ve Hedef arayuzleri
template <typename O>
class Source {
public:
    virtual ~Source() = default;
    virtual void produce(Emitter<O>& out) = 0;
    virtual void open() {}
    virtual void close() {}
};

template <typename I>
class Sink {
public:
    virtual ~Sink() = default;
    virtual void consume(const I& item) = 0;
    virtual void open() {}
    virtual void close() {}
};