#ifndef NSYSNET_ENUM_H
#define NSYSNET_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(nsysnet)

FLAGS_BEG(GetHostError, int32_t)
   FLAGS_VALUE(OK,            0)
   FLAGS_VALUE(HostNotFound,  1)
   FLAGS_VALUE(TryAgain,      2)
   FLAGS_VALUE(NoRecovery,    3)
   FLAGS_VALUE(NoData,        4)
FLAGS_END(GetHostError)

FLAGS_BEG(Error, int32_t)
   FLAGS_VALUE(OK,            0)
FLAGS_END(Error)

ENUM_NAMESPACE_EXIT(nsysnet)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.inl>

#endif // ifdef NSYSNET_ENUM_H
