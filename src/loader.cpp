#define ZLIB_CONST
#include <cassert>
#include <limits>
#include <string>
#include <vector>
#include <zlib.h>
#include "bigendianview.h"
#include "elf.h"
#include "instructiondata.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit_dynload.h"
#include "modules/coreinit/coreinit_memory.h"
#include "system.h"
#include "kernelmodule.h"
#include "usermodule.h"
#include "util.h"

static void
loadFileInfo(elf::FileInfo &info, std::vector<elf::Section> &sections)
{
   for (auto &section : sections) {
      if (section.header.type != elf::SHT_RPL_FILEINFO) {
         continue;
      }

      auto in = BigEndianView { section.data.data(), section.data.size() };
      elf::readFileInfo(in, info);
      break;
   }
}

static void
processSections(UserModule &module, std::vector<elf::Section> &sections, const char *strData)
{
   auto dataRange = std::make_pair(0u, 0u);
   auto codeRange = std::make_pair(0u, 0u);

   // Find all code & data sections and their address space
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &rplSection = sections[i];
      auto &header = rplSection.header;

      if (header.type != elf::SHT_PROGBITS && header.type != elf::SHT_NOBITS) {
         continue;
      }

      auto section = new UserModule::Section();
      section->index = i;
      section->address = header.addr;
      section->name = strData + header.name;
      section->size = static_cast<uint32_t>(rplSection.data.size());

      if (header.type == elf::SHT_NOBITS) {
         section->size = header.size;
      }

      auto start = section->address;
      auto end = section->address + section->size;

      if (header.flags & elf::SHF_EXECINSTR) {
         section->type = UserModule::Section::Code;
         
         if (codeRange.first == 0 || start < codeRange.first) {
            codeRange.first = start;
         }

         if (codeRange.second == 0 || end > codeRange.second) {
            codeRange.second = end;
         }
      } else {
         section->type = UserModule::Section::Data;

         if (dataRange.first == 0 || start < dataRange.first) {
            dataRange.first = start;
         }

         if (dataRange.second == 0 || end > dataRange.second) {
            dataRange.second = end;
         }
      }

      rplSection.section = section;
      module.sections.push_back(section);
      module.sectionMap[section->name] = section;
   }

   /* SHT_RPL_EXPORTS
   u32 num_entries
   u32 code_sig
   struct {
     u32 hash?
     u32 type?
     u32 ?
     u32 ?
     null_terminated_string func_name
     u32 ?
     u32 ?
     u32 ?
     u16 ?
   } entries[num_entries];
   */

   // Create thunk sections for SHT_RPL_IMPORTS!
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &rplSection = sections[i];
      auto &header = rplSection.header;

      if (header.type != elf::SHT_RPL_IMPORTS) {
         continue;
      }

      auto section = new UserModule::Section();
      section->index = i;
      section->name = strData + header.name;
      section->library = rplSection.data.data() + 8;
      section->size = static_cast<uint32_t>(rplSection.data.size());

      if (header.type == elf::SHT_NOBITS) {
         section->size = header.size;
      }

      if (header.flags & elf::SHF_EXECINSTR) {
         section->type = UserModule::Section::CodeImports;
         section->address = alignUp(codeRange.second, header.addralign);
         codeRange.second = section->address + section->size;
      } else {
         section->type = UserModule::Section::DataImports;
         section->address = alignUp(dataRange.second, header.addralign);
         dataRange.second = section->address + section->size;
      }

      rplSection.section = section;
      module.sections.push_back(section);
      module.sectionMap[section->name] = section;
   }

   // Allocate code & data sections in memory
   module.codeAddressRange = codeRange;
   module.dataAddressRange = dataRange;
}

static void
relocateSections(std::vector<elf::Section> &sections, uint32_t origCode, uint32_t newCode, uint32_t origData, uint32_t newData)
{
   int32_t diffCode = static_cast<int32_t>(newCode) - static_cast<int32_t>(origCode);
   int32_t diffData = static_cast<int32_t>(newData) - static_cast<int32_t>(origData);

   for (auto &rplSection : sections) {
      auto section = rplSection.section;

      if (!section) {
         continue;
      }

      if (rplSection.section->type == UserModule::Section::Code) {
         section->address = static_cast<uint32_t>(static_cast<int32_t>(section->address) + diffCode);
      } else if (rplSection.section->type == UserModule::Section::Data) {
         section->address = static_cast<uint32_t>(static_cast<int32_t>(section->address) + diffData);
      } else if (rplSection.section->type == UserModule::Section::DataImports) {
         section->address = static_cast<uint32_t>(static_cast<int32_t>(section->address) + diffData);
      }
   }
}

