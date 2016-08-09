#pragma once
#include "common/decaf_assert.h"
#include "cpu.h"
#include <array>
#include <asmjit/asmjit.h>
#include <map>
#include <spdlog/fmt/fmt.h>
#include <vector>

#define offsetof2(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))

namespace cpu
{

namespace jit
{

/*
Register Assignments:
RAX    . Scratch
RCX    . Scratch
RDX    . Scratch
RDI    . Scratch
RSI    . Scratch
RBX    . Core*
RBP    . mem::base()
RSP    . Emu Stack Pointer.
R8-R15 . Scratch
*/

class PPCEmuAssembler : public asmjit::X86Assembler
{
private:
   class ErrorHandler : public asmjit::ErrorHandler
   {
   public:
      bool handleError(asmjit::Error code, const char* message, void *origin) noexcept override;
   } errHandler;

public:
   static const auto MaxGpRegSlots = 13;
   static const auto MaxXmmRegSlots = 5;
   static const auto MaxRegSlots = MaxGpRegSlots + MaxXmmRegSlots;

   enum class RegType
   {
      None,
      Gp,
      Xmm
   };

   struct HostRegister {
      HostRegister() { }

      HostRegister(RegType type, uint32_t id)
         : regType(type), regId(id) { }

      // Information about the type of register this is
      RegType regType = RegType::None;
      uint32_t regId = 0;

      // Is currently in use by a Variable
      uint32_t useCount = 0;
      uint32_t lruValue = 0;

      // Information used for automatic eviction
      bool loaded = false;
      bool written = false;
      uint32_t size = 0;
      uint32_t content = 0xFFFFFFFF;
   };

   struct PpcRef {
      uint32_t offset;
      uint32_t size;
   };

   struct PpcGpRef : public PpcRef { };
   struct PpcXmmRef : public PpcRef { };

   PPCEmuAssembler(asmjit::Runtime* runtime) :
      asmjit::X86Assembler(runtime, asmjit::kArchX64)
   {
      setErrorHandler(&errHandler);

      // This holds the platform specific, calling convention arg registers
#ifdef PLATFORM_WINDOWS
      // Windows x64 Calling Convention
      sysArgReg[0] = asmjit::x86::rcx;
      sysArgReg[1] = asmjit::x86::rdx;
#else
      // System-V x64 Calling Convention
      sysArgReg[0] = asmjit::x86::rdi;
      sysArgReg[1] = asmjit::x86::rsi;
#endif

      // Some convenient aliases to make the code more easy to grok.
      finaleNiaArgReg = sysArgReg[0].r32();
      finaleJmpSrcArgReg = sysArgReg[1];

      // These registers must NOT collide with the calling conventions
      //  of any of our platforms, and must be callee-preserved
      stateReg = asmjit::x86::rbx;
      membaseReg = asmjit::x86::rbp;

#define PPCMemRef(s, mm) s = asmjit::X86Mem(stateReg, (int32_t)offsetof2(Core, mm), sizeof(Core::mm))

      PPCMemRef(lrMem, lr);
      PPCMemRef(ctrMem, ctr);

      PPCMemRef(niaMem, nia);
      PPCMemRef(coreIdMem, id);
      PPCMemRef(interruptMem, interrupt);

#undef PPCMemRef

#define PPCRegRef(which, mm) which.offset = (uint32_t)offsetof2(Core, mm); which.size = sizeof(Core::mm);

      for (auto i = 0; i < 32; ++i) {
         PPCRegRef(gpr[i], gpr[i]);
      }

      for (auto i = 0; i < 32; ++i) {
         PPCRegRef(fprps[i], fpr[i]);
      }

      PPCRegRef(cr, cr.value);
      PPCRegRef(xer, xer.value);
      PPCRegRef(fpscr, fpscr.value);

      for (auto i = 0; i < 8; ++i) {
         PPCRegRef(gqr[i], gqr[i].value);
      }

      PPCRegRef(reserve, reserve);

#undef PPCRegRef

      static_assert(std::tuple_size<decltype(mGpRegVals)>::value == 13,
         "Gp register count mismatch");
      mGpRegVals[0] = asmjit::x86::rax;
      mGpRegVals[1] = asmjit::x86::rcx;
      mGpRegVals[2] = asmjit::x86::rdx;
      mGpRegVals[3] = asmjit::x86::rdi;
      mGpRegVals[4] = asmjit::x86::rsi;
      mGpRegVals[5] = asmjit::x86::r8;
      mGpRegVals[6] = asmjit::x86::r9;
      mGpRegVals[7] = asmjit::x86::r10;
      mGpRegVals[8] = asmjit::x86::r11;
      mGpRegVals[9] = asmjit::x86::r12;
      mGpRegVals[10] = asmjit::x86::r13;
      mGpRegVals[11] = asmjit::x86::r14;
      mGpRegVals[12] = asmjit::x86::r15;

      static_assert(std::tuple_size<decltype(mXmmRegVals)>::value == 5,
         "Xmm register count mismatch");
      mXmmRegVals[0] = asmjit::x86::xmm0;
      mXmmRegVals[1] = asmjit::x86::xmm1;
      mXmmRegVals[2] = asmjit::x86::xmm2;
      mXmmRegVals[3] = asmjit::x86::xmm3;
      mXmmRegVals[4] = asmjit::x86::xmm4;

      uint32_t regIdx = 0;
      for (auto i = 0; i < mGpRegVals.size(); ++i) {
         mRegs[regIdx++] = HostRegister(RegType::Gp, i);
      }
      for (auto i = 0; i < mXmmRegVals.size(); ++i) {
         mRegs[regIdx++] = HostRegister(RegType::Xmm, i);
      }
   }

