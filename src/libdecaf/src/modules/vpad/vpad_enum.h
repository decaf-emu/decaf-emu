#ifndef VPAD_ENUM_H
#define VPAD_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(vpad)

ENUM_BEG(Buttons, uint32_t)
   ENUM_VALUE(Sync,        1 << 0)
   ENUM_VALUE(Home,        1 << 1)
   ENUM_VALUE(Minus,       1 << 2)
   ENUM_VALUE(Plus,        1 << 3)
   ENUM_VALUE(R,           1 << 4)
   ENUM_VALUE(L,           1 << 5)
   ENUM_VALUE(ZR,          1 << 6)
   ENUM_VALUE(ZL,          1 << 7)
   ENUM_VALUE(Down,        1 << 8)
   ENUM_VALUE(Up,          1 << 9)
   ENUM_VALUE(Right,       1 << 10)
   ENUM_VALUE(Left,        1 << 11)
   ENUM_VALUE(Y,           1 << 12)
   ENUM_VALUE(X,           1 << 13)
   ENUM_VALUE(B,           1 << 14)
   ENUM_VALUE(A,           1 << 15)
   ENUM_VALUE(StickR,      1 << 17)
   ENUM_VALUE(StickL,      1 << 18)
ENUM_END(Buttons)

ENUM_BEG(TouchPadValidity, uint16_t)
   //! Both X and Y touchpad positions are accurate
   ENUM_VALUE(Valid,       0)

   //! X position is inaccurate
   ENUM_VALUE(InvalidX,    1 << 0)

   //! Y position is inaccurate
   ENUM_VALUE(InvalidY,    1 << 1)
ENUM_END(TouchPadValidity)

ENUM_BEG(VPADReadError, int32_t)
   ENUM_VALUE(Success,            0)
   ENUM_VALUE(NoSamples,         -1)
   ENUM_VALUE(InvalidController, -2)
ENUM_END(VPADReadError)

ENUM_NAMESPACE_END(vpad)

#include <common/enum_end.h>

#endif // ifdef VPAD_ENUM_H
