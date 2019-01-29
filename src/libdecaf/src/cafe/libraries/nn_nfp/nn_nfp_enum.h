#ifndef NN_NFP_ENUM_H
#define NN_NFP_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nn_nfp)

ENUM_BEG(State, uint32_t)
   ENUM_VALUE(Uninitialised,  0)
   ENUM_VALUE(Initialised,    1)
   ENUM_VALUE(Detecting,      2)
ENUM_END(State)

ENUM_NAMESPACE_EXIT(nn_nfp)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef NN_NFP_ENUM_H
