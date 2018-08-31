#include "nsysnet.h"

namespace cafe::nsysnet
{

/*
 * These are all no-op because the Wii U's host byte order is net order.
 */

static uint16_t
nsysnet_htons(uint16_t value)
{
   return value;
}

static uint32_t
nsysnet_htonl(uint32_t value)
{
   return value;
}

static uint16_t
nsysnet_ntohs(uint16_t value)
{
   return value;
}

static uint32_t
nsysnet_ntohl(uint32_t value)
{
   return value;
}

void
Library::registerEndianSymbols()
{
   RegisterFunctionExportName("htons", nsysnet_htons);
   RegisterFunctionExportName("htonl", nsysnet_htonl);
   RegisterFunctionExportName("ntohs", nsysnet_ntohs);
   RegisterFunctionExportName("ntohl", nsysnet_ntohl);
}

} // namespace cafe::nsysnet
