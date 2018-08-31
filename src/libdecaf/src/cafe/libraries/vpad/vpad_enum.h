#ifndef VPAD_ENUM_H
#define VPAD_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(vpad)

FLAGS_BEG(VPADButtons, uint32_t)
   FLAGS_VALUE(Sync,        1 << 0)
   FLAGS_VALUE(Home,        1 << 1)
   FLAGS_VALUE(Minus,       1 << 2)
   FLAGS_VALUE(Plus,        1 << 3)
   FLAGS_VALUE(R,           1 << 4)
   FLAGS_VALUE(L,           1 << 5)
   FLAGS_VALUE(ZR,          1 << 6)
   FLAGS_VALUE(ZL,          1 << 7)
   FLAGS_VALUE(Down,        1 << 8)
   FLAGS_VALUE(Up,          1 << 9)
   FLAGS_VALUE(Right,       1 << 10)
   FLAGS_VALUE(Left,        1 << 11)
   FLAGS_VALUE(Y,           1 << 12)
   FLAGS_VALUE(X,           1 << 13)
   FLAGS_VALUE(B,           1 << 14)
   FLAGS_VALUE(A,           1 << 15)
   FLAGS_VALUE(StickR,      1 << 17)
   FLAGS_VALUE(StickL,      1 << 18)
FLAGS_END(VPADButtons)

ENUM_BEG(VPADChan, int32_t)
   ENUM_VALUE(Chan0,                   0)
   ENUM_VALUE(Chan1,                   1)
ENUM_END(VPADChan)

FLAGS_BEG(VPADTouchPadValidity, uint16_t)
   //! Both X and Y touchpad positions are accurate
   FLAGS_VALUE(Valid,       0)

   //! X position is inaccurate
   FLAGS_VALUE(InvalidX,    1 << 0)

   //! Y position is inaccurate
   FLAGS_VALUE(InvalidY,    1 << 1)
FLAGS_END(VPADTouchPadValidity)

ENUM_BEG(VPADReadError, int32_t)
   ENUM_VALUE(Success,            0)
   ENUM_VALUE(NoSamples,         -1)
   ENUM_VALUE(InvalidController, -2)
ENUM_END(VPADReadError)

ENUM_NAMESPACE_END(vpad)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef VPAD_ENUM_H
