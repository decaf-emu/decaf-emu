#ifndef SCI_ENUM_H
#define SCI_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(sci)

ENUM_BEG(SCICountry, uint32_t)
   ENUM_VALUE(Japan,                1)
   ENUM_VALUE(UnitedStates,         49)
   ENUM_VALUE(UnitedKingdom,        110)
   ENUM_VALUE(Max,                  255)
ENUM_END(SCICountry)

ENUM_BEG(SCIError, int32_t)
   ENUM_VALUE(OK,                   1)
   ENUM_VALUE(Error,                0)
   ENUM_VALUE(ReadError,            -1)
   ENUM_VALUE(WriteError,           -2)
ENUM_END(SCIError)

ENUM_BEG(SCILanguage, uint32_t)
   ENUM_VALUE(Japanese,             0)
   ENUM_VALUE(English,              1)
   ENUM_VALUE(French,               2)
   ENUM_VALUE(German,               3)
   ENUM_VALUE(Italian,              4)
   ENUM_VALUE(Spanish,              5)
   ENUM_VALUE(Chinese,              6)
   ENUM_VALUE(Korean,               7)
   ENUM_VALUE(Dutch,                8)
   ENUM_VALUE(Portugese,            9)
   ENUM_VALUE(Russian,              10)
   ENUM_VALUE(Taiwanese,            11)
   ENUM_VALUE(Max,                  12)
   ENUM_VALUE(Invalid,              255)
ENUM_END(SCILanguage)

ENUM_NAMESPACE_END(sci)

#include <common/enum_end.h>

#endif // ifdef SCI_ENUM_H
