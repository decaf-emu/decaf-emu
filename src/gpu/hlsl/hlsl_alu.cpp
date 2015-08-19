#include "hlsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;

namespace hlsl
{

void translateChannel(GenerateState &state, latte::alu::Channel::Channel channel)
{
   switch (channel) {
   case latte::alu::Channel::X:
      state.out << 'x';
      break;
   case latte::alu::Channel::Y:
      state.out << 'y';
      break;
   case latte::alu::Channel::Z:
      state.out << 'z';
      break;
   case latte::alu::Channel::W:
      state.out << 'w';
      break;
   }
}

void translateAluDestStart(GenerateState &state, AluInstruction *ins)
{
   if (ins->unit != AluInstruction::T) {
      state.out << "PV" << '.';
      translateChannel(state, static_cast<latte::alu::Channel::Channel>(ins->unit));
   } else {
      state.out << "PS";
   }

   state.out << " = ";

   if (ins->writeMask) {
      state.out << 'R' << ins->dest.id << '.';
      translateChannel(state, ins->dest.chan);
      state.out << " = ";
   }

   switch (ins->outputModifier) {
   case latte::alu::OutputModifier::Multiply2:
   case latte::alu::OutputModifier::Multiply4:
   case latte::alu::OutputModifier::Divide2:
      state.out << '(';
      break;
   }

   if (ins->dest.clamp) {
      state.out << "clamp(";
   }
}

void translateAluDestEnd(GenerateState &state, AluInstruction *ins)
{
   if (ins->dest.clamp) {
      state.out << ", 0, 1)";
   }

   switch (ins->outputModifier) {
   case latte::alu::OutputModifier::Multiply2:
      state.out << ") * 2";
      break;
   case latte::alu::OutputModifier::Multiply4:
      state.out << ") * 4";
      break;
   case latte::alu::OutputModifier::Divide2:
      state.out << ") / 2";
      break;
   }
}

void translateAluSource(GenerateState &state, AluSource &src)
{
   if (src.absolute) {
      state.out << "abs(";
   }

   if (src.negate) {
      state.out << '-';
   }

   switch (src.type) {
   case AluSource::Register:
      state.out << 'R' << src.id;
      break;
   case AluSource::KcacheBank0:
      state.out << "KcackeBank0_" << src.id;
      break;
   case AluSource::KcacheBank1:
      state.out << "KcackeBank1_" << src.id;
      break;
   case AluSource::PreviousVector:
      state.out << "PV";
      break;
   case AluSource::PreviousScalar:
      state.out << "PS";
      break;
   case AluSource::ConstantFile:
      state.out << 'C' << src.id;
      break;
   case AluSource::ConstantFloat:
      state.out.write("{:.6f}f", src.floatValue);
      break;
   case AluSource::ConstantDouble:
      state.out.write("{:.6f}", src.doubleValue);
      break;
   case AluSource::ConstantInt:
      state.out << src.intValue;
      break;
   }

   switch (src.type) {
   case AluSource::Register:
   case AluSource::KcacheBank0:
   case AluSource::KcacheBank1:
   case AluSource::ConstantFile:
   case AluSource::PreviousVector:
      state.out << '.';
      translateChannel(state, src.chan);
   }

   if (src.absolute) {
      state.out << ")";
   }
}

void translateAluSourceVector(GenerateState &state, AluSource &srcX, AluSource &srcY, AluSource &srcZ, AluSource &srcW)
{
   state.out << "float4(";
   translateAluSource(state, srcX);
   state.out << ", ";
   translateAluSource(state, srcY);
   state.out << ", ";
   translateAluSource(state, srcZ);
   state.out << ", ";
   translateAluSource(state, srcW);
   state.out << ')';
}

} // namespace hlsl
