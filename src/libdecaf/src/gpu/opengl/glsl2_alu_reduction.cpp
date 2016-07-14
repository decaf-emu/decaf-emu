#include "glsl2_alu.h"
#include "glsl2_translate.h"

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
   // x/y/z can be anything as long as they're distinct, and some shaders
   // have them in different elements presumably due to optimization, so
   // we need to detect what this particular instruction is using before
   // we verify the syntax.
   const latte::SQ_CHAN xChan = group[2].word0.SRC0_CHAN();
   const latte::SQ_CHAN yChan = group[3].word0.SRC0_CHAN();
   const latte::SQ_CHAN zChan = group[0].word0.SRC0_CHAN();
   if (xChan == yChan || xChan == zChan || yChan == zChan) {
      throw translate_exception("Invalid CUBE syntax: X/Y/Z elements are not distinct");
   }
   for (auto i = 0u; i < group.size(); ++i) {
      if (group[i].word0.SRC0_SEL() != group[i].word0.SRC1_SEL()
       || group[i].word0.SRC0_REL() != group[i].word0.SRC1_REL()) {
         throw translate_exception(fmt::format("Invalid CUBE syntax: register mismatch in element {}", i));
      }
      // ABS/NEG might actually work as long as the flags are consistent
      // for all uses of that channel in the instruction, but let's wait
      // to see a real-world example before we deal with that.
      if (group[i].op2.SRC0_ABS() || group[i].op2.SRC1_ABS()
       || group[i].word0.SRC0_NEG() || group[i].word0.SRC1_NEG()) {
         throw translate_exception(fmt::format("Invalid CUBE syntax: ABS/NEG used in element {}", i));
      }
   }
   if (group[1].word0.SRC0_CHAN() != zChan
    || group[0].word0.SRC1_CHAN() != yChan
    || group[1].word0.SRC1_CHAN() != xChan
    || group[2].word0.SRC1_CHAN() != zChan
    || group[3].word0.SRC1_CHAN() != zChan) {
      throw translate_exception(fmt::format("Invalid CUBE syntax: incorrect swizzling (detected x/y/z in channels {}/{}/{})", xChan, yChan, zChan));
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

   const AluInst &xInsn = group[2];
   const AluInst &yInsn = group[3];
   const AluInst &zInsn = group[0];

   insertLineStart(state);
   state.out << "if (abs(";
   insertSource0(state, state.out, cf, xInsn);
   state.out << ") >= abs(";
   insertSource0(state, state.out, cf, yInsn);
   state.out << ") && abs(";
   insertSource0(state, state.out, cf, xInsn);
   state.out << ") >= abs(";
   insertSource0(state, state.out, cf, zInsn);
   state.out << ")) {";
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_X);
      insertSource0(state, state.out, cf, yInsn);
      state.out << ";";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Y);
      state.out << "sign(";
      insertSource0(state, state.out, cf, xInsn);
      state.out << ") * ";
      insertSource0(state, state.out, cf, zInsn);
      state.out << ";";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Z);
      insertSource0(state, state.out, cf, xInsn);
      state.out << " * 2.0;";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_W);
      insertSource0(state, state.out, cf, xInsn);
      state.out << " >= 0 ? 0 : 1;";
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   state.out << "} else if (abs(";
   insertSource0(state, state.out, cf, yInsn);
   state.out << ") >= abs(";
   insertSource0(state, state.out, cf, xInsn);
   state.out << ") && abs(";
   insertSource0(state, state.out, cf, yInsn);
   state.out << ") >= abs(";
   insertSource0(state, state.out, cf, zInsn);
   state.out << ")) {";
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_X);
      state.out << "sign(";
      insertSource0(state, state.out, cf, yInsn);
      state.out << ") * -(";
      insertSource0(state, state.out, cf, xInsn);
      state.out << ");";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Y);
      state.out << "-(";
      insertSource0(state, state.out, cf, zInsn);
      state.out << ");";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Z);
      insertSource0(state, state.out, cf, yInsn);
      state.out << " * 2.0;";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_W);
      state.out << "(";
      insertSource0(state, state.out, cf, yInsn);
      state.out << " >= 0) ? 2 : 3;";
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   state.out << "} else {";
   insertLineEnd(state);
   increaseIndent(state);

   {
      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_X);
      insertSource0(state, state.out, cf, yInsn);
      state.out << ";";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Y);
      state.out << "sign(";
      insertSource0(state, state.out, cf, zInsn);
      state.out << ") * -(";
      insertSource0(state, state.out, cf, xInsn);
      state.out << ");";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_Z);
      insertSource0(state, state.out, cf, zInsn);
      state.out << " * 2.0;";
      insertLineEnd(state);

      insertLineStart(state);
      insertPreviousValueUpdate(state.out, SQ_CHAN_W);
      state.out << "(";
      insertSource0(state, state.out, cf, zInsn);
      state.out << " >= 0) ? 4 : 5;";
      insertLineEnd(state);
   }

   decreaseIndent(state);
   insertLineStart(state);
   state.out << "}";
   insertLineEnd(state);
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
         insertDestBegin(state.out, cf, group[i], SQ_CHAN_X);
         break;
      }
   }

   // If no instruction in the group has a dest, then we must still write to PV.x
   if (!hasWriteMask) {
      insertPreviousValueUpdate(state.out, SQ_CHAN_X);
   }

   state.out << "dot(";
   insertSource0Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   state.out << ", ";
   insertSource1Vector(state, state.out, cf, group[0], group[1], group[2], group[3]);
   state.out << ")";

   if (hasWriteMask) {
      insertDestEnd(state.out, cf, group[writeUnit]);
   }

   state.out << ';';
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
