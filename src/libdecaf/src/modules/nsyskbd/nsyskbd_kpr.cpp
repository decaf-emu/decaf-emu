#include "nsyskbd.h"
#include "nsyskbd_kpr.h"

namespace nsyskbd
{

void
KPRInitQueue(KPRQueue *queue)
{
   KPRSetMode(queue, KPRMode::AltCode);
}

KPRMode
KPRGetMode(KPRQueue *queue)
{
   return queue->mode;
}

void
KPRSetMode(KPRQueue *queue,
           KPRMode mode)
{
   queue->mode = mode;
   KPRClearQueue(queue);
}

void
KPRClearQueue(KPRQueue *queue)
{
   queue->numCharsOut = 0;
   queue->numCharsIn = 0;
   queue->unk0x14 = 0;
}

kpr_char_t
KPRGetChar(KPRQueue *queue)
{
   auto result = kpr_char_t { 0 };

   // TODO: Once we implement processing of input -> ouput, use numCharsOut
   if (queue->numCharsIn > 0) { // numCharsOut > 0
      result = queue->buffer[0];
      queue->numCharsIn -= 1;
   }

   return result;
}

uint8_t
KPRPutChar(KPRQueue *queue,
           kpr_char_t chr)
{
   decaf_check(queue->numCharsOut + queue->numCharsIn < 5);

   queue->buffer[queue->numCharsOut + queue->numCharsIn] = chr;
   queue->numCharsIn += 1;

   // TODO: Process characters from out -> in
   return queue->numCharsIn; // return queue->numCharsOut;
}

kpr_char_t
KPRRemoveChar(KPRQueue *queue)
{
   if (queue->numCharsIn == 0) {
      return 0;
   }

   auto result = queue->buffer[queue->numCharsOut + queue->numCharsIn - 1];
   queue->numCharsIn -= 1;

   return result;
}

uint8_t
KPRLookAhead(KPRQueue *queue,
             be_val<kpr_char_t> *buffer,
             uint32_t size)
{
   if (!buffer || !size) {
      return 0;
   }

   auto length = static_cast<uint8_t>(queue->numCharsOut + queue->numCharsIn);

   for (auto i = 0u; i < length && i < size; ++i) {
      buffer[i] = queue->buffer[i];
   }

   if (length < size) {
      buffer[length] = 0;
   }

   return length;
}

void
Module::registerKprFunctions()
{
   RegisterKernelFunction(KPRInitQueue);
   RegisterKernelFunction(KPRSetMode);
   RegisterKernelFunction(KPRGetMode);
   RegisterKernelFunction(KPRClearQueue);
   RegisterKernelFunction(KPRPutChar);
   RegisterKernelFunction(KPRGetChar);
   RegisterKernelFunction(KPRRemoveChar);
   RegisterKernelFunction(KPRLookAhead);
}

} // namespace nsyskbd
