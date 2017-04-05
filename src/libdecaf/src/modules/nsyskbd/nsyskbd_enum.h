#ifndef NSYSKBD_ENUM_H
#define NSYSKBD_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(nsyskbd)

ENUM_BEG(KPRMode, uint32_t)
   ENUM_VALUE(AltCode,           1 << 0)
ENUM_END(KPRMode)

ENUM_BEG(SKBDChannelStatus, uint32_t)
   ENUM_VALUE(Connected,         0)
   ENUM_VALUE(Disconnected,      1)
ENUM_END(SKBDChannelStatus)

ENUM_BEG(SKBDCountry, uint32_t)
   ENUM_VALUE(Max,               0x13)
ENUM_END(SKBDCountry)

ENUM_BEG(SKBDError, uint32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(InvalidCountry,    4)
ENUM_END(SKBDError)

ENUM_BEG(SKBDModState, uint32_t)
   ENUM_VALUE(NoModifiers,       0)
ENUM_END(SKBDModState)

ENUM_NAMESPACE_END(nsyskbd)

#include <common/enum_end.h>

#endif // ifdef NSYSKBD_ENUM_H
