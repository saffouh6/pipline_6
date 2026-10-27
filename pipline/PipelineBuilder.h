#pragma once
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include "Interfaces.h"
#include "Pipeline.h"
#include "LogRecord.h"

template <typename T>
class SinkEmitterAdapter : public Emitter<T> {
    std::shared_ptr<Sink<T>> sink;
public:
    explicit SinkEmitterAdapter(std::shared_ptr<Sink<T>> s) : sink(s) {}
    void emit(const T& item) override {
        if (sink) sink->consume(item);
    }
};

template <typename I, typename O>
class StageEmitterAdapter : public Emitter<I> {
    std::shared_ptr<Stage<I, O>> stage;
    std::shared_ptr<Emitter<O>> nextEmitter;
    std::shared_ptr<Sink<FailedRecord>> deadLetterSink;
    std::string stageName;
public:
    StageEmitterAdapter(std::shared_ptr<Stage<I, O>> s, std::shared_ptr<Emitter<O>> next, 
                        std::shared_ptr<Sink<FailedRecord>> dlSink, std::string name) 
        : stage(s), nextEmitter(next), deadLetterSink(dlSink), stageName(name) {}
        
    void emit(const I& item) override {
        if (!stage || !nextEmitter) return;
        try {
            stage->process(item, *nextEmitter);
        } catch (const StageException& e) {
            // Hata yakalandiginda sessizce atlamak yerine Dead-Letter (Karantina) kanalina yolla
            if (deadLetterSink) {
                std::string raw = "";
                if constexpr (std::is_same_v<I, std::string>) {
                    raw = item;
                } else if constexpr (std::is_same_v<I, LogRecord>) {
                    raw = item.raw;
                }
                deadLetterSink->consume(FailedRecord(raw, e.getStageName(), e.what()));
            }
        } catch (const std::exception& e) {
            if (deadLetterSink) {
                deadLetterSink->consume(FailedRecord("UNKNOWN", stageName, e.what()));
            }
        }
    }
};

class PipelineBuilder {
private:
    std::shared_ptr<Source<std::string>> source;
    std::shared_ptr<Stage<std::string, LogRecord>> parserStage;
    std::vector<std::shared_ptr<Stage<LogRecord, LogRecord>>> filterStages;
    std::shared_ptr<Sink<FailedRecord>> deadLetterSink;
    
public:
    static PipelineBuilder from(std::shared_ptr<Source<std::string>> src) {
        PipelineBuilder builder;
        builder.source = src;
        return builder;
    }

    PipelineBuilder& withDeadLetterSink(std::shared_ptr<Sink<FailedRecord>> dlSink) {
        this->deadLetterSink = dlSink;
        return *this;
    }

    PipelineBuilder& via(std::shared_ptr<Stage<std::string, LogRecord>> parser) {
        this->parserStage = parser;
        return *this;
    }

    PipelineBuilder& via(std::shared_ptr<Stage<LogRecord, LogRecord>> stage) {
        this->filterStages.push_back(stage);
        return *this;
    }

    Pipeline to(std::shared_ptr<Sink<LogRecord>> sink) {
        auto src = this->source;
        auto parser = this->parserStage;
        auto filters = this->filterStages;
        auto dlSink = this->deadLetterSink;

        auto runLogic = [src, parser, filters, sink, dlSink]() {
            if (dlSink) dlSink->open();
            if (sink) sink->open();
            for (auto& stage : filters) if (stage) stage->open();
            if (parser) parser->open();
            if (src) src->open();

            std::shared_ptr<Emitter<LogRecord>> currentEmitter = 
                std::make_shared<SinkEmitterAdapter<LogRecord>>(sink);
            
            int filterIndex = filters.size();
            for (auto it = filters.rbegin(); it != filters.rend(); ++it) {
                std::string sName = "FilterStage_" + std::to_string(filterIndex--);
                currentEmitter = std::make_shared<StageEmitterAdapter<LogRecord, LogRecord>>(*it, currentEmitter, dlSink, sName);
            }
            
            std::shared_ptr<Emitter<std::string>> parserAdapter = 
                std::make_shared<StageEmitterAdapter<std::string, LogRecord>>(parser, currentEmitter, dlSink, "ParserStage");

            if (src) {
                src->produce(*parserAdapter);
            }

            if (src) src->close();
            if (parser) parser->close();
            for (auto& stage : filters) if (stage) stage->close();
            if (sink) sink->close();
            if (dlSink) dlSink->close();
        };

        return Pipeline(runLogic);
    }
};