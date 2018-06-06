#include <algorithm>
#include <sstream>
#include <iomanip>
#include <list>
#include "espresso_disassembler.h"
#include "espresso_instructionset.h"
#include <common/bitutils.h>

namespace espresso
{

static void
checkBranchConditionAlias(Instruction instr, Disassembly &dis);

static bool
disassembleField(Disassembly::Argument &result, uint32_t cia, Instruction instr, InstructionInfo *data, InstructionField field)
{
   switch (field) {
   case InstructionField::bd:
      result.type = Disassembly::Argument::Address;
      result.address = sign_extend<16>(instr.bd << 2) + (instr.aa ? 0 : cia);
      break;
   case InstructionField::bi:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.bi;
      break;
   case InstructionField::bo:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.bo;
      break;
   case InstructionField::crbA:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbA;
      break;
   case InstructionField::crbB:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbB;
      break;
   case InstructionField::crbD:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbD;
      break;
   case InstructionField::crfD:
      result.type = Disassembly::Argument::Register;
      result.text = "crf" + std::to_string(instr.crfD);
      break;
   case InstructionField::crfS:
      result.type = Disassembly::Argument::Register;
      result.text = "crf" + std::to_string(instr.crfS);
      break;
   case InstructionField::crm:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crm;
      break;
   case InstructionField::d:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<16>(instr.d);
      break;
   case InstructionField::fm:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.fm;
      break;
   case InstructionField::frA:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frA);
      break;
   case InstructionField::frB:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frB);
      break;
   case InstructionField::frC:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frC);
      break;
   case InstructionField::frD:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frD);
      break;
   case InstructionField::frS:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frS);
      break;
   case InstructionField::i:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.i;
      break;
   case InstructionField::imm:
      result.type = Disassembly::Argument::ValueUnsigned;
      result.valueUnsigned = instr.imm;
      break;
   case InstructionField::kcn:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.kcn;
      break;
   case InstructionField::li:
      result.type = Disassembly::Argument::Address;
      result.address = sign_extend<26>(instr.li << 2) + (instr.aa ? 0 : cia);
      break;
   case InstructionField::mb:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.mb;
      break;
   case InstructionField::me:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.me;
      break;
   case InstructionField::nb:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.nb;
      break;
   case InstructionField::qd:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<12>(instr.qd);
      break;
   case InstructionField::rA:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rA);
      break;
   case InstructionField::rB:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rB);
      break;
   case InstructionField::rD:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rD);
      break;
   case InstructionField::rS:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rS);
      break;
   case InstructionField::sh:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.sh;
      break;
   case InstructionField::simm:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<16>(instr.simm);
      break;
   case InstructionField::sr:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.sr;
      break;
   case InstructionField::spr: // TODO: Real SPR name
      result.type = Disassembly::Argument::Register;
      result.text = "spr" + std::to_string(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
      break;
   case InstructionField::to:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.to;
      break;
   case InstructionField::tbr:
      result.type = Disassembly::Argument::Register;
      result.text = "tbr" + std::to_string(instr.spr);
      break;
   case InstructionField::uimm:
      result.type = Disassembly::Argument::ValueUnsigned;
      result.valueUnsigned = instr.uimm;
      break;
   // Ignore opcode fields
   case InstructionField::opcd:
   case InstructionField::xo1:
   case InstructionField::xo2:
   case InstructionField::xo3:
   case InstructionField::xo4:

   // Ignore name modifiers
   case InstructionField::aa:
   case InstructionField::lk:
   case InstructionField::oe:
   case InstructionField::rc:

   // Ignore marker fields
   case InstructionField::PS:
   case InstructionField::XERO:
   case InstructionField::XERSO:
   case InstructionField::XERC:
   case InstructionField::CR0:
   case InstructionField::CR1:
   case InstructionField::FCRISI:
   case InstructionField::FCRIDI:
   case InstructionField::FCRSNAN:
   case InstructionField::FCRZDZ:
   case InstructionField::FPRF:
   case InstructionField::AOE:
   case InstructionField::ARC:
   case InstructionField::LR:
   case InstructionField::CTR:
   case InstructionField::FPSCR:
   case InstructionField::RSRV:
      return false;
   default:
      result.type = Disassembly::Argument::Invalid;
      break;
   }

   return true;
}

