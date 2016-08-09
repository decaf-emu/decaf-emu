#pragma once
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include "state.h"

template<typename T>
static std::string toHexString(T i)
{
   std::ostringstream oss;
   oss << "0x" << std::hex << i;
   return oss.str();
}

inline bool dbgStateCmp(cpu::Core* state, cpu::Core* estate, std::vector<std::string>& errors)
{
   if (memcmp(state, estate, sizeof(cpu::Core)) == 0) {
      return true;
   }

#define CHECKONE(n, m) if (state->n != estate->n) errors.push_back(std::string(m) + " (got:" + toHexString(state->n)  + " expected:" + toHexString(estate->n) + ")")
#define CHECKONEI(n, m, i) CHECKONE(n, std::string(m) + "[" + std::to_string(i) + "]")
   CHECKONE(cia, "CIA");
   CHECKONE(nia, "NIA");
   for (auto i = 0; i < 32; ++i) {
      CHECKONEI(gpr[i], "GPR", i);
   }
   for (auto i = 0; i < 32; ++i) {
      CHECKONEI(fpr[i].idw, "FPR", i);
   }
   CHECKONE(cr.value, "CR");
   CHECKONE(xer.value, "XER");
   CHECKONE(lr, "CTR");
   CHECKONE(ctr, "CTR");
   CHECKONE(fpscr.value, "FPSCR");
   CHECKONE(pvr.value, "PVR");
   CHECKONE(msr.value, "MSR");
   for (auto i = 0; i < 16; ++i) {
      CHECKONEI(sr[i], "SR", i);
   }
   for (auto i = 0; i < 8; ++i) {
      CHECKONEI(gqr[i].value, "GQR", i);
   }
#undef CHECKONEI
#undef CHECKONE

   return false;
}
