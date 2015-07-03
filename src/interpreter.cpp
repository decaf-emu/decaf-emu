#include <sstream>
#include "disassembler.h"
#include "interpreter.h"
#include "instructiondata.h"
#include "memory.h"
#include "log.h"

Interpreter
gInterpreter;

static std::vector<instrfptr_t>
sInstructionMap;

void Interpreter::RegisterFunctions()
{
   static bool didInit = false;

   if (!didInit) {
      // Reserve instruction map
      sInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);

      // Register instruction handlers
      registerBranchInstructions();
      registerConditionInstructions();
      registerFloatInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerPairedInstructions();
      registerSystemInstructions();

      didInit = true;
   }
}

void Interpreter::registerInstruction(InstructionID id, instrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

template<typename T>
std::string to_hex_string(T i) {
   std::ostringstream oss;
   oss << "0x" << std::hex << i;
   return oss.str();
}
bool dbgStateCmp(ThreadState* state, ThreadState* estate, std::vector<std::string>& errors) {
   if (memcmp(state, estate, sizeof(ThreadState)) == 0) {
      return true;
   }

#define CHECKONE(n, m) if (state->n != estate->n) errors.push_back(std::string(m) + " (got:" + to_hex_string(state->n)  + " expected:" + to_hex_string(estate->n) + ")")
#define CHECKONEI(n, m, i) CHECKONE(n, std::string(m) + "[" + std::to_string(i) + "]")
   CHECKONE(cia, "CIA");
   CHECKONE(nia, "NIA");
   for (auto i = 0; i < 32; ++i) {
      CHECKONEI(gpr[i], "GPR", i);
   }
   for (auto i = 0; i < 32; ++i) {
      CHECKONEI(fpr[i].idw, "GPR", i);
   }
   CHECKONE(cr.value, "XER");
   CHECKONE(xer.value, "XER");
   CHECKONE(lr, "CTR");
   CHECKONE(ctr, "CTR");
   CHECKONE(fpscr.value, "FPSCR");
   CHECKONE(pvr.value, "PVR");
   CHECKONE(msr.value, "MSR");
   for (auto i = 0; i < 16; ++i) {
      CHECKONEI(sr[i], "SR", i);
   }
   CHECKONE(tbu, "TBU");
   CHECKONE(tbl, "TBL");
   for (auto i = 0; i < 8; ++i) {
      CHECKONEI(gqr[i].value, "GQR", i);
   }
   CHECKONE(reserve, "reserve");
   CHECKONE(reserveAddress, "reserveAddress");
#undef CHECKONEI
#undef CHECKONE

   return false;
}

void Interpreter::setJitMode(InterpJitMode val) {
   mJitMode = val;
}

// Address used to signify a return to emulator-land.
const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

void
Interpreter::execute(ThreadState *state)
{
   bool hasJumped = false;

   while (state->nia != CALLBACK_ADDR) {
      // JIT Attempt!
      if (mJitMode == InterpJitMode::Enabled) {
         if (state->nia != state->cia + 4) {
            // We jumped, try to enter JIT
            JitCode jitFn = gJitManager.get(state->nia);
            if (jitFn) {
               auto newNia = gJitManager.execute(state, jitFn);
               state->cia = 0;
               state->nia = newNia;
               continue;
            }
         }
      }

      // Interpreter Loop!
      state->cia = state->nia;
      state->nia = state->cia + 4;

      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);

      if (!data) {
         xLog() << Log::hex(state->cia) << " " << Log::hex(instr.value);
      }

      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
      auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), state->cia);

      if (bpitr != mBreakpoints.end()) {
         xLog() << "Hit breakpoint!";
      }

      if (!fptr) {
         xLog() << "Unimplemented interpreter instruction!";
      } else {
         if (mJitMode != InterpJitMode::Debug) {
            fptr(state, instr);
         } else {
            // Save original state for debugging
            ThreadState ostate = *state;

            // Save thread-state for JIT run.
            ThreadState jstate = *state;

            // Fetch the JIT instruction block
            JitCode jitInstr = gJitManager.getSingle(state->cia);
            if (jitInstr == nullptr) {
               DebugBreak();
            }

            // Execute with Interpreter
            fptr(state, instr);

            // Kernel calls are not stateless
            if (data->id != InstructionID::kc) {
               // Execute with JIT
               jstate.nia = gJitManager.execute(&jstate, jitInstr);

               // Ensure compliance!
               std::vector<std::string> errors;
               if (!dbgStateCmp(&jstate, state, errors)) {
                  DebugBreak();
               }
            }
         }
      }
   }
}

void
Interpreter::execute(ThreadState *state, uint32_t addr)
{
   auto saveLR = state->lr;
   auto saveCIA = state->cia;
   auto saveNIA = state->nia;

   state->lr = CALLBACK_ADDR;
   state->cia = 0;
   state->nia = addr;

   execute(state);

   state->lr = saveLR;
   state->cia = saveCIA;
   state->nia = saveNIA;
}

void Interpreter::addBreakpoint(uint32_t addr)
{
   mBreakpoints.push_back(addr);
}
