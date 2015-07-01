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
   RegisterKernelFunction(MCP_Open);
   RegisterKernelFunction(MCP_Close);
   RegisterKernelFunction(MCP_GetSysProdSettings);
}
