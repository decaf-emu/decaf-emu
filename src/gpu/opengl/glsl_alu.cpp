#include "glsl_generator.h"
#include "utils/bit_cast.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluSource;
using latte::shadir::AluDest;

namespace gpu
{

namespace opengl
{

namespace glsl
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
   case latte::alu::Channel::Unknown:
      state.out << '?';
      break;
   }
}

void translateAluDestStart(GenerateState &state, AluInstruction *ins)
{
   if (ins->unit != AluInstruction::T) {
      if (state.shader->pvUsed.find(ins->groupPC) != state.shader->pvUsed.end()) {
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            // If we read and write PV in this group we must use a temp to prevent clash
            state.out << "PVo.";
         } else {
            state.out << "PV.";
         }

         if (ins->isReduction) {
            // Reduction targets PV.x always
            state.out << 'x';
         } else {
            translateChannel(state, static_cast<latte::alu::Channel::Channel>(ins->unit));
         }

         state.out << " = ";
      }
   } else {
      if (state.shader->psUsed.find(ins->groupPC) != state.shader->psUsed.end()) {
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            state.out << "PSo = ";
         } else {
            state.out << "PS = ";
         }
      }
   }

   if (ins->writeMask) {
      state.out << 'R' << ins->dest.id << '.';
      translateChannel(state, ins->dest.chan);
      state.out << " = ";
   }

   if (ins->dest.valueType == AluDest::Uint) {
      state.out << "uintBitsToFloat(";
   } else if (ins->dest.valueType == AluDest::Int) {
      state.out << "intBitsToFloat(";
   }

   switch (ins->outputModifier) {
   case latte::alu::OutputModifier::Multiply2:
   case latte::alu::OutputModifier::Multiply4:
   case latte::alu::OutputModifier::Divide2:
      state.out << '(';
      break;
   case latte::alu::OutputModifier::Off:
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
   case latte::alu::OutputModifier::Off:
      break;
   }

   if (ins->dest.valueType == AluDest::Int || ins->dest.valueType == AluDest::Uint) {
      state.out << ")";
   }
}

void translateIndex(GenerateState &state, latte::alu::IndexMode::IndexMode index)
{
   switch (index) {
   case latte::alu::IndexMode::ArX:
      state.out << "AR.x";
      break;
   case latte::alu::IndexMode::ArY:
      state.out << "AR.y";
      break;
   case latte::alu::IndexMode::ArZ:
      state.out << "AR.z";
      break;
   case latte::alu::IndexMode::ArW:
      state.out << "AR.w";
      break;
   case latte::alu::IndexMode::Loop:
      state.out << "AL";
      break;
   default:
      assert(0);
   }
}

