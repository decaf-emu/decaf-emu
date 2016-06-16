#include "debugger_analysis.h"
#include "debugger_branchcalc.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include "libcpu/mem.h"
#include <map>
#include <unordered_map>

namespace debugger
{

namespace analysis
{

static std::map<uint32_t, FuncData>
sFuncData;

static std::unordered_map<uint32_t, InstrData>
sInstrData;

InstrInfo get(uint32_t address)
{
   InstrInfo info = { 0 };

   auto instrIter = sInstrData.find(address);
   if (instrIter != sInstrData.end()) {
      info.instr = &instrIter->second;
   }

   if (sFuncData.size() > 0) {
      auto funcIter = sFuncData.upper_bound(address);
      funcIter--;
      if (funcIter != sFuncData.end()) {
         auto &func = funcIter->second;
         if (address >= func.start && address < func.end) {
            info.func = &func;
         }
      }
   }

   return info;
}

void analyse(uint32_t start, uint32_t end)
{

}

void markAsFunction(uint32_t address)
{

}

} // namespace analysis

} // namespace debugger
