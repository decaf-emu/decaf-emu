#ifndef CAFE_TEMP_ENUM_H
#define CAFE_TEMP_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn_temp)

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
   ENUM_VALUE(Cancelled,            -1)
   ENUM_VALUE(End,                  -2)
   ENUM_VALUE(Max,                  -3)
   ENUM_VALUE(AlreadyOpen,          -4)
   ENUM_VALUE(Exists,               -5)
   ENUM_VALUE(NotFound,             -6)
   ENUM_VALUE(NotFile,              -7)
   ENUM_VALUE(NotDirectory,         -8)
   ENUM_VALUE(AccessError,          -9)
   ENUM_VALUE(PermissionError,      -10)
   ENUM_VALUE(FileTooBig,           -11)
   ENUM_VALUE(StorageFull,          -12)
   ENUM_VALUE(JournalFull,          -13)
   ENUM_VALUE(UnsupportedCmd,       -14)
   ENUM_VALUE(MediaNotReady,        -15)
   ENUM_VALUE(MediaError,           -17)
   ENUM_VALUE(Corrupted,            -18)
   ENUM_VALUE(FatalError,           -0x400)
   ENUM_VALUE(InvalidParam,         -0x401)
ENUM_END(TEMPStatus)

ENUM_NAMESPACE_EXIT(nn_temp)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_TEMP_ENUM_H
