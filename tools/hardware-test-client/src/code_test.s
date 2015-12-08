# rD, rA, rB, oe, rc, simm, XER
#
# rA, rB, rD -> affects gpr
# rc -> affects cr0 (cr)
# oe -> affects overflow flag (xer)
#
# r3...r6
# f1...f4
#
# struct TestState
# {
#    uint32_t xer;   // 0x00
#    uint32_t cr;    // 0x04
#    uint32_t ctr    // 0x08
#    uint32_t _      // 0x0c
#    uint32_t r3;    // 0x10
#    uint32_t r4;    // 0x14
#    uint32_t r5;    // 0x18
#    uint32_t r6;    // 0x1C
#    uint64_t fpscr  // 0x20
#    double f1       // 0x28
#    double f2       // 0x30
#    double f3       // 0x38
#    double f4       // 0x40
# } // 0x48
#
# struct StackSavedState
# {
#    uint32_t xer;   // 0x00
#    uint32_t cr;    // 0x04
#    double/u64 fpscr; // 0x08
#    uint32_t lr;    // 0x10
# } // 0x14

.global funcTest
funcTest:
   li r3, 1337
   blr

# void executeCodeTest(TestState *state, void *func)
.global executeCodeTest
executeCodeTest:
   # Create space on stack for StackSavedState
   stwu r1, -0x20(r1)

   # r10 = state
   mr r10, r3

   # Save System registers
   mfxer r3
   stw r3, 0x00(r1)

   mfcr r3
   stw r3, 0x04(r1)

   mffs f3
   stfd f3, 0x08(r1)

   mflr r3
   stw r3, 0x10(r1)

   # Put func to call into lr
   mtlr r4

   # Load Test State
   lwz r3, 0x00(r10)
   mtxer r3

   lwz r3, 0x04(r10)
   mtcr r3

   lwz r3, 0x08(r10)
   mtctr r3

   lwz r3, 0x10(r10)
   lwz r4, 0x14(r10)
   lwz r5, 0x18(r10)
   lwz r6, 0x1C(r10)
   li r0, 0

   lfd f3, 0x20(r10)
   mtfsf 0xFF, r3

   lfd f1, 0x28(r10)
   lfd f2, 0x30(r10)
   lfd f3, 0x38(r10)
   lfd f4, 0x40(r10)

   # Execute test
   blrl

   # Save Test State
   stw r3, 0x10(r10)
   stw r4, 0x14(r10)
   stw r5, 0x18(r10)
   stw r6, 0x1C(r10)

   mfxer r3
   stw r3, 0x00(r10)

   mfcr r3
   stw r3, 0x04(r10)

   mfctr r3
   stw r3, 0x08(r10)

   mffs f5
   stfd f5, 0x20(r10)

   stfd f1, 0x28(r10)
   stfd f2, 0x30(r10)
   stfd f3, 0x38(r10)
   stfd f4, 0x40(r10)

   # Restore System registers
   lwz r3, 0x00(r1)
   mtxer r3

   lwz r3, 0x04(r1)
   mtcr r3

   lfd f3, 0x08(r1)
   mtfsf 0xFF, f3

   lwz r3, 0x10(r1)
   mtlr r3

   # Restore stack pointer and return
   addi r1, r1, 0x20
   blr
