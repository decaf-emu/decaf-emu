#include "debugger_analysis.h"
#include "debugger_branchcalc.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include "libcpu/mem.h"
#include "kernel/kernel_loader.h"
#include <map>
#include <spdlog/formatter.h>
#include <unordered_map>

namespace debugger
{

namespace analysis
{

static std::map<uint32_t, FuncData>
sFuncData;

static std::unordered_map<uint32_t, InstrData>
sInstrData;

FuncData *
getFunction(uint32_t address)
{
   auto funcIter = sFuncData.find(address);
   if (funcIter != sFuncData.end()) {
      return &funcIter->second;
   }
   return nullptr;
}

InstrInfo
get(uint32_t address)
{
   InstrInfo info = { 0 };

   auto instrIter = sInstrData.find(address);
   if (instrIter != sInstrData.end()) {
      info.instr = &instrIter->second;
   }

   if (sFuncData.size() > 0) {
      auto funcIter = sFuncData.upper_bound(address);
      if (funcIter != sFuncData.begin()) {
         funcIter--;
         if (funcIter != sFuncData.end()) {
            auto &func = funcIter->second;
            if (address >= func.start && address < func.end) {
               // The function needs to have an end, or be the first two instructions
               //  since we apply some special display logic to the first two instructions
               //  in a never-ending function...
               if (func.end != 0xFFFFFFFF || (address == func.start || address == func.start + 4)) {
                  info.func = &func;
               }
            }
         }
      }
   }

   return info;
}

uint32_t findFunctionEnd(uint32_t start)
{
   static const uint32_t MaxScannedBytes = 0x400;
   uint32_t fnStart = start;
   uint32_t fnMax = start;
   uint32_t fnEnd = 0xFFFFFFFF;
   for (uint32_t addr = start; addr < start + MaxScannedBytes; addr += 4) {
      auto instr = mem::read<espresso::Instruction>(addr);
      auto data = espresso::decodeInstruction(instr);
      if (!data) {
         // If we can't decode this instruction, then we gone done fucked up
         break;
      }

      if (addr > fnMax) {
         fnMax = addr;
      }

      if (isBranchInstr(data)) {
         auto meta = getBranchMeta(addr, instr, data, nullptr);
         // Ignore call instructions
         if (!meta.isCall) {
            if (meta.isVariable) {
               // We hit a variable non-call instruction, we can't scan
               //  any further than this.  If we don't have any instructions
               //  further down, this is the final.
               if (fnMax > addr) {
                  addr = fnMax;
                  continue;
               } else {
                  fnEnd = fnMax + 4;
                  break;
               }
            } else {
               if (addr == fnMax && !meta.isConditional) {
                  if (meta.target >= fnStart && meta.target < addr) {
                     // If we are the last instruction, and this instruction unconditionally
                     //   branches backwards, that means that we must be at the end of the func.
                     fnEnd = fnMax + 4;
                     break;
                  }
               }

               // We cannot follow unconditional branches outside of the function body
               //  that we have already determined, this is because we don't want to follow
               //  tail calls!
               if (meta.target > fnMax && meta.isConditional) {
                  fnMax = meta.target;
               }
            }
         }

      }
   }

   return fnEnd;
}

void markAsFunction(uint32_t address, const std::string &name)
{
   // We use get instead of searching the sFuncData directly
   //  because we can't set a function in the middle of a function
   auto info = get(address);
   if (!info.func) {
      FuncData func;
      func.start = address;
      func.end = findFunctionEnd(address);
      func.name = name;
      sFuncData.emplace(func.start, func);
   }
}

void markAsFunction(uint32_t address)
{
   markAsFunction(address, fmt::format("sub_{:08x}", address));
}

void toggleAsFunction(uint32_t address)
{
   auto fIter = sFuncData.find(address);
   if (fIter != sFuncData.end()) {
      sFuncData.erase(fIter);
   } else {
      markAsFunction(address);
   }
}

void analyse(uint32_t start, uint32_t end)
{
   uint32_t testStart = 0x025B81F4;
   uint32_t testEnd = 0x25B8374 + 4;

   // Scan through all our symbols and mark them as functions.
   // We do this first as they will not receive names if they are
   //  detected by the analysis pass due to its naming system.
   kernel::loader::lockLoader();
   const auto &modules = kernel::loader::getLoadedModules();
   for (auto &mod : modules) {
      uint32_t codeRangeStart = 0x00000000;
      uint32_t codeRangeEnd = 0x00000000;
      for (auto &sec : mod.second->sections) {
         if (sec.name.compare(".text") == 0) {
            codeRangeStart = sec.start;
            codeRangeEnd = sec.end;
            break;
         }
      }
      for (auto &sym : mod.second->symbols) {
         if (sym.second >= codeRangeStart && sym.second < codeRangeEnd) {
            markAsFunction(sym.second, sym.first);
         }
      }
   }
   kernel::loader::unlockLoader();

   for (uint32_t addr = start; addr < end; addr += 4) {
      auto instr = mem::read<espresso::Instruction>(addr);
      auto data = espresso::decodeInstruction(instr);
      if (!data) {
         continue;
      }

      if (isBranchInstr(data)) {
         auto meta = getBranchMeta(addr, instr, data, nullptr);
         if (!meta.isCall && !meta.isVariable) {
            sInstrData[meta.target].sourceBranches.push_back(addr);
         }

         // If this is a call, and its not variable, we should mark
         //  the target as a function, since it likely is...
         if (meta.isCall && !meta.isVariable) {
            markAsFunction(meta.target);
         }
      }
   }
}

} // namespace analysis

} // namespace debugger