static std::string
argumentToText(Disassembly::Argument &arg)
{
   std::stringstream ss;

   switch (arg.type) {
   case Disassembly::Argument::Address:
      ss << "@" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << arg.address;
      return ss.str();
   case Disassembly::Argument::Register:
      return arg.text;
   case Disassembly::Argument::ValueUnsigned:
      if (arg.valueUnsigned > 9) {
         ss << "0x" << std::hex << arg.valueUnsigned;
      } else {
         ss << arg.valueUnsigned;
      }
      return ss.str();
   case Disassembly::Argument::ValueSigned:
      if (arg.valueSigned < -9) {
         ss << "-0x" << std::hex << -arg.valueSigned;
      } else if (arg.valueSigned > 9) {
         ss << "0x" << std::hex << arg.valueSigned;
      } else {
         ss << arg.valueSigned;
      }
      return ss.str();
   case Disassembly::Argument::ConstantUnsigned:
      return std::to_string(arg.constantUnsigned);
   case Disassembly::Argument::ConstantSigned:
      return std::to_string(arg.constantSigned);
   case Disassembly::Argument::Invalid:
      return std::string("???");
   }

   return std::string();
}

bool
disassemble(Instruction instr, Disassembly &dis, uint32_t address)
{
   auto data = decodeInstruction(instr);

   if (!data) {
      return false;
   }

   auto alias = findInstructionAlias(data, instr);
   dis.name = alias ? alias->name : data->name;
   dis.instruction = data;
   dis.address = address;

   std::list<InstructionField> args;

   for (auto &field : data->write) {
      // Skip arguments that are in read list as well
      if (std::find(data->read.begin(), data->read.end(), field) != data->read.end()) {
         continue;
      }

      // Add only unique arguements
      if (std::find(args.begin(), args.end(), field) != args.end()) {
         continue;
      }

      // Ignore trace only fields for disassembly
      if (isInstructionFieldMarker(field)) {
         continue;
      }

      args.push_back(field);
   }

   for (auto &field : data->read) {
      // Add only unique arguements
      if (std::find(args.begin(), args.end(), field) != args.end()) {
         continue;
      }

      args.push_back(field);
   }

   for (auto &field : args) {
      espresso::Disassembly::Argument arg;

      // If we have an alias, then skip the LHS field of each alias comparison
      if (alias) {
         bool skipField = false;

         for (auto &op : alias->opcode) {
            if (field == op.field) {
               skipField = true;
               break;
            }
         }

         if (skipField) {
            continue;
         }
      }

      if (disassembleField(arg, dis.address, instr, data, field)) {
         dis.args.push_back(arg);
      }
   }

   // Check for bc alias
   if (data->id == InstructionID::bc || data->id == InstructionID::bcctr || data->id == InstructionID::bclr) {
      checkBranchConditionAlias(instr, dis);
   }

   for (auto &field : data->flags) {
      if (field == InstructionField::aa && instr.aa) {
         dis.name += 'a';
      } else if (field == InstructionField::lk && instr.lk) {
         dis.name += 'l';
      } else if (field == InstructionField::oe && instr.oe) {
         dis.name += 'o';
      } else if (field == InstructionField::rc && instr.rc) {
         dis.name += '.';
      }
   }

   // Create text disassembly
   dis.text = dis.name;

   for (auto &arg : dis.args) {
      if (&arg == &dis.args[0]) {
         dis.text += " ";
      } else {
         dis.text += ", ";
      }

      dis.text += argumentToText(arg);
   }

   return true;
}

void
checkBranchConditionAlias(Instruction instr, Disassembly &dis)
{
   auto bi = instr.bi % 4;
   auto bo = instr.bo;
   auto name = std::string {};

   // Check for unconditional branch
   if (bo == 20 && bi == 0) {
      // Remove bo, bi from args
      dis.args.erase(dis.args.begin(), dis.args.begin() + 2);

      if (dis.instruction->id == espresso::InstructionID::bcctr) {
         dis.name = "bctr";
      } else if (dis.instruction->id == espresso::InstructionID::bclr) {
         dis.name = "blr";
      } else if (dis.instruction->id == espresso::InstructionID::bc) {
         dis.name = "b";
      }

      return;
   }

   if (bo == 12 && bi == 0) {
      name = "blt";
   } else if (bo == 4 && bi == 1) {
      name = "ble";
   } else if (bo == 12 && bi == 2) {
      name = "beq";
   } else if (bo == 4 && bi == 0) {
      name = "bge";
   } else if (bo == 12 && bi == 1) {
      name = "bgt";
   } else if (bo == 4 && bi == 2) {
      name = "bne";
   } else if (bo == 12 && bi == 3) {
      name = "bso";
   } else if (bo == 4 && bi == 3) {
      name = "bns";
   }

   if (!name.empty()) {
      // Remove bo, bi from args
      dis.args.erase(dis.args.begin(), dis.args.begin() + 2);

      // Add crS argument
      espresso::Disassembly::Argument cr;
      cr.type = Disassembly::Argument::Register;
      cr.text = "cr" + std::to_string(instr.bi / 4);
      dis.args.push_back(cr);

      // Update name
      dis.name = name;
   }
}

} // namespace espresso
