#ifndef CAFE_NN_AC_ENUM_H
#define CAFE_NN_AC_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(nn)
ENUM_NAMESPACE_BEG(ac)

ENUM_BEG(Status, int32_t)
   ENUM_VALUE(OK,             0)
   ENUM_VALUE(Error,          -1)
ENUM_END(Status)

ENUM_NAMESPACE_END(ac)
ENUM_NAMESPACE_END(nn)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_NN_AC_ENUM_H
