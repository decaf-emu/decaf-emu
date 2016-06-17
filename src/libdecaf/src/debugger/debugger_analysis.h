#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace debugger
{

namespace analysis
{

struct FuncData
{
   uint32_t start;
   uint32_t end;

   std::string name;
};

struct InstrData
{
   // Addresses of instructions which jump to this one
   std::vector<uint32_t> sourceBranches;

   // User-left comments
   std::string comments;
};

struct InstrInfo
{
   FuncData *func;
   InstrData *instr;
};

InstrInfo get(uint32_t address);
void markAsFunction(uint32_t address);
void toggleAsFunction(uint32_t address);
void analyse(uint32_t start, uint32_t end);

} // namespace analysis

} // namespace debugger
