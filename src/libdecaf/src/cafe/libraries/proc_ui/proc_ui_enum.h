#ifndef CAFE_PROC_UI_ENUM_H
#define CAFE_PROC_UI_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(proc_ui)

ENUM_BEG(ProcUICallbackType, uint32_t)
   ENUM_VALUE(Acquire,           0)
   ENUM_VALUE(Release,           1)
   ENUM_VALUE(Exit,              2)
   ENUM_VALUE(NetIoStart,        3)
   ENUM_VALUE(NetIoStop,         4)
   ENUM_VALUE(HomeButtonDenied,  5)
   ENUM_VALUE(Max,               6)
ENUM_END(ProcUICallbackType)

ENUM_BEG(ProcUIStatus, uint32_t)
   ENUM_VALUE(InForeground,      0)
   ENUM_VALUE(InBackground,      1)
   ENUM_VALUE(ReleaseForeground, 2)
   ENUM_VALUE(Exiting,           3)
ENUM_END(ProcUIStatus)

ENUM_NAMESPACE_END(proc_ui)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // CAFE_PROC_UI_ENUM_H
