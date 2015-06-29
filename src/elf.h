#pragma once
#include <cstdint>

#pragma pack(push, 1)

enum // e_machine
{
   EM_PPC = 20 // PowerPC
};

enum // e_encoding
{
   ELFDATANONE = 0,
   ELFDATA2LSB = 1,
   ELFDATA2MSB = 2
};

enum // e_class
{
   ELFCLASSNONE = 0,
   ELFCLASS32 = 1,
   ELFCLASS64 = 2
};

enum // e_elf_version
{
   EV_NONE = 0,
   EV_CURRENT = 1,
};

enum // e_type
{
   ET_NONE = 0,         // No file type
   ET_REL = 1,          // Relocatable file
   ET_EXEC = 2,         // Executable file
   ET_DYN = 3,          // Shared object file
   ET_CORE = 4,         // Core file
   ET_LOPROC = 0xff00,  // Beginning of processor-specific codes
   ET_HIPROC = 0xffff   // Processor-specific
};

enum // e_abi
{
   EABI_CAFE = 0xcafe   // WiiU CafeOS
};

enum : uint32_t // sh_flags
{
   SHF_WRITE = 0x1,
   SHF_ALLOC = 0x2,
   SHF_EXECINSTR = 0x4,
   SHF_DEFLATED = 0x08000000,
   SHF_MASKPROC = 0xF0000000,
};

enum : uint32_t // sh_type
{
   SHT_NULL = 0,                 // No associated section (inactive entry).
   SHT_PROGBITS = 1,             // Program-defined contents.
   SHT_SYMTAB = 2,               // Symbol table.
   SHT_STRTAB = 3,               // String table.
   SHT_RELA = 4,                 // Relocation entries; explicit addends.
   SHT_HASH = 5,                 // Symbol hash table.
   SHT_DYNAMIC = 6,              // Information for dynamic linking.
   SHT_NOTE = 7,                 // Information about the file.
   SHT_NOBITS = 8,               // Data occupies no space in the file.
   SHT_REL = 9,                  // Relocation entries; no explicit addends.
   SHT_SHLIB = 10,               // Reserved.
   SHT_DYNSYM = 11,              // Symbol table.
   SHT_INIT_ARRAY = 14,          // Pointers to initialization functions.
   SHT_FINI_ARRAY = 15,          // Pointers to termination functions.
   SHT_PREINIT_ARRAY = 16,       // Pointers to pre-init functions.
   SHT_GROUP = 17,               // Section group.
   SHT_SYMTAB_SHNDX = 18,        // Indices for SHN_XINDEX entries.
   SHT_LOPROC = 0x70000000,      // Lowest processor arch-specific type.
   SHT_HIPROC = 0x7fffffff,      // Highest processor arch-specific type.
   SHT_LOUSER = 0x80000000,      // Lowest type reserved for applications.
   SHT_RPL_IMPORTS = 0x80000002, // RPL Imports
   SHT_RPL_CRCS = 0x80000003,    // RPL CRCs
   SHT_RPL_FILEINFO = 0x80000004,// RPL FileInfo
   SHT_HIUSER = 0xffffffff       // Highest type reserved for applications.
};

enum // st_info >> 4
{
   STB_LOCAL = 0,       // Local symbol, not visible outside obj file containing def
   STB_GLOBAL = 1,      // Global symbol, visible to all object files being combined
   STB_WEAK = 2,        // Weak symbol, like global but lower-precedence
   STB_GNU_UNIQUE = 10,
   STB_LOOS = 10,       // Lowest operating system-specific binding type
   STB_HIOS = 12,       // Highest operating system-specific binding type
   STB_LOPROC = 13,     // Lowest processor-specific binding type
   STB_HIPROC = 15      // Highest processor-specific binding type
};

