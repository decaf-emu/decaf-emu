#pragma once

namespace decaf::debug
{

void handleDebugBreakInterrupt();
void notifyEntry(uint32_t preinit, uint32_t entry);

} // namespace decaf::debug
