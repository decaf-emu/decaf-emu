#ifndef PROC_UI_ENUM_H
#define PROC_UI_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(proc_ui)

ENUM_BEG(ProcUICallbackType, uint32_t)
   // Unknown
ENUM_END(ProcUICallbackType)

ENUM_BEG(ProcUIStatus, uint32_t)
   ENUM_VALUE(InForeground,      0)
   ENUM_VALUE(InBackground,      1)
   ENUM_VALUE(ReleaseForeground, 2)
   ENUM_VALUE(Exiting,           3)
ENUM_END(ProcUIStatus)

ENUM_NAMESPACE_END(proc_ui)

#include <common/enum_end.h>

#endif // PROC_UI_ENUM_H