   void shiftTo(asmjit::X86GpReg reg, int s, int d)
   {
      if (s > d) {
         shr(reg, s - d);
      } else if (d > s) {
         shl(reg, d - s);
      }
   }

   uint32_t genCia;
   std::vector<std::pair<uint32_t, asmjit::Label>> relocLabels;

   asmjit::X86GpReg sysArgReg[2];
   asmjit::X86GpReg finaleNiaArgReg;
   asmjit::X86GpReg finaleJmpSrcArgReg;

   asmjit::X86GpReg stateReg;
   asmjit::X86GpReg membaseReg;

   asmjit::X86Mem lrMem;
   asmjit::X86Mem ctrMem;

   asmjit::X86Mem niaMem;
   asmjit::X86Mem coreIdMem;
   asmjit::X86Mem interruptMem;

   PpcGpRef gpr[32];
   PpcXmmRef fprps[32];
   PpcGpRef cr;
   PpcGpRef xer;
   PpcGpRef fpscr;
   PpcGpRef gqr[8];
   PpcGpRef reserve;

   std::array<asmjit::X86GpReg, MaxGpRegSlots> mGpRegVals;
   std::array<asmjit::X86XmmReg, MaxXmmRegSlots> mXmmRegVals;

   std::array<HostRegister, MaxRegSlots> mRegs;

   uint32_t mLruCounter = 0;

   static bool
   isSameRegister(const asmjit::X86Reg &a, const asmjit::X86Reg &b)
   {
      return a.getRegType() == b.getRegType() && a.getRegIndex() == b.getRegIndex();
   }

   bool isSameHostRegister(HostRegister *hreg, const asmjit::X86Reg& reg) {
      if (hreg->regType == RegType::Gp) {
         return isSameRegister(mGpRegVals[hreg->regId], reg);
      } else if (hreg->regType == RegType::Xmm) {
         return isSameRegister(mXmmRegVals[hreg->regId], reg);
      } else if (hreg->regType == RegType::None) {
         return false;
      } else {
         decaf_abort(fmt::format("Unexpected register type {}", static_cast<int>(hreg->regType)));
      }
   }

   class Register {
      friend PPCEmuAssembler;
   public:
      Register()
         : mParent(nullptr), mReg(nullptr), mSize(0), mMarkWrittenOnUse(false)
      {
      }

      Register(const Register &other) {
         if (other.mReg) {
            other.mReg->useCount++;
         }

         mParent = other.mParent;
         mReg = other.mReg;
         mSize = other.mSize;
         mMarkWrittenOnUse = other.mMarkWrittenOnUse;
      }

      Register & operator=(const Register &other) {
         invalidate();

         if (other.mReg) {
            other.mReg->useCount++;
         }

         mParent = other.mParent;
         mReg = other.mReg;
         mSize = other.mSize;
         mMarkWrittenOnUse = other.mMarkWrittenOnUse;
         return *this;
      }

      ~Register()
      {
         if (mReg) {
            mParent->freeRegister(mReg);
         }
      }

