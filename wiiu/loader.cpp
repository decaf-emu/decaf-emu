#define ZLIB_CONST
#include <cassert>
#include <string>
#include <vector>
#include <zlib.h>
#include "bigendianview.h"
#include "elf.h"
#include "instructiondata.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "system.h"

struct RplSection
{
   Section *section = nullptr;
   ModuleSymbol *msym = nullptr;
   ElfSectionHeader header;
   std::vector<char> data;
};

static bool
readHeader(BigEndianView &in, ElfHeader &header)
{
   in.read(header.e_magic);
   in.read(header.e_class);
   in.read(header.e_encoding);
   in.read(header.e_elf_version);
   in.read(header.e_abi);
   in.read(header.e_pad);
   in.read(header.e_type);
   in.read(header.e_machine);
   in.read(header.e_version);
   in.read(header.e_entry);
   in.read(header.e_phoff);
   in.read(header.e_shoff);
   in.read(header.e_flags);
   in.read(header.e_ehsize);
   in.read(header.e_phentsize);
   in.read(header.e_phnum);
   in.read(header.e_shentsize);
   in.read(header.e_shnum);
   in.read(header.e_shstrndx);
      
   if (header.e_magic != ElfHeader::Magic) {
      xError() << "Unexpected elf e_magic "
               << Log::hex(header.e_magic) << " != " << Log::hex(ElfHeader::Magic);
      return false;
   }

   if (header.e_class != ELFCLASS32) {
      xError() << "Unexpected elf e_class "
         << Log::hex(header.e_class) << " != " << Log::hex(ELFCLASS32);
      return false;
   }

   if (header.e_encoding != ELFDATA2MSB) {
      xError() << "Unexpected elf e_encoding "
         << Log::hex(header.e_encoding) << " != " << Log::hex(ELFDATA2MSB);
      return false;
   }

   if (header.e_abi != EABI_CAFE) {
      xError() << "Unexpected elf e_abi "
         << Log::hex(header.e_abi) << " != " << Log::hex(EABI_CAFE);
      return false;
   }

   if (header.e_machine != EM_PPC) {
      xError() << "Unexpected elf e_machine "
         << Log::hex(header.e_machine) << " != " << Log::hex(EM_PPC);
      return false;
   }

   if (header.e_elf_version != EV_CURRENT) {
      xError() << "Unexpected elf e_elf_version "
         << Log::hex(header.e_elf_version) << " != " << Log::hex(EV_CURRENT);
      return false;
   }

   assert(header.e_shentsize == sizeof(ElfSectionHeader));
   return true;
}

static bool
readSectionHeader(BigEndianView &in, ElfSectionHeader &shdr)
{
   in.read(shdr.sh_name);
   in.read(shdr.sh_type);
   in.read(shdr.sh_flags);
   in.read(shdr.sh_addr);
   in.read(shdr.sh_offset);
   in.read(shdr.sh_size);
   in.read(shdr.sh_link);
   in.read(shdr.sh_info);
   in.read(shdr.sh_addralign);
   in.read(shdr.sh_entsize);
   return true;
}

static bool
readSymbol(BigEndianView &in, ElfSymbol &sym)
{
   in.read(sym.st_name);
   in.read(sym.st_value);
   in.read(sym.st_size);
   in.read(sym.st_info);
   in.read(sym.st_other);
   in.read(sym.st_shndx);
   return true;
}

static bool
readRelocationAddend(BigEndianView &in, ElfRela &rela)
{
   in.read(rela.r_offset);
   in.read(rela.r_info);
   in.read(rela.r_addend);
   return true;
}

static bool
readFileInfo(BigEndianView &in, RplFileInfo &info)
{
   in.read(info.rpl_min_version);
   in.read(info.rpl_text_size);
   in.read(info.rpl_text_align);
   in.read(info.rpl_data_size);
   in.read(info.rpl_data_align);
   in.read(info.rpl_loader_size);
   in.read(info.rpl_loader_info);
   in.read(info.rpl_temp_size);
   in.read(info.rpl_tramp_adjust);
   in.read(info.rpl_sda_base);
   in.read(info.rpl_sda2_base);
   in.read(info.rpl_stack_size);
   in.read(info.rpl_filename);
   in.read(info.rpl_flags);
   in.read(info.rpl_heap_size);
   in.read(info.rpl_tags);
   in.read(info.rpl_unk1);
   in.read(info.rpl_compression_level);
   in.read(info.rpl_unk2);
   in.read(info.rpl_file_info_pad);
   in.read(info.rpl_cafe_os_sdk_version);
   in.read(info.rpl_cafe_os_sdk_revision);
   in.read(info.rpl_unk3);
   in.read(info.rpl_unk4);
   return true;
}

