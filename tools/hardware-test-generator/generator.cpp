#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <common/be_val.h>
#include <common/bitutils.h>
#include "libcpu/state.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include "hardware-test/hardwaretests.h"
#include "generator_testlist.h"
#include "generator_valuelist.h"

using namespace espresso;

std::shared_ptr<spdlog::logger>
gLog;

static void
setCRB(hwtest::RegisterState &state, uint32_t bit, uint32_t value)
{
   state.cr.value = set_bit_value(state.cr.value, 31 - bit, value);
}

static void
generateTests(InstructionInfo *data)
{
   std::vector<size_t> indexCur, indexMax;
   std::vector<bool> flagSet;
   hwtest::TestFile testFile;
   auto complete = false;
   auto completeIndices = false;

   for (auto i = 0; i < data->read.size(); ++i) {
      auto &field = data->read[i];
      indexCur.push_back(0);

      switch (field) {
      case InstructionField::rA:
      case InstructionField::rB:
      case InstructionField::rS:
         indexMax.push_back(gValuesGPR.size());
         break;
      case InstructionField::frA:
      case InstructionField::frB:
      case InstructionField::frC:
      case InstructionField::frS:
         indexMax.push_back(gValuesFPR.size());
         break;
      case InstructionField::crbA:
      case InstructionField::crbB:
         indexMax.push_back(gValuesCRB.size());
         break;
      case InstructionField::simm:
         indexMax.push_back(gValuesSIMM.size());
         break;
      case InstructionField::sh:
         indexMax.push_back(gValuesSH.size());
         break;
      case InstructionField::mb:
         indexMax.push_back(gValuesMB.size());
         break;
      case InstructionField::me:
         indexMax.push_back(gValuesME.size());
         break;
      case InstructionField::uimm:
         indexMax.push_back(gValuesUIMM.size());
         break;
      case InstructionField::XERC:
         indexMax.push_back(gValuesXERC.size());
         break;
      case InstructionField::XERSO:
         indexMax.push_back(gValuesXERSO.size());
         break;
      default:
         assert(false);
      }
   }

   for (auto i = 0; i < data->flags.size(); ++i) {
      flagSet.push_back(false);
   }

   while (!complete) {
      uint32_t gpr = 0, fpr = 0, crf = 0, crb = 0;
      hwtest::TestData test;
      memset(&test, 0, sizeof(hwtest::TestData));

      test.instr = encodeInstruction(data->id);

      for (auto i = 0; i < data->read.size(); ++i) {
         auto index = indexCur[i];

         // Generate read field values
         switch (data->read[i]) {
         case InstructionField::rA:
            test.instr.rA = gpr + hwtest::GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case InstructionField::rB:
            test.instr.rB = gpr + hwtest::GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case InstructionField::rS:
            test.instr.rS = gpr + hwtest::GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case InstructionField::frA:
            test.instr.frA = fpr + hwtest::FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case InstructionField::frB:
            test.instr.frB = fpr + hwtest::FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case InstructionField::frC:
            test.instr.frC = fpr + hwtest::FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case InstructionField::frS:
            test.instr.frS = fpr + hwtest::FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case InstructionField::crbA:
            test.instr.crbA = (crb++) + hwtest::CRB_BASE;
            setCRB(test.input, test.instr.crbA, gValuesCRB[index]);
            break;
         case InstructionField::crbB:
            test.instr.crbB = (crb++) + hwtest::CRB_BASE;
            setCRB(test.input, test.instr.crbB, gValuesCRB[index]);
            break;
         case InstructionField::simm:
            test.instr.simm = gValuesSIMM[index];
            break;
         case InstructionField::sh:
            test.instr.sh = gValuesSH[index];
            break;
         case InstructionField::mb:
            test.instr.mb = gValuesMB[index];
            break;
         case InstructionField::me:
            test.instr.me = gValuesME[index];
            break;
         case InstructionField::uimm:
            test.instr.uimm = gValuesUIMM[index];
            break;
         case InstructionField::XERC:
            test.input.xer.ca = gValuesXERC[index];
            break;
         case InstructionField::XERSO:
            test.input.xer.so = gValuesXERSO[index];
            break;
         default:
            assert(false);
         }
      }

      // Generate write field values
      for (auto field : data->write) {
         switch (field) {
         case InstructionField::rA:
            test.instr.rA = gpr + hwtest::GPR_BASE;
            gpr++;
            break;
         case InstructionField::rD:
            test.instr.rD = gpr + hwtest::GPR_BASE;
            gpr++;
            break;
         case InstructionField::frD:
            test.instr.frD = fpr + hwtest::FPR_BASE;
            fpr++;
            break;
         case InstructionField::crfD:
            test.instr.crfD = crf + hwtest::CRF_BASE;
            crf++;
            break;
         case InstructionField::crbD:
            test.instr.crbD = crb + hwtest::CRB_BASE;
            crb++;
            break;
         case InstructionField::XERC:
         case InstructionField::XERSO:
         case InstructionField::FCRISI:
         case InstructionField::FCRZDZ:
         case InstructionField::FCRIDI:
         case InstructionField::FCRSNAN:
            break;
         default:
            assert(false);
         }
      }

      testFile.tests.emplace_back(test);

      // Increase indices
      for (auto i = 0; i < indexCur.size(); ++i) {
         indexCur[i]++;

         if (indexCur[i] < indexMax[i]) {
            break;
         } else if (indexCur[i] == indexMax[i]) {
            indexCur[i] = 0;

            if (i == indexCur.size() - 1) {
               completeIndices = true;
            }
         }
      }

      if (completeIndices) {
         if (flagSet.size() == 0) {
            complete = true;
            break;
         }

         completeIndices = false;

         // Do next flag!
         for (auto i = 0; i < flagSet.size(); ++i) {
            if (!flagSet[i]) {
               flagSet[i] = true;
               break;
            } else {
               flagSet[i] = false;

               if (i == flagSet.size() - 1) {
                  complete = true;
                  break;
               }
            }
         }
      }
   }

   // Save tests to file
   auto filename = std::string("tests/cpu/input/") + data->name;
   std::ofstream out { filename, std::ofstream::out | std::ofstream::binary };
   cereal::BinaryOutputArchive archive(out);
   archive(testFile);
}