      void invalidate() {
         if (mReg) {
            mReg->useCount--;
         }

         mParent = nullptr;
         mReg = nullptr;
         mSize = 0;
         mMarkWrittenOnUse = false;
      }

   private:
      Register(PPCEmuAssembler *parent, HostRegister *varData, uint32_t size, bool writtenOnUse)
         : mParent(parent), mReg(varData), mSize(size), mMarkWrittenOnUse(writtenOnUse)
      {
      }

      void markUsed() const
      {
         decaf_check(mReg);
         mReg->lruValue = mParent->mLruCounter++;
         if (mMarkWrittenOnUse) {
            mReg->written = true;
            mReg->loaded = true;
         }
      }

      PPCEmuAssembler *mParent;
      HostRegister *mReg;
      uint32_t mSize;
      bool mMarkWrittenOnUse;

   };

   class GpRegister : public Register {
      friend PPCEmuAssembler;
   public:
      GpRegister()
         : Register() {}

      GpRegister(const GpRegister &other)
         : Register(other) { }

      GpRegister & operator=(const GpRegister &other) {
         Register::operator=(other);
         return *this;
      }

      operator asmjit::X86GpReg() const {
         decaf_check(mReg->regType == RegType::Gp);
         markUsed();
         if (mSize == 1) {
            return mParent->mGpRegVals[mReg->regId].r8();
         } else if (mSize == 2) {
            return mParent->mGpRegVals[mReg->regId].r16();
         } else if (mSize == 4) {
            return mParent->mGpRegVals[mReg->regId].r32();
         } else if (mSize == 8) {
            return mParent->mGpRegVals[mReg->regId].r64();
         } else {
            decaf_abort(fmt::format("Unexpected register size {}", mSize));
         }
      }

      GpRegister r8() const {
         mReg->useCount++;
         return GpRegister(mParent, mReg, 1, mMarkWrittenOnUse);
      }

      GpRegister r16() const {
         mReg->useCount++;
         return GpRegister(mParent, mReg, 2, mMarkWrittenOnUse);
      }

      GpRegister r32() const {
         mReg->useCount++;
         return GpRegister(mParent, mReg, 4, mMarkWrittenOnUse);
      }

      GpRegister r64() const {
         mReg->useCount++;
         return GpRegister(mParent, mReg, 8, mMarkWrittenOnUse);
      }

   private:
      GpRegister(PPCEmuAssembler *parent, HostRegister *reg, uint32_t size, bool writtenOnUse)
         : Register(parent, reg, size, writtenOnUse) { }

   };

   class XmmRegister : public Register {
      friend PPCEmuAssembler;
   public:
      XmmRegister()
         : Register() {}

      XmmRegister(const XmmRegister &other)
         : Register(other) { }

      XmmRegister & operator=(const XmmRegister &other) {
         Register::operator=(other);
         return *this;
      }

      operator asmjit::X86XmmReg() const {
         decaf_check(mReg->regType == RegType::Xmm);
         decaf_check(mSize == 16);
         markUsed();
         return mParent->mXmmRegVals[mReg->regId];
      }

   private:
      XmmRegister(PPCEmuAssembler *parent, HostRegister *reg, uint32_t size, bool writtenOnUse)
         : Register(parent, reg, size, writtenOnUse) { }

   };

   class RegLockout {
      friend PPCEmuAssembler;
   public:
      bool isRegister(const asmjit::X86Reg &reg) const {
         if (!mReg.mReg) {
            return false;
         }

         return mReg.mParent->isSameHostRegister(mReg.mReg, reg);
      }

      void unlock() {
         mReg = Register();
      }

   private:
      RegLockout()
         : mReg() { }

      RegLockout(PPCEmuAssembler *parent, HostRegister *reg)
         : mReg(parent, reg, 0, false) { }

      Register mReg;
   };

   RegLockout lockRegister(const asmjit::X86GpReg& reg)
   {
      for (auto &hreg : mRegs) {
         if (isSameHostRegister(&hreg, reg)) {
            decaf_check(hreg.useCount == 0);
            if (hreg.content != 0xFFFFFFFF) {
               evictOne(&hreg);
            }
            hreg.useCount++;
            return RegLockout(this, &hreg);
         }
      }

      // For debugging
      decaf_abort("Attempted to lockout register which isn't used for JIT");

      // We couldn't find the register, so there is probably no need
      //  to preform lockout on it.  Why did you call me!
      return RegLockout();
   }

