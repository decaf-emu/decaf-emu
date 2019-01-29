#ifndef CAFE_NN_AC_ENUM_H
#define CAFE_NN_AC_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn_ac)

ENUM_BEG(Status, int32_t)
   ENUM_VALUE(OK,             0)
   ENUM_VALUE(Error,          -1)
ENUM_END(Status)

ENUM_NAMESPACE_EXIT(nn_ac)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_NN_AC_ENUM_H