static void
loadFileInfo(RplFileInfo &info, std::vector<RplSection> &sections)
{
   for (auto &section : sections) {
      if (section.header.sh_type != SHT_RPL_FILEINFO) {
         continue;
      }

      auto in = BigEndianView { section.data.data(), section.data.size() };
      readFileInfo(in, info);
      break;
   }
}

static bool
readSections(BigEndianView &in, ElfHeader &header, std::vector<RplSection> &sections)
{
   sections.resize(header.e_shnum);

   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      // Read section header
      in.seek(header.e_shoff + header.e_shentsize * i);
      readSectionHeader(in, section.header);

      if ((section.header.sh_type == SHT_NOBITS) || !section.header.sh_size) {
         continue;
      }

      // Read section data
      if (section.header.sh_flags & SHF_DEFLATED) {
         auto stream = z_stream {};
         auto ret = Z_OK;

         // Read the original size
         in.seek(section.header.sh_offset);
         section.data.resize(in.read<uint32_t>());

         // Inflate
         memset(&stream, 0, sizeof(stream));
         stream.zalloc = Z_NULL;
         stream.zfree = Z_NULL;
         stream.opaque = Z_NULL;

         ret = inflateInit(&stream);

         if (ret != Z_OK) {
            xError() << "Couldn't decompress .rpx section because inflateInit returned " << ret;
            section.data.clear();
         } else {
            stream.avail_in = section.header.sh_size;
            stream.next_in = in.readRaw<Bytef>(section.header.sh_size);
            stream.avail_out = static_cast<uInt>(section.data.size());
            stream.next_out = reinterpret_cast<Bytef*>(section.data.data());

            ret = inflate(&stream, Z_FINISH);

            if (ret != Z_OK && ret != Z_STREAM_END) {
               xError() << "Couldn't decompress .rpx section because inflate returned " << ret;
               section.data.clear();
            }

            inflateEnd(&stream);
         }
      } else {
         section.data.resize(section.header.sh_size);
         in.seek(section.header.sh_offset);
         in.read(section.data.data(), section.header.sh_size);
      }
   }

   return true;
}

static void
processSections(Module &module, std::vector<RplSection> &sections, const char *strData)
{
   auto dataRange = std::make_pair(0u, 0u);
   auto codeRange = std::make_pair(0u, 0u);

   // Find all code & data sections and their address space
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &rplSection = sections[i];
      auto &header = rplSection.header;

      if (header.sh_type != SHT_PROGBITS && header.sh_type != SHT_NOBITS) {
         continue;
      }

      auto section = new Section();
      section->index = i;
      section->address = header.sh_addr;
      section->name = strData + header.sh_name;
      section->size = static_cast<uint32_t>(rplSection.data.size());

      if (header.sh_type == SHT_NOBITS) {
         section->size = header.sh_size;
      }

      auto start = section->address;
      auto end = section->address + section->size;

      if (header.sh_flags & SHF_EXECINSTR) {
         section->type = Section::Code;
         
         if (codeRange.first == 0 || start < codeRange.first) {
            codeRange.first = start;
         }

         if (codeRange.second == 0 || end > codeRange.second) {
            codeRange.second = end;
         }
      } else {
         section->type = Section::Data;

         if (dataRange.first == 0 || start < dataRange.first) {
            dataRange.first = start;
         }

         if (dataRange.second == 0 || end > dataRange.second) {
            dataRange.second = end;
         }
      }

      rplSection.section = section;
      module.sections.push_back(section);
   }

   // Create thunk sections for SHT_RPL_IMPORTS!
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &rplSection = sections[i];
      auto &header = rplSection.header;

      if (header.sh_type != SHT_RPL_IMPORTS) {
         continue;
      }

      auto section = new Section();
      section->index = i;
      section->name = strData + header.sh_name;
      section->size = static_cast<uint32_t>(rplSection.data.size());

      if (header.sh_type == SHT_NOBITS) {
         section->size = header.sh_size;
      }

      // Relocate address
      assert(header.sh_addr & 0xc0000000);
      auto align = header.sh_addralign;

      if (header.sh_flags & SHF_EXECINSTR) {
         section->type = Section::CodeImports;
         section->address = (codeRange.second + align) & ~align;
         codeRange.second = section->address + section->size;
      } else {
         section->type = Section::DataImports;
         section->address = (dataRange.second + align) & ~align;
         dataRange.second = section->address + section->size;
      }

      rplSection.section = section;
      module.sections.push_back(section);
   }

   // Allocate code & data sections in memory
   module.codeAddressRange = codeRange;
   module.dataAddressRange = dataRange;
}

