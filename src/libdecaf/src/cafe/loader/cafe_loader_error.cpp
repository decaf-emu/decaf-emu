#pragma optimize("", off)
#include "cafe_loader_error.h"

#include <cstdint>
#include <string>

namespace cafe::loader::internal
{

static int32_t gFatalErr = 0;                // 0xEFE1AEB0
static uint32_t gFatalLine = 0u;             // 0xEFE1AEB4
static uint32_t gFatalMsgType = 0u;          // 0xEFE1AEB8
static std::string gFatalFunction;           // 0xEFE1AEBC

void
LiSetFatalError(int32_t baseError,
                uint32_t fileType,
                uint32_t unk,
                std::string_view function,
                uint32_t line)
{
   if (gFatalErr) {
      // Only one fatal error at a time!
      return;
   }

   gFatalErr = baseError;
   gFatalMsgType = fileType;
   gFatalFunction = function;
   gFatalLine = line;
}

int32_t
LiGetFatalError()
{
   return gFatalErr;
}

const std::string &
LiGetFatalFunction()
{
   return gFatalFunction;
}

uint32_t
LiGetFatalLine()
{
   return gFatalLine;
}

uint32_t
LiGetFatalMsgType()
{
   return gFatalMsgType;
}

void
LiResetFatalError()
{
   gFatalErr = 0;
   gFatalFunction.clear();
   gFatalLine = 0;
   gFatalMsgType = 0;
}

} // namespace cafe::loader::internal
