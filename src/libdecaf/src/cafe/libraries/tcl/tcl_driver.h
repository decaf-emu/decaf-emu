#pragma once
#include "tcl_enum.h"

namespace cafe::tcl
{

struct TCLInfo
{
   be2_val<TCLAsicType> asicType;
   be2_val<TCLChipRevision> chipRevision;
   be2_val<TCLCpMicrocodeVersion> cpMicrocodeVersion;
   be2_val<uint32_t> quadPipes;
   be2_val<uint32_t> parameterCacheWidth;
   be2_val<uint32_t> rb;
   be2_virt_ptr<void> addrLibHandle;
   be2_val<uint32_t> sclk;
};

TCLStatus
TCLGetInfo(virt_ptr<TCLInfo> info);

void
TCLSetHangWait(BOOL hangWait);

namespace internal
{

void
initialiseTclDriver();

bool
tclDriverInitialised();

} // namespace internal

} // namespace cafe::tcl
