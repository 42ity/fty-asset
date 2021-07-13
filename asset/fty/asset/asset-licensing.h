#pragma once
#include "error.h"

namespace fty::asset {

struct LimitationsStruct
{
    int max_active_power_devices;
    int global_configurability;
};

AssetExpected<LimitationsStruct> getLicensingLimitation();

}


