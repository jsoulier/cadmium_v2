/**
 * TODO
 * Copyright (C) 2022  Román Cárdenas Rodríguez
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

#include <cadmium/core/logger/csv.hpp>
#include <cadmium/core/simulation/coordinator.hpp>
#include <fstream>
#include <string>
#include "coupled.hpp"

int main(int argc, char ** argv) {
	if (argc < 2) {
		std::cout << "Program used with wrong parameters. The program must be invoked as follows:";
		std::cout << argv[0] << " SCENARIO_CONFIG.json [MAX_SIMULATION_TIME (default: 500)]" << std::endl;
		return -1;
	}
	std::string configFilePath = argv[1];
	double simTime = (argc > 2)? atof(argv[2]) : 500;

	auto model = SIRDSCoupled("sirds", configFilePath);
	model.buildModel();
	auto coordinator = cadmium::Coordinator(model);
	auto logger = std::make_shared<cadmium::CSVLogger>("log.csv", ";");
	coordinator.setLogger(logger);
	coordinator.start();
	coordinator.simulate(simTime);
	coordinator.stop();
}
