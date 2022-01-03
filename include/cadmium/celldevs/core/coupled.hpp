/**
 * Abstract Cell-DEVS coupled model.
 * Copyright (C) 2021  Román Cárdenas Rodríguez
 * ARSLab - Carleton University
 * GreenLSI - Polytechnic University of Madrid
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _CADMIUM_CELLDEVS_CORE_COUPLED_HPP_
#define _CADMIUM_CELLDEVS_CORE_COUPLED_HPP_

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include "../../core/modeling/coupled.hpp"
#include "cell.hpp"
#include "config.hpp"

namespace cadmium::celldevs {
	template<typename C, typename S, typename V>
	class CellDEVSCoupled : public Coupled {
	 protected:
		nlohmann::json rawConfig;                                                             /// JSON configuration file.
		std::unordered_map<std::string, std::shared_ptr<CellConfig<C, S, V>>> cellConfigs;    /// unordered map containing all the different cell configurations
	 public:
		CellDEVSCoupled(const std::string& id, const std::string& configFilePath): Coupled(id), rawConfig(), cellConfigs() {
			std::ifstream i(configFilePath);
			i >> rawConfig;
		}

		/**
	     * Generates a cell configuration struct from a JSON object.
		 * @param configId ID of the configuration to be loaded.
		 * @param cellConfig cell configuration parameters (JSON object).
		 * @return pointer to the cell configuration created.
		 */
		virtual std::shared_ptr<CellConfig<C, S, V>> loadCellConfig(const std::string& configId, const nlohmann::json& cellConfig) const = 0;

		/// It adds all the cells according to the provided JSON configuration file.
		virtual void addCells(const std::shared_ptr<CellConfig<C, S, V>>& cellConfig) = 0;

		/// It adds all the default cells according to the provided JSON configuration file.
		virtual void addDefaultCells(const std::shared_ptr<CellConfig<C, S, V>>& defaultConfig) {}

		/// It builds the Cell-DEVS model completely
		void buildModel() {
			loadCellConfigs();
			addCells();
			addCouplings();
		}

		void loadCellConfigs() {
			auto configs = rawConfig.contains("cells") ? rawConfig["cells"] : nlohmann::json::object();
			auto rawDefault = (configs.contains("default")) ? configs["default"] : nlohmann::json::object();
			auto defaultConfig = this->loadCellConfig("default", rawDefault);
			cellConfigs["default"] = defaultConfig;
			for (auto const&[configId, patch]: configs.items()) {
				if (configId != "default") {
					cellConfigs[configId] = loadCellConfig(configId, rawDefault, patch);
				}
			}
		}

		/// It adds all the cells according to the provided JSON configuration file.
		void addCells() {
			for (auto const&[configId, cellConfig]: cellConfigs) {
				if (configId != "default") {  // Default cells are added at the end of this method
					this->addCells(cellConfig);
				}
			}
			this->addDefaultCells(cellConfigs.at("default"));
		}

		/**
		 * It generates a cell configuration parameter from a default configuration and a patch.
		 * @param configId ID of the cell-DEVS configuration.
		 * @param cellConfig default configuration (JSON object).
		 * @param patch patch to be applied to the default configuration (JSON object).
		 * @return unique pointer to the created cell configuration struct.
		 */
		std::shared_ptr<CellConfig<C, S, V>> loadCellConfig(const std::string& configId, const nlohmann::json& cellConfig, const nlohmann::json& patch) const {
			auto copyConfig = nlohmann::json::parse(cellConfig.dump());
			copyConfig.merge_patch(patch);
			return this->loadCellConfig(configId, copyConfig);
		}

		/// It adds all the couplings required in the scenario according to the configuration file.
		void addCouplings() {
			for (const auto& cell: Coupled::components) {
				auto cellModel = std::dynamic_pointer_cast<Cell<C, S, V>>(cell);
				if (cellModel == nullptr) {
					throw std::bad_exception();  // TODO custom exception: unable to treat component as a cell
				}
				for (const auto& neighbor: cellModel->getNeighborhood()) {
					addInternalCoupling(cellId(neighbor.first), "neighborhoodOutput",
						cellModel->getId(), "neighborhoodInput");
				}
				auto cellConfig = cellModel->getCellConfig();
				for (const auto& eic: cellConfig->EIC) {
					addExternalInputCoupling(eic.first, cellModel->getId(), eic.second);
				}
				for (const auto& eoc: cellConfig->EOC) {
					addExternalOutputCoupling(cellModel->getId(), eoc.first, eoc.second);
				}
			}
		}

		/**
		 * Returns a string representation of a cell.
		 * @param id ID of a cell.
		 * @return string representation of the cell ID.
		 */
		static std::string cellId(const C& id) {
			std::stringstream ss;
			ss << id;
			return ss.str();
		}
	};
}

#endif //_CADMIUM_CELLDEVS_CORE_COUPLED_HPP_