static void
loadSections(std::vector<RplSection> &sections)
{
   for (auto &rplSection : sections) {
      auto section = rplSection.section;

      if (!section) {
         continue;
      }

      if (rplSection.header.sh_type == SHT_NOBITS) {
         auto ptr = gMemory.translate(section->address);
         memset(ptr, 0, section->size);
      } else if (rplSection.section->type == Section::Code || rplSection.section->type == Section::Data) {
         auto ptr = gMemory.translate(section->address);
         memcpy(ptr, rplSection.data.data(), rplSection.data.size());
      }
   }
}

static SystemModule *
findSystemModule(const char *name)
{
   if (strstr(name, ".fimport_") == name) {
      name += strlen(".fimport_");
   }

   if (strstr(name, ".dimport_") == name) {
      name += strlen(".dimport_");
   }

   return gSystem.findModule(name);
}

static SystemFunction *
findSystemFunction(ModuleSymbol *msym, const char *name)
{
   if (!msym || !msym->systemModule) {
      return nullptr;
   }

   auto module = msym->systemModule;
   auto sysexp = module->findExport(name);

   if (!sysexp) {
      return nullptr;
   }

   assert(sysexp->type == SystemExport::Function);
   return reinterpret_cast<SystemFunction*>(sysexp);
}

static SystemData *
findSystemData(ModuleSymbol *msym, const char *name)
{
   if (!msym || !msym->systemModule) {
      return nullptr;
   }

   auto module = msym->systemModule;
   auto sysexp = module->findExport(name);

   if (!sysexp) {
      return nullptr;
   }

   assert(sysexp->type == SystemExport::Data);
   return reinterpret_cast<SystemData*>(sysexp);
}

static void
writeFunctionThunk(uint32_t address, uint32_t id)
{
   // Write syscall with symbol id
   auto sc = gInstructionTable.encode(InstructionID::sc);
   sc.bd = id;
   gMemory.write(address, sc.value);

   // Return by Branch to LR
   auto bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   gMemory.write(address + 4, bclr.value);
}

static void
writeDataThunk(uint32_t address, uint32_t id)
{
   // Write some invalid known value to show up when debugging
   gMemory.write<uint32_t>(address, 0xdd000000 | id);
}

static void
processSymbols(Module &module, RplSection &symtab, std::vector<RplSection> &sections)
{
   BigEndianView in = { symtab.data.data(), symtab.data.size() };
   auto count = symtab.data.size() / sizeof(ElfSymbol);
   auto &strtab = sections[symtab.header.sh_link];

   // TODO: Support more than one symbol section
   assert(module.symbols.size() == 0);
   module.symbols.resize(count);

   for (auto i = 0u; i < count; ++i) {
      ElfSymbol header;
      readSymbol(in, header);

      auto binding = header.st_info >> 4; // STB_*
      auto type = header.st_info & 0xf;   // STT_*

      // Calculate relocated address
      auto &impsec = sections[header.st_shndx];
      auto offset = header.st_value - impsec.header.sh_addr;
      auto baseAddress = impsec.section ? impsec.section->address : 0;
      auto virtAddress = baseAddress + offset;

      // Create symbol data
      SymbolInfo *symbol = nullptr;
      auto name = strtab.data.data() + header.st_name;

      if (type == STT_FUNC) {
         auto fsym = new FunctionSymbol();
         fsym->systemFunction = findSystemFunction(impsec.msym, name);
         fsym->functionType = fsym->systemFunction ? FunctionSymbol::System : FunctionSymbol::User;
         symbol = fsym;
      } else if (type == STT_OBJECT) {
         auto dsym = new DataSymbol();
         dsym->systemData = findSystemData(impsec.msym, name);
         dsym->dataType = dsym->systemData ? DataSymbol::System : DataSymbol::User;
         symbol = dsym;
      } else if (type == STT_SECTION) {
         auto msym = new ModuleSymbol();
         msym->systemModule = findSystemModule(name);
         msym->moduleType = msym->systemModule ? ModuleSymbol::System : ModuleSymbol::User;
         impsec.msym = msym;
         symbol = msym;
      } else {
         symbol = new SymbolInfo();
         symbol->type = SymbolInfo::Invalid;
      }

      assert(symbol);
      symbol->index = i;
      symbol->name = name;
      symbol->address = virtAddress;
      module.symbols[i] = symbol;
      xLog() << Log::hex(symbol->address) << " " << symbol->name << " " << symbol->type;

      // Write thunks
      if (symbol->address) {
         if (type == STT_FUNC) {
            writeFunctionThunk(symbol->address, symbol->index);
         } else if (type == STT_OBJECT) {
            writeDataThunk(symbol->address, symbol->index);
         }
      }
   }
}

