#ifndef IOS_IM_ENUM_H
#define IOS_IM_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(kernel)

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(im)

ENUM_BEG(IMCommand, uint32_t)
   ENUM_VALUE(SetNvParameter,       1)
   ENUM_VALUE(SetParameter,         2)
   ENUM_VALUE(GetParameter,         3)
   ENUM_VALUE(GetHomeButtonParams,  7)
   ENUM_VALUE(GetTimerRemaining,    8)
   ENUM_VALUE(GetNvParameter,       9)
ENUM_END(IMCommand)

ENUM_BEG(IMError, uint32_t)
   ENUM_VALUE(OK,                   0)
ENUM_END(IMError)

ENUM_BEG(IMTimer, uint32_t)
   ENUM_VALUE(Dim,                  0)
   ENUM_VALUE(APD,                  1)
ENUM_END(IMTimer)

ENUM_BEG(IMParameter, uint32_t)
   ENUM_VALUE(DimEnabled,           1)
   ENUM_VALUE(DimPeriod,            2)
   ENUM_VALUE(APDEnabled,           3)
   ENUM_VALUE(APDPeriod,            4)
   ENUM_VALUE(Unknown5,             5)
   ENUM_VALUE(DimEnableTv,          9)
   ENUM_VALUE(DimEnableDrc,         10)
ENUM_END(IMParameter)

ENUM_NAMESPACE_END(im)

ENUM_NAMESPACE_END(ios)

ENUM_NAMESPACE_END(kernel)

#include <common/enum_end.h>

#endif // ifdef IOS_IM_ENUM_H
