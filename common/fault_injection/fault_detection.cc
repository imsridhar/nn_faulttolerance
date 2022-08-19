#include <vector>
#include "fault_injection.h"
#include "simulator.h"
#include "core_manager.h"
#include "scheduler.h"

void FaultDetector::detectFault(UInt32 core_id)
{
    printf("This core is at fault ---------------------------> %d\n\n", core_id);
    tolerateFault(core_id);
}

void FaultDetector::tolerateFault(UInt32 core_id)
{
    printf("Fault is being tolerated at core ---------------------------> %d\n\n", core_id);

    Core *core = Sim()->getCoreManager()->getCoreFromID(core_id);
    Thread *m_thread = core->getThread();

    Scheduler *scheduler = Sim()->getThreadManager()->getScheduler();

    // std::vector < int > v1;
    // std::vector < bool > v2;
    // DVFSTSP *lol = DVFSTSP::getFrequencies(v1, v2);
    // vector<double> budgets = thermalModel->powerBudgetMaxSteadyState(activeCores);

    printf("Fault has been tolerated at core ---------------------------> %d\n\n", core_id);
}

