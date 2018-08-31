#ifndef SYSAPP_ENUM_H
#define SYSAPP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(sysapp)

ENUM_BEG(SystemAppId, int32_t)
   ENUM_VALUE(Updater,                 0)
   ENUM_VALUE(SystemSettings,          1)
   ENUM_VALUE(ParentalControls,        2)
   ENUM_VALUE(UserSettings,            3)
   ENUM_VALUE(MiiMaker,                4)
   ENUM_VALUE(AccountSettings,         5)
   ENUM_VALUE(DailyLog,                6)
   ENUM_VALUE(Notifications,           7)
   ENUM_VALUE(HealthAndSafety,         8)
   ENUM_VALUE(ElectronicManual,        9)
   ENUM_VALUE(WiiUChat,                10)
   ENUM_VALUE(SoftwareDataTransfer,    11)
   ENUM_VALUE(Max,                     12)
ENUM_END(SystemAppId)

ENUM_NAMESPACE_END(sysapp)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef SYSAPP_ENUM_H