int main(int argc, char **argv)
{
   std::vector<spdlog::sink_ptr> sinks;
   sinks.push_back(spdlog::sinks::stdout_sink_st::instance());
   gLog = std::make_shared<spdlog::logger>("decaf", begin(sinks), end(sinks));

   initialiseInstructionSet();

   for (auto &group : gTestInstructions) {
      for (auto id : group) {
         auto data = findInstructionInfo(id);
         generateTests(data);
      }
   }

   return 0;
}

/*
Unimplemented instructions:

// Floating-Point Status and Control Register
INS(mcrfs, (crfD), (crfS), (), (opcd == 63, xo1 == 64, !_9_10, !_14_15, !_16_20, !_31), "")
INS(mffs, (frD), (), (rc), (opcd == 63, xo1 == 583, !_11_15, !_16_20), "")
INS(mtfsf, (), (fm, frB), (rc), (opcd == 63, xo1 == 711, !_6, !_15), "")
INS(mtfsfi, (crfD), (), (rc, imm), (opcd == 63, xo1 == 134, !_9_10, !_11_15, !_20), "")

// Integer Load
INS(lbz, (rD), (rA, d), (), (opcd == 34), "Load Byte and Zero")
INS(lbzu, (rD, rA), (rA, d), (), (opcd == 35), "Load Byte and Zero with Update")
INS(lbzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 87, !_31), "Load Byte and Zero Indexed")
INS(lbzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 119, !_31), "Load Byte and Zero with Update Indexed")
INS(lha, (rD), (rA, d), (), (opcd == 42), "Load Half Word Algebraic")
INS(lhau, (rD, rA), (rA, d), (), (opcd == 43), "Load Half Word Algebraic with Update")
INS(lhax, (rD), (rA, rB), (), (opcd == 31, xo1 == 343, !_31), "Load Half Word Algebraic Indexed")
INS(lhaux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 375, !_31), "Load Half Word Algebraic with Update Indexed")
INS(lhz, (rD), (rA, d), (), (opcd == 40), "Load Half Word and Zero")
INS(lhzu, (rD, rA), (rA, d), (), (opcd == 41), "Load Half Word and Zero with Update")
INS(lhzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 279, !_31), "Load Half Word and Zero Indexed")
INS(lhzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 311, !_31), "Load Half Word and Zero with Update Indexed")
INS(lwz, (rD), (rA, d), (), (opcd == 32), "Load Word and Zero")
INS(lwzu, (rD, rA), (rA, d), (), (opcd == 33), "Load Word and Zero with Update")
INS(lwzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 23, !_31), "Load Word and Zero Indexed")
INS(lwzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 55, !_31), "Load Word and Zero with Update Indexed")

// Integer Store
INS(stb, (), (rS, rA, d), (), (opcd == 38), "Store Byte")
INS(stbu, (rA), (rS, rA, d), (), (opcd == 39), "Store Byte with Update")
INS(stbx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 215, !_31), "Store Byte Indexed")
INS(stbux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 247, !_31), "Store Byte with Update Indexed")
INS(sth, (), (rS, rA, d), (), (opcd == 44), "Store Half Word")
INS(sthu, (rA), (rS, rA, d), (), (opcd == 45), "Store Half Word with Update")
INS(sthx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 407, !_31), "Store Half Word Indexed")
INS(sthux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 439, !_31), "Store Half Word with Update Indexed")
INS(stw, (), (rS, rA, d), (), (opcd == 36), "Store Word")
INS(stwu, (rA), (rS, rA, d), (), (opcd == 37), "Store Word with Update")
INS(stwx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 151, !_31), "Store Word Indexed")
INS(stwux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 183, !_31), "Store Word with Update Indexed")

// Integer Load and Store with Byte Reverse
INS(lhbrx, (rD), (rA, rB), (), (opcd == 31, xo1 == 790, !_31), "Load Half Word Byte-Reverse Indexed")
INS(lwbrx, (rD), (rA, rB), (), (opcd == 31, xo1 == 534, !_31), "Load Word Byte-Reverse Indexed")
INS(sthbrx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 918, !_31), "Store Half Word Byte-Reverse Indexed")
INS(stwbrx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 662, !_31), "Store Word Byte-Reverse Indexed")

// Integer Load and Store Multiple
INS(lmw, (rD), (rA, d), (), (opcd == 46), "Load Multiple Words")
INS(stmw, (), (rS, rA, d), (), (opcd == 47), "Store Multiple Words")

// Integer Load and Store String
INS(lswi, (rD), (rA, nb), (), (opcd == 31, xo1 == 597, !_31), "Load String Word Immediate")
INS(lswx, (rD), (rA, rB), (), (opcd == 31, xo1 == 533, !_31), "Load String Word Indexed")
INS(stswi, (), (rS, rA, nb), (), (opcd == 31, xo1 == 725, !_31), "Store String Word Immediate")
INS(stswx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 661, !_31), "Store String Word Indexed")

// Memory Synchronisation
INS(eieio, (), (), (), (opcd == 31, xo1 == 854, !_6_10, !_11_15, !_16_20, !_31), "Enforce In-Order Execution of I/O")
INS(isync, (), (), (), (opcd == 19, xo1 == 150, !_6_10, !_11_15, !_16_20, !_31), "Instruction Synchronise")
INS(lwarx, (rD, RSRV), (rA, rB), (), (opcd == 31, xo1 == 20, !_31), "Load Word and Reserve Indexed")
INS(stwcx, (RSRV), (rS, rA, rB), (), (opcd == 31, xo1 == 150, _31 == 1), "Store Word Conditional Indexed")
INS(sync, (), (), (), (opcd == 31, xo1 == 598, !_6_10, !_11_15, !_16_20, !_31), "Synchronise")

// Floating-Point Load
INS(lfd, (frD), (rA, d), (), (opcd == 50), "Load Floating-Point Double")
INS(lfdu, (frD, rA), (rA, d), (), (opcd == 51), "Load Floating-Point Double with Update")
INS(lfdx, (frD), (rA, rB), (), (opcd == 31, xo1 == 599, !_31), "Load Floating-Point Double Indexed")
INS(lfdux, (frD, rA), (rA, rB), (), (opcd == 31, xo1 == 631, !_31), "Load Floating-Point Double with Update Indexed")
INS(lfs, (frD), (rA, d), (), (opcd == 48), "Load Floating-Point Single")
INS(lfsu, (frD, rA), (rA, d), (), (opcd == 49), "Load Floating-Point Single with Update")
INS(lfsx, (frD), (rA, rB), (), (opcd == 31, xo1 == 535, !_31), "Load Floating-Point Single Indexed")
INS(lfsux, (frD, rA), (rA, rB), (), (opcd == 31, xo1 == 567, !_31), "Load Floating-Point Single with Update Indexed")

// Floating-Point Store
INS(stfd, (), (frS, rA, d), (), (opcd == 54), "Store Floating-Point Double")
INS(stfdu, (rA), (frS, rA, d), (), (opcd == 55), "Store Floating-Point Double with Update")
INS(stfdx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 727, !_31), "Store Floating-Point Double Indexed")
INS(stfdux, (rA), (frS, rA, rB), (), (opcd == 31, xo1 == 759, !_31), "Store Floating-Point Double with Update Indexed")
INS(stfiwx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 983, !_31), "Store Floating-Point as Integer Word Indexed")
INS(stfs, (), (frS, rA, d), (), (opcd == 52), "Store Floating-Point Single")
INS(stfsu, (rA), (frS, rA, d), (), (opcd == 53), "Store Floating-Point Single with Update")
INS(stfsx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 663, !_31), "Store Floating-Point Single Indexed")
INS(stfsux, (rA), (frS, rA, rB), (), (opcd == 31, xo1 == 695, !_31), "Store Floating-Point Single with Update Indexed")

// Branch
INS(b, (), (li), (aa, lk), (opcd == 18), "Branch")
INS(bc, (bo), (bi, bd), (aa, lk), (opcd == 16), "Branch Conditional")
INS(bcctr, (bo), (bi, CTR), (lk), (opcd == 19, xo1 == 528, !_16_20), "Branch Conditional to CTR")
INS(bclr, (bo), (bi, LR), (lk), (opcd == 19, xo1 == 16, !_16_20), "Branch Conditional to LR")

// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50, !_6_10, !_11_15, !_16_20, !_31), "")
INS(sc, (), (), (), (opcd == 17, !_6_10, !_11_15, !_16_29, _30 == 1, !_31), "Syscall")
INS(kc, (), (kcn), (), (opcd == 1), "krncall")

// Trap
INS(tw, (), (to, rA, rB), (), (opcd == 31, xo1 == 4, !_31), "")
INS(twi, (), (to, rA, simm), (), (opcd == 3), "")

// Processor Control
INS(mcrxr, (crfD), (XERO), (), (opcd == 31, xo1 == 512, !_9_10, !_11_15, !_16_20, !_31), "Move to Condition Register from XERO")
INS(mfcr, (rD), (), (), (opcd == 31, xo1 == 19, !_11_15, !_16_20, !_31), "Move from Condition Register")
INS(mfmsr, (rD), (), (), (opcd == 31, xo1 == 83, !_11_15, !_16_20, !_31), "Move from Machine State Register")
INS(mfspr, (rD), (spr), (), (opcd == 31, xo1 == 339, !_31), "Move from Special Purpose Register")
INS(mftb, (rD), (tbr), (), (opcd == 31, xo1 == 371, !_31), "Move from Time Base Register")
INS(mtcrf, (crm), (rS), (), (opcd == 31, xo1 == 144, !_11, !_20, !_31), "Move to Condition Register Fields")
INS(mtmsr, (), (rS), (), (opcd == 31, xo1 == 146, !_11_15, !_16_20, !_31), "Move to Machine State Register")
INS(mtspr, (spr), (rS), (), (opcd == 31, xo1 == 467, !_31), "Move to Special Purpose Register")

// Cache Management
INS(dcbf, (), (rA, rB), (), (opcd == 31, xo1 == 86, !_6_10, !_31), "")
INS(dcbi, (), (rA, rB), (), (opcd == 31, xo1 == 470, !_6_10, !_31), "")
INS(dcbst, (), (rA, rB), (), (opcd == 31, xo1 == 54, !_6_10, !_31), "")
INS(dcbt, (), (rA, rB), (), (opcd == 31, xo1 == 278, !_6_10, !_31), "")
INS(dcbtst, (), (rA, rB), (), (opcd == 31, xo1 == 246, !_6_10, !_31), "")
INS(dcbz, (), (rA, rB), (), (opcd == 31, xo1 == 1014, !_6_10, !_31), "")
INS(icbi, (), (rA, rB), (), (opcd == 31, xo1 == 982, !_6_10, !_31), "")
INS(dcbz_l, (), (rA, rB), (), (opcd == 4, xo1 == 1014, !_6_10, !_31), "")

// Segment Register Manipulation
INS(mfsr, (rD), (sr), (), (opcd == 31, xo1 == 595, !_11, !_16_20, !_31), "Move from Segment Register")
INS(mfsrin, (rD), (rB), (), (opcd == 31, xo1 == 659, !_11_15, !_31), "Move from Segment Register Indirect")
INS(mtsr, (), (rD, sr), (), (opcd == 31, xo1 == 210, !_11, !_16_20, !_31), "Move to Segment Register")
INS(mtsrin, (), (rD, rB), (), (opcd == 31, xo1 == 242, !_11_15, !_31), "Move to Segment Register Indirect")

// Lookaside Buffer Management
INS(tlbie, (), (rB), (), (opcd == 31, xo1 == 306, !_6_10, !_11_15, !_31), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566, !_6_10, !_11_15, !_16_20, !_31), "")

// External Control
INS(eciwx, (rD), (rA, rB), (), (opcd == 31, xo1 == 310, !_31), "")
INS(ecowx, (rD), (rA, rB), (), (opcd == 31, xo1 == 438, !_31), "")

// Paired-Single Load and Store
INS(psq_l, (frD), (rA, qd), (w, i), (opcd == 56), "Paired Single Load")
INS(psq_lu, (frD), (rA, qd), (w, i), (opcd == 57), "Paired Single Load with Update")
INS(psq_lx, (frD), (rA, rB), (qw, qi), (opcd == 4, xo3 == 6, !_31), "Paired Single Load Indexed")
INS(psq_lux, (frD), (rA, rB), (qw, qi), (opcd == 4, xo3 == 38, !_31), "Paired Single Load with Update Indexed")
INS(psq_st, (frD), (rA, qd), (w, i), (opcd == 60), "Paired Single Store")
INS(psq_stu, (frD), (rA, qd), (w, i), (opcd == 61), "Paired Single Store with Update")
INS(psq_stx, (frS), (rA, rB), (qw, qi), (opcd == 4, xo3 == 7, !_31), "Paired Single Store Indexed")
INS(psq_stux, (frS), (rA, rB), (qw, qi), (opcd == 4, xo3 == 39, !_31), "Paired Single Store with Update Indexed")

// Paired-Single Floating Point Arithmetic
INS(ps_add, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 21, !_21_25), "Paired Single Add")
INS(ps_div, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 18, !_21_25), "Paired Single Divide")
INS(ps_mul, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 25, !_16_20), "Paired Single Multiply")
INS(ps_sub, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 20, !_21_25), "Paired Single Subtract")
INS(ps_abs, (frD), (frB), (rc), (opcd == 4, xo1 == 264, !_11_15), "Paired Single Absolute")
INS(ps_nabs, (frD), (frB), (rc), (opcd == 4, xo1 == 136, !_11_15), "Paired Single Negate Absolute")
INS(ps_neg, (frD), (frB), (rc), (opcd == 4, xo1 == 40, !_11_15), "Paired Single Negate")
INS(ps_sel, (frD), (frA, frB, frC), (rc), (opcd == 4, xo4 == 23), "Paired Single Select")
INS(ps_res, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 24, !_11_15, !_21_25), "Paired Single Reciprocal")
INS(ps_rsqrte, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 26, !_11_15, !_21_25), "Paired Single Reciprocal Square Root Estimate")
INS(ps_msub, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 28), "Paired Single Multiply and Subtract")
INS(ps_madd, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 29), "Paired Single Multiply and Add")
INS(ps_nmsub, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 30), "Paired Single Negate Multiply and Subtract")
INS(ps_nmadd, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 31), "Paired Single Negate Multiply and Add")
INS(ps_mr, (frD), (frB), (rc), (opcd == 4, xo1 == 72, !_11_15), "Paired Single Move Register")
INS(ps_sum0, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 10), "Paired Single Sum High")
INS(ps_sum1, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 11), "Paired Single Sum Low")
INS(ps_muls0, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 12, !_16_20), "Paired Single Multiply Scalar High")
INS(ps_muls1, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 13, !_16_20), "Paired Single Multiply Scalar Low")
INS(ps_madds0, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 14), "Paired Single Multiply and Add Scalar High")
INS(ps_madds1, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 15), "Paired Single Multiply and Add Scalar Low")
INS(ps_cmpu0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 0, !_9_10, !_31), "Paired Single Compare Unordered High")
INS(ps_cmpo0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 32, !_9_10, !_31), "Paired Single Compare Ordered High")
INS(ps_cmpu1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 64, !_9_10, !_31), "Paired Single Compare Unordered Low")
INS(ps_cmpo1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 96, !_9_10, !_31), "Paired Single Compare Ordered Low")
INS(ps_merge00, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 528), "Paired Single Merge High")
INS(ps_merge01, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 560), "Paired Single Merge Direct")
INS(ps_merge10, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 592), "Paired Single Merge Swapped")
INS(ps_merge11, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 624), "Paired Single Merge Low")
*/
