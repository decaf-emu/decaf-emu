#ifndef CAFE_ERREULA_ENUM_H
#define CAFE_ERREULA_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(nn)
ENUM_NAMESPACE_BEG(erreula)

ENUM_BEG(ControllerType, uint32_t)
ENUM_END(ControllerType)

ENUM_BEG(ErrorViewerState, uint32_t)
   ENUM_VALUE(None,              0)
ENUM_END(ErrorViewerState)

ENUM_BEG(LangType, uint32_t)
ENUM_END(LangType)

ENUM_BEG(RegionType, uint32_t)
ENUM_END(RegionType)

ENUM_BEG(ResultType, uint32_t)
ENUM_END(ResultType)

ENUM_NAMESPACE_END(erreula)
ENUM_NAMESPACE_END(nn)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef CAFE_ERREULA_ENUM_H
