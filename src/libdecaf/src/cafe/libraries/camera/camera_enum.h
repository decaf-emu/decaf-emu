#ifndef CAFE_CAMERA_ENUM_H
#define CAFE_CAMERA_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(camera)

ENUM_BEG(CAMError, int32_t)
   ENUM_VALUE(OK,                      0)
   ENUM_VALUE(GenericError,            -1)
   ENUM_VALUE(AlreadyInitialised,      -12)
ENUM_END(CAMError)

ENUM_NAMESPACE_EXIT(camera)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_CAMERA_ENUM_H
