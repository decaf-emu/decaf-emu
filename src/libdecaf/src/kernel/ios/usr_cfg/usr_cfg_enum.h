#ifndef IOS_USR_CFG_ENUM_H
#define IOS_USR_CFG_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(kernel)

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(usr_cfg)

ENUM_BEG(UCCommand, uint32_t)
   ENUM_VALUE(ReadSysConfig,        0x30)
   ENUM_VALUE(WriteSysConfig,       0x31)
ENUM_END(UCCommand)

ENUM_BEG(UCDataType, uint32_t)
   ENUM_VALUE(Uint8,                0x01)
   ENUM_VALUE(Uint16,               0x02)
   ENUM_VALUE(Uint32,               0x03)
   ENUM_VALUE(String,               0x06)
   ENUM_VALUE(Blob,                 0x07)
   ENUM_VALUE(Group,                0x08)
ENUM_END(UCDataType)

ENUM_BEG(UCError, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(Error,                1)
   ENUM_VALUE(InvalidBuffer,        -0x200005)
   ENUM_VALUE(UnkError9,            -0x200009)
   ENUM_VALUE(OutOfMemory,          -0x200022)
ENUM_END(UCError)

ENUM_NAMESPACE_END(usr_cfg)

ENUM_NAMESPACE_END(ios)

ENUM_NAMESPACE_END(kernel)

#include <common/enum_end.h>

#endif // ifdef IOS_USR_CFG_ENUM_H
