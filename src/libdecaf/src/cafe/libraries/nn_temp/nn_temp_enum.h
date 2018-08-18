#ifndef CAFE_TEMP_ENUM_H
#define CAFE_TEMP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(nn)
ENUM_NAMESPACE_BEG(temp)

ENUM_BEG(TEMPDevicePreference, uint32_t)
   //! Largest free space between USB and MLC
   ENUM_VALUE(LargestFreeSpace,     0)

   //! Prefer USB instead of MLC
   ENUM_VALUE(USB,                  1)
ENUM_END(TEMPDevicePreference)

ENUM_BEG(TEMPDeviceType, uint32_t)
   ENUM_VALUE(Invalid,              0)
   ENUM_VALUE(MLC,                  1)
   ENUM_VALUE(USB,                  2)
ENUM_END(TEMPDeviceType)

ENUM_BEG(TEMPStatus, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(FatalError,           -0x400)
   ENUM_VALUE(InvalidParam,         -0x401)
ENUM_END(TEMPStatus)

ENUM_NAMESPACE_END(temp)
ENUM_NAMESPACE_END(nn)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_TEMP_ENUM_H
