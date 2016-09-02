#pragma once
#include "common/types.h"
#include "modules/coreinit/coreinit_enum.h"

namespace sysapp
{

enum SystemAppId : uint32_t
{
   Updater                 = 0,
   SystemSettings          = 1,
   ParentalControls        = 2,
   UserSettings            = 3,
   MiiMaker                = 4,
   AccountSettings         = 5,
   DailyLog                = 6,
   Notifications           = 7,
   HealthAndSafety         = 8,
   ElectronicManual        = 9,
   WiiUChat                = 10,
   SoftwareDataTransfer    = 11,
   Max,
};

uint64_t
SYSGetSystemApplicationTitleId(SystemAppId id);

uint64_t
SYSGetSystemApplicationTitleIdByProdArea(SystemAppId id,
                                         coreinit::SCIRegion region);

} // namespace sysapp
