#include "cafe_ppc_interface_invoke_trace.h"

#include <common/log.h>
#include <string_view>

namespace cafe::detail
{

// We moved this function out to its own file to avoid including spdlog
// throughout the entire codebase even if its not directly used.
void
logTraceMessage(const fmt::memory_buffer &message)
{
   gLog->debug(std::string_view { message.data(), message.size() });
}

} // namespace cafe::detail
