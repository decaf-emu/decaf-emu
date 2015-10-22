#pragma once
#include <vector>
#include <string>
#include "state.h"

template<typename T>
static std::string to_hex_string(T i) {
   std::ostringstream oss;
   oss << "0x" << std::hex << i;
   return oss.str();
}
static inline bool dbgStateCmp(ThreadState* state, ThreadState* estate, std::vector<std::string>& errors) {
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
      CHECKONEI(fpr[i].idw, "FPR{0}", i);
      CHECKONEI(fpr[i].idw1, "FPR{1}", i);
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
   CHECKONE(tbu, "TBU");
   CHECKONE(tbl, "TBL");
   for (auto i = 0; i < 8; ++i) {
      CHECKONEI(gqr[i].value, "GQR", i);
   }
   CHECKONE(reserve, "reserve");
   CHECKONE(reserveAddress, "reserveAddress");
   CHECKONE(reserveData, "reserveData");
#undef CHECKONEI
#undef CHECKONE

   return false;
}