#ifndef NN_ACP_ENUM_H
#define NN_ACP_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(nn)
ENUM_NAMESPACE_ENTER(acp)

ENUM_BEG(ACPDeviceType, int32_t)
   ENUM_VALUE(Unknown1,             1)
ENUM_END(ACPDeviceType)

ENUM_NAMESPACE_EXIT(acp)
ENUM_NAMESPACE_EXIT(nn)

#include <common/enum_end.inl>

#endif // ifdef NN_ACP_ENUM_H
