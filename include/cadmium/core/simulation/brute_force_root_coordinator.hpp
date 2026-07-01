/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2021-present jsoulier
 * ARSLab - Carleton University
 */


#ifndef CADMIUM_CORE_SIMULATION_BRUTE_FORCE_ROOT_COORDINATOR_HPP_
#define CADMIUM_CORE_SIMULATION_BRUTE_FORCE_ROOT_COORDINATOR_HPP_

#include <algorithm>
#include <execution>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>
#include "coordinator.hpp"
#include "../logger/logger.hpp"
#if 1 // [cadmium_v2]
#include <cadmium/core/profile/profile.hpp>
#endif

namespace cadmium {
    class BruteForceRootCoordinator {
     protected:
        using ManyToOneCoupling = std::pair<std::shared_ptr<PortInterface>, std::vector<std::shared_ptr<PortInterface>>>;

        std::shared_ptr<Coordinator> topCoordinator;
        std::vector<ManyToOneCoupling> couplings;
        std::shared_ptr<Logger> logger;
        double lastTime;

     public:
        explicit BruteForceRootCoordinator(std::shared_ptr<Coupled> model): lastTime(0.0) {
            model->flatten();
            topCoordinator = std::make_shared<Coordinator>(model, 0.0);
            for (const auto& [portTo, portsFrom]: model->getICs()) {
                couplings.emplace_back(portTo, portsFrom);
            }
            for (auto& [componentId, component]: model->getComponents()) {
                component->releasePortLookups();
            }
        }

        virtual ~BruteForceRootCoordinator() = default;

        void setLogger(const std::shared_ptr<Logger>& log) {
            logger = log;
            topCoordinator->setLogger(log);
        }

        void start() {
            CADMIUM_PROFILE_TAG;
            if (logger != nullptr) {
                logger->start();
            }
            topCoordinator->setModelId(0);
            topCoordinator->start(topCoordinator->getTimeLast());
        }

        void stop() {
            CADMIUM_PROFILE_TAG;
            topCoordinator->stop(lastTime);
            if (logger != nullptr) {
                logger->stop();
            }
        }

        void simulate(double maxTime = std::numeric_limits<double>::infinity()) {
            const std::vector<std::shared_ptr<AbstractSimulator>>& subcomponents = topCoordinator->getSubcomponents();
            double currentTime = topCoordinator->getTimeNext();
            while (currentTime < maxTime) {
                std::for_each(std::execution::par, subcomponents.begin(), subcomponents.end(),
                    [currentTime](const std::shared_ptr<AbstractSimulator>& simulator) {
                        simulator->collection(currentTime);
                    });
                std::for_each(std::execution::par, couplings.begin(), couplings.end(),
                    [](const ManyToOneCoupling& coupling) {
                        for (const auto& portFrom: coupling.second) {
                            if (!portFrom->empty()) {
                                coupling.first->propagate(portFrom);
                            }
                        }
                    });
                std::for_each(std::execution::par, subcomponents.begin(), subcomponents.end(),
                    [currentTime](const std::shared_ptr<AbstractSimulator>& simulator) {
                        simulator->transition(currentTime);
                        simulator->clear();
                    });
                double nextTime = std::transform_reduce(std::execution::par,
                    subcomponents.begin(), subcomponents.end(), maxTime,
                    [](double lhs, double rhs) -> double {
                        return lhs < rhs ? lhs : rhs;
                    },
                    [](const std::shared_ptr<AbstractSimulator>& simulator) -> double {
                        return simulator->getTimeNext();
                    });
                lastTime = currentTime;
                currentTime = nextTime;
                CADMIUM_PROFILE_FRAME;
            }
        }
    };
}

#endif //CADMIUM_CORE_SIMULATION_BRUTE_FORCE_ROOT_COORDINATOR_HPP_
