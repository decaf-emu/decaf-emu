#pragma once
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::coreinit
{

struct OSFatalError
{
   be2_val<uint32_t> messageType;
   be2_val<uint32_t> errorCode;
   be2_val<uint32_t> processId;
   be2_val<uint32_t> internalErrorCode;
   be2_val<uint32_t> line;
   be2_array<char, 64> functionName;
   UNKNOWN(0xD4 - 0x54);
};
CHECK_OFFSET(OSFatalError, 0x00, messageType);
CHECK_OFFSET(OSFatalError, 0x04, errorCode);
CHECK_OFFSET(OSFatalError, 0x08, processId);
CHECK_OFFSET(OSFatalError, 0x0C, internalErrorCode);
CHECK_OFFSET(OSFatalError, 0x10, line);
CHECK_OFFSET(OSFatalError, 0x14, functionName);
CHECK_SIZE(OSFatalError, 0xD4);

void
OSSendFatalError(virt_ptr<OSFatalError> error,
                 virt_ptr<const char> functionName,
                 uint32_t line);

namespace internal
{

void
OSPanic(std::string_view file,
        unsigned line,
        std::string_view msg);

} // namespace internal

} // namespace cafe::coreinit
