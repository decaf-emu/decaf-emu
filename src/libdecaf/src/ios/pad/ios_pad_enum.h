#ifndef IOS_PAD_ENUM_H
#define IOS_PAD_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(ios)
ENUM_NAMESPACE_ENTER(pad)

ENUM_BEG(BtrmCommand, uint8_t)
   ENUM_VALUE(PpcInitDone,             1)
   ENUM_VALUE(Wud,                     3)
   ENUM_VALUE(Bte,                     4)
ENUM_END(BtrmCommand)

ENUM_BEG(BtrmSubCommand, uint8_t)
   // Category 3
   ENUM_VALUE(UpdateBTDevSize,         29)
ENUM_END(BtrmSubCommand)

ENUM_NAMESPACE_EXIT(pad)
ENUM_NAMESPACE_EXIT(ios)

#include <common/enum_end.inl>

#endif // ifdef IOS_PAD_ENUM_H
