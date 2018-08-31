#pragma once
#include <libcpu/be2_struct.h>

#pragma pack(push, 1)

namespace cafe::loader::rpl
{

enum Machine : uint16_t
{
   EM_PPC = 20,
};

enum Encoding : uint8_t
{
   ELFDATANONE = 0,
   ELFDATA2LSB = 1,
   ELFDATA2MSB = 2,
};

enum Class : uint8_t
{
   ELFCLASS32 = 1,
};

enum : uint8_t
{
   EABI_CAFE = 0xCA,
};

enum : uint8_t
{
   EABI_VERSION_CAFE = 0xFE,
};

enum Version : uint8_t
{
   EV_NONE = 0,
   EV_CURRENT = 1,
};

enum SectionFlags : uint32_t
{
   SHF_WRITE      = 0x1,
   SHF_ALLOC      = 0x2,
   SHF_EXECINSTR  = 0x4,
   SHF_TLS        = 0x04000000,
   SHF_DEFLATED   = 0x08000000,
   SHF_MASKPROC   = 0xF0000000,
};

enum SectionType : uint32_t
{
   SHT_NULL = 0,
   SHT_PROGBITS = 1,
   SHT_SYMTAB = 2,
   SHT_STRTAB = 3,
   SHT_RELA = 4,
   SHT_HASH = 5,
   SHT_DYNAMIC = 6,
   SHT_NOTE = 7,
   SHT_NOBITS = 8,
   SHT_REL = 9,
   SHT_SHLIB = 10,
   SHT_DYNSYM = 11,
   SHT_INIT_ARRAY = 14,
   SHT_FINI_ARRAY = 15,
   SHT_PREINIT_ARRAY = 16,
   SHT_GROUP = 17,
   SHT_SYMTAB_SHNDX = 18,
   SHT_LOPROC = 0x70000000u,
   SHT_HIPROC = 0x7fffffffu,
   SHT_LOUSER = 0x80000000u,
   SHT_RPL_EXPORTS = 0x80000001u,
   SHT_RPL_IMPORTS = 0x80000002u,
   SHT_RPL_CRCS = 0x80000003u,
   SHT_RPL_FILEINFO = 0x80000004u,
   SHT_HIUSER = 0xffffffffu,
};

enum SymbolBinding : uint32_t
{
   STB_LOCAL = 0,
   STB_GLOBAL = 1,
   STB_WEAK = 2,
   STB_GNU_UNIQUE = 10,
   STB_LOOS = 10,
   STB_HIOS = 12,
   STB_LOPROC = 13,
   STB_HIPROC = 15,
};

enum SymbolType : uint32_t
{
   STT_NOTYPE = 0,
   STT_OBJECT = 1,
   STT_FUNC = 2,
   STT_SECTION = 3,
   STT_FILE = 4,
   STT_COMMON = 5,
   STT_TLS = 6,
   STT_LOOS = 7,
   STT_HIOS = 8,
   STT_GNU_IFUNC = 10,
   STT_LOPROC = 13,
   STT_HIPROC = 15,
};

enum SectionIndex : uint16_t
{
   SHN_UNDEF = 0,
   SHN_LORESERVE = 0xff00,
   SHN_ABS = 0xfff1,
   SHN_COMMON = 0xfff2,
   SHN_XINDEX = 0xffff,
   SHN_HIRESERVE = 0xffff
};

enum RelocationType : uint32_t
{
   R_PPC_NONE = 0,
   R_PPC_ADDR32 = 1,
   R_PPC_ADDR16_LO = 4,
   R_PPC_ADDR16_HI = 5,
   R_PPC_ADDR16_HA = 6,
   R_PPC_REL24 = 10,
   R_PPC_REL14 = 11,
   R_PPC_DTPMOD32 = 68,
   R_PPC_DTPREL32 = 78,
   R_PPC_EMB_SDA21 = 109,
   R_PPC_EMB_RELSDA = 116,
   R_PPC_DIAB_SDA21_LO = 180,
   R_PPC_DIAB_SDA21_HI = 181,
   R_PPC_DIAB_SDA21_HA = 182,
   R_PPC_DIAB_RELSDA_LO = 183,
   R_PPC_DIAB_RELSDA_HI = 184,
   R_PPC_DIAB_RELSDA_HA = 185,
   R_PPC_GHS_REL16_HA = 251,
   R_PPC_GHS_REL16_HI = 252,
   R_PPC_GHS_REL16_LO = 253,
};

enum FileInfoFlags : uint32_t
{
   RPL_IS_RPX     = 1 << 1,
   RPL_FLAG_4     = 1 << 2,
   RPL_HAS_TLS    = 1 << 3,
};

struct Header
{
   be2_array<uint8_t, 4> magic;   // File identification.
   be2_val<uint8_t> fileClass;    // File class.
   be2_val<uint8_t> encoding;     // Data encoding.
   be2_val<uint8_t> elfVersion;   // File version.
   be2_val<uint8_t> abi;          // OS/ABI identification.
   be2_val<uint8_t> abiVersion;   // OS/ABI version.
   be2_array<uint8_t, 7> pad;

