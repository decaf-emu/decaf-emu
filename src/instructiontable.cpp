#include <cassert>
#include "bitutils.h"
#include "instructiondata.h"

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

static BitRange gFieldBits[] = {
   { -1, -1 },
   { 30, 30 }, // aa
   { 16, 29 }, // bd
   { 11, 15 }, // bi
   { 6, 10 },  // bo
   { 11, 15 }, // crbA
   { 16, 20 }, // crbB
   { 6, 10 },  // crbD
   { 6, 8 },   // crfD
   { 11, 13 }, // crfS
   { 12, 19 }, // crm
   { 16, 31 }, // d
   { 7, 14 },  // fm
   { 11, 15 }, // frA
   { 16, 20 }, // frB
   { 21, 25 }, // frC
   { 6, 10 },  // frD
   { 6, 10 },  // frS
   { 17, 19 }, // i
   { 16, 19 }, // imm
   { 30, 30 }, // kci
   { 6, 29 },  // kcn
   { 10, 10 }, // l
   { 6, 29 },  // li
   { 31, 31 }, // lk
   { 21, 25 }, // mb
   { 26, 30 }, // me
   { 16, 20 }, // nb
   { 21, 21 }, // oe
   { 0, 5 },   // opcd
   { 20, 31},  // qd
   { 22, 24 }, // qi
   { 21, 21 }, // qw
   { 11, 15 }, // rA
   { 16, 20 }, // rB
   { 31, 31 }, // rc
   { 6, 10 },  // rD
   { 6, 10 },  // rS
   { 16, 20 }, // sh
   { 16, 31 }, // simm
   { 12, 15 }, // sr
   { 11, 20 }, // spr
   { 6, 10 },  // to
   { 11, 20 }, // tbr
   { 16, 31 }, // uimm
   { 16, 16 }, // w
   { 21, 30 }, // xo1
   { 22, 30 }, // xo2
   { 25, 30 }, // xo3
   { 26, 30 }, // xo4
};

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
#define aa FieldIndex(Field::aa)
#define bd FieldIndex(Field::bd)
#define bo FieldIndex(Field::bo)
#define bi FieldIndex(Field::bi)
#define crba FieldIndex(Field::crbA)
#define crbb FieldIndex(Field::crbB)
#define crbd FieldIndex(Field::crbD)
#define crfd FieldIndex(Field::crfD)
#define crfs FieldIndex(Field::crfS)
#define crm FieldIndex(Field::crm)
#define d FieldIndex(Field::d)
#define fm FieldIndex(Field::fm)
#define fra FieldIndex(Field::frA)
#define frb FieldIndex(Field::frB)
#define frc FieldIndex(Field::frC)
#define frd FieldIndex(Field::frD)
#define frs FieldIndex(Field::frS)
#define i FieldIndex(Field::i)
#define imm FieldIndex(Field::imm)
#define kci FieldIndex(Field::kci)
#define kcn FieldIndex(Field::kcn)
#define l FieldIndex(Field::l)
#define li FieldIndex(Field::li)
#define lk FieldIndex(Field::lk)
#define mb FieldIndex(Field::mb)
#define me FieldIndex(Field::me)
#define nb FieldIndex(Field::nb)
#define oe FieldIndex(Field::oe)
#define opcd FieldIndex(Field::opcd)
#define qd FieldIndex(Field::qd)
#define qi FieldIndex(Field::qi)
#define qw FieldIndex(Field::qw)
#define ra FieldIndex(Field::rA)
#define rb FieldIndex(Field::rB)
#define rc FieldIndex(Field::rc)
#define rd FieldIndex(Field::rD)
#define rs FieldIndex(Field::rS)
#define sh FieldIndex(Field::sh)
#define simm FieldIndex(Field::simm)
#define sr FieldIndex(Field::sr)
#define spr FieldIndex(Field::spr)
#define to FieldIndex(Field::to)
#define tbr FieldIndex(Field::tbr)
#define uimm FieldIndex(Field::uimm)
#define w FieldIndex(Field::w)
#define xo1 FieldIndex(Field::xo1)
#define xo2 FieldIndex(Field::xo2)
#define xo3 FieldIndex(Field::xo3)
#define xo4 FieldIndex(Field::xo4)

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

void
initData()
{
#include "instructions.inl"
};
