/**
 * spdlog-based asynchronous logger.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2021-present jsoulier
 * ARSLab - Carleton University
 */


#ifndef CADMIUM_CORE_LOGGER_SPDLOG_HPP_
#define CADMIUM_CORE_LOGGER_SPDLOG_HPP_

#if USE_SPDLOG

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include "logger.hpp"

namespace cadmium {
    //! Cadmium spdlog asynchronous logger class.
    class SpdlogLogger: public Logger {
     private:
        static constexpr int kQueueCapacity = 8192;  //!< Capacity of the async thread pool queue.
        std::shared_ptr<spdlog::logger> logger;      //!< Underlying spdlog logger.
     public:
        /**
         * Constructor function.
         * @param filepath path to the output file.
         */
        explicit SpdlogLogger(const std::string& filepath): Logger() {
            spdlog::init_thread_pool(kQueueCapacity, 1);
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
            logger = std::make_shared<spdlog::async_logger>("cadmium", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            logger->set_pattern("%v");
        }

        //! It prints the CSV header.
        void start() override {
            logger->info("time,x,y,longitude,latitude,elevation,size,status");
        }

        //! It flushes the logger after the simulation.
        void stop() override {
            logger->flush();
        }

        void logOutput(double time, long modelId, const std::string& modelName, const std::string& portName, const std::string& output) override {}

        void logState(double time, long modelId, const std::string& modelName, const std::string& state) override {
            logger->info("{},{},{}", time, modelName, state);
        }
    };
}

#endif // USE_SPDLOG

#endif //CADMIUM_CORE_LOGGER_SPDLOG_HPP_
