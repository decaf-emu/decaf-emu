#ifndef NN_NFP_ENUM_H
#define NN_NFP_ENUM_H

#include "common/enum_start.h"

ENUM_NAMESPACE_BEG(nn)

ENUM_NAMESPACE_BEG(nfp)

ENUM_BEG(State, uint32_t)
   ENUM_VALUE(Uninitialised,  0)
   ENUM_VALUE(Initialised,    1)
   ENUM_VALUE(Detecting,      2)
ENUM_END(State)

ENUM_NAMESPACE_END(nfp)

ENUM_NAMESPACE_END(nn)

#include "common/enum_end.h"

#endif // ifdef NN_NFP_ENUM_H
