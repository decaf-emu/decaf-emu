#pragma once
#include <cstdint>

namespace cafe::sysapp
{

uint32_t
SYSGetCallerPFID();

uint64_t
SYSGetCallerTitleId();

uint32_t
SYSGetCallerUPID();

int32_t
SYSGetLauncherArgs(virt_ptr<void> args);

int32_t
SYSGetStandardResult(virt_ptr<uint32_t> arg1,
                     uint32_t arg2,
                     uint32_t arg3);

} // namespace cafe::sysapp
