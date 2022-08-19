#include "fixed_types.h"
#include "core.h"

class FaultDetector
{
   public:
      void detectFault(UInt32 core_id);
      void tolerateFault(UInt32 core_id);
};
