#ifndef GHS_ENUM_H
#define GHS_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(ghs)

FLAGS_BEG(DestructorFlags, uint32_t)
   FLAGS_VALUE(None,                0)
   FLAGS_VALUE(FreeMemory,          0x40)
FLAGS_END(DestructorFlags)

ENUM_NAMESPACE_EXIT(ghs)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef GHS_ENUM_H
