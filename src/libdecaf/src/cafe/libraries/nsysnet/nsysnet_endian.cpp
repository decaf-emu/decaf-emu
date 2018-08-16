#include "nsysnet.h"

namespace cafe::nsysnet
{

/*
 * These are all no-op because the Wii U's host byte order is net order.
 */

uint16_t
htons(uint16_t value)
{
   return value;
}

uint32_t
htonl(uint32_t value)
{
   return value;
}

uint16_t
ntohs(uint16_t value)
{
   return value;
}

uint32_t
ntohl(uint32_t value)
{
   return value;
}

void
Library::registerEndianSymbols()
{
   RegisterFunctionExport(htons);
   RegisterFunctionExport(htonl);
   RegisterFunctionExport(ntohs);
   RegisterFunctionExport(ntohl);
}

} // namespace cafe::nsysnet