   HostRegister * allocReg(RegType regType) {
      uint32_t lruReg = 0xFFFFFFFF;
      uint32_t lruValue = 0xFFFFFFFF;

      // Pick a register from completely empty ones, track the
      //  least-recently used register at the same time.
      for (auto i = 0; i < mRegs.size(); ++i) {
         auto &reg = mRegs[i];
         if (reg.regType != regType) {
            continue;
         }
         if (!reg.useCount) {
            if (reg.content == 0xFFFFFFFF) {
               reg.useCount++;
               return &reg;
            }

            if (reg.lruValue < lruValue) {
               lruReg = i;
               lruValue = reg.lruValue;
            }
         }
      }

      // If we have an LRU register, lets evict it and use that one.
      if (lruReg != 0xFFFFFFFF) {
         auto &reg = mRegs[lruReg];
         evictOne(&reg);
         reg.useCount++;
         return &reg;
      }

      decaf_abort("Failed to locate a free host register to allocate");
   }

   HostRegister * findReg(const PpcRef& which)
   {
      // Search for this PpcRef inside the GP registers
      for (auto i = 0; i < mRegs.size(); ++i) {
         auto &reg = mRegs[i];
         if (reg.content == which.offset) {
            decaf_check(reg.size == which.size);
            return &reg;
         }
      }
      return nullptr;
   }

   void freeRegister(HostRegister *reg)
   {
      decaf_check(reg->useCount > 0);
      reg->useCount--;
   }

   GpRegister allocGpTmp()
   {
      return GpRegister(this, allocReg(RegType::Gp), 8, false);
   }

   XmmRegister allocXmmTmp()
   {
      return XmmRegister(this, allocReg(RegType::Xmm), 16, false);
   }

   GpRegister allocGpTmp(const GpRegister &source)
   {
      auto tmp = allocGpTmp();

      if (source.mSize == 1) {
         tmp = tmp.r8();
         mov(tmp, source);
      } else if (source.mSize == 2) {
         tmp = tmp.r16();
         mov(tmp, source);
      } else if (source.mSize == 4) {
         tmp = tmp.r32();
         mov(tmp, source);
      } else if (source.mSize == 8) {
         tmp = tmp.r64();
         mov(tmp, source);
      } else {
         decaf_abort(fmt::format("Unexpected register size {}", source.mSize));
      }

      return tmp;
   }

   XmmRegister allocXmmTmp(const XmmRegister &source)
   {
      auto tmp = allocXmmTmp();

      if (source.mSize == 16) {
         movapd(tmp, source);
      } else {
         decaf_abort(fmt::format("Unexpected register size {}", source.mSize));
      }

      return tmp;
   }

   GpRegister _getGpRegister(const PpcRef &which, bool shouldLoad, bool writeOnUse)
   {
      decaf_check(which.size == 4 || which.size == 8);

      auto reg = findReg(which);
      if (reg && reg->regType != RegType::Gp) {
         evictOne(reg);
         reg = nullptr;
      }
      if (reg) {
         decaf_check(reg->size == which.size);
         reg->useCount++;
      } else {
         reg = allocReg(RegType::Gp);
         reg->content = which.offset;
         reg->size = which.size;
      }

      if (shouldLoad && !reg->loaded) {
         if (reg->size == 4) {
            mov(mGpRegVals[reg->regId].r32(), asmjit::X86Mem(stateReg, which.offset, 4));
         } else if (reg->size == 8) {
            mov(mGpRegVals[reg->regId].r64(), asmjit::X86Mem(stateReg, which.offset, 8));
         } else {
            decaf_abort(fmt::format("Unexpected register size {}", reg->size));
         }
         reg->loaded = true;
      }

      return GpRegister(this, reg, which.size, writeOnUse);
   }

   XmmRegister _getXmmRegister(const PpcRef &which, bool shouldLoad, bool writeOnUse)
   {
      decaf_check(which.size == 16);

      auto reg = findReg(which);
      if (reg && reg->regType != RegType::Xmm) {
         evictOne(reg);
         reg = nullptr;
      }
      if (reg) {
         decaf_check(reg->size == which.size);
         reg->useCount++;
      } else {
         reg = allocReg(RegType::Xmm);
         reg->content = which.offset;
         reg->size = which.size;
      }

      if (shouldLoad && !reg->loaded) {
         if (reg->size == 16) {
            movapd(mXmmRegVals[reg->regId], asmjit::X86Mem(stateReg, which.offset, 16));
         } else {
            decaf_abort(fmt::format("Unexpected register size {}", reg->size));
         }
         reg->loaded = true;
      }

      return XmmRegister(this, reg, which.size, writeOnUse);
   }

