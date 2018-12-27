#include "debugger_analysis.h"
#include "debugger_branchcalc.h"

#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include <fmt/format.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libcpu/mem.h>
#include <map>
#include <unordered_map>

namespace debugger
{

namespace analysis
{

static std::map<uint32_t, FuncData, std::greater<uint32_t>>
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
   auto info = InstrInfo { 0 };
   auto instrIter = sInstrData.find(address);

   if (instrIter != sInstrData.end()) {
      info.instr = &instrIter->second;
   }

   if (sFuncData.size() > 0) {
      auto funcIter = sFuncData.lower_bound(address);

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

   return info;
}

uint32_t
findFunctionEnd(uint32_t start)
{
   static const uint32_t MaxScannedBytes = 0x400u;
   auto fnStart = start;
   auto fnMax = start;
   auto fnEnd = uint32_t { 0xFFFFFFFFu };

   for (auto addr = start; addr < start + MaxScannedBytes; addr += 4) {
      if (!cpu::isValidAddress(cpu::VirtualAddress { addr })) {
         break;
      }

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
         auto meta = getBranchMeta(addr, instr, data, 0, 0, 0);

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

static void
markAsFunction(uint32_t address,
               std::string_view name)
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

static void
markAsFunction(uint32_t address)
{
   markAsFunction(address, fmt::format("sub_{:08x}", address));
}

void
toggleAsFunction(uint32_t address)
{
   auto fIter = sFuncData.find(address);

   if (fIter != sFuncData.end()) {
      sFuncData.erase(fIter);
   } else {
      markAsFunction(address);
   }
}

void
analyse(uint32_t start,
        uint32_t end)
{
   // Scan through all our symbols and mark them as functions.
   // We do this first as they will not receive names if they are
   //  detected by the analysis pass due to its naming system.
   cafe::loader::lockLoader();
   for (auto rpl = cafe::loader::getLoadedRplLinkedList(); rpl; rpl = rpl->nextLoadedRpl) {
      auto symTabHdr = virt_ptr<cafe::loader::rpl::SectionHeader> { nullptr };
      auto symTabAddr = virt_addr { 0 };
      auto strTabAddr = virt_addr { 0 };
      auto textStartAddr = rpl->textAddr;
      auto textEndAddr = rpl->textAddr + rpl->textSize;

      // Find symbol section
      if (rpl->sectionHeaderBuffer) {
         for (auto i = 0u; i < rpl->elfHeader.shnum; ++i) {
            auto sectionHeader =
               virt_cast<cafe::loader::rpl::SectionHeader *>(
                  virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
                  (i * rpl->elfHeader.shentsize));

            if (sectionHeader->type == cafe::loader::rpl::SHT_SYMTAB) {
               symTabHdr = sectionHeader;
               symTabAddr = rpl->sectionAddressBuffer[i];
               strTabAddr = rpl->sectionAddressBuffer[symTabHdr->link];
               break;
            }
         }
      }

      if (symTabHdr && symTabAddr && strTabAddr) {
         auto symTabEntSize =
            symTabHdr->entsize ?
            static_cast<size_t>(symTabHdr->entsize) :
            sizeof(cafe::loader::rpl::Symbol);
         auto symTabEntries = symTabHdr->size / symTabEntSize;

         for (auto i = 0u; i < symTabEntries; ++i) {
            auto symbol =
               virt_cast<cafe::loader::rpl::Symbol *>(
                  symTabAddr + (i * symTabEntSize));
            auto symbolAddress = virt_addr { static_cast<uint32_t>(symbol->value) };
            if ((symbol->info & 0xf) == cafe::loader::rpl::STT_FUNC &&
                symbolAddress >= textStartAddr && symbolAddress < textEndAddr) {
               auto name = virt_cast<const char *>(strTabAddr + symbol->name);
               markAsFunction(symbol->value, name.get());
            }
         }
      }
   }
   cafe::loader::unlockLoader();

   for (auto addr = start; addr < end; addr += 4) {
      auto instr = mem::read<espresso::Instruction>(addr);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         continue;
      }

      if (isBranchInstr(data)) {
         auto meta = getBranchMeta(addr, instr, data, 0, 0, 0);

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
