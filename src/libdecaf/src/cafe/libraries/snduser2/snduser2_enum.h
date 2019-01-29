#ifndef CAFE_SNDUSER2_ENUM_H
#define CAFE_SNDUSER2_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(snduser2)

ENUM_BEG(AXFXSampleRate, uint32_t)
   ENUM_VALUE(Rate32khz,         1)
   ENUM_VALUE(Rate48khz,         2)
ENUM_END(AXFXSampleRate)

ENUM_BEG(AXFXReverbPreset, uint32_t)
   ENUM_VALUE(Unknown1,          1)
   ENUM_VALUE(Unknown2,          2)
   ENUM_VALUE(Unknown3,          3)
   ENUM_VALUE(Unknown4,          4)
   ENUM_VALUE(Unknown5,          5)
ENUM_END(AXFXReverbPreset)

ENUM_BEG(AXFXReverbType, uint32_t)
   ENUM_VALUE(Unknown1,          1)
   ENUM_VALUE(Unknown2,          2)
   ENUM_VALUE(Unknown3,          3)
   ENUM_VALUE(Unknown4,          4)
ENUM_END(AXFXReverbType)

ENUM_NAMESPACE_EXIT(snduser2)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef CAFE_SNDUSER2_ENUM_H
