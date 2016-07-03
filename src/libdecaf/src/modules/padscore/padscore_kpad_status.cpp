#include "padscore.h"
#include "padscore_kpad_status.h"

namespace padscore
{

/**
 * \return Returns number of KPADStatus buffers filled or negative is an error code
 */
int32_t
KPADRead(uint32_t chan,
         KPADStatus *buffers,
         uint32_t count)
{
   be_val<int32_t> error = 0;
   auto result = KPADReadEx(chan, buffers, count, &error);

   if (error != 0) {
      return error;
   } else {
      return result;
   }
}

/**
 * \return Returns number of KPADStatus buffers filled
 */
int32_t
KPADReadEx(uint32_t chan, KPADStatus *buffers, uint32_t count, be_val<int32_t> *error)
{
   if (count == 0) {
      return 0;
   }

   // Clear error
   if (error) {
      *error = 0;
   }

   std::memset(&buffers[0], 0, sizeof(KPADStatus));
   return 1;
}

void
Module::registerKPADStatusFunctions()
{
   RegisterKernelFunction(KPADRead);
   RegisterKernelFunction(KPADReadEx);
}

} // namespace padscore
