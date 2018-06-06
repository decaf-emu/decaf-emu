#pragma once
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <vector>
#include <libcpu/state.h>
#include <libcpu/espresso/espresso_instruction.h>
#include <libcpu/espresso/espresso_registerformats.h>
#include <common/be_val.h>

namespace hwtest
{

static const auto GPR_BASE = 3;
static const auto FPR_BASE = 1;
static const auto CRF_BASE = 2;
static const auto CRB_BASE = 8;

struct RegisterState
{
   espresso::FixedPointExceptionRegister xer;
   espresso::ConditionRegister cr;
   espresso::FloatingPointStatusAndControlRegister fpscr;
   uint32_t ctr;
   uint32_t gpr[4];
   double fr[4];

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(xer.value);
      ar(cr.value);
      ar(fpscr.value);
      ar(ctr);

      for (auto i = 0; i < 4; ++i) {
         ar(gpr[i]);
      }

      for (auto i = 0; i < 4; ++i) {
         ar(fr[i]);
      }
   }
};

struct TestData
{
   espresso::Instruction instr;
   RegisterState input;
   RegisterState output;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(instr.value);
      ar(input);
      ar(output);
   }
};

struct TestFile
{
   std::string name;
   std::vector<TestData> tests;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(tests);
   }
};

int runTests(const std::string &path);

} // namespace hwtest