enum // st_info & f
{
   STT_NOTYPE = 0,      // Symbol's type is not specified
   STT_OBJECT = 1,      // Symbol is a data object (variable, array, etc.)
   STT_FUNC = 2,        // Symbol is executable code (function, etc.)
   STT_SECTION = 3,     // Symbol refers to a section
   STT_FILE = 4,        // Local, absolute symbol that refers to a file
   STT_COMMON = 5,      // An uninitialized common block
   STT_TLS = 6,         // Thread local data object
   STT_LOOS = 7,        // Lowest operating system-specific symbol type
   STT_HIOS = 8,        // Highest operating system-specific symbol type
   STT_GNU_IFUNC = 10,  // GNU indirect function
   STT_LOPROC = 13,     // Lowest processor-specific symbol type
   STT_HIPROC = 15      // Highest processor-specific symbol type
};

enum : uint16_t // st_shndx
{
   SHN_UNDEF      = 0,        // Undefined
   SHN_LORESERVE  = 0xff00,   // Reserved range
   SHN_ABS        = 0xfff1,   // Absolute symbols
   SHN_COMMON     = 0xfff2,   // Common symbols
   SHN_XINDEX     = 0xffff,   // Escape -- index stored elsewhere
   SHN_HIRESERVE  = 0xffff
};

enum // r_info & 0xff
{
   R_PPC_NONE = 0,
   R_PPC_ADDR32 = 1,
   R_PPC_ADDR24 = 2,
   R_PPC_ADDR16 = 3,
   R_PPC_ADDR16_LO = 4,
   R_PPC_ADDR16_HI = 5,
   R_PPC_ADDR16_HA = 6,
   R_PPC_ADDR14 = 7,
   R_PPC_ADDR14_BRTAKEN = 8,
   R_PPC_ADDR14_BRNTAKEN = 9,
   R_PPC_REL24 = 10,
   R_PPC_REL14 = 11,
   R_PPC_REL14_BRTAKEN = 12,
   R_PPC_REL14_BRNTAKEN = 13,
   R_PPC_GOT16 = 14,
   R_PPC_GOT16_LO = 15,
   R_PPC_GOT16_HI = 16,
   R_PPC_GOT16_HA = 17,
   R_PPC_PLTREL24 = 18,
   R_PPC_JMP_SLOT = 21,
   R_PPC_LOCAL24PC = 23,
   R_PPC_REL32 = 26,
   R_PPC_TLS = 67,
   R_PPC_DTPMOD32 = 68,
   R_PPC_TPREL16 = 69,
   R_PPC_TPREL16_LO = 70,
   R_PPC_TPREL16_HI = 71,
   R_PPC_TPREL16_HA = 72,
   R_PPC_TPREL32 = 73,
   R_PPC_DTPREL16 = 74,
   R_PPC_DTPREL16_LO = 75,
   R_PPC_DTPREL16_HI = 76,
   R_PPC_DTPREL16_HA = 77,
   R_PPC_DTPREL32 = 78,
   R_PPC_GOT_TLSGD16 = 79,
   R_PPC_GOT_TLSGD16_LO = 80,
   R_PPC_GOT_TLSGD16_HI = 81,
   R_PPC_GOT_TLSGD16_HA = 82,
   R_PPC_GOT_TLSLD16 = 83,
   R_PPC_GOT_TLSLD16_LO = 84,
   R_PPC_GOT_TLSLD16_HI = 85,
   R_PPC_GOT_TLSLD16_HA = 86,
   R_PPC_GOT_TPREL16 = 87,
   R_PPC_GOT_TPREL16_LO = 88,
   R_PPC_GOT_TPREL16_HI = 89,
   R_PPC_GOT_TPREL16_HA = 90,
   R_PPC_GOT_DTPREL16 = 91,
   R_PPC_GOT_DTPREL16_LO = 92,
   R_PPC_GOT_DTPREL16_HI = 93,
   R_PPC_GOT_DTPREL16_HA = 94,
   R_PPC_TLSGD = 95,
   R_PPC_TLSLD = 96,
   R_PPC_REL16 = 249,
   R_PPC_REL16_LO = 250,
   R_PPC_REL16_HI = 251,
   R_PPC_REL16_HA = 252,
};

