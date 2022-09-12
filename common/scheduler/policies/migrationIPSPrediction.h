/**
 * This header implements the migration policy that uses the IPS prediction model
 */

#ifndef __MIGRATION_IPS_PREDICTION_H
#define __MIGRATION_IPS_PREDICTION_H

#include <vector>
#include "migrationpolicy.h"
#include "performance_counters.h"
#include "thermalModel.h"

class MigrationIPSPrediction : public MigrationPolicy {
public:
    MigrationIPSPrediction(int migrationPeriod, int ignoreAfterMigration, ThermalModel* thermalModel, const PerformanceCounters *performanceCounters, int coreRows, int coreColumns);
    virtual std::vector<migration> migrate(SubsecondTime time, const std::vector<int> &taskIds, const std::vector<bool> &activeCores);

private:
    const float delta = 0.03f;

    struct thread {
        float currentAMD;
        float currentPowerBudget;
        float currentIPS;
        float currentRelNuca;
        float targetAMD;
        float targetPowerBudget;
        unsigned int currentCore;
        unsigned int targetCore;
    };

    int migrationPeriod;
    int ignoreAfterMigration;
    ThermalModel *thermalModel;
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;

    std::vector<float> ipsPerCore;
    std::vector<float> relNucaPerCore;
    int iterationCounter;

    float getAMD(int coreNb);
    float predictIPS(const MigrationIPSPrediction::thread &thread);
    void updateTraces();
    void resetTraces();
    std::vector<migration> performMigration(const std::vector<int> &taskIds, const std::vector<bool> &activeCores);
};

#endif