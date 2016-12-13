#include "espresso_instructionset.h"
#include "espresso_spr.h"
#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <algorithm>

namespace espresso
{

struct TableEntry
{
   struct FieldMap
   {
      InstructionField field;
      std::vector<TableEntry> children;
   };

   void
   addInstruction(InstructionField field, uint32_t value, InstructionInfo *instrInfo)
   {
      auto fieldMap = getFieldMap(field);
      decaf_check(fieldMap);
      decaf_check(value < fieldMap->children.size());
      fieldMap->children[value].instr = instrInfo;
   }

   void
   addTable(InstructionField field)
   {
      if (!getFieldMap(field)) {
         auto fieldMap = FieldMap {};
         auto size = 1 << getInstructionFieldWidth(field);
         fieldMap.field = field;
         fieldMap.children.resize(size);
         fieldMaps.emplace_back(fieldMap);
      }
   }

   TableEntry *
   getEntry(InstructionField field, uint32_t value)
   {
      auto fieldMap = getFieldMap(field);
      decaf_check(fieldMap);
      decaf_check(value < fieldMap->children.size());
      return &fieldMap->children[value];
   }

   FieldMap *
   getFieldMap(InstructionField field)
   {
      for (auto &fieldMap : fieldMaps) {
         if (fieldMap.field == field) {
            return &fieldMap;
         }
      }

      return nullptr;
   }

