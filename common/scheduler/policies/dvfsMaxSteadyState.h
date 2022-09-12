/**
 * This header implements the max. steady state DVFS policy
 */

#ifndef __DVFS_MAX_STEADY_STATE_H
#define __DVFS_MAX_STEADY_STATE_H

#include <vector>
#include "dvfspolicy.h"
#include "thermalModel.h"

class DVFSMaxSteadyState : public DVFSPolicy {
public:
    DVFSMaxSteadyState(ThermalModel* thermalModel, const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int minFrequency, int maxFrequency, int frequencyStepSize);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);

private:
    ThermalModel* thermalModel;
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;
};

#endif