#ifndef CAFE_ERREULA_ENUM_H
#define CAFE_ERREULA_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn_erreula)

ENUM_BEG(ControllerType, uint32_t)
   ENUM_VALUE(WiiRemote0,        0)
   ENUM_VALUE(WiiRemote1,        1)
   ENUM_VALUE(WiiRemote2,        2)
   ENUM_VALUE(WiiRemote3,        3)
   ENUM_VALUE(DrcGamepad,        4)
ENUM_END(ControllerType)

ENUM_BEG(ErrorType, uint32_t)
   ENUM_VALUE(Code,              0)
   ENUM_VALUE(Message,           1)
   ENUM_VALUE(Message1Button,    2)
   ENUM_VALUE(Message2Button,    3)
ENUM_END(ErrorType)

ENUM_BEG(ErrorViewerState, uint32_t)
   ENUM_VALUE(Hidden,            0)
   ENUM_VALUE(FadeIn,            1)
   ENUM_VALUE(Visible,           2)
   ENUM_VALUE(FadeOut,           3)
ENUM_END(ErrorViewerState)

ENUM_BEG(LangType, uint32_t)
   ENUM_VALUE(Japanese,          0)
   ENUM_VALUE(English,           1)
   // TODO: More values..
ENUM_END(LangType)

ENUM_BEG(RegionType, uint32_t)
   ENUM_VALUE(Japan,             0)
   ENUM_VALUE(USA,               1)
   ENUM_VALUE(Europe,            2)
   ENUM_VALUE(China,             3)
   ENUM_VALUE(Korea,             4)
   ENUM_VALUE(Taiwan,            5)
ENUM_END(RegionType)

ENUM_BEG(RenderTarget, uint32_t)
   ENUM_VALUE(Tv,                0)
   ENUM_VALUE(Drc,               1)
   ENUM_VALUE(Both,              2)
ENUM_END(RenderTarget)

ENUM_BEG(ResultType, uint32_t)
   ENUM_VALUE(None,              0)
   ENUM_VALUE(Exited,            1)
   // TODO: More values..
ENUM_END(ResultType)

ENUM_NAMESPACE_EXIT(nn_erreula)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_ERREULA_ENUM_H
