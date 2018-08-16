#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::hle
{

struct BaseTypeDescriptor;
struct TypeDescriptor;
struct VirtualTable;

struct BaseTypeDescriptor
{
   be2_virt_ptr<TypeDescriptor> typeDescriptor;
   be2_val<uint32_t> flags;
};
CHECK_OFFSET(BaseTypeDescriptor, 0x00, typeDescriptor);
CHECK_OFFSET(BaseTypeDescriptor, 0x04, flags);
CHECK_SIZE(BaseTypeDescriptor, 0x08);

struct TypeDescriptor
{
   //! Pointer to virtual table for std::typeinfo
   be2_virt_ptr<VirtualTable> typeInfoVTable;

   //! Name of this type
   be2_virt_ptr<const char> name;

   //! Unique ID for this type
   be2_val<uint32_t> typeID;

   //! Pointer to a list of base types, the last base type has flags=0x1600
   be2_virt_ptr<BaseTypeDescriptor> baseTypes;
};
CHECK_OFFSET(TypeDescriptor, 0x00, typeInfoVTable);
CHECK_OFFSET(TypeDescriptor, 0x04, name);
CHECK_OFFSET(TypeDescriptor, 0x08, typeID);
CHECK_OFFSET(TypeDescriptor, 0x0C, baseTypes);
CHECK_SIZE(TypeDescriptor, 0x10);

struct VirtualTable
{
   be2_val<uint32_t> flags;
   be2_virt_ptr<void> ptr;
};
CHECK_OFFSET(VirtualTable, 0x00, flags);
CHECK_OFFSET(VirtualTable, 0x04, ptr);
CHECK_SIZE(VirtualTable, 0x08);

struct LibraryTypeInfo
{
   const char *name = nullptr;
   std::vector<const char *> virtualTable;
   std::vector<const char *> baseTypes;
   virt_ptr<VirtualTable> *hostVirtualTablePtr = nullptr;
   virt_ptr<TypeDescriptor> *hostTypeDescriptorPtr = nullptr;

   uint32_t nameOffset = 0u;
   uint32_t baseTypeOffset = 0u;
   uint32_t typeDescriptorOffset = 0u;
   uint32_t typeIdOffset = 0u;
   uint32_t virtualTableOffset = 0u;
};

} // namespace cafe::hle
