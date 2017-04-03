#pragma once
#include <cstdint>
#include <vector>
#include <memory>

namespace cpu
{

struct Breakpoint
{
   enum Type : uint32_t
   {
      SingleFire,
      MultiFire,
   };

   //! Breakpoint type.
   Type type;

   //! Address of breakpoint.
   uint32_t address;

   //! Code at address before we inserted a TW instruction.
   uint32_t savedCode;
};

using BreakpointList = std::vector<Breakpoint>;

void
addBreakpoint(uint32_t address,
              Breakpoint::Type type);

void
removeBreakpoint(uint32_t address);

bool
testBreakpoint(uint32_t address);

bool
hasBreakpoints();

bool
hasBreakpoint(uint32_t address);

std::shared_ptr<BreakpointList>
getBreakpoints();

uint32_t
getBreakpointSavedCode(uint32_t address);

} // namespace cpu
