#pragma once
#include "systemtypes.h"

namespace nn
{

namespace ac
{

enum class Result : uint32_t
{
   OK = 0,
   Error = 0x80000000,
   LibraryNotInitialized = 0xC0D0CC00,
};

enum class Status : int32_t
{
   Error = -1,
   OK = 0
};

Result
Initialize();

void
Finalize();

Result
Connect();

Result
IsApplicationConnected(bool *connected);

Result
GetConnectStatus(Status *status);

Result
GetLastErrorCode(uint32_t *error);

}  // namespace act

}  // namespace nn
