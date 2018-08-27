#include "glsl2_alu.h"
#include "glsl2_translate.h"
#include <fmt/format.h>

using namespace latte;

namespace glsl2
{

static const AluInst *
findWriteInstruction(const std::array<AluInst, 4> &group)
{
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].op2.WRITE_MASK()) {
         return &group[i];
      }
   }

   return nullptr;
}

// TODO: Detect a CUBE...SAMPLE sequence and optimize to a simple GLSL
// textureCube call.
// Sample operation sequence:
//     1 x: CUBE            R[out].x, R[in].z, R[in].y
//       y: CUBE            R[out].y, R[in].z, R[in].x
//       z: CUBE            R[out].z, R[in].x, R[in].z
//       w: CUBE            R[out].w, R[in].y, R[in].z
//     2 t: RECIP_IEEE      ____, |PV1.z| SCL_210
//     3 x: MULADD          R[out].x, R[out].x, PS2, (0x3FC00000, 1.5)
//       y: MULADD          R[out].y, R[out].y, PS2, (0x3FC00000, 1.5)
//     4    SAMPLE          R[texel].xyzw, R[out].yxwy, t[N], s[N]
static void
CUBE(State &state, const ControlFlowInst &cf, const std::array<AluInst, 4> &group)
{
   // The CUBE instruction requires a particular format:
   //    CUBE R[out], R[in].zzxy, R[in].yxzz
   // Each of the x/y/z inputs can come from anywhere (register, PV/PS,
   // constant) as long as the same value is used for all instances of that
   // input, so we have to detect what this particular instruction is using.
   const AluInst &xInsn = group[2];
   const latte::SQ_ALU_SRC xSel = xInsn.word0.SRC0_SEL();
   const latte::SQ_REL xRel = xInsn.word0.SRC0_REL();
   const latte::SQ_CHAN xChan = xInsn.word0.SRC0_CHAN();
   const AluInst &yInsn = group[3];
   const latte::SQ_ALU_SRC ySel = yInsn.word0.SRC0_SEL();
   const latte::SQ_REL yRel = yInsn.word0.SRC0_REL();
   const latte::SQ_CHAN yChan = yInsn.word0.SRC0_CHAN();
   const AluInst &zInsn = group[0];
   const latte::SQ_ALU_SRC zSel = zInsn.word0.SRC0_SEL();
   const latte::SQ_REL zRel = zInsn.word0.SRC0_REL();
   const latte::SQ_CHAN zChan = zInsn.word0.SRC0_CHAN();

   // Verify that the instruction is in fact using the expected syntax.
   if (group[1].word0.SRC0_SEL() != zSel
    || group[1].word0.SRC0_REL() != zRel
    || group[1].word0.SRC0_CHAN() != zChan
    || group[0].word0.SRC1_SEL() != ySel
    || group[0].word0.SRC1_REL() != yRel
    || group[0].word0.SRC1_CHAN() != yChan
    || group[1].word0.SRC1_SEL() != xSel
    || group[1].word0.SRC1_REL() != xRel
    || group[1].word0.SRC1_CHAN() != xChan
    || group[2].word0.SRC1_SEL() != zSel
    || group[2].word0.SRC1_REL() != zRel
    || group[2].word0.SRC1_CHAN() != zChan
    || group[3].word0.SRC1_SEL() != zSel
    || group[3].word0.SRC1_REL() != zRel
    || group[3].word0.SRC1_CHAN() != zChan) {
      fmt::memory_buffer xSource, ySource, zSource;
      insertSource0(state, xSource, cf, xInsn);
      insertSource0(state, ySource, cf, yInsn);
      insertSource0(state, zSource, cf, zInsn);
      throw translate_exception(fmt::format("Invalid CUBE syntax: inconsistent operands (detected x={}, y={}, z={})",
                                            to_string(xSource), to_string(ySource), to_string(zSource)));
   }

   // Concise pseudocode:
   //    if (|x| >= |y| && |x| >= |z|) {
   //       out = vec4(y, sign(x)*z, 2x, x>=0 ? 0 : 1)
   //    } else if (|y| >= |x| && |y| >= |z|) {
   //       out = vec4(sign(y)*-x, -z, 2y, y>=0 ? 2 : 3)
   //    } else {
   //       out = vec4(y, sign(z)*-x, 2z, z>=0 ? 4 : 5)
   //    }
   // Note that CUBE reverses the order of the texture coordinates in the
   // output: out.yx = face.st

   insertLineStart(state);
   fmt::format_to(state.out, "if (abs(");
   insertSource0(state, state.out, cf, xInsn);
   fmt::format_to(state.out, ") >= abs(");
   insertSource0(state, state.out, cf, yInsn);
   fmt::format_to(state.out, ") && abs(");
   insertSource0(state, state.out, cf, xInsn);
   fmt::format_to(state.out, ") >= abs(");
   insertSource0(state, state.out, cf, zInsn);
   fmt::format_to(state.out, ")) {{");
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::X);
      insertSource0(state, state.out, cf, yInsn);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Y);
      fmt::format_to(state.out, "sign(");
      insertSource0(state, state.out, cf, xInsn);
      fmt::format_to(state.out, ") * ");
      insertSource0(state, state.out, cf, zInsn);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Z);
      insertSource0(state, state.out, cf, xInsn);
      fmt::format_to(state.out, " * 2.0;");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::W);
      insertSource0(state, state.out, cf, xInsn);
      fmt::format_to(state.out, " >= 0 ? 0 : 1;");
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   fmt::format_to(state.out, "}} else if (abs(");
   insertSource0(state, state.out, cf, yInsn);
   fmt::format_to(state.out, ") >= abs(");
   insertSource0(state, state.out, cf, xInsn);
   fmt::format_to(state.out, ") && abs(");
   insertSource0(state, state.out, cf, yInsn);
   fmt::format_to(state.out, ") >= abs(");
   insertSource0(state, state.out, cf, zInsn);
   fmt::format_to(state.out, ")) {{");
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::X);
      fmt::format_to(state.out, "sign(");
      insertSource0(state, state.out, cf, yInsn);
      fmt::format_to(state.out, ") * -(");
      insertSource0(state, state.out, cf, xInsn);
      fmt::format_to(state.out, ");");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Y);
      fmt::format_to(state.out, "-(");
      insertSource0(state, state.out, cf, zInsn);
      fmt::format_to(state.out, ");");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Z);
      insertSource0(state, state.out, cf, yInsn);
      fmt::format_to(state.out, " * 2.0;");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::W);
      fmt::format_to(state.out, "(");
      insertSource0(state, state.out, cf, yInsn);
      fmt::format_to(state.out, " >= 0) ? 2 : 3;");
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   fmt::format_to(state.out, "}} else {{");
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::X);
      insertSource0(state, state.out, cf, yInsn);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Y);
      fmt::format_to(state.out, "sign(");
      insertSource0(state, state.out, cf, zInsn);
      fmt::format_to(state.out, ") * -(");
      insertSource0(state, state.out, cf, xInsn);
      fmt::format_to(state.out, ");");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::Z);
      insertSource0(state, state.out, cf, zInsn);
      fmt::format_to(state.out, " * 2.0;");
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN::W);
      fmt::format_to(state.out, "(");
      insertSource0(state, state.out, cf, zInsn);
      fmt::format_to(state.out, " >= 0) ? 4 : 5;");
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   fmt::format_to(state.out, "}}");
   insertLineEnd(state);

   // If any instructions write to a register, copy appropriately from PVo
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].op2.WRITE_MASK()) {
         fmt::memory_buffer postWrite;

         auto gpr = group[i].word1.DST_GPR();
         fmt::format_to(postWrite, "R[{}].", gpr);
         insertChannel(postWrite, group[i].word1.DST_CHAN());
         fmt::format_to(postWrite, " = PVo.");
         insertChannel(postWrite, static_cast<latte::SQ_CHAN>(i));
         fmt::format_to(postWrite, ";");

         state.postGroupWrites.push_back(to_string(postWrite));
      }
   }
}

