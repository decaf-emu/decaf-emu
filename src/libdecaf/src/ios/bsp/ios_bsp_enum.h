#ifndef IOS_BSP_ENUM_H
#define IOS_BSP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(bsp)

ENUM_BEG(HardwareVersion, uint32_t)
   ENUM_VALUE(Unknown,                       0x00000000)

   // vWii Hardware Versions
   ENUM_VALUE(HOLLYWOOD_ENG_SAMPLE_1,        0x00000001)
   ENUM_VALUE(HOLLYWOOD_ENG_SAMPLE_2,        0x10000001)
   ENUM_VALUE(HOLLYWOOD_PROD_FOR_WII,        0x10100001)
   ENUM_VALUE(HOLLYWOOD_CORTADO,             0x10100008)
   ENUM_VALUE(HOLLYWOOD_CORTADO_ESPRESSO,    0x1010000C)
   ENUM_VALUE(BOLLYWOOD,                     0x20000001)
   ENUM_VALUE(BOLLYWOOD_PROD_FOR_WII,        0x20100001)

   // WiiU Hardware Versions
   ENUM_VALUE(LATTE_A11_EV,                  0x21100010)
   ENUM_VALUE(LATTE_A11_CAT,                 0x21100020)
   ENUM_VALUE(LATTE_A12_EV,                  0x21200010)
   ENUM_VALUE(LATTE_A12_CAT,                 0x21200020)
   ENUM_VALUE(LATTE_A2X_EV,                  0x22100010)
   ENUM_VALUE(LATTE_A2X_CAT,                 0x22100020)
   ENUM_VALUE(LATTE_A3X_EV,                  0x23100010)
   ENUM_VALUE(LATTE_A3X_CAT,                 0x23100020)
   ENUM_VALUE(LATTE_A3X_CAFE,                0x23100028)
   ENUM_VALUE(LATTE_A4X_EV,                  0x24100010)
   ENUM_VALUE(LATTE_A4X_CAT,                 0x24100020)
   ENUM_VALUE(LATTE_A4X_CAFE,                0x24100028)
   ENUM_VALUE(LATTE_A5X_EV,                  0x25100010)
   ENUM_VALUE(LATTE_A5X_EV_Y,                0x25100011)
   ENUM_VALUE(LATTE_A5X_CAT,                 0x25100020)
   ENUM_VALUE(LATTE_A5X_CAFE,                0x25100028)
   ENUM_VALUE(LATTE_B1X_EV,                  0x26100010)
   ENUM_VALUE(LATTE_B1X_EV_Y,                0x26100011)
   ENUM_VALUE(LATTE_B1X_CAT,                 0x26100020)
   ENUM_VALUE(LATTE_B1X_CAFE,                0x26100028)
ENUM_END(HardwareVersion)

ENUM_NAMESPACE_END(bsp)

ENUM_NAMESPACE_END(ios)

#include <common/enum_end.h>

#endif // ifdef IOS_BSP_ENUM_H
