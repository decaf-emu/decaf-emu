#include <cassert>
#include "instructiondata.h"
#include "utils/bitutils.h"

InstructionTable gInstructionTable;

struct TableEntry
{
   struct FieldMap
   {
      Field field;
      std::vector<TableEntry> children;
   };

   void addInstruction(Field field, uint32_t value, InstructionData *instr)
   {
      auto fieldMap = getFieldMap(field);
      assert(fieldMap);
      assert(value < fieldMap->children.size());
      fieldMap->children[value].instr = instr;
   }

   void addTable(Field field)
   {
      if (!getFieldMap(field)) {
         fieldMaps.push_back({});
         auto &fieldMap = fieldMaps.back();
         auto size = 1 << getFieldWidth(field);
         fieldMap.field = field;
         fieldMap.children.resize(size);
      }
   }

   TableEntry *getEntry(Field field, uint32_t value)
   {
      auto fieldMap = getFieldMap(field);
      assert(fieldMap);
      assert(value < fieldMap->children.size());
      return &fieldMap->children[value];
   }

   FieldMap *getFieldMap(Field field)
   {
      for (auto &fieldMap : fieldMaps) {
         if (fieldMap.field == field) {
            return &fieldMap;
         }
      }

      return nullptr;
   }

   InstructionData *instr = nullptr;
   std::vector<FieldMap> fieldMaps;
};

static std::vector<InstructionData> instructionData;
static std::vector<InstructionAlias> aliasData;
static TableEntry instructionTable;

static void
initData();

static void
initTable();

#define FLD(x, y, z, ...) {y, z},
#define MRKR(x, ...) {-1, -1},
static BitRange gFieldBits[] = {
   { -1, -1 },
   #include "instructionfields.inl"
};
#undef FLD
#undef MRKR

#define FLD(x, ...) { #x },
#define MRKR(x, ...) { #x },
static const char * gFieldNames[] = {
#include "instructionfields.inl"
};
#undef FLD
#undef MRKR

bool
isFieldMarker(Field field)
{
   return gFieldBits[static_cast<int>(field)].start == -1;
}

const char * getFieldName(Field field) {
   return gFieldNames[static_cast<int>(field)];
}

// First bit of instruction field
uint32_t
getFieldStart(Field field)
{
   return 31 - gFieldBits[static_cast<int>(field)].end;
}

// Last bit of instruction field
uint32_t
getFieldEnd(Field field)
{
   return 31 - gFieldBits[static_cast<int>(field)].start;
}

// Width of instruction field in bits
uint32_t
getFieldWidth(Field field)
{
   return getFieldEnd(field) - getFieldStart(field) + 1;
}

// Absolute bitmask of instruction field
uint32_t
getFieldBitmask(Field field)
{
   return make_bitmask(getFieldStart(field), getFieldEnd(field));
}

uint32_t getFieldValue(const Field& field, Instruction instr)
{
   auto mask = getFieldBitmask(field);
   auto start = getFieldStart(field);
   auto res = (instr & mask) >> start;
   if (field == Field::spr) {
      res = ((res << 5) & 0x3E0) | ((res >> 5) & 0x1F);
   }
   return res;
}

InstructionData *
InstructionTable::find(InstructionID instrId)
{
   return &instructionData[static_cast<size_t>(instrId)];
}

// Decode Instruction to InstructionData
InstructionData *
InstructionTable::decode(Instruction instr)
{
   TableEntry *table = &instructionTable;
   InstructionData *data = nullptr;

   while (table) {
      for (auto &fieldMap : table->fieldMaps) {
         auto value = getFieldValue(fieldMap.field, instr);
         table = &fieldMap.children[value];

         if (table->instr || table->fieldMaps.size()) {
            break;
         }
      }

      if (table->fieldMaps.size() == 0) {
         return table->instr;
      }
   }

   return nullptr;
}

