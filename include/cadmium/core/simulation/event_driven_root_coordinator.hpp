#ifndef CADMIUM_CORE_SIMULATION_EVENT_DRIVEN_ROOT_COORDINATOR_HPP_
#define CADMIUM_CORE_SIMULATION_EVENT_DRIVEN_ROOT_COORDINATOR_HPP_

#include <ankerl/unordered_dense.h>

#include <algorithm>
#include <cstdint>
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
    class EventDrivenRootCoordinator {
     protected:
        using ManyToOneCoupling = std::pair<std::shared_ptr<PortInterface>, std::vector<std::shared_ptr<PortInterface>>>;
        using OneToOneCoupling = std::pair<std::shared_ptr<PortInterface>, std::shared_ptr<PortInterface>>;

        std::shared_ptr<Coordinator> topCoordinator;
        std::vector<ManyToOneCoupling> couplings;
        std::shared_ptr<Logger> logger;
        double lastTime;

     public:
        explicit EventDrivenRootCoordinator(std::shared_ptr<Coupled> model): lastTime(0.0) {
            model->flatten();
            topCoordinator = std::make_shared<Coordinator>(model, 0.0);
            for (const auto& [portTo, portsFrom]: model->getICs()) {
                couplings.emplace_back(portTo, portsFrom);
            }
            for (auto& [componentId, component]: model->getComponents()) {
                component->releasePortLookups();
            }
        }

        virtual ~EventDrivenRootCoordinator() = default;

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
            const auto forEach = [](const std::vector<size_t>& cells, const auto& function) {
                static constexpr size_t kParallelThreshold = 256;
                if (cells.size() >= kParallelThreshold) {
                    std::for_each(std::execution::par, cells.begin(), cells.end(), function);
                } else {
                    for (size_t cell: cells) {
                        function(cell);
                    }
                }
            };

            CADMIUM_PROFILE_TAG;
            const std::vector<std::shared_ptr<AbstractSimulator>>& subcomponents = topCoordinator->getSubcomponents();
            const size_t cellCount = subcomponents.size();
            ankerl::unordered_dense::map<const Component*, size_t> cellToIndex;
            cellToIndex.reserve(cellCount);
            for (size_t i = 0; i < cellCount; i++) {
                cellToIndex[subcomponents[i]->getComponent().get()] = i;
            }
            // inCouplings are sending to the current cell, outCellIndices are indices for the cells it sends to
            std::vector<std::vector<OneToOneCoupling>> inCouplings(cellCount);
            std::vector<std::vector<size_t>> outCellIndices(cellCount);
            for (const auto& [portTo, portsFrom]: couplings) {
                const size_t dstCellIndex = cellToIndex.at(portTo->getParent());
                for (const auto& portFrom: portsFrom) {
                    const size_t srcCellIndex = cellToIndex.at(portFrom->getParent());
                    inCouplings[dstCellIndex].emplace_back(portTo, portFrom);
                    outCellIndices[srcCellIndex].push_back(dstCellIndex);
                }
            }
            // frame 1: all cells need to be computed
            // frame 2: cells that received messages from inCouplings need to be computed (where inCouplings comes from current or neighbors)
            // frame 3: repeat frame 2
            std::vector<size_t> cellIndices1(cellCount);
            std::iota(cellIndices1.begin(), cellIndices1.end(), size_t(0));
            std::vector<size_t> cellIndices2;
            std::uint64_t dedupeIndicesFrame = 0;
            std::vector<size_t> dedupeIndices(cellCount, dedupeIndicesFrame);
            const auto tryQueue = [&cellIndices2, &dedupeIndices, &dedupeIndicesFrame](size_t cellIndex) {
                if (dedupeIndices[cellIndex] != dedupeIndicesFrame) {
                    dedupeIndices[cellIndex] = dedupeIndicesFrame;
                    cellIndices2.push_back(cellIndex);
                }
            };

            double currentTime = topCoordinator->getTimeNext();
            while (currentTime < maxTime) {
                {
                    CADMIUM_PROFILE_TAG_BLOCK("Collection");
                    forEach(cellIndices1, [&](size_t cell) {
                        subcomponents[cell]->collection(currentTime);
                    });
                }
                {
                    CADMIUM_PROFILE_TAG_BLOCK("Propagate");
                    forEach(cellIndices1, [&](size_t cell) {
                        for (const auto& [portTo, portFrom]: inCouplings[cell]) {
                            if (!portFrom->empty()) {
                                portTo->propagate(portFrom);
                            }
                        }
                    });
                }
                {
                    CADMIUM_PROFILE_TAG_BLOCK("Transition");
                    forEach(cellIndices1, [&](size_t cell) {
                        subcomponents[cell]->transition(currentTime);
                        subcomponents[cell]->clear();
                    });
                }
                {
                    CADMIUM_PROFILE_TAG_BLOCK("TimeNext");
                    double nextTime = maxTime;
                    for (size_t cell: cellIndices1) {
                        nextTime = std::min(nextTime, subcomponents[cell]->getTimeNext());
                    }

                    dedupeIndicesFrame++;
                    cellIndices2.clear();
                    for (size_t cellIndex: cellIndices1) {
                        const double cellTime = subcomponents[cellIndex]->getTimeNext();
                        if (cellTime >= maxTime) {
                            continue;
                        }
                        tryQueue(cellIndex);
                        if (cellTime == nextTime) {
                            for (size_t neighborIndex: outCellIndices[cellIndex]) {
                                tryQueue(neighborIndex);
                            }
                        }
                    }
                    cellIndices1.swap(cellIndices2);
                    lastTime = currentTime;
                    currentTime = nextTime;
                }
                CADMIUM_PROFILE_PLOT("CellCount", int64_t(cellIndices1.size()));
                CADMIUM_PROFILE_FRAME;
            }
        }
    };
}

#endif //CADMIUM_CORE_SIMULATION_EVENT_DRIVEN_ROOT_COORDINATOR_HPP_
