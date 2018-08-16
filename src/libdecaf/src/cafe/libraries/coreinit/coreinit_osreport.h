#pragma once
#include <string_view>

namespace cafe::coreinit
{

namespace internal
{

void
OSPanic(std::string_view file,
        unsigned line,
        std::string_view msg);

} // namespace internal

} // namespace cafe::coreinit