static void
loadSections(std::vector<elf::Section> &sections)
{
   for (auto &rplSection : sections) {
      auto section = rplSection.section;

      if (!section) {
         continue;
      }

      if (rplSection.header.type == elf::SHT_NOBITS) {
         auto ptr = gMemory.translate(section->address);
         memset(ptr, 0, section->size);
      } else if (rplSection.section->type == UserModule::Section::Code || rplSection.section->type == UserModule::Section::Data) {
         auto ptr = gMemory.translate(section->address);
         memcpy(ptr, rplSection.data.data(), rplSection.data.size());
      }
   }
}

static void
processSmallDataSections(UserModule &module)
{
   auto findSectionRange = [](auto data, auto bss) {
      uint32_t start, end;

      if (data && bss) {
         start = std::min(data->address, bss->address);
         end = std::max(data->address + data->size, bss->address + bss->size);
      } else if (data) {
         start = data->address;
         end = data->address + data->size;
      } else if (bss) {
         start = bss->address;
         end = bss->address + bss->size;
      } else {
         start = 0;
         end = 0;
      }

      return std::make_pair(start, end);
   };

   // Find _SDA_BASE_
   auto data = module.findSection(".sdata");
   auto bss = module.findSection(".sbss");
   auto range = findSectionRange(data, bss);

   if ((range.second - range.first) > 0xffff) {
      gLog->error(".sdata and .sbss do not fit in a 16 bit range");
   }

   module.sdaBase = ((range.second - range.first) / 2) + range.first;

   // Find _SDA2_BASE_
   data = module.findSection(".sdata2");
   bss = module.findSection(".sbss2");
   range = findSectionRange(data, bss);

   if ((range.second - range.first) > 0xffff) {
      gLog->error(".sdata and .sbss do not fit in a 16 bit range");
   }

   module.sda2Base = ((range.second - range.first) / 2) + range.first;
}

static KernelModule *
findSystemModule(const char *name)
{
   return gSystem.findModule(name);
}

static KernelFunction *
findKernelFunction(ModuleSymbol *msym, const char *name)
{
   if (!msym || !msym->systemModule) {
      return nullptr;
   }

   auto module = msym->systemModule;
   auto sysexp = module->findExport(name);

   if (!sysexp) {
      return nullptr;
   }

   assert(sysexp->type == KernelExport::Function);
   return reinterpret_cast<KernelFunction*>(sysexp);
}

static KernelData *
findKernelData(ModuleSymbol *msym, const char *name)
{
   if (!msym || !msym->systemModule) {
      return nullptr;
   }

   auto module = msym->systemModule;
   auto sysexp = module->findExport(name);

   if (!sysexp) {
      return nullptr;
   }

   assert(sysexp->type == KernelExport::Data);
   return reinterpret_cast<KernelData*>(sysexp);
}

static void
writeFunctionThunk(uint32_t address, uint32_t id)
{
   // Write syscall with symbol id
   auto kc = gInstructionTable.encode(InstructionID::kc);
   kc.li = id;
   kc.aa = 0;
   gMemory.write(address, kc.value);

   // Return by Branch to LR
   auto bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   gMemory.write(address + 4, bclr.value);
}

static void
writeDataThunk(uint32_t address, uint32_t id)
{
   // Write some invalid known value to show up when debugging
   gMemory.write<uint32_t>(address, 0xddddd000 | id);
}

static SymbolInfo *
createFunctionSymbol(uint32_t index, elf::Section &section, const char *name, uint32_t virtAddress, uint32_t size)
{
   auto symbol = new FunctionSymbol();
   symbol->index = index;
   symbol->name = name;
   symbol->size = size;

   if (section.header.type == elf::SHT_RPL_IMPORTS) {
      symbol->functionType = FunctionSymbol::Kernel;
      symbol->kernelFunction = findKernelFunction(section.msym, name);

      if (symbol->kernelFunction) {
         symbol->address = symbol->kernelFunction->vaddr;
      } else {
         symbol->address = virtAddress;

         if (symbol->address) {
            writeFunctionThunk(symbol->address, index);
         }
      }
   } else {
      symbol->functionType = FunctionSymbol::User;
      symbol->address = virtAddress;
   }

   return symbol;
}

static SymbolInfo *
createDataSymbol(uint32_t index, elf::Section &section, const char *name, uint32_t virtAddress, uint32_t size)
{
   auto symbol = new DataSymbol();
   symbol->index = index;
   symbol->name = name;
   symbol->size = size;

   if (section.header.type == elf::SHT_RPL_IMPORTS) {
      symbol->dataType = DataSymbol::Kernel;
      symbol->kernelData = findKernelData(section.msym, name);

      if (symbol->kernelData) {
         symbol->address = static_cast<uint32_t>(*symbol->kernelData->vptr);
      } else {
         symbol->address = virtAddress;

         if (symbol->address) {
            writeDataThunk(symbol->address, index);
         }
      }
   } else {
      symbol->dataType = DataSymbol::User;
      symbol->address = virtAddress;
   }

   return symbol;
}

