#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

class KernelModule;
struct KernelFunction;
struct KernelData;
struct LoadedModule;

struct SymbolInfo
{
   enum Type
   {
      Invalid,
      Module,
      Function,
      Data
   };

   uint32_t index;
   Type type;
   std::string name;
   uint32_t address;
   uint32_t size;
};

// TODO: Support user loaded modules??
struct ModuleSymbol : public SymbolInfo
{
   enum ModuleType
   {
      System,
      User
   };

   ModuleSymbol() :
      systemModule(nullptr)
   {
      type = SymbolInfo::Module;
   }

   ModuleType moduleType;
   KernelModule *systemModule;
};

struct DataSymbol : public SymbolInfo
{
   enum DataType
   {
      Kernel,
      User
   };

   DataSymbol() :
      kernelData(nullptr)
   {
      type = SymbolInfo::Data;
   }

   DataType dataType;
   KernelData *kernelData;
};

struct FunctionSymbol : public SymbolInfo
{
   enum FunctionType
   {
      Kernel,
      User
   };

   FunctionSymbol() :
      kernelFunction(nullptr)
   {
      type = SymbolInfo::Function;
   }

   FunctionType functionType;
   KernelFunction *kernelFunction;
};

struct UserModule
{
   struct Section
   {
      enum Type
      {
         Invalid,
         Code,
         Data,
         CodeImports,
         DataImports
      };

      size_t index;
      std::string name;
      std::string library;
      Type type;
      uint32_t address;
      uint32_t size;
   };

   uint32_t maxCodeSize;
   uint32_t entryPoint;
   uint32_t defaultStackSize;
   std::pair<uint32_t, uint32_t> codeAddressRange;
   std::pair<uint32_t, uint32_t> dataAddressRange;
   std::vector<SymbolInfo*> symbols;
   std::vector<Section*> sections;
   std::map<std::string, Section *> sectionMap;
   uint32_t sdaBase;
   uint32_t sda2Base;

   inline Section *
   findSection(const std::string &name)
   {
      auto itr = sectionMap.find(name);

      if (itr == sectionMap.end()) {
         return nullptr;
      } else {
         return itr->second;
      }
   }

   inline Section *
   findSection(uint32_t address)
   {
      for (auto section : sections) {
         if (address >= section->address && address < section->address + section->size) {
            return section;
         }
      }

      return nullptr;
   }
};