   be2_val<uint16_t> type;        // Type of file (ET_*)
   be2_val<uint16_t> machine;     // Required architecture for this file (EM_*)
   be2_val<uint32_t> version;     // Must be equal to 1
   be2_val<uint32_t> entry;       // Address to jump to in order to start program
   be2_val<uint32_t> phoff;       // Program header table's file offset, in bytes
   be2_val<uint32_t> shoff;       // Section header table's file offset, in bytes
   be2_val<uint32_t> flags;       // Processor-specific flags
   be2_val<uint16_t> ehsize;      // Size of ELF header, in bytes
   be2_val<uint16_t> phentsize;   // Size of an entry in the program header table
   be2_val<uint16_t> phnum;       // Number of entries in the program header table
   be2_val<uint16_t> shentsize;   // Size of an entry in the section header table
   be2_val<uint16_t> shnum;       // Number of entries in the section header table
   be2_val<uint16_t> shstrndx;    // Sect hdr table index of sect name string table
};
CHECK_OFFSET(Header, 0x00, magic);
CHECK_OFFSET(Header, 0x04, fileClass);
CHECK_OFFSET(Header, 0x05, encoding);
CHECK_OFFSET(Header, 0x06, elfVersion);
CHECK_OFFSET(Header, 0x07, abi);
CHECK_OFFSET(Header, 0x08, abiVersion);
CHECK_OFFSET(Header, 0x10, type);
CHECK_OFFSET(Header, 0x12, machine);
CHECK_OFFSET(Header, 0x14, version);
CHECK_OFFSET(Header, 0x18, entry);
CHECK_OFFSET(Header, 0x1C, phoff);
CHECK_OFFSET(Header, 0x20, shoff);
CHECK_OFFSET(Header, 0x24, flags);
CHECK_OFFSET(Header, 0x28, ehsize);
CHECK_OFFSET(Header, 0x2A, phentsize);
CHECK_OFFSET(Header, 0x2C, phnum);
CHECK_OFFSET(Header, 0x2E, shentsize);
CHECK_OFFSET(Header, 0x30, shnum);
CHECK_OFFSET(Header, 0x32, shstrndx);
CHECK_SIZE(Header, 0x34);

struct ProgramHeader
{
   be2_val<uint32_t> type;
   be2_val<uint32_t> offset;
   be2_val<uint32_t> vaddr;
   be2_val<uint32_t> paddr;
   be2_val<uint32_t> filesz;
   be2_val<uint32_t> memsz;
   be2_val<uint32_t> flags;
   be2_val<uint32_t> align;
};
CHECK_OFFSET(ProgramHeader, 0x00, type);
CHECK_OFFSET(ProgramHeader, 0x04, offset);
CHECK_OFFSET(ProgramHeader, 0x08, vaddr);
CHECK_OFFSET(ProgramHeader, 0x0C, paddr);
CHECK_OFFSET(ProgramHeader, 0x10, filesz);
CHECK_OFFSET(ProgramHeader, 0x14, memsz);
CHECK_OFFSET(ProgramHeader, 0x18, flags);
CHECK_OFFSET(ProgramHeader, 0x1C, align);
CHECK_SIZE(ProgramHeader, 0x20);

struct SectionHeader
{
   //! Section name (index into string table)
   be2_val<uint32_t> name;

   //! Section type (SHT_*)
   be2_val<uint32_t> type;

   //! Section flags (SHF_*)
   be2_val<uint32_t> flags;

