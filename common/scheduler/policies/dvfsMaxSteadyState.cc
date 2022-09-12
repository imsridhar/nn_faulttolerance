#include "dvfsMaxSteadyState.h"
#include "powermodel.h"
#include <iomanip>
#include <iostream>

using namespace std;

DVFSMaxSteadyState::DVFSMaxSteadyState(ThermalModel *thermalModel, const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize)
	: thermalModel(thermalModel), performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), minFrequency(minFrequency), maxFrequency(maxFrequency), frequencyStepSize(frequencyStepSize) {
	
}

std::vector<int> DVFSMaxSteadyState::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	std::vector<int> frequencies(coreRows * coreColumns);

	vector<double> budgets = thermalModel->powerBudgetMaxSteadyState(activeCores);

	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		if (activeCores.at(coreCounter)) {
			float power = performanceCounters->getPowerOfCore(coreCounter);
			float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
			int frequency = oldFrequencies.at(coreCounter);
			float utilization = performanceCounters->getUtilizationOfCore(coreCounter);
			float ips = performanceCounters->getIPSOfCore(coreCounter);

			cout << "[Scheduler][DVFSMaxSteadyState]: Core " << setw(2) << coreCounter << ":";
			cout << " P=" << fixed << setprecision(3) << power << " W";
			cout << " (budget: " << fixed << setprecision(3) << budgets.at(coreCounter) << " W)";
			cout << " f=" << frequency << " MHz";
			cout << " T=" << fixed << setprecision(1) << temperature << "°C";
			cout << " U=" << fixed << setprecision(3) << utilization;
			cout << " GIPS=" << fixed << setprecision(2) << (ips / 1e9) << endl;

			int expectedGoodFrequency = PowerModel::getExpectedGoodFrequency(frequency, power, budgets.at(coreCounter), minFrequency, maxFrequency, frequencyStepSize);
			frequencies.at(coreCounter) = expectedGoodFrequency;
		} else {
			frequencies.at(coreCounter) = minFrequency;
		}
	}

	return frequencies;
}