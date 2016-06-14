#pragma once
#include <vector>
#include "libcpu/espresso/espresso_instructionid.h"

using espresso::InstructionID;

static const auto gIntegerArithmetic =
{
   InstructionID::add,
   InstructionID::addc,
   InstructionID::adde,
   InstructionID::addi,
   InstructionID::addic,
   InstructionID::addicx,
   InstructionID::addis,
   InstructionID::addme,
   InstructionID::addze,
   InstructionID::divw,
   InstructionID::divwu,
   InstructionID::mulhw,
   InstructionID::mulhwu,
   InstructionID::mulli,
   InstructionID::mullw,
   InstructionID::neg,
   InstructionID::subf,
   InstructionID::subfc,
   InstructionID::subfe,
   InstructionID::subfic,
   InstructionID::subfme,
   InstructionID::subfze,
};

static const auto gIntegerLogical =
{
   InstructionID::and_,
   InstructionID::andc,
   InstructionID::andi,
   InstructionID::andis,
   InstructionID::cntlzw,
   InstructionID::eqv,
   InstructionID::extsb,
   InstructionID::extsh,
   InstructionID::nand,
   InstructionID::nor,
   InstructionID::or_,
   InstructionID::orc,
   InstructionID::ori,
   InstructionID::oris,
   InstructionID::xor_,
   InstructionID::xori,
   InstructionID::xoris,
};

static const auto gIntegerCompare =
{
   InstructionID::cmp,
   InstructionID::cmpi,
   InstructionID::cmpl,
   InstructionID::cmpli,
};

static const auto gIntegerShift =
{
   InstructionID::slw,
   InstructionID::sraw,
   InstructionID::srawi,
   InstructionID::srw,
};

static const auto gIntegerRotate =
{
   InstructionID::rlwimi,
   InstructionID::rlwinm,
   InstructionID::rlwnm,
};

static const auto gConditionRegisterLogical =
{
   InstructionID::crand,
   InstructionID::crandc,
   InstructionID::creqv,
   InstructionID::crnand,
   InstructionID::crnor,
   InstructionID::cror,
   InstructionID::crorc,
   InstructionID::crxor,
   //InstructionID::mcrf,
};

static const auto gFloatArithmetic =
{
   InstructionID::fadd,
   InstructionID::fadds,
   InstructionID::fdiv,
   InstructionID::fdivs,
   InstructionID::fmul,
   InstructionID::fmuls,
   InstructionID::fres,
   InstructionID::fsub,
   InstructionID::fsubs,
   InstructionID::fsel,
};

static const auto gFloatArithmeticMuladd =
{
   InstructionID::fmadd,
   InstructionID::fmadds,
   InstructionID::fmsub,
   InstructionID::fmsubs,
   InstructionID::fnmadd,
   InstructionID::fnmadds,
   InstructionID::fnmsub,
   InstructionID::fnmsubs,
};

static const auto gFloatRound =
{
   InstructionID::fctiw,
   InstructionID::fctiwz,
   InstructionID::frsp,
};

static const auto gFloatMove =
{
   InstructionID::fabs,
   InstructionID::fmr,
   InstructionID::fnabs,
   InstructionID::fneg,
};

static const auto gFloatCompare =
{
   InstructionID::fcmpo,
   InstructionID::fcmpu,
};

static const auto gTestInstructions =
{
   gIntegerArithmetic,
   gIntegerLogical,
   gIntegerCompare,
   gIntegerShift,
   gIntegerRotate,
   gConditionRegisterLogical,
   gFloatArithmetic,
   gFloatArithmeticMuladd,
   gFloatRound,
   gFloatMove,
};