   //! Address where section is to be loaded
   be2_val<uint32_t> addr;

   //! File offset of section data, in bytes
   be2_val<uint32_t> offset;

   //! Size of section, in bytes
   be2_val<uint32_t> size;

   //! Section type-specific header table index link
   be2_val<uint32_t> link;

   //! Section type-specific extra information
   be2_val<uint32_t> info;

   //! Section address alignment
   be2_val<uint32_t> addralign;

   //! Size of records contained within the section
   be2_val<uint32_t> entsize;
};
CHECK_OFFSET(SectionHeader, 0x00, name);
CHECK_OFFSET(SectionHeader, 0x04, type);
CHECK_OFFSET(SectionHeader, 0x08, flags);
CHECK_OFFSET(SectionHeader, 0x0C, addr);
CHECK_OFFSET(SectionHeader, 0x10, offset);
CHECK_OFFSET(SectionHeader, 0x14, size);
CHECK_OFFSET(SectionHeader, 0x18, link);
CHECK_OFFSET(SectionHeader, 0x1C, info);
CHECK_OFFSET(SectionHeader, 0x20, addralign);
CHECK_OFFSET(SectionHeader, 0x24, entsize);
CHECK_SIZE(SectionHeader, 0x28);

struct Symbol
{
   //! Symbol name (index into string table)
   be2_val<uint32_t> name;

   //! Value or address associated with the symbol
   be2_val<uint32_t> value;

   //! Size of the symbol
   be2_val<uint32_t> size;

   //! Symbol's type and binding attributes
   be2_val<uint8_t> info;

   //! Must be zero; reserved
   be2_val<uint8_t> other;

   //! Which section (header table index) it's defined in (SHN_*)
   be2_val<uint16_t> shndx;
};
CHECK_OFFSET(Symbol, 0x00, name);
CHECK_OFFSET(Symbol, 0x04, value);
CHECK_OFFSET(Symbol, 0x08, size);
CHECK_OFFSET(Symbol, 0x0C, info);
CHECK_OFFSET(Symbol, 0x0D, other);
CHECK_OFFSET(Symbol, 0x0E, shndx);
CHECK_SIZE(Symbol, 0x10);

struct Rela
{
   be2_val<uint32_t> offset;
   be2_val<uint32_t> info;
   be2_val<int32_t> addend;
};
CHECK_OFFSET(Rela, 0x00, offset);
CHECK_OFFSET(Rela, 0x04, info);
CHECK_OFFSET(Rela, 0x08, addend);
CHECK_SIZE(Rela, 0x0C);

struct Imports
{
   be2_val<uint32_t> count;
   be2_val<uint32_t> signature;
};
CHECK_OFFSET(Imports, 0x00, count);
CHECK_OFFSET(Imports, 0x04, signature);
CHECK_SIZE(Imports, 0x08);

struct Exports
{
   be2_val<uint32_t> count;
   be2_val<uint32_t> signature;
};
CHECK_OFFSET(Exports, 0x00, count);
CHECK_OFFSET(Exports, 0x04, signature);
CHECK_SIZE(Exports, 0x08);

struct Export
{
   be2_val<uint32_t> value;
   be2_val<uint32_t> name;
};
CHECK_OFFSET(Export, 0x00, value);
CHECK_OFFSET(Export, 0x04, name);
CHECK_SIZE(Export, 0x08);

struct DeflatedHeader
{
   be2_val<uint32_t> inflatedSize;
};
CHECK_OFFSET(DeflatedHeader, 0x00, inflatedSize);
CHECK_SIZE(DeflatedHeader, 0x04);

struct RplCrc
{
   be2_val<uint32_t> crc;
};
CHECK_OFFSET(RplCrc, 0x00, crc);
CHECK_SIZE(RplCrc, 0x04);

struct RPLFileInfo_v3_0
{
   static constexpr auto Version = 0xCAFE0300u;
   be2_val<uint32_t> version;
   be2_val<uint32_t> textSize;
   be2_val<uint32_t> textAlign;
   be2_val<uint32_t> dataSize;
   be2_val<uint32_t> dataAlign;
   be2_val<uint32_t> loadSize;
   be2_val<uint32_t> loadAlign;
   be2_val<uint32_t> tempSize;
   be2_val<uint32_t> trampAdjust;
   be2_val<uint32_t> sdaBase;
   be2_val<uint32_t> sda2Base;
   be2_val<uint32_t> stackSize;
   be2_val<uint32_t> filename;
   UNKNOWN(0x0C);
};
CHECK_OFFSET(RPLFileInfo_v3_0, 0x00, version);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x04, textSize);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x08, textAlign);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x0C, dataSize);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x10, dataAlign);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x14, loadSize);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x18, loadAlign);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x1C, tempSize);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x20, trampAdjust);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x24, sdaBase);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x28, sda2Base);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x2C, stackSize);
CHECK_OFFSET(RPLFileInfo_v3_0, 0x30, filename);
CHECK_SIZE(RPLFileInfo_v3_0, 0x40);

