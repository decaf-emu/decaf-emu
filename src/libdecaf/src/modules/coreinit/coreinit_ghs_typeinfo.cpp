#include "coreinit.h"
#include "coreinit_ghs_typeinfo.h"
#include "coreinit_memheap.h"

namespace ghs
{

VirtualTableEntry *
type_info::VirtualTable = nullptr;

TypeDescriptor *
type_info::TypeInfo = nullptr;

void *
PureVirtualCall = nullptr;

type_info::~type_info()
{
}

static void
pure_virtual_called()
{
   gLog->error("Pure virtual called");
}

namespace internal
{

static uint32_t
getUniqueTypeID()
{
   static uint32_t typeID = 1;
   return typeID++;
}

ghs::TypeDescriptor *
makeTypeDescriptor(const std::string &name)
{
   auto result = coreinit::internal::sysAlloc<ghs::TypeDescriptor>();
   result->typeInfoVTable = ghs::type_info::VirtualTable;
   result->typeID = getUniqueTypeID();
   result->name = coreinit::internal::sysStrDup(name);
   result->baseTypes = nullptr;
   return result;
}

ghs::TypeDescriptor *
makeTypeDescriptor(const std::string &name, std::initializer_list<ghs::BaseTypeDescriptor> bases)
{
   auto baseTypes = reinterpret_cast<ghs::BaseTypeDescriptor *>(coreinit::internal::sysAlloc(bases.size() * sizeof(ghs::BaseTypeDescriptor)));
   auto idx = 0u;

   for (auto &entry : bases) {
      if (!entry.typeDescriptor) {
         throw std::logic_error("Attempted to intialise type info before initialising a base class type info.");
      }

      baseTypes[idx].flags = entry.flags;
      baseTypes[idx].typeDescriptor = entry.typeDescriptor;
   }

   auto result = makeTypeDescriptor(name);
   result->baseTypes = baseTypes;
   return result;
}

ghs::VirtualTableEntry *
makeVirtualTable(std::initializer_list<ghs::VirtualTableEntry> entries)
{
   auto vtable = reinterpret_cast<ghs::VirtualTableEntry *>(coreinit::internal::sysAlloc(entries.size() * sizeof(ghs::VirtualTableEntry)));
   auto idx = 0u;

   for (auto &entry : entries) {
      vtable[idx].flags = entry.flags;
      vtable[idx].ptr = entry.ptr;
   }

   return vtable;
}

} // namespace internal

} // namespace ghs

namespace coreinit
{

void
Module::registerGhsTypeInfoFunctions()
{
   RegisterKernelFunctionDestructor("__dt__Q2_3std9type_infoFv", ghs::type_info);
   RegisterKernelFunctionName("__pure_virtual_called", ghs::pure_virtual_called);
}

void
Module::initialiseGhsTypeInfo()
{
   ghs::type_info::VirtualTable = reinterpret_cast<ghs::VirtualTableEntry *>(coreinit::internal::sysAlloc(sizeof(ghs::VirtualTableEntry) * 2));
   ghs::type_info::TypeInfo = ghs::internal::makeTypeDescriptor("std::type_info");

   ghs::type_info::VirtualTable[0] = { 0, ghs::type_info::TypeInfo };
   ghs::type_info::VirtualTable[1] = { 0, findExportAddress("__dt__Q2_3std9type_infoFv") };

   ghs::PureVirtualCall = findExportAddress("__pure_virtual_called");
}

} // namespace coreinit
