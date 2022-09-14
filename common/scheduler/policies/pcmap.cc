#include "pcmap.h"
#include <iomanip>
#include <limits>
#include <tuple>
#include "powermodel.h"

using namespace std;

PCMap::PCMap(int coreRows, int coreColumns, ThermalModel *thermalModel)
     : coreRows(coreRows), coreColumns(coreColumns), thermalModel(thermalModel) {
	// get core AMD info
    for (int y = 0; y < coreColumns; y++) {
		for (int x = 0; x < coreRows; x++) {
			float amd = getCoreAMD(y, x);
			amds.push_back(amd);
			uniqueAMDs.insert(amd);
		}
	}
}

int PCMap::manhattanDistance(int y1, int x1, int y2, int x2) {
	int dy = y1 - y2;
	int dx = x1 - x2;
	return abs(dy) + abs(dx);
}

float PCMap::getCoreAMD(int coreY, int coreX) {
	int md_sum = 0;
	for (unsigned int y = 0; y < coreColumns; y++) {
		for (unsigned int x = 0; x < coreRows; x++) {
			md_sum += manhattanDistance(coreY, coreX, y, x);
		}
	}
	return (float)md_sum / coreColumns / coreRows;
}


/** getMappingCandidates
 * Get all near-Pareto-optimal mappings considered in PCMap.
 * Return a vector of tuples (max. AMD, power budget, used cores)
 */
vector<tuple<float, float, vector<int>>> PCMap::getMappingCandidates(int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
	// get the candidates
	vector<tuple<float, float, vector<int>>> candidates;
	for (const float &amdMax : uniqueAMDs) {
		// get the candidate for the given AMD_max

		vector<int> availableCoresAMD;
		for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
			if (availableCores.at(i) && (amds.at(i) <= amdMax)) {
				availableCoresAMD.push_back (i);
			}
		}

		if ((int)availableCoresAMD.size() >= taskCoreRequirement) {
			vector<int> selectedCores;

			vector<bool> activeCoresCandidate = activeCores;

			float mappingTSP = 0;
			float maxUsedAMD = 0;
			while ((int)selectedCores.size() < taskCoreRequirement) {
				// greedily select one core
#if true
				float bestTSP = 0;
				int bestIndex = 0;
				for (unsigned int i = 0; i < availableCoresAMD.size(); i++) {
					// try each core
					activeCoresCandidate[availableCoresAMD.at(i)] = true;
					float thisTSP = thermalModel->tsp(activeCoresCandidate);
					activeCoresCandidate[availableCoresAMD.at(i)] = false;

					if (thisTSP > bestTSP) {
						bestTSP = thisTSP;
						bestIndex = i;
					}
				}
#else
				vector<double> tsps = thermalModel->tspForManyCandidates(activeCoresCandidate, availableCoresAMD);
				float bestTSP = 0;
				int bestIndex = 0;
				for (unsigned int i = 0; i < tsps.size(); i++) {
					if (tsps.at(i) > bestTSP) {
						bestTSP = tsps.at(i);
						bestIndex = i;
					}
				}
#endif

				activeCoresCandidate[availableCoresAMD.at(bestIndex)] = true;
				selectedCores.push_back(availableCoresAMD.at(bestIndex));

				mappingTSP = bestTSP;
				maxUsedAMD = max(maxUsedAMD, amds.at(availableCoresAMD.at(bestIndex)));

				availableCoresAMD.erase(availableCoresAMD.begin() + bestIndex);
			}

			if (maxUsedAMD == amdMax) {
				// add the mapping to the list of mappings
				tuple<float, float, vector<int>> mapping(amdMax, mappingTSP, selectedCores);
				candidates.push_back(mapping);
			}
		}
	}

	return candidates;
}

/** map
    This function performs patterning after PCMap
*/
std::vector<int> PCMap::map(String taskName, int taskCoreRequirement, const vector<bool> &availableCores, const vector<bool> &activeCores) {
	// get the mapping candidates
	vector<tuple<float, float, vector<int>>> mappingCandidates = getMappingCandidates(taskCoreRequirement, availableCores, activeCores);

	// find the best mapping
	int bestMappingNb = 0;
	if (mappingCandidates.size() == 0) {
        vector<int> empty;
		return empty;
	} else if (mappingCandidates.size() == 1) {
		bestMappingNb = 0;
	} else {
		float minAmd = get<0>(mappingCandidates.front());
		float minPowerBudget = get<1>(mappingCandidates.front());
		float deltaAmd = get<0>(mappingCandidates.back()) - minAmd;
		float deltaPowerBudget = get<1>(mappingCandidates.back()) - minPowerBudget;

		float alpha = deltaPowerBudget / deltaAmd;

		float bestRating = numeric_limits<float>::lowest();

		for (unsigned int mappingNb = 0; mappingNb < mappingCandidates.size(); mappingNb++) {
			float amdMax = get<0>(mappingCandidates.at(mappingNb));
			float powerBudget = get<1>(mappingCandidates.at(mappingNb));
			float rating = (powerBudget - minPowerBudget) - alpha * (amdMax - minAmd);
			vector<int> cores = get<2>(mappingCandidates.at(mappingNb));

			cout << "Mapping Candidate: rating; " << setprecision(3) << rating << ", power budget: " << setprecision(3) << powerBudget << " W, max. AMD: " << setprecision(3) << amdMax;
			cout << ", used cores:";
			for (unsigned int i = 0; i < cores.size(); i++) {
				cout << " " << cores.at(i);
			}
			cout << endl;

			if (rating > bestRating) {
				bestRating = rating;
				bestMappingNb = mappingNb;
			}
		}
	}

	// return the cores
	return get<2>(mappingCandidates.at(bestMappingNb));
}