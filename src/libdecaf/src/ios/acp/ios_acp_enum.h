#ifndef IOS_ACP_ENUM_H
#define IOS_ACP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_ENTER(ios)
ENUM_NAMESPACE_ENTER(acp)

ENUM_BEG(NssmCommand, uint32_t)
   ENUM_VALUE(RegisterService,                  1)
   ENUM_VALUE(UnregisterService,                2)
ENUM_END(NssmCommand)

ENUM_NAMESPACE_EXIT(acp)
ENUM_NAMESPACE_EXIT(ios)

#include <common/enum_end.h>

#endif // ifdef IOS_ACP_ENUM_H