static SymbolInfo *
createModuleSymbol(uint32_t index, const char *library, const char *name, uint32_t virtAddress, uint32_t size)
{
   auto symbol = new ModuleSymbol();
   symbol->index = index;
   symbol->name = name;
   symbol->address = virtAddress;
   symbol->size = size;

   if (library) {
      symbol->systemModule = findSystemModule(library);
   } else {
      symbol->systemModule = nullptr;
   }

   if (symbol->systemModule) {
      symbol->moduleType = ModuleSymbol::System;
   } else {
      symbol->moduleType = ModuleSymbol::User;
   }

   return symbol;
}

static void
processSymbols(UserModule &module, elf::Section &symtab, std::vector<elf::Section> &sections)
{
   auto in = BigEndianView { symtab.data.data(), symtab.data.size() };
   auto count = symtab.data.size() / sizeof(elf::Symbol);
   auto &strtab = sections[symtab.header.link];

   // TODO: Support more than one symbol section
   assert(module.symbols.size() == 0);
   module.symbols.resize(count);

   for (auto i = 0u; i < count; ++i) {
      SymbolInfo *symbol = nullptr;
      auto header = elf::Symbol { };
      elf::readSymbol(in, header);

      if (header.shndx >= elf::SHN_LORESERVE) {
         continue;
      }

      // Calculate relocated address
      auto &impsec = sections[header.shndx];
      auto offset = header.value - impsec.header.addr;
      auto baseAddress = impsec.section ? impsec.section->address : 0;
      auto virtAddress = baseAddress + offset;

      // Create symbol data
      auto name = strtab.data.data() + header.name;
      auto binding = header.info >> 4;
      auto type = header.info & 0xf;

      if (type == elf::STT_FUNC) {
         symbol = createFunctionSymbol(i, impsec, name, virtAddress, header.size);
      } else if (type == elf::STT_OBJECT) {
         symbol = createDataSymbol(i, impsec, name, virtAddress, header.size);
      } else if (type == elf::STT_SECTION) {
         const char *library = nullptr;

         if (impsec.section) {
            library = impsec.section->library.c_str();
         }

         symbol = createModuleSymbol(i, library, name, virtAddress, header.size);
         impsec.msym = reinterpret_cast<ModuleSymbol*>(symbol);
      } else {
         symbol = new SymbolInfo();
         symbol->type = SymbolInfo::Invalid;
         symbol->index = i;
         symbol->name = name;
         symbol->address = virtAddress;
      }

      module.symbols[i] = symbol;
   }
}

static void
processRelocations(UserModule &module, elf::Section &section, std::vector<elf::Section> &sections)
{
   // TODO: Support more than one symbol section (symsec)
   auto &symsec = sections[section.header.link];
   auto in = BigEndianView { section.data.data(), section.data.size() };

   // Find our relocation section addresses
   auto &relsec = sections[section.header.info];
   auto baseAddress = relsec.header.addr;
   auto virtAddress = relsec.section->address;

   // EABI sda sections
   auto sdata  = module.findSection(".sdata");
   auto sbss   = module.findSection(".sbss");
   auto sdata0 = module.findSection(".sdata0");
   auto sbss0  = module.findSection(".sbss0");
   auto sdata2 = module.findSection(".sdata2");
   auto sbss2  = module.findSection(".sbss2");

   while (!in.eof()) {
      elf::Rela rela;
      elf::readRelocationAddend(in, rela);

      auto index = rela.info >> 8;
      auto type  = rela.info & 0xff;

      auto &symbol = module.symbols[index];
      auto value = symbol->address + rela.addend;
      auto addr = (rela.offset - baseAddress) + virtAddress;

      auto ptr8 = gMemory.translate(addr);
      auto ptr16 = reinterpret_cast<uint16_t*>(ptr8);
      auto ptr32 = reinterpret_cast<uint32_t*>(ptr8);

      switch (type) {
      case elf::R_PPC_ADDR32:
         *ptr32 = byte_swap(value);
         break;
      case elf::R_PPC_ADDR16_LO:
         *ptr16 = byte_swap<uint16_t>(value & 0xffff);
         break;
      case elf::R_PPC_ADDR16_HI:
         *ptr16 = byte_swap<uint16_t>(value >> 16);
         break;
      case elf::R_PPC_ADDR16_HA:
         *ptr16 = byte_swap<uint16_t>((value + 0x8000) >> 16);
         break;
      case elf::R_PPC_REL24:
         *ptr32 = byte_swap((byte_swap(*ptr32) & ~0x03fffffc) | ((value - addr) & 0x03fffffc));
         break;
      case elf::R_PPC_EMB_SDA21: {
         auto ins = Instruction { byte_swap(*ptr32) };
         auto section = module.findSection(value);
         uint32_t base;
         int32_t offset;

         if (section == sdata || section == sbss) {
            ins.rA = 13;
            base = module.sdaBase;
         } else if (section == sdata0 || section == sbss0) {
            ins.rA = 0;
            base = 0;
         } else if (section == sdata2 || section == sbss2) {
            ins.rA = 2;
            base = module.sda2Base;
         } else {
            gLog->error("Invalid relocation for EMB_SDA21: symbol target addr {:x}", value);
            break;
         }

         offset = static_cast<int32_t>(value) - static_cast<int32_t>(base);

         if (offset < std::numeric_limits<int16_t>::min() || offset > std::numeric_limits<int16_t>::max()) {
            gLog->error("Expected SDA relocation {:x} to be within signed 16 bit offset of base {:x}", value, addr);
            break;
         }

         ins.simm = offset;
         *ptr32 = byte_swap(ins.value);
         break;
      }
      default:
         gLog->error("Unknown relocation type {}", type);
      }
   }
}