static void
DOT4(State &state, const ControlFlowInst &cf, const std::array<AluInst, 4> &group)
{
   auto hasWriteMask = false;
   auto writeUnit = 0u;
   insertLineStart(state);

   // Find which, if any, instruction has a write mask set
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].op2.WRITE_MASK()) {
         hasWriteMask = true;
         writeUnit = i;
         insertDestBegin(state, cf, group[i], SQ_CHAN::X);
         break;
      }
   }

   // If no instruction in the group has a dest, then we must still write to PV.x
   if (!hasWriteMask) {
      insertPreviousValueUpdate(state.out, SQ_CHAN::X);
   }

   fmt::format_to(state.out, "dot(");
   insertSource0Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   fmt::format_to(state.out, ", ");
   insertSource1Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   fmt::format_to(state.out, ")");

   if (hasWriteMask) {
      insertDestEnd(state, cf, group[writeUnit]);
   }

   fmt::format_to(state.out, ";");
   insertLineEnd(state);
}

void
registerOP2ReductionFunctions()
{
   registerInstruction(latte::SQ_OP2_INST_CUBE, CUBE);
   registerInstruction(latte::SQ_OP2_INST_DOT4, DOT4);
   registerInstruction(latte::SQ_OP2_INST_DOT4_IEEE, DOT4);
}

void
registerOP3ReductionFunctions()
{

}

} // namespace glsl2