struct ElfHeader
{
   static const unsigned Magic = 0x7f454c46;

   uint32_t e_magic;       // File identification.
   uint8_t e_class;        // File class.
   uint8_t e_encoding;     // Data encoding.
   uint8_t e_elf_version;  // File version.
   uint16_t e_abi;         // OS/ABI identification. (EABI_*)
   uint8_t e_pad[7];

   uint16_t e_type;        // Type of file (ET_*)
   uint16_t e_machine;     // Required architecture for this file (EM_*)
   uint32_t e_version;     // Must be equal to 1
   uint32_t e_entry;       // Address to jump to in order to start program
   uint32_t e_phoff;       // Program header table's file offset, in bytes
   uint32_t e_shoff;       // Section header table's file offset, in bytes
   uint32_t e_flags;       // Processor-specific flags
   uint16_t e_ehsize;      // Size of ELF header, in bytes
   uint16_t e_phentsize;   // Size of an entry in the program header table
   uint16_t e_phnum;       // Number of entries in the program header table
   uint16_t e_shentsize;   // Size of an entry in the section header table
   uint16_t e_shnum;       // Number of entries in the section header table
   uint16_t e_shstrndx;    // Sect hdr table index of sect name string table
};

struct ElfSectionHeader
{
   uint32_t sh_name;      // Section name (index into string table)
   uint32_t sh_type;      // Section type (SHT_*)
   uint32_t sh_flags;     // Section flags (SHF_*)
   uint32_t sh_addr;      // Address where section is to be loaded
   uint32_t sh_offset;    // File offset of section data, in bytes
   uint32_t sh_size;      // Size of section, in bytes
   uint32_t sh_link;      // Section type-specific header table index link
   uint32_t sh_info;      // Section type-specific extra information
   uint32_t sh_addralign; // Section address alignment
   uint32_t sh_entsize;   // Size of records contained within the section
};

struct ElfSymbol
{
   uint32_t st_name;  // Symbol name (index into string table)
   uint32_t st_value; // Value or address associated with the symbol
   uint32_t st_size;  // Size of the symbol
   uint8_t  st_info;  // Symbol's type and binding attributes
   uint8_t  st_other; // Must be zero; reserved
   uint16_t st_shndx; // Which section (header table index) it's defined in (SHN_*)
};

struct ElfRela
{
   uint32_t r_offset;
   uint32_t r_info;
   int32_t r_addend;
};

struct RplFileInfo
{
   uint32_t rpl_min_version;
   uint32_t rpl_text_size;
   uint32_t rpl_text_align;
   uint32_t rpl_data_size;
   uint32_t rpl_data_align;
   uint32_t rpl_loader_size;
   uint32_t rpl_loader_info;
   uint32_t rpl_temp_size;
   uint32_t rpl_tramp_adjust; // uint16_t adjust / uint16_t addition
   uint32_t rpl_sda_base;
   uint32_t rpl_sda2_base;
   uint32_t rpl_stack_size;
   uint32_t rpl_filename;
   uint32_t rpl_flags;
   uint32_t rpl_heap_size;
   uint32_t rpl_tags; // Offset to TAGS section, where is TAGS tho?
   uint32_t rpl_unk1;
   uint32_t rpl_compression_level;
   uint32_t rpl_unk2;
   uint32_t rpl_file_info_pad;
   uint32_t rpl_cafe_os_sdk_version;
   uint32_t rpl_cafe_os_sdk_revision;
   uint32_t rpl_unk3;
   uint32_t rpl_unk4;
};

struct RplFile
{
   ElfHeader header;
   std::vector<ElfSectionHeader> sections;
   std::vector<ElfSymbol> symbols;
   std::vector<ElfRela> rela;
   RplFileInfo fileInfo;
   std::vector<uint32_t> crcs;
};

#pragma pack(pop)