static void
processRelocations(Module &module, RplSection &section, std::vector<RplSection> &sections)
{
   // TODO: Support more than one symbol section (symsec)
   auto symsec = sections[section.header.sh_link];
   auto in = BigEndianView { section.data.data(), section.data.size() };

   // Find our relocation section addresses
   auto relsec = sections[section.header.sh_info];
   auto baseAddress = relsec.header.sh_addr;
   auto virtAddress = relsec.section->address;

   while (!in.eof()) {
      ElfRela rela;
      readRelocationAddend(in, rela);

      auto index = rela.r_info >> 8;
      auto type = rela.r_info & 0xff;
      auto symbol = module.symbols[index];
      if (!symbol) {
         xLog() << "weird";
      }
      auto base = symbol ? symbol->address : 0;
      auto value = base + rela.r_addend;

      auto addr = rela.r_offset - baseAddress + virtAddress;
      auto ptr8 = gMemory.translate(addr);
      auto ptr16 = reinterpret_cast<uint16_t*>(ptr8);
      auto ptr32 = reinterpret_cast<uint32_t*>(ptr8);

      switch (type) {
      case R_PPC_ADDR32:
         *ptr32 = byte_swap(value);
         break;
      case R_PPC_ADDR16_LO:
         *ptr16 = byte_swap<uint16_t>(value & 0xffff);
         break;
      case R_PPC_ADDR16_HI:
         *ptr16 = byte_swap<uint16_t>(value >> 16);
         break;
      case R_PPC_ADDR16_HA:
         *ptr16 = byte_swap<uint16_t>((value + 0x8000) >> 16);
         break;
      case R_PPC_REL24:
         *ptr32 = byte_swap((byte_swap(*ptr32) & ~0x03fffffc) | ((value - rela.r_offset) & 0x03fffffc));
         break;
      default:
         xDebug() << "Unknown relocation type: " << type;
      }
   }
}

bool
Loader::loadRPL(Module &module, EntryInfo &entry, const char *buffer, size_t size)
{
   auto in = BigEndianView { buffer, size };
   auto header = ElfHeader { };
   auto info = RplFileInfo { };
   auto sections = std::vector<RplSection> { };

   // Read header
   if (!readHeader(in, header)) {
      xError() << "Failed to readHeader";
      return false;
   }

   // Read sections
   if (!readSections(in, header, sections)) {
      xError() << "Failed to readRPLSections";
      return false;
   }

   // Process sections, find our data and code sections
   processSections(module, sections, sections[header.e_shstrndx].data.data());

   // Update EntryInfo
   loadFileInfo(info, sections);
   entry.address = header.e_entry;
   entry.stackSize = info.rpl_stack_size;

   // Allocate code & data sections in memory
   auto codeStart = module.codeAddressRange.first;
   auto codeSize = module.codeAddressRange.second - codeStart;
   gMemory.alloc(codeStart, codeSize);

   auto dataStart = module.dataAddressRange.first;
   auto dataSize = module.dataAddressRange.second - dataStart;
   gMemory.alloc(dataStart, dataSize);

   // Load sections into memory
   loadSections(sections);

   // Process symbols
   // TODO: Support more than one symbol section
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      if (section.header.sh_type != SHT_SYMTAB) {
         continue;
      }

      processSymbols(module, section, sections);
   }

   // Process relocations
   for (auto i = 0u; i < sections.size(); ++i) {
      auto &section = sections[i];

      if (section.header.sh_type != SHT_RELA) {
         continue;
      }

      processRelocations(module, section, sections);
   }

   // Log our module sections
   xLog() << "Loaded module!";
   xLog() << "Code: " << Log::hex(module.codeAddressRange.first) << ":" << Log::hex(module.codeAddressRange.second);
   xLog() << "Data: " << Log::hex(module.dataAddressRange.first) << ":" << Log::hex(module.dataAddressRange.second);

   for (auto i = 0u; i < module.sections.size(); ++i) {
      auto section = module.sections[i];
      xLog() << Log::hex(section->address) << " " << section->name << " " << Log::hex(section->size);
   }

   for (auto i = 0u; i < module.symbols.size(); ++i) {
      auto &symbol = module.symbols[i];
      xLog() << Log::hex(symbol->address) << " " << symbol->name;
   }

   return true;
}
