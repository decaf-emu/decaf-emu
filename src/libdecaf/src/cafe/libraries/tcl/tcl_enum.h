#ifndef TCL_ENUM_H
#define TCL_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(tcl)

ENUM_BEG(TCLAsicType, uint32_t)
   ENUM_VALUE(Unknown5,                         5)
ENUM_END(TCLAsicType)

ENUM_BEG(TCLChipRevision, uint32_t)
   ENUM_VALUE(Unknown78,                        78)
ENUM_END(TCLChipRevision)

ENUM_BEG(TCLCpMicrocodeVersion, uint32_t)
   ENUM_VALUE(Unknown16,                        16)
ENUM_END(TCLCpMicrocodeVersion)

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

ENUM_NAMESPACE_EXIT(tcl)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef TCL_ENUM_H
