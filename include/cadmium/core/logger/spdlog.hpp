/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2021-present jsoulier
 * ARSLab - Carleton University
 */


#ifndef CADMIUM_CORE_LOGGER_SPDLOG_HPP_
#define CADMIUM_CORE_LOGGER_SPDLOG_HPP_

#if SPDLOG_ENABLE

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include "logger.hpp"

namespace cadmium {
    class SpdlogLogger: public Logger {
     protected:
        static constexpr int kQueueCapacity = 8192;
        std::shared_ptr<spdlog::logger> logger;
     public:
        explicit SpdlogLogger(const std::string& filepath): Logger() {
            spdlog::init_thread_pool(kQueueCapacity, 1);
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
            logger = std::make_shared<spdlog::async_logger>("cadmium", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            logger->set_pattern("%v");
        }

        virtual void start() override {
            logger->info("time,model_name,state");
        }

        virtual void stop() override {
            logger->flush();
        }

        virtual void logState(double time, long modelId, const std::string& modelName, const std::string& state) override {
            logger->info("{},{},{}", time, modelName, state);
        }
    };
}

#endif // USE_SPDLOG

#endif //CADMIUM_CORE_LOGGER_SPDLOG_HPP_
