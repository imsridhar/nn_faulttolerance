/**
 * pcmap
 * This header implements the PCMap policy
 */

#ifndef __PCMAP_H
#define __PCMAP_H

#include <set>
#include <vector>
#include "thermalModel.h"
#include "mappingpolicy.h"

class PCMap : public MappingPolicy {
public:
    PCMap(int coreRows, int coreColumns, ThermalModel *thermalModel);
    virtual std::vector<int> map(String taskName, int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);

private:
    unsigned int coreRows;
    unsigned int coreColumns;
    ThermalModel *thermalModel;
    std::vector<float> amds;
    std::set<float> uniqueAMDs;

    std::vector<std::tuple<float, float, std::vector<int>>> getMappingCandidates(int taskCoreRequirement, const std::vector<bool> &availableCores, const std::vector<bool> &activeCores);
    int manhattanDistance(int y1, int x1, int y2, int x2);
	float getCoreAMD(int coreY, int coreX);
};

#endif