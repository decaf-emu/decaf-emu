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

// Decode Instruction to InstructionData
InstructionData *
InstructionTable::decode(Instruction instr)
{
   TableEntry *table = &instructionTable;
   InstructionData *data = nullptr;

   while (table) {
      for (auto &fieldMap : table->fieldMaps) {
         auto field = fieldMap.field;
         auto mask = getFieldBitmask(field);
         auto start = getFieldStart(field);
         auto value = (instr & mask) >> start;
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

// Initialise instructionData
#define aa Field::aa
#define bd Field::bd
#define bo Field::bo
#define bi Field::bi
#define crba Field::crbA
#define crbb Field::crbB
#define crbd Field::crbD
#define crfd Field::crfD
#define crfs Field::crfS
#define crm Field::crm
#define d Field::d
#define fm Field::fm
#define fra Field::frA
#define frb Field::frB
#define frc Field::frC
#define frd Field::frD
#define frs Field::frS
#define i Field::i
#define imm Field::imm
#define kci Field::kci
#define kcn Field::kcn
#define l Field::l
#define li Field::li
#define lk Field::lk
#define mb Field::mb
#define me Field::me
#define nb Field::nb
#define oe Field::oe
#define opcd Field::opcd
#define qd Field::qd
#define qi Field::qi
#define qw Field::qw
#define ra Field::rA
#define rb Field::rB
#define rc Field::rc
#define rd Field::rD
#define rs Field::rS
#define sh Field::sh
#define simm Field::simm
#define sr Field::sr
#define spr Field::spr
#define to Field::to
#define tbr Field::tbr
#define uimm Field::uimm
#define w Field::w
#define xo1 Field::xo1
#define xo2 Field::xo2
#define xo3 Field::xo3
#define xo4 Field::xo4

#define PRINTOPS(...) __VA_ARGS__
#define INS(name, write, read, flags, opcodes, fullname) \
   instructionData.emplace_back(InstructionData { \
                                   InstructionID::name, #name, fullname, \
                                   { PRINTOPS opcodes }, { PRINTOPS read }, \
                                   { PRINTOPS write }, { PRINTOPS flags } });

static InstructionData::Opcode
operator==(const Field &lhs, const int &rhs)
{
   return InstructionData::Opcode(lhs, rhs);
}

void
initData()
{
#include "instructions.inl"
};
