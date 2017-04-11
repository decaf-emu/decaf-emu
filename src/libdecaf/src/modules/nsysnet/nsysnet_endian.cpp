#include "nsysnet.h"
#include "nsysnet_endian.h"

namespace nsysnet
{

/*
 * These are all no-op because the Wii U's host byte order is net order.
 */

uint16_t
htons_(uint16_t value)
{
   return value;
}

uint32_t
htonl_(uint32_t value)
{
   return value;
}

uint16_t
ntohs_(uint16_t value)
{
   return value;
}

uint32_t
ntohl_(uint32_t value)
{
   return value;
}

void
Module::registerEndianFunctions()
{
   RegisterKernelFunctionName("htons", htons_);
   RegisterKernelFunctionName("htonl", htonl_);
   RegisterKernelFunctionName("ntohs", ntohs_);
   RegisterKernelFunctionName("ntohl", ntohl_);
}

} // namespace nsysnet
