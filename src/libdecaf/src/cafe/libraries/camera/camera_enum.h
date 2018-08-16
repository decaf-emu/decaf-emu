#ifndef CAFE_CAMERA_ENUM_H
#define CAFE_CAMERA_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(camera)

ENUM_BEG(CAMError, uint32_t)
   ENUM_VALUE(OK,                      0)
   ENUM_VALUE(GenericError,            -1)
   ENUM_VALUE(AlreadyInitialised,      -12)
ENUM_END(CAMError)

ENUM_NAMESPACE_END(camera)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_CAMERA_ENUM_H
