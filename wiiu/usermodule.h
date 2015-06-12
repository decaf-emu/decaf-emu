#pragma once
#include <cstdint>
#include <string>
#include <vector>

class SystemModule;
struct SystemFunction;
struct SystemData;

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
   SystemModule *systemModule;
};

struct DataSymbol : public SymbolInfo
{
   enum DataType
   {
      System,
      User
   };

   DataSymbol() :
      systemData(nullptr)
   {
      type = SymbolInfo::Data;
   }

   DataType dataType;
   SystemData *systemData;
};

struct FunctionSymbol : public SymbolInfo
{
   static const uint32_t Unimplemented = 0x2000;

   enum FunctionType
   {
      System,
      User
   };

   FunctionSymbol() :
      systemFunction(nullptr)
   {
      type = SymbolInfo::Function;
   }

   FunctionType functionType;
   SystemFunction *systemFunction;
};

struct UserModule
{
   std::pair<uint32_t, uint32_t> codeAddressRange;
   std::pair<uint32_t, uint32_t> dataAddressRange;
   std::vector<SymbolInfo*> symbols;
   std::vector<Section*> sections;
};
