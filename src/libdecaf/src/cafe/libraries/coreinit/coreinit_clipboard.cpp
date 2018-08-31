#include "coreinit.h"
#include "coreinit_clipboard.h"
#include "coreinit_driver.h"
#include "coreinit_memory.h"
#include "cafe/cafe_stackobject.h"

#include <vector>

namespace cafe::coreinit
{

constexpr auto MaxClipboardSize = 0x800u;

struct ClipboardSaveData
{
   be2_val<uint32_t> size;
   UNKNOWN(0xC);
   be2_array<uint8_t, MaxClipboardSize> buffer;
};

static virt_ptr<ClipboardSaveData>
sClipboardSaveData = nullptr;

BOOL
OSCopyFromClipboard(virt_ptr<void> buffer,
                    virt_ptr<uint32_t> size)
{
   if (!size) {
      return FALSE;
   }

   if (buffer && *size == 0) {
      return FALSE;
   }

   if (!OSGetForegroundBucket(nullptr, nullptr)) {
      // Can only use clipboard when application is in foreground.
      return FALSE;
   }

   if (OSDriver_CopyFromSaveArea(10,
                                 sClipboardSaveData,
                                 sizeof(ClipboardSaveData)) != OSDriver_Error::OK) {
      return false;
   }

   if (buffer && sClipboardSaveData->size) {
      std::memcpy(buffer.getRawPointer(),
                  virt_addrof(sClipboardSaveData->buffer).getRawPointer(),
                  std::min<size_t>(sClipboardSaveData->size, *size));
   }

   *size = sClipboardSaveData->size;
   return TRUE;
}

BOOL
OSCopyToClipboard(virt_ptr<const void> buffer,
                  uint32_t size)
{
   if (!OSGetForegroundBucket(nullptr, nullptr)) {
      // Can only use clipboard when application is in foreground.
      return FALSE;
   }

   if (size > MaxClipboardSize) {
      return FALSE;
   }

   if (buffer) {
      sClipboardSaveData->size = size;

      std::memcpy(virt_addrof(sClipboardSaveData->buffer).getRawPointer(),
                  buffer.getRawPointer(),
                  size);
   } else {
      if (size != 0) {
         return FALSE;
      }

      sClipboardSaveData->size = 0u;
   }

   if (OSDriver_CopyToSaveArea(10,
                               sClipboardSaveData,
                               sizeof(ClipboardSaveData)) != OSDriver_Error::OK) {
      return FALSE;
   }

   return TRUE;
}

void
Library::registerClipboardSymbols()
{
   RegisterFunctionExport(OSCopyFromClipboard);
   RegisterFunctionExport(OSCopyToClipboard);

   RegisterDataInternal(sClipboardSaveData);
}

} // namespace cafe::coreinit
