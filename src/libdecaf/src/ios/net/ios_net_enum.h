#ifndef IOS_NET_ENUM_H
#define IOS_NET_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(net)

ENUM_BEG(SocketCommand, uint32_t)
   ENUM_VALUE(Bind,              0x2)
   ENUM_VALUE(Close,             0x3)
   ENUM_VALUE(Connect,           0x4)
   ENUM_VALUE(Socket,            0x11)
ENUM_END(SocketCommand)

ENUM_BEG(SocketError, uint32_t)
   ENUM_VALUE(OK,                0)
   ENUM_VALUE(NoBufs,            1)
   ENUM_VALUE(TimedOut,          2)
   ENUM_VALUE(IsConn,            3)
   ENUM_VALUE(OpNotSupp,         4)
   ENUM_VALUE(ConnAborted,       5)
   ENUM_VALUE(WouldBlock,        6)
   ENUM_VALUE(ConnRefused,       7)
   ENUM_VALUE(ConnReset,         8)
   ENUM_VALUE(NotConn,           9)
   ENUM_VALUE(Already,           10)
   ENUM_VALUE(Inval,             11)
   ENUM_VALUE(MsgSize,           12)
   ENUM_VALUE(Pipe,              13)
   ENUM_VALUE(DestAddrReq,       14)
   ENUM_VALUE(Shutdown,          15)
   ENUM_VALUE(NoProtoOpt,        16)
   ENUM_VALUE(HaveOob,           17)
   ENUM_VALUE(NoMem,             18)
   ENUM_VALUE(AddrNotAvail,      19)
   ENUM_VALUE(AddrInUse,         20)
   ENUM_VALUE(AfNoSupport,       21)
   ENUM_VALUE(InProgress,        22)
   ENUM_VALUE(Lower,             23)
   ENUM_VALUE(NotSock,           24)
   ENUM_VALUE(Eieio,             27)
   ENUM_VALUE(TooManyRefs,       28)
   ENUM_VALUE(Fault,             29)
   ENUM_VALUE(NetUnreach,        30)
   ENUM_VALUE(ProtoNoSupport,    31)
   ENUM_VALUE(Prototype,         32)
   ENUM_VALUE(GenericError,      41)
   ENUM_VALUE(NoLibRm,           42)
   ENUM_VALUE(NotInitialised,    43)
   ENUM_VALUE(Busy,              44)
   ENUM_VALUE(Unknown,           45)
   ENUM_VALUE(NoResources,       48)
   ENUM_VALUE(BadFd,             49)
   ENUM_VALUE(Aborted,           50)
   ENUM_VALUE(MFile,             51)
ENUM_END(SocketError)

ENUM_BEG(SocketFamily, uint32_t)
   ENUM_VALUE(Inet,              0x2)
ENUM_END(SocketFamily)

ENUM_NAMESPACE_END(net)

ENUM_NAMESPACE_END(ios)

#include <common/enum_end.h>

#endif // ifdef IOS_NET_ENUM_H
