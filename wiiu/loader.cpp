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

// TODO: Support for relocatable binarys

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
readSections(BigEndianView &in, Binary &bin)
{
   bin.sections.resize(bin.header.e_shnum);

   for (auto i = 0; i < bin.header.e_shnum; ++i) {
      auto &section = bin.sections[i];
      section.index = i;

      // Read header
      in.seek(bin.header.e_shoff + bin.header.e_shentsize * i);
      readSectionHeader(in, section.header);

      if ((section.header.sh_type == SHT_NOBITS) || !section.header.sh_size) {
         continue;
      }

      // Decompress if needed
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
            stream.avail_out = section.data.size();
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

static bool
loadSections(Binary &bin)
{
   auto rplImportVA = uint32_t { 0 };
   auto &strSection = bin.sections[bin.header.e_shstrndx];

   for (auto &section : bin.sections) {
      section.name = strSection.data.data() + section.header.sh_name;

      // Place RPL_IMPORTS after .text
      if (section.name.compare(".text") == 0) {
         rplImportVA = ((section.header.sh_addr + section.data.size()) + 0xf) & ~0xf;
      }

      // Relocate RPL_IMPORTS, TODO: Full relocation
      if (section.header.sh_type == SHT_RPL_IMPORTS) {
         assert(section.header.sh_addr & 0xc0000000);
         section.virtualAddress = rplImportVA;
         rplImportVA += section.data.size();
      } else {
         section.virtualAddress = section.header.sh_addr;
      }

      if (section.header.sh_flags & SHF_ALLOC) {
         auto addr = section.virtualAddress;

         if (section.header.sh_type == SHT_NOBITS) {
            auto size = section.header.sh_size;
            auto ptr = gMemory.allocate(addr, size);

            if (ptr) {
               memset(reinterpret_cast<void*>(ptr), 0, size);
            }
         } else {
            auto size = section.data.size();
            auto ptr = gMemory.allocate(addr, size);

            if (ptr) {
               memcpy(reinterpret_cast<void*>(ptr), section.data.data(), size);
            }
         }
      }

      xDebug() << section.name;
   }

   return true;
}

static bool
loadSymbols(Binary &bin)
{
   auto symSectionPtr = bin.findSection(".symtab");

   if (!symSectionPtr) {
      return false;
   }

   auto &symSection = *symSectionPtr;
   auto &strSection = bin.sections[symSection.header.sh_link];
   auto symIn = BigEndianView { symSection.data.data(), symSection.data.size() };

   auto count = symSection.data.size() / sizeof(ElfSymbol);
   bin.symbols.resize(count);

   for (auto i = 0u; i < count; ++i) {
      auto &symbol = bin.symbols[i];
      readSymbol(symIn, symbol.header);

      auto &impSection = bin.sections[symbol.header.st_shndx];
      auto binding = symbol.header.st_info >> 4; // STB_*
      auto type = symbol.header.st_info & 0xf;   // STT_*

      symbol.value = symbol.header.st_value;
      symbol.name = strSection.data.data() + symbol.header.st_name;
      symbol.section = &impSection;

      switch (type) {
      case STT_SECTION:
         symbol.type = SymbolType::Section;
         break;
      case STT_FUNC:
         symbol.type = SymbolType::Function;

         // Relocate functions nearer to code
         if (impSection.virtualAddress != impSection.header.sh_addr) {
            symbol.value = impSection.virtualAddress + symbol.value - impSection.header.sh_addr;

            // Setup trampoline as sc, bclr
            auto addr = gMemory.translate(symbol.value);

            auto sc = gInstructionTable.encode(InstructionID::sc);
            sc.bd = i;

            auto bclr = gInstructionTable.encode(InstructionID::bclr);
            bclr.bo = 0x1f;

            gMemory.write(symbol.value, sc.value);
            gMemory.write(symbol.value + 4, bclr.value);
         }

         break;
      case STT_OBJECT:
         symbol.type = SymbolType::Object;
         break;
      default:
         xError() << "Unhandled symbol type " << type;
      }
   }

   return true;
}

static bool
loadRelocations(Binary &bin)
{
   for (auto &section : bin.sections) {
      assert(section.header.sh_type != SHT_REL);

      if (section.header.sh_type == SHT_RELA) {
         auto modifySection = bin.sections[section.header.sh_info];
         auto symbolSection = bin.sections[section.header.sh_link];
         auto in = BigEndianView { section.data.data(), section.data.size() };
         auto count = section.data.size() / sizeof(ElfRela);

         // TODO: Not sure if this is always true, but we can lazily assume so for now
         assert(symbolSection.name.compare(".symtab") == 0);

         for (auto i = 0u; i < count; ++i) {
            ElfRela rela;
            readRelocationAddend(in, rela);

            auto sym = rela.r_info >> 8;
            auto type = rela.r_info & 0xff;
            auto &symbol = bin.symbols[sym];
            auto value = symbol.value + rela.r_addend;

            // TODO: Use relocation address
            auto addr = gMemory.translate(rela.r_offset);
            auto ptr16 = reinterpret_cast<uint16_t*>(addr);
            auto ptr32 = reinterpret_cast<uint32_t*>(addr);

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
   }

   return true;
}

bool
Loader::loadElf(Binary &bin, const char *buffer, size_t size)
{
   auto in = BigEndianView { buffer, size };

   // Read header
   if (!readHeader(in, bin.header)) {
      xError() << "Failed to readHeader";
      return false;
   }

   // Read and decompress sections
   if (!readSections(in, bin)) {
      xError() << "Failed to readSections";
      return false;
   }

   // Copy sections to virtual memory
   if (!loadSections(bin)) {
      xError() << "Failed to loadSections";
      return false;
   }

   // Process symbols
   if (!loadSymbols(bin)) {
      xError() << "Failed to loadSymbols";
      return false;
   }

   // Process relocations
   if (!loadRelocations(bin)) {
      xError() << "Failed to loadRelocations";
      return false;
   }

   return true;
}
