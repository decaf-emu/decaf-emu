#pragma once
#include "fsa_enum.h"
#include "fsa_request.h"
#include "fsa_response.h"

namespace ios
{

namespace dev
{

namespace fsa
{

FSAStatus
openFile(const char *path,
         const char *mode,
         be_val<FSAFileHandle> *fileHandle)
{
   FSARequestOpenFile *request;
   FSAResponseOpenFile *response;
   /*
CHECK_OFFSET(IPCBuffer, 0x00, command);
CHECK_OFFSET(IPCBuffer, 0x04, reply);
CHECK_OFFSET(IPCBuffer, 0x08, handle);
CHECK_OFFSET(IPCBuffer, 0x0C, flags);
CHECK_OFFSET(IPCBuffer, 0x10, cpuId);
CHECK_OFFSET(IPCBuffer, 0x14, processId);
CHECK_OFFSET(IPCBuffer, 0x18, titleId);
CHECK_OFFSET(IPCBuffer, 0x24, args);
CHECK_OFFSET(IPCBuffer, 0x38, prevCommand);
CHECK_OFFSET(IPCBuffer, 0x3C, prevHandle);
CHECK_OFFSET(IPCBuffer, 0x40, buffer1);
CHECK_OFFSET(IPCBuffer, 0x44, buffer2);
CHECK_OFFSET(IPCBuffer, 0x48, nameBuffer);
*/
}

} // fsa

} // dev

} // ios