   GpRegister loadGpRegisterWrite(const PpcRef &which) {
      return _getGpRegister(which, false, true);
   }

   XmmRegister loadXmmRegisterWrite(const PpcRef &which) {
      return _getXmmRegister(which, false, true);
   }

   GpRegister loadGpRegisterRead(const PpcRef &which) {
      return _getGpRegister(which, true, false);
   }

   XmmRegister loadXmmRegisterRead(const PpcRef &which) {
      return _getXmmRegister(which, true, false);
   }

   GpRegister loadGpRegisterReadWrite(const PpcRef &which) {
      return _getGpRegister(which, true, true);
   }

   XmmRegister loadXmmRegisterReadWrite(const PpcRef &which) {
      return _getXmmRegister(which, true, true);
   }

   // Some helpers which automatically pick the correct type
   GpRegister loadRegisterRead(const PpcGpRef &which) {
      return loadGpRegisterRead(which);
   }

   XmmRegister loadRegisterRead(const PpcXmmRef &which) {
      return loadXmmRegisterRead(which);
   }

   GpRegister loadRegisterWrite(const PpcGpRef &which) {
      return loadGpRegisterWrite(which);
   }

   XmmRegister loadRegisterWrite(const PpcXmmRef &which) {
      return loadXmmRegisterWrite(which);
   }

   GpRegister loadRegisterReadWrite(const PpcGpRef &which) {
      return loadGpRegisterReadWrite(which);
   }

   XmmRegister loadRegisterReadWrite(const PpcXmmRef &which) {
      return loadXmmRegisterReadWrite(which);
   }

   void saveOne(HostRegister *reg)
   {
      decaf_check(reg->useCount == 0);
      decaf_check(reg->content != 0xFFFFFFFF);

      if (reg->written) {
         decaf_check(reg->loaded);

         if (reg->regType == RegType::Gp) {
            decaf_check(reg->size == 4 || reg->size == 8);

            if (reg->size == 4) {
               mov(asmjit::X86Mem(stateReg, reg->content, 4), mGpRegVals[reg->regId].r32());
            } else if (reg->size == 8) {
               mov(asmjit::X86Mem(stateReg, reg->content, 8), mGpRegVals[reg->regId].r64());
            } else {
               decaf_abort(fmt::format("Unexpected register size {}", reg->size));
            }
         } else if (reg->regType == RegType::Xmm) {
            decaf_check(reg->size == 16);

            movapd(asmjit::X86Mem(stateReg, reg->content, 16), mXmmRegVals[reg->regId]);
         } else {
            decaf_abort(fmt::format("Unexpected register type {}", static_cast<int>(reg->regType)));
         }
      }
   }

   void saveAll()
   {
      for (auto i = 0; i < mRegs.size(); ++i) {
         if (mRegs[i].content != 0xFFFFFFFF) {
            saveOne(&mRegs[i]);
         }
      }
   }

   void evictOne(HostRegister *reg)
   {
      saveOne(reg);

      reg->content = 0xFFFFFFFF;
      reg->size = 0;
      reg->loaded = false;
      reg->written = false;
   }

   void evictAll()
   {
      for (auto i = 0; i < mRegs.size(); ++i) {
         if (mRegs[i].content != 0xFFFFFFFF) {
            evictOne(&mRegs[i]);
         }
      }
   }

};

template<typename T, typename Z>
T asmjit_cast(Z* base, size_t offset = 0)
{
   return reinterpret_cast<T>(((char*)base) + offset);
}

using JitCode = void *;
using JitCall = Core*(*)(Core*, JitCode);
using JitFinale = JitCall;

using JumpLabelMap = std::map<uint32_t, asmjit::Label>;

extern JitCall gCallFn;
extern JitFinale gFinaleFn;

struct JitBlock
{
   JitBlock(uint32_t _start) {
      start = _start;
      end = _start;
      entry = nullptr;
   }

   uint32_t start;
   uint32_t end;

   JitCode entry;
   std::vector<std::pair<uint32_t, JitCode>> targets;
};

} // namespace jit

} // namespace cpu