   InstructionInfo *instr = nullptr;
   std::vector<FieldMap> fieldMaps;
};

static std::vector<InstructionInfo>
sInstructionInfo;

static std::vector<InstructionAlias>
sAliasData;

static TableEntry
sInstructionTable;

#define FLD(x, y, z, ...) {y, z},
#define MRKR(x, ...) {-1, -1},
static std::pair<int, int>
sFieldBits[] = {
   { -1, -1 },
#include "espresso_instruction_fields.inl"
};
#undef FLD
#undef MRKR

#define FLD(x, ...) #x,
#define MRKR(x, ...) #x,
static const char *
sFieldNames[] = {
#include "espresso_instruction_fields.inl"
};
#undef FLD
#undef MRKR

// Returns true if InstructionField is a marker only field
bool
isInstructionFieldMarker(InstructionField field)
{
   return sFieldBits[static_cast<int>(field)].first == -1;
}

// Get name of InstructionField
const char *
getInstructionFieldName(InstructionField field)
{
   return sFieldNames[static_cast<int>(field)];
}

// First bit of instruction field
uint32_t
getInstructionFieldStart(InstructionField field)
{
   return 31 - sFieldBits[static_cast<int>(field)].second;
}

// Last bit of instruction field
uint32_t
getInstructionFieldEnd(InstructionField field)
{
   return 31 - sFieldBits[static_cast<int>(field)].first;
}

// Width of instruction field in bits
uint32_t
getInstructionFieldWidth(InstructionField field)
{
   auto end = getInstructionFieldEnd(field);
   auto start = getInstructionFieldStart(field);
   return end - start + 1;
}

// Absolute bitmask of instruction field
uint32_t
getInstructionFieldBitmask(InstructionField field)
{
   auto end = getInstructionFieldEnd(field);
   auto start = getInstructionFieldStart(field);
   return make_bitmask(start, end);
}

uint32_t
getInstructionFieldValue(const InstructionField& field, Instruction instr)
{
   if (field == InstructionField::spr) {
      return static_cast<uint32_t>(decodeSPR(instr));
   } else {
      auto mask = getInstructionFieldBitmask(field);
      auto start = getInstructionFieldStart(field);

      return (instr & mask) >> start;
   }
}

SPR
decodeSPR(Instruction instr)
{
   return static_cast<SPR>(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
}

void
encodeSPR(Instruction &instr, SPR spr)
{
   auto sprInt = (uint32_t)spr;
   instr.spr = ((sprInt << 5) & 0x3E0) | ((sprInt >> 5) & 0x1F);
}

// Decode Instruction to InstructionInfo
InstructionInfo *
decodeInstruction(Instruction instr)
{
   auto table = &sInstructionTable;

   while (table) {
      for (auto &fieldMap : table->fieldMaps) {
         auto value = getInstructionFieldValue(fieldMap.field, instr);
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

// Encode specified instruction
Instruction
encodeInstruction(InstructionID id)
{
   auto &data = sInstructionInfo[static_cast<size_t>(id)];
   auto instr = 0u;

   for (auto &op : data.opcode) {
      auto field = op.field;
      auto value = op.value;
      auto start = getInstructionFieldStart(field);

      instr |= value << start;
   }

   return instr;
}

// Find InstructionInfo for InstructionID
InstructionInfo *
findInstructionInfo(InstructionID instrId)
{
   return &sInstructionInfo[static_cast<size_t>(instrId)];
}

// Find any alias which matches instruction
InstructionAlias *
findInstructionAlias(InstructionInfo *info, Instruction instr)
{
   for (auto &alias : sAliasData) {
      if (alias.id != info->id) {
         // Not an alias for this field
         continue;
      }

      auto opMatch = std::all_of(alias.opcode.begin(), alias.opcode.end(), [instr](const auto &op) {
         auto x = getInstructionFieldValue(op.field, instr);
         auto y = op.value;

         if (op.field2 != InstructionField::Invalid) {
            y = getInstructionFieldValue(op.field2, instr);
         }

         return x == y;
      });

      if (opMatch) {
         return &alias;
      }
   }

   return nullptr;
}

// Check if instruction is a certain instruction
bool
isA(InstructionID id, Instruction instr)
{
   const auto &data = sInstructionInfo[static_cast<size_t>(id)];

   return std::all_of(data.opcode.begin(), data.opcode.end(), [instr](const auto &op) {
      auto field = op.field;
      auto value = op.value;
      auto start = getInstructionFieldStart(field);
      auto mask = getInstructionFieldBitmask(field);

      return ((instr.value & mask) >> start) == value;
   });
}

// Initialise instructionTable
static void
initialiseInstructionTable()
{
   for (auto &instr : sInstructionInfo) {
      TableEntry *table = &sInstructionTable;

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
      table->addTable(field);
      table->addInstruction(field, value, &instr);
   }
}

static std::string
cleanInsName(const std::string& name)
{
   if (name[name.size() - 1] == '_') {
      return name.substr(0, name.size() - 1);
   }

   return name;
}

/*
 * FieldIndex is used to generate the decoding rules.
 *
 * For example the add instruction is defined as:
 * INS(add, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 266), "Add")
 *
 * Here both opcd and xo2 will be defined as a FieldIndex, and then during
 * initialiseInstructionSet they will use FieldIndex::operator== below to generate
 * an InstructionOpcode which we then use to setup the instruction table decoding.
 */
struct FieldIndex
{
   FieldIndex(InstructionField _id) : id(_id)
   {
   }

   operator InstructionField() const
   {
      return id;
   }

   InstructionOpcode
   operator==(const int &other) const
   {
      return InstructionOpcode(id, other);
   }

   InstructionOpcode
   operator==(const FieldIndex &other) const
   {
      return InstructionOpcode(id, other.id);
   }

   InstructionOpcode
   operator!() const
   {
      return InstructionOpcode(id, 0);
   }

   InstructionField id;
};

// Create FieldIndex to match the field names we use inside the spec files
#define FLD(x, ...)  static const FieldIndex x(InstructionField::x);
#define MRKR(x, ...) static const FieldIndex x(InstructionField::x);
#include "espresso_instruction_fields.inl"
#undef FLD
#undef MRKR

#define PRINTOPS(...) __VA_ARGS__

// Used to populate sInstructionInfo
#define INS(name, write, read, flags, opcodes, fullname) \
   sInstructionInfo.emplace_back(InstructionInfo { \
                                    InstructionID::name, cleanInsName(#name), fullname, \
                                    { PRINTOPS opcodes }, { PRINTOPS read }, \
                                    { PRINTOPS write }, { PRINTOPS flags } \
                                 });

// Used to populate sInstructionAlias
#define INSA(name, op, opcodes) \
   sAliasData.emplace_back(InstructionAlias { \
                              #name, InstructionID::op, \
                              { PRINTOPS opcodes } \
                           });

void
initialiseInstructionSet()
{
   // Populate sInstructionInfo
#  include "espresso_instruction_definitions.inl"

   // Populate sInstructionAlias
#  include "espresso_instruction_aliases.inl"

   // Create instruction table
   initialiseInstructionTable();
};

#undef INS
#undef INSA

} // namespace espresso
