#include "decaf_debug_api.h"

#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include <fmt/format.h>
#include <libcpu/espresso/espresso_disassembler.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libcpu/mem.h>

namespace decaf::debug
{

const AnalyseDatabase::Function *
analyseLookupFunction(const AnalyseDatabase &db,
                      VirtualAddress address)
{
   if (auto itr = db.functions.find(address); itr != db.functions.end()) {
      return &itr->second;
   }

   return nullptr;
}

template<typename ConstOptionalDatabase>
static auto
findFunctionContainingAddress(ConstOptionalDatabase &db,
                              VirtualAddress address)
{
   using ReturnType =
      typename std::conditional<std::is_const<ConstOptionalDatabase>::value,
                       const AnalyseDatabase::Function *,
                       AnalyseDatabase::Function *>::type;

   if (db.functions.empty()) {
      return static_cast<ReturnType>(nullptr);
   }

   if (auto itr = db.functions.lower_bound(address); itr != db.functions.end()) {
      auto &func = itr->second;

      if (address >= func.start && address < func.end) {
         // The function needs to have an end, or be the first two instructions
         //  since we apply some special display logic to the first two instructions
         //  in a never-ending function...
         if (func.end != 0xFFFFFFFF || (address == func.start || address == func.start + 4)) {
            return &func;
         }
      }
   }

   return static_cast<ReturnType>(nullptr);
}

AnalyseDatabase::Lookup
analyseLookupAddress(const AnalyseDatabase &db,
                     VirtualAddress address)
{
   auto info = AnalyseDatabase::Lookup { };
   if (auto itr = db.instructions.find(address); itr != db.instructions.end()) {
      info.instruction = &itr->second;
   }

   info.function = findFunctionContainingAddress(db, address);
   return info;
}

uint32_t
analyseScanFunctionEnd(VirtualAddress start)
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

      if (espresso::isBranchInstruction(data->id)) {
         auto branchInfo =
            espresso::disassembleBranchInfo(data->id, instr, addr, 0, 0, 0);

         // Ignore call instructions
         if (!branchInfo.isCall) {
            if (branchInfo.isVariable) {
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
               if (addr == fnMax && !branchInfo.isConditional) {
                  if (branchInfo.target >= fnStart && branchInfo.target < addr) {
                     // If we are the last instruction, and this instruction unconditionally
                     //   branches backwards, that means that we must be at the end of the func.
                     fnEnd = fnMax + 4;
                     break;
                  }
               }

               // We cannot follow unconditional branches outside of the function body
               //  that we have already determined, this is because we don't want to follow
               //  tail calls!
               if (branchInfo.target > fnMax && branchInfo.isConditional) {
                  fnMax = branchInfo.target;
               }
            }
         }
      }
   }

   return fnEnd;
}

static std::string
defaultFunctionName(VirtualAddress address)
{
   return fmt::format("sub_{:08x}", address);
}

static void
markAsFunction(AnalyseDatabase &db,
               VirtualAddress address,
               std::string_view name = {})
{
   // Check if the address is already marked as a function
   auto function = findFunctionContainingAddress(db, address);
   if (function) {
      if (!name.empty()) {
         // Update the name if a name was passed in
         function->name = name;
      }

      return;
   }

   auto func = AnalyseDatabase::Function { };
   func.start = address;
   func.end = analyseScanFunctionEnd(address);

   if (name.empty()) {
      func.name = defaultFunctionName(address);
   } else {
      func.name = name;
   }

   db.functions.emplace(func.start, func);
}

void
analyseToggleAsFunction(AnalyseDatabase &db,
                        VirtualAddress address)
{
   if (auto itr = db.functions.find(address); itr != db.functions.end()) {
      db.functions.erase(itr);
   } else {
      markAsFunction(db, address);
   }
}

void
analyseLoadedModules(AnalyseDatabase &db)
{
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
               markAsFunction(db, symbol->value, name.get());
            }
         }
      }
   }
   cafe::loader::unlockLoader();
}

void
analyseCode(AnalyseDatabase &db,
            VirtualAddress start,
            VirtualAddress end)
{
   for (auto addr = start; addr < end; addr += 4) {
      auto instr = mem::read<espresso::Instruction>(addr);
      auto data = espresso::decodeInstruction(instr);

      if (!data) {
         continue;
      }

      if (espresso::isBranchInstruction(data->id)) {
         auto branchInfo =
            espresso::disassembleBranchInfo(data->id, instr, addr, 0, 0, 0);

         if (!branchInfo.isCall && !branchInfo.isVariable) {
            db.instructions[branchInfo.target].sourceBranches.push_back(addr);
         }

         // If this is a call, and its not variable, we should mark
         //  the target as a function, since it likely is...
         if (branchInfo.isCall && !branchInfo.isVariable) {
            markAsFunction(db, branchInfo.target);
         }
      }
   }
}

} // namespace decaf::debug
