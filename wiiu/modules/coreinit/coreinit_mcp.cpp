#include "coreinit.h"
#include "coreinit_mcp.h"

IOHandle
MCP_Open()
{
   return IOInvalidHandle;
}

void
MCP_Close(IOHandle handle)
{
}

IOError
MCP_GetSysProdSettings(IOHandle handle, MCPSysProdSettings *settings)
{
   return IOError::Generic;
}

void
CoreInit::registerMcpFunctions()
{
   RegisterSystemFunction(MCP_Open);
   RegisterSystemFunction(MCP_Close);
   RegisterSystemFunction(MCP_GetSysProdSettings);
}