void translateAluSource(GenerateState &state, const AluSource &src)
{
   if (src.absolute) {
      state.out << "abs(";
   }

   if (src.negate) {
      state.out << '-';
   }

   if (src.valueType == AluSource::Int) {
      switch (src.type) {
      case AluSource::Register:
      case AluSource::UniformBlock0:
      case AluSource::UniformBlock1:
      case AluSource::UniformBlock2:
      case AluSource::UniformBlock3:
      case AluSource::UniformBlock4:
      case AluSource::UniformBlock5:
      case AluSource::UniformBlock6:
      case AluSource::UniformBlock7:
      case AluSource::UniformBlock8:
      case AluSource::UniformBlock9:
      case AluSource::UniformBlock10:
      case AluSource::UniformBlock11:
      case AluSource::UniformBlock12:
      case AluSource::UniformBlock13:
      case AluSource::UniformBlock14:
      case AluSource::UniformBlock15:
      case AluSource::PreviousVector:
      case AluSource::PreviousScalar:
      case AluSource::ConstantFile:
         state.out << "floatBitsToInt(";
         break;
      case AluSource::ConstantFloat:
      case AluSource::ConstantDouble:
         state.out << "int(";
         break;
      case AluSource::ConstantInt:
      case AluSource::ConstantLiteral:
         break;
      }
   } else if (src.valueType == AluSource::Uint) {
      switch (src.type) {
      case AluSource::Register:
      case AluSource::UniformBlock0:
      case AluSource::UniformBlock1:
      case AluSource::UniformBlock2:
      case AluSource::UniformBlock3:
      case AluSource::UniformBlock4:
      case AluSource::UniformBlock5:
      case AluSource::UniformBlock6:
      case AluSource::UniformBlock7:
      case AluSource::UniformBlock8:
      case AluSource::UniformBlock9:
      case AluSource::UniformBlock10:
      case AluSource::UniformBlock11:
      case AluSource::UniformBlock12:
      case AluSource::UniformBlock13:
      case AluSource::UniformBlock14:
      case AluSource::UniformBlock15:
      case AluSource::PreviousVector:
      case AluSource::PreviousScalar:
      case AluSource::ConstantFile:
         state.out << "floatBitsToUint(";
         break;
      case AluSource::ConstantFloat:
      case AluSource::ConstantDouble:
         state.out << "uint(";
         break;
      case AluSource::ConstantInt:
      case AluSource::ConstantLiteral:
         break;
      }
   }

   switch (src.type) {
   case AluSource::Register:
      state.out << 'R' << src.id;
      break;
   case AluSource::PreviousVector:
      state.out << "PV";
      break;
   case AluSource::PreviousScalar:
      state.out << "PS";
      break;
   case AluSource::ConstantFloat:
      state.out.write("{:.6f}f", src.floatValue);
      break;
   case AluSource::ConstantDouble:
      state.out.write("{:.6f}f", src.doubleValue);
      break;
   case AluSource::ConstantInt:
      state.out << src.intValue;
      break;
   case AluSource::ConstantLiteral:
      if (src.valueType == AluSource::Float) {
         state.out.write("{:.6f}f", bit_cast<float>(src.literalValue));
      } else if (src.valueType == AluSource::Int) {
         state.out << bit_cast<int32_t>(src.literalValue);
      } else if (src.valueType == AluSource::Uint) {
         state.out << bit_cast<uint32_t>(src.literalValue);
      }
      break;
   case AluSource::UniformBlock0:
   case AluSource::UniformBlock1:
   case AluSource::UniformBlock2:
   case AluSource::UniformBlock3:
   case AluSource::UniformBlock4:
   case AluSource::UniformBlock5:
   case AluSource::UniformBlock6:
   case AluSource::UniformBlock7:
   case AluSource::UniformBlock8:
   case AluSource::UniformBlock9:
   case AluSource::UniformBlock10:
   case AluSource::UniformBlock11:
   case AluSource::UniformBlock12:
   case AluSource::UniformBlock13:
   case AluSource::UniformBlock14:
   case AluSource::UniformBlock15:
   {
      uint32_t blockIdx = src.type - AluSource::UniformBlock0;

      if (state.shader->type == latte::Shader::Vertex) {
         state.out << "VB[" << blockIdx << "].values[" << src.id;
      } else if (state.shader->type == latte::Shader::Pixel) {
         state.out << "PB[" << blockIdx << "].values[" << src.id;
      } else {
         throw;
      }

      if (src.rel) {
         state.out << " + ";
         translateIndex(state, src.indexMode);
      }

      state.out << ']';
      break;
   }
   case AluSource::ConstantFile: // Uniform register
      if (state.shader->type == latte::Shader::Vertex) {
         state.out << "VR[" << src.id;
      } else if (state.shader->type == latte::Shader::Pixel) {
         state.out << "PR[" << src.id;
      } else {
         throw std::runtime_error("Unexpected shader type for ConstantFile");
      }

      if (src.rel) {
         state.out << " + ";
         translateIndex(state, src.indexMode);
      }

      state.out << ']';
      break;
   }

   if (src.rel) {
      assert(src.type == AluSource::ConstantFile);
   }

   switch (src.type) {
   case AluSource::Register:
   case AluSource::UniformBlock0:
   case AluSource::UniformBlock1:
   case AluSource::UniformBlock2:
   case AluSource::UniformBlock3:
   case AluSource::UniformBlock4:
   case AluSource::UniformBlock5:
   case AluSource::UniformBlock6:
   case AluSource::UniformBlock7:
   case AluSource::UniformBlock8:
   case AluSource::UniformBlock9:
   case AluSource::UniformBlock10:
   case AluSource::UniformBlock11:
   case AluSource::UniformBlock12:
   case AluSource::UniformBlock13:
   case AluSource::UniformBlock14:
   case AluSource::UniformBlock15:
   case AluSource::ConstantFile:
   case AluSource::PreviousVector:
      state.out << '.';
      translateChannel(state, src.chan);
   case AluSource::PreviousScalar:
   case AluSource::ConstantFloat:
   case AluSource::ConstantDouble:
   case AluSource::ConstantInt:
   case AluSource::ConstantLiteral:
      break;
   }

   if (src.valueType == AluSource::Int || src.valueType == AluSource::Uint) {
      if (src.type != AluSource::ConstantInt && src.type != AluSource::ConstantLiteral) {
         state.out << ')';
      }
   }

   if (src.absolute) {
      state.out << ')';
   }
}

void translateAluSourceVector(GenerateState &state,
                              const AluSource &srcX,
                              const AluSource &srcY,
                              const AluSource &srcZ,
                              const AluSource &srcW)
{
   state.out << "vec4(";
   translateAluSource(state, srcX);
   state.out << ", ";
   translateAluSource(state, srcY);
   state.out << ", ";
   translateAluSource(state, srcZ);
   state.out << ", ";
   translateAluSource(state, srcW);
   state.out << ')';
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
