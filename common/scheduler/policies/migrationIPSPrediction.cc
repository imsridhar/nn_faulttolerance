#include "migrationIPSPrediction.h"
#include <iostream>
#include <iomanip>
#include <math.h>
#include "simulator.h"
#include "magic_server.h"

using namespace std;

MigrationIPSPrediction::MigrationIPSPrediction(int migrationPeriod, int ignoreAfterMigration, ThermalModel *thermalModel, const PerformanceCounters *performanceCounters, int coreRows, int coreColumns)
    : migrationPeriod(migrationPeriod), ignoreAfterMigration(ignoreAfterMigration), thermalModel(thermalModel), performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns) {
    for (unsigned int coreCounter = 0; coreCounter < this->coreRows * this->coreColumns; coreCounter++) {
        ipsPerCore.push_back(0);
        relNucaPerCore.push_back(0);
    }
    iterationCounter = 0;
}

float MigrationIPSPrediction::predictIPS(const MigrationIPSPrediction::thread &thread) {
    if (thread.currentIPS < 0.01e9) {
        return 0;
    }
    float input[6];

    input[0] = thread.currentAMD;
    input[1] = min(thread.currentPowerBudget, 3.5f);
    input[2] = thread.currentIPS / 1e9f;
    input[3] = thread.currentRelNuca;
    input[4] = thread.targetAMD;
    input[5] = min(thread.targetPowerBudget, 3.5f);

    #include "model.cpp"

    return nn_out * 1e9f;
}

float MigrationIPSPrediction::getAMD(int coreNb) {
    int y = coreNb / coreColumns;
    int x = coreNb % coreColumns;

    float dx = x + 1 - 0.5 * (coreColumns + 1);
    float dy = y + 1 - 0.5 * (coreRows + 1);

    return dx * dx / coreColumns + dy * dy / coreRows + (coreColumns + coreRows) / 4.0f - 1 / (4.0f * coreColumns) - 1 / (4.0f * coreRows);
}

