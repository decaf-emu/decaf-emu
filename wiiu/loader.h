#pragma once
#include <string>
#include <vector>
#include "elf.h"

struct Section
{
   unsigned index;
   std::string name;
   ElfSectionHeader header;
   std::vector<char> data;
   size_t virtualAddress;
};

enum class SymbolType
{
   Unknown,
   Section,
   Function,
   Object
};

struct Symbol
{
   ElfSymbol header;
   std::string name;
   uint32_t value;
   SymbolType type;
};

struct Binary
{
   ElfHeader header;
   std::vector<Symbol> symbols;
   std::vector<Section> sections;

   Section *findSection(const std::string &name)
   {
      for (auto &section : sections) {
         if (section.name == name) {
            return &section;
         }
      }

      return nullptr;
   }
};

class Loader
{
public:
   bool loadElf(Binary &binary, const char *buffer, size_t size);

private:
};
