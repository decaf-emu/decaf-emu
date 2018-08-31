#ifndef TCL_ENUM_H
#define TCL_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(tcl)

ENUM_BEG(TCLRegisterID, uint32_t)
   ENUM_VALUE(Max,               0x10000)
ENUM_END(TCLRegisterID)

ENUM_BEG(TCLStatus, int32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(InvalidArg,        3)
   ENUM_VALUE(NotInitialised,    5)
ENUM_END(TCLStatus)

ENUM_BEG(TCLTimestampID, int32_t)
   ENUM_VALUE(Submitted,         0)
   ENUM_VALUE(GX2Retired,        1)
   ENUM_VALUE(DMAERetired,       2)
ENUM_END(TCLTimestampID)

ENUM_NAMESPACE_END(tcl)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef TCL_ENUM_H
