#pragma once
#include "common/be_val.h"
#include "common/virtual_ptr.h"

namespace ghs
{

struct VirtualTableEntry;
struct TypeDescriptor;

struct BaseTypeDescriptor
{
   be_ptr<TypeDescriptor> typeDescriptor;
   be_val<uint32_t> flags;
};

struct TypeDescriptor
{
   be_ptr<VirtualTableEntry> typeInfoVTable;
   be_ptr<char> name;
   uint32_t typeID;
   be_ptr<BaseTypeDescriptor> baseTypes;
};

struct VirtualTableEntry
{
   be_val<uint32_t> flags;
   be_ptr<void> ptr;
};

class type_info
{
public:
   static VirtualTableEntry *VirtualTable;
   static TypeDescriptor *TypeInfo;

public:
   ~type_info();
};

extern void *
PureVirtualCall;

namespace internal
{

ghs::TypeDescriptor *
makeTypeDescriptor(const std::string &name);

ghs::TypeDescriptor *
makeTypeDescriptor(const std::string &name, std::initializer_list<ghs::BaseTypeDescriptor> bases);

ghs::VirtualTableEntry *
makeVirtualTable(std::initializer_list<ghs::VirtualTableEntry> entries);

} // namespace internal

} // namespace ghs
