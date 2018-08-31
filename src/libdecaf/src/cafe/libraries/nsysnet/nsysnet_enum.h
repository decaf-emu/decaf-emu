#ifndef NSYSNET_ENUM_H
#define NSYSNET_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(cafe)
ENUM_NAMESPACE_BEG(nsysnet)

FLAGS_BEG(Error, int32_t)
   FLAGS_VALUE(OK,            0)
FLAGS_END(Error)

ENUM_NAMESPACE_END(nsysnet)
ENUM_NAMESPACE_END(cafe)

#include <common/enum_end.h>

#endif // ifdef NSYSNET_ENUM_H