struct RPLFileInfo_v4_1
{
   static constexpr auto Version = 0xCAFE0401u;
   be2_val<uint32_t> version;
   be2_val<uint32_t> textSize;
   be2_val<uint32_t> textAlign;
   be2_val<uint32_t> dataSize;
   be2_val<uint32_t> dataAlign;
   be2_val<uint32_t> loadSize;
   be2_val<uint32_t> loadAlign;
   be2_val<uint32_t> tempSize;
   be2_val<uint32_t> trampAdjust;
   be2_val<uint32_t> sdaBase;
   be2_val<uint32_t> sda2Base;
   be2_val<uint32_t> stackSize;
   be2_val<uint32_t> filename;
   be2_val<uint32_t> flags;
   be2_val<uint32_t> heapSize;
   be2_val<uint32_t> tagOffset;
};
CHECK_OFFSET(RPLFileInfo_v4_1, 0x00, version);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x04, textSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x08, textAlign);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x0C, dataSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x10, dataAlign);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x14, loadSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x18, loadAlign);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x1C, tempSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x20, trampAdjust);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x24, sdaBase);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x28, sda2Base);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x2C, stackSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x30, filename);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x34, flags);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x38, heapSize);
CHECK_OFFSET(RPLFileInfo_v4_1, 0x3C, tagOffset);
CHECK_SIZE(RPLFileInfo_v4_1, 0x40);

struct RPLFileInfo_v4_2
{
   static constexpr auto Version = 0xCAFE0402u;
   be2_val<uint32_t> version;
   be2_val<uint32_t> textSize;
   be2_val<uint32_t> textAlign;
   be2_val<uint32_t> dataSize;
   be2_val<uint32_t> dataAlign;
   be2_val<uint32_t> loadSize;
   be2_val<uint32_t> loadAlign;
   be2_val<uint32_t> tempSize;
   be2_val<uint32_t> trampAdjust;
   be2_val<uint32_t> sdaBase;
   be2_val<uint32_t> sda2Base;
   be2_val<uint32_t> stackSize;
   be2_val<uint32_t> filename;
   be2_val<uint32_t> flags;
   be2_val<uint32_t> heapSize;
   be2_val<uint32_t> tagOffset;
   be2_val<uint32_t> minVersion;
   be2_val<int32_t> compressionLevel;
   be2_val<uint32_t> trampAddition;
   be2_val<uint32_t> fileInfoPad;
   be2_val<uint32_t> cafeSdkVersion;
   be2_val<uint32_t> cafeSdkRevision;
   be2_val<int16_t> tlsModuleIndex;
   be2_val<uint16_t> tlsAlignShift;
   be2_val<uint32_t> runtimeFileInfoSize;
};
CHECK_OFFSET(RPLFileInfo_v4_2, 0x00, version);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x04, textSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x08, textAlign);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x0C, dataSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x10, dataAlign);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x14, loadSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x18, loadAlign);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x1C, tempSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x20, trampAdjust);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x24, sdaBase);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x28, sda2Base);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x2C, stackSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x30, filename);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x34, flags);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x38, heapSize);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x3C, tagOffset);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x40, minVersion);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x44, compressionLevel);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x48, trampAddition);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x4C, fileInfoPad);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x50, cafeSdkVersion);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x54, cafeSdkRevision);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x58, tlsModuleIndex);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x5A, tlsAlignShift);
CHECK_OFFSET(RPLFileInfo_v4_2, 0x5C, runtimeFileInfoSize);
CHECK_SIZE(RPLFileInfo_v4_2, 0x60);

} // namespace cafe::loader::rpl

#pragma pack(pop)