bool
Loader::loadRPL(UserModule &module, const char *buffer, size_t size)
{
   auto in = BigEndianView { buffer, size };
   auto header = elf::Header { };
   auto info = elf::FileInfo { };
   auto sections = std::vector<elf::Section> { };

   // Read header
   if (!elf::readHeader(in, header)) {
      gLog->error("Failed elf::readHeader");
      return false;
   }

   // Check it is a CAFE abi rpl
   if (header.abi != elf::EABI_CAFE) {
      gLog->error("Unexpected elf abi found {:02x} expected {:02x}", header.abi, elf::EABI_CAFE);
      return false;
   }

   // Read sections
   if (!elf::readSections(in, header, sections)) {
      gLog->error("Failed elf::readSections");
      return false;
   }

   // Process sections, find our data and code sections
   processSections(module, sections, sections[header.shstrndx].data.data());

   // Update EntryInfo
   loadFileInfo(info, sections);
   module.entryPoint = header.entry;
   module.defaultStackSize = info.stackSize;

   // Allocate code & data sections in memory
   auto codeStart = module.codeAddressRange.first;
   auto codeSize = module.maxCodeSize;
   gMemory.alloc(codeStart, codeSize); // TODO: Append code to end of other loaded code sections

   auto dataStart = alignUp(codeStart + codeSize, 4096);
   auto dataSize = alignUp(module.dataAddressRange.second - module.dataAddressRange.first, 4096);
   auto dataEnd = dataStart + dataSize;
   gMemory.alloc(dataStart, dataSize); // TODO: Use OSDynLoad_MemAlloc for data section allocation

   // Update MEM2 memory bounds
   be_val<uint32_t> mem2start, mem2size;
   OSGetMemBound(OSMemoryType::MEM2, &mem2start, &mem2size);
   OSSetMemBound(OSMemoryType::MEM2, dataEnd, mem2size - (dataEnd - mem2start));

   // Relocate sections
   relocateSections(sections, module.codeAddressRange.first, codeStart, module.dataAddressRange.first, dataStart);

   module.codeAddressRange.first = codeStart;
   module.codeAddressRange.second = codeStart + codeSize;

   module.dataAddressRange.first = dataStart;
   module.dataAddressRange.second = dataStart + dataSize;

   // Relocate entry point
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      if (section.header.addr <= header.entry && section.header.addr + section.data.size() > header.entry) {
         auto offset = section.section->address - section.header.addr;
         module.entryPoint = header.entry + offset;
         break;
      }
   }

   // Load sections into memory
   loadSections(sections);

   // Process small data sections
   processSmallDataSections(module);

   // Process symbols
   // TODO: Support more than one symbol section?
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      if (section.header.type != elf::SHT_SYMTAB) {
         continue;
      }

      processSymbols(module, section, sections);
   }

   // Process relocations
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      if (section.header.type != elf::SHT_RELA) {
         continue;
      }

      processRelocations(module, section, sections);
   }
   
   if (0) {
      // Print address ranges
      gLog->debug("Loaded module!");
      gLog->debug("Code {:08x} -> {:08x}", module.codeAddressRange.first, module.codeAddressRange.second);
      gLog->debug("Data {:08x} -> {:08x}", module.dataAddressRange.first, module.dataAddressRange.second);

      // Print all sections
      gLog->debug("Sections:");

      for (auto i = 0u; i < module.sections.size(); ++i) {
         auto section = module.sections[i];
         gLog->debug("{:08x} {} {:x}", section->address, section->name, section->size);
      }
      
      // Print all symbols
      gLog->debug("Symbols:");

      for (auto i = 0u; i < module.symbols.size(); ++i) {
         auto symbol = module.symbols[i];

         if (symbol && symbol->name.size()) {
            gLog->debug("{:08x} {}", symbol->address, symbol->name);
         }
      }
   }

   return true;
}
