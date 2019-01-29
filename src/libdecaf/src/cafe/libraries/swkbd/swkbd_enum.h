#ifndef SWKBD_ENUM_H
#define SWKBD_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(swkbd)

ENUM_BEG(ControllerType, int32_t)
   ENUM_VALUE(Unknown0,                0)
ENUM_END(ControllerType)

ENUM_BEG(LanguageType, int32_t)
   ENUM_VALUE(Japanese,                0)
   ENUM_VALUE(English,                 1)
ENUM_END(LanguageType)

ENUM_BEG(RegionType, int32_t)
   ENUM_VALUE(Japan,                   0)
   ENUM_VALUE(USA,                     1)
   ENUM_VALUE(Europe,                  2)
ENUM_END(RegionType)

ENUM_BEG(State, int32_t)
   ENUM_VALUE(Hidden,                  0)
   ENUM_VALUE(FadeIn,                  1)
   ENUM_VALUE(Visible,                 2)
   ENUM_VALUE(FadeOut,                 3)
   ENUM_VALUE(Max,                     4)
ENUM_END(State)

ENUM_NAMESPACE_EXIT(swkbd)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef SWKBD_ENUM_H