std::vector<migration> MigrationIPSPrediction::performMigration(const std::vector<int> &taskIds, const std::vector<bool> &activeCores) {
    vector<double> budgets = thermalModel->powerBudgetMaxSteadyState(activeCores);

    std::vector<MigrationIPSPrediction::thread> threads;
    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
        if (activeCores.at(coreCounter)) {
            MigrationIPSPrediction::thread thread;
            thread.currentAMD = getAMD(coreCounter);
            thread.currentPowerBudget = budgets.at(coreCounter);
            thread.currentIPS = ipsPerCore.at(coreCounter);
            thread.currentRelNuca = relNucaPerCore.at(coreCounter);
            thread.currentCore = coreCounter;
            thread.targetPowerBudget = thread.currentPowerBudget;
            thread.targetAMD = thread.currentAMD;
            thread.targetCore = thread.currentCore;
            threads.push_back(thread);
        }
    }
    vector<float> baseIPS(threads.size());
    for (unsigned int i = 0; i < threads.size(); i++) {
        baseIPS.at(i) = predictIPS(threads.at(i));
    }
    float bestSpeedup = -1;
    migration m;

    for (unsigned int threadCounter = 0; threadCounter < threads.size(); threadCounter++) {
        // try to migrate this thread

        /*cout << "[MigrationIPSPrediction]Trying migrating thread " << threadCounter << " on core " << threads.at(threadCounter).currentCore << " (";
        cout << " amd=" << threads.at(threadCounter).currentAMD;
        cout << " pb=" << threads.at(threadCounter).currentPowerBudget;
        cout << " ips=" << threads.at(threadCounter).currentIPS / 1e9;
        cout << " nuca=" << threads.at(threadCounter).currentRelNuca;
        cout << ")" << endl;*/

        for (unsigned int targetCore = 0; targetCore < coreColumns * coreRows; targetCore++) {
            if (!activeCores.at(targetCore)) {
                // not occupied
                //cout << "considering moving " << threadCounter << " from core " << threads.at(threadCounter).targetCore << " to core " << targetCore << "...";

                threads.at(threadCounter).targetAMD = getAMD(targetCore);
                threads.at(threadCounter).targetCore = targetCore;

                vector<bool> migratedActiveCores = activeCores;
                migratedActiveCores.at(threads.at(threadCounter).currentCore) = 0;
                migratedActiveCores.at(targetCore) = 1;

                vector<double> migratedBudgets = thermalModel->powerBudgetMaxSteadyState(migratedActiveCores);
                for (MigrationIPSPrediction::thread &thread : threads) {
                    thread.targetPowerBudget = migratedBudgets.at(thread.targetCore);
                }

                float speedup = 0;
                for (unsigned int i = 0; i < threads.size(); i++) {
                    if ((i == threadCounter) || (fabsf(threads.at(i).targetPowerBudget - threads.at(i).currentPowerBudget) > 0.1f)) {
                        speedup += predictIPS(threads.at(i)) / baseIPS.at(i) - 1;
                    }
                }

                //cout << " speedup: " << setprecision(3) << speedup << endl;
                if (speedup > bestSpeedup) {
                    m.fromCore = threads.at(threadCounter).currentCore;
                    m.toCore = targetCore;
                    m.swap = (taskIds.at(targetCore) != -1);
                    bestSpeedup = speedup;
                }

                // no need to reset all target power budgets, they will be overwritten anyways
                threads.at(threadCounter).targetAMD = threads.at(threadCounter).currentAMD;
                threads.at(threadCounter).targetCore = threads.at(threadCounter).currentCore;
            } else if (targetCore > threads.at(threadCounter).currentCore) {
                // try swapping
                int swapThreadCounter = -1;
                for (unsigned int tc = 0; tc < threads.size(); tc++) {
                    if (threads.at(tc).currentCore == targetCore) {
                        swapThreadCounter = tc;
                        break;
                    }
                }
                if (swapThreadCounter == -1) {
                    cout << "[Scheduler][MigrationIPSPrediction] Internal error: swapThreadCounter == -1" << endl;
                    exit(1);
                }
                //cout << "considering swapping " << threadCounter << " (core " << threads.at(threadCounter).targetCore << ") and " << swapThreadCounter<< " (core " << threads.at(swapThreadCounter).targetCore << "...";

                // swap cores, AMD and power budget
                swap(threads.at(threadCounter).targetCore, threads.at(swapThreadCounter).targetCore);
                swap(threads.at(threadCounter).targetAMD, threads.at(swapThreadCounter).targetAMD);
                swap(threads.at(threadCounter).targetPowerBudget, threads.at(swapThreadCounter).targetPowerBudget);
                
                float speedup = predictIPS(threads.at(threadCounter)) / baseIPS.at(threadCounter) - 1
                              + predictIPS(threads.at(swapThreadCounter)) / baseIPS.at(swapThreadCounter) - 1;

                //cout << " speedup: " << setprecision(3) << speedup << endl;
                if (speedup > bestSpeedup) {
                    m.fromCore = threads.at(threadCounter).currentCore;
                    m.toCore = targetCore;
                    m.swap = true;
                    bestSpeedup = speedup;
                }

                // re-swap cores, AMD and power budget
                swap(threads.at(threadCounter).targetCore, threads.at(swapThreadCounter).targetCore);
                swap(threads.at(threadCounter).targetAMD, threads.at(swapThreadCounter).targetAMD);
                swap(threads.at(threadCounter).targetPowerBudget, threads.at(swapThreadCounter).targetPowerBudget);
            }
        }
    }

    std::vector<migration> migrations;
    if (bestSpeedup > delta) {
        cout << "[Scheduler][MigrationIPSPrediction] expected speedup for migrating thread from core " << m.fromCore << " to " << m.toCore << " is high (" << setprecision(3) << bestSpeedup << ")" << endl;
        migrations.push_back(m);
    } else {
        cout << "[Scheduler][MigrationIPSPrediction] expected speedup for migrating thread from core " << m.fromCore << " to " << m.toCore << " is too low (" << setprecision(3) << bestSpeedup << ")" << endl;
    }
    return migrations;
}

void MigrationIPSPrediction::updateTraces() {
    float factor = 1.0f / (migrationPeriod - ignoreAfterMigration);
    //std::cout << "factor" << factor << std::endl;
    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
        ipsPerCore.at(coreCounter) += factor * performanceCounters->getIPSOfCore(coreCounter);
        relNucaPerCore.at(coreCounter) += factor * performanceCounters->getRelNUCACPIOfCore(coreCounter);
    }
}

void MigrationIPSPrediction::resetTraces() {
    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
        ipsPerCore.at(coreCounter) = 0;
        relNucaPerCore.at(coreCounter) = 0;
    }
}

std::vector<migration> MigrationIPSPrediction::migrate(SubsecondTime time, const std::vector<int> &taskIds, const std::vector<bool> &activeCores) {
    iterationCounter += 1;
    if (iterationCounter > ignoreAfterMigration) {
        updateTraces();
    }

    //std::cout << iterationCounter << "/" << migrationPeriod << std::endl;

    std::vector<migration> migrations;
    if (iterationCounter >= migrationPeriod) {
        migrations = performMigration(taskIds, activeCores);
        resetTraces();
        iterationCounter = 0;
    }
    return migrations;
}