#include "coreinit.h"
#include "coreinit_clipboard.h"

#include <vector>

namespace coreinit
{

static std::vector<uint8_t>
sClipboard;

BOOL
OSCopyFromClipboard(void *buffer,
                    be_val<uint32_t> *size)
{
   if (!size) {
      return FALSE;
   }

   if (buffer && *size >= sClipboard.size()) {
      std::memcpy(buffer, sClipboard.data(), sClipboard.size());
   }

   *size = sClipboard.size();
   return TRUE;
}

BOOL
OSCopyToClipboard(const void *buffer,
                  uint32_t size)
{
   sClipboard.resize(size);
   std::memcpy(sClipboard.data(), buffer, size);
   return TRUE;
}

void
Module::registerClipboardFunctions()
{
   RegisterKernelFunction(OSCopyFromClipboard);
   RegisterKernelFunction(OSCopyToClipboard);
}

} // namespace coreinit
