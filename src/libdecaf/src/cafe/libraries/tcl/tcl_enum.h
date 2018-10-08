#ifndef TCL_ENUM_H
#define TCL_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(tcl)

ENUM_BEG(TCLRegisterID, uint32_t)
   ENUM_VALUE(Max,                              0x10000)
ENUM_END(TCLRegisterID)

ENUM_BEG(TCLStatus, int32_t)
   ENUM_VALUE(OK,                               0)
   ENUM_VALUE(InvalidArg,                       3)
   ENUM_VALUE(NotInitialised,                   5)
   ENUM_VALUE(OutOfMemory,                      6)
   ENUM_VALUE(Timeout,                          22)
ENUM_END(TCLStatus)

FLAGS_BEG(TCLSubmitFlags, uint32_t)
   FLAGS_VALUE(None,                            0)
   FLAGS_VALUE(NoWriteConfirmTimestamp,         1 << 21)
   FLAGS_VALUE(NoCacheFlush,                    1 << 27)
   FLAGS_VALUE(CacheFlushInvalidate,            1 << 28)
   FLAGS_VALUE(UpdateTimestamp,                 1 << 29)
FLAGS_END(TCLSubmitFlags)

ENUM_BEG(TCLTimestampID, int32_t)
   ENUM_VALUE(CPSubmitted,                      0)
   ENUM_VALUE(CPRetired,                        1)
   ENUM_VALUE(DMAERetired,                      2)
ENUM_END(TCLTimestampID)

ENUM_NAMESPACE_END(tcl)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef TCL_ENUM_H