InstructionAlias *
InstructionTable::findAlias(InstructionData *data, Instruction instr)
{
   for (auto &alias : aliasData) {
      if (alias.id != data->id) {
         // Not an alias for this field
         continue;
      }

      bool opMatch = true;
      for (auto &op : alias.opcode) {
         uint32_t x = getFieldValue(op.field, instr);
         uint32_t y = op.value;
         if (op.field2 != Field::Invalid) {
            y = getFieldValue(op.field2, instr);
         }

         if (x != y) {
            opMatch = false;
            break;
         }
      }

      if (opMatch) {
         return &alias;
      }
   }

   return nullptr;
}

// Check if is a specific instruction
bool
InstructionTable::isA(InstructionID id, Instruction instr)
{
   auto &data = instructionData[static_cast<size_t>(id)];

   for (auto &op : data.opcode) {
      auto field = op.field;
      auto value = op.value;
      auto start = getFieldStart(field);
      auto mask = getFieldBitmask(field);

      if (((instr.value & mask) >> start) != value) {
         return false;
      }
   }

   return true;
}

Instruction
InstructionTable::encode(InstructionID id)
{
   uint32_t instr = 0;
   auto &data = instructionData[static_cast<size_t>(id)];

   for (auto &op : data.opcode) {
      auto field = op.field;
      auto value = op.value;
      auto start = getFieldStart(field);

      instr |= value << start;
   }

   return instr;
}

void
InstructionTable::initialise()
{
   initData();
   initTable();
}

// Initialise instructionTable
void
initTable()
{
   for (auto &instr : instructionData) {
      TableEntry *table = &instructionTable;

      // Resolve opcodes
      for (auto i = 0u; i < instr.opcode.size() - 1; ++i) {
         auto field = instr.opcode[i].field;
         auto value = instr.opcode[i].value;
         table->addTable(field);
         table = table->getEntry(field, value);
      }

      // Add the actual instruction entry
      auto field = instr.opcode.back().field;
      auto value = instr.opcode.back().value;
      table->addTable(instr.opcode.back().field);
      table->addInstruction(field, value, &instr);
   }
}

std::string cleanInsName(const std::string& name)
{
   if (name[name.size() - 1] == '_') {
      return name.substr(0, name.size() - 1);
   }
   return name;
}

struct FieldIndex {
   FieldIndex(Field _id) : id(_id) { }
   operator Field() const { return id; }
   Field id;
};

// Initialise instructionData
#define FLD(x, ...) \
   static const FieldIndex x(Field::x);
#define MRKR(x, ...) \
   static const FieldIndex x(Field::x);
#include "instructionfields.inl"
#undef FLD
#undef MRKR

#define PRINTOPS(...) __VA_ARGS__
#define INS(name, write, read, flags, opcodes, fullname) \
   instructionData.emplace_back(InstructionData { \
                                   InstructionID::name, cleanInsName(#name), fullname, \
                                   { PRINTOPS opcodes }, { PRINTOPS read }, \
                                   { PRINTOPS write }, { PRINTOPS flags } });

#define INSA(name, op, opcodes) \
   aliasData.emplace_back(InstructionAlias { \
                             #name, InstructionID::op, \
                             { PRINTOPS opcodes } });

static InstructionOpcode
operator==(const FieldIndex &lhs, const int &rhs)
{
   return InstructionOpcode(lhs.id, rhs);
}

static InstructionOpcode
operator==(const FieldIndex &lhs, const FieldIndex &rhs)
{
   return InstructionOpcode(lhs.id, rhs.id);
}

static InstructionOpcode
operator!(const FieldIndex &lhs)
{
   return InstructionOpcode(lhs.id, 0);
}

void
initData()
{
#include "instructions.inl"
#include "instructionaliases.inl"

   // Verify instruction fields
#define FLD(x, y, z, ...) \
   static_assert(z >= y, "Field " #x " z < y"); \
   Instruction ins_##x(make_bitmask<31-z,31-y,uint32_t>()); \
   uint32_t insv_##x = make_bitmask<0, z - y, uint32_t>(); \
   if (ins_##x.x != insv_##x) { \
      printf("%s %08x %08x\n", #x, ins_##x.x, insv_##x); \
   }
#define MRKR(...)
#include "instructionfields.inl"
#undef FLD
#undef MRKR
};

#undef INS
#undef INSA