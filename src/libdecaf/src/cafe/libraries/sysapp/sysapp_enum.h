#ifndef SYSAPP_ENUM_H
#define SYSAPP_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(sysapp)

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

ENUM_NAMESPACE_EXIT(sysapp)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef SYSAPP_ENUM_H
