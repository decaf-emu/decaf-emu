#pragma once
#include <cstdint>
#include <string_view>

namespace cafe::loader
{

enum Error : int32_t
{
   DifferentProcess                 = -470008,
   CheckFileBoundsFailed            = -470026,
   ZlibMemError                     = -470084,
   ZlibVersionError                 = -470085,
   ZlibStreamError                  = -470086,
   ZlibDataError                    = -470087,
   ZlibBufError                     = -470088,
   ZlibUnknownError                 = -470100,
};

namespace internal
{

void
LiSetFatalError(int32_t baseError,
                uint32_t fileType,
                uint32_t unk,
                std::string_view function,
                uint32_t line);

int32_t
LiGetFatalError();

const std::string &
LiGetFatalFunction();

uint32_t
LiGetFatalLine();

uint32_t
LiGetFatalMsgType();

void
LiResetFatalError();

} // namespace internal

} // namespace cafe::loader
