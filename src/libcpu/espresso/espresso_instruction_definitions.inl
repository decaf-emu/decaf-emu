//INS(name, write, read, flags, opcodes, fullname)

// Intger Arithmetic
INS(add, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 266), "Add")
INS(addc, (rD, XERC), (rA, rB), (oe, rc), (opcd == 31, xo2 == 10), "Add with Carry")
INS(adde, (rD), (rA, rB, XERC), (oe, rc), (opcd == 31, xo2 == 138), "Add Extended")
INS(addi, (rD), (rA, simm), (), (opcd == 14), "Add Immediate")
INS(addic, (rD, XERC), (rA, simm), (), (opcd == 12), "Add Immediate with Carry")
INS(addicx, (rD, XERC), (rA, simm), (AOE, ARC), (opcd == 13), "Add Immediate with Carry and Record")
INS(addis, (rD), (rA, simm), (), (opcd == 15), "Add Immediate Shifted")
INS(addme, (rD), (rA, XERC), (oe, rc), (opcd == 31, xo2 == 234, !_16_20), "Add to Minus One Extended")
INS(addze, (rD), (rA, XERC), (oe, rc), (opcd == 31, xo2 == 202, !_16_20), "Add to Zero Extended")
INS(divw, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 491), "Divide Word")
INS(divwu, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 459), "Divide Word Unsigned")
INS(mulhw, (rD), (rA, rB), (rc), (opcd == 31, xo2 == 75), "Multiply High Word")
INS(mulhwu, (rD), (rA, rB), (rc), (opcd == 31, xo2 == 11), "Multiply High Word Unsigned")
INS(mulli, (rD), (rA, simm), (), (opcd == 7), "Multiply Low Immediate")
INS(mullw, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 235), "Multiply Low Word")
INS(neg, (rD), (rA), (oe, rc), (opcd == 31, xo2 == 104, !_16_20), "Negate")
INS(subf, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 40), "Subtract From")
INS(subfc, (rD), (rA, rB), (oe, rc), (opcd == 31, xo2 == 8), "Subtract From with Carry")
INS(subfe, (rD), (rA, rB, XERC), (oe, rc), (opcd == 31, xo2 == 136), "Subtract From Extended")
INS(subfic, (rD, XERC), (rA, simm), (), (opcd == 8), "Subtract From Immediate with Carry")
INS(subfme, (rD), (rA, XERC), (oe, rc), (opcd == 31, xo2 == 232, !_16_20), "Subtract From Minus One Extended")
INS(subfze, (rD), (rA, XERC), (oe, rc), (opcd == 31, xo2 == 200, !_16_20), "Subtract From Zero Extended")

// Integer Compare
INS(cmp, (crfD), (rA, rB, XERSO), (l), (opcd == 31, xo1 == 0, !_9, !_31), "Compare")
INS(cmpi, (crfD), (rA, simm, XERSO), (l), (opcd == 11, !_9), "Compare Immediate")
INS(cmpl, (crfD), (rA, rB, XERSO), (l), (opcd == 31, xo1 == 32, !_9, !_31), "Compare Logical")
INS(cmpli, (crfD), (rA, uimm, XERSO), (l), (opcd == 10, !_9), "Compare Logical Immediate")

// Integer Logical
INS(and_, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 28), "AND")
INS(andc, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 60), "AND with Complement")
INS(andi, (rA), (rS, uimm), (AOE, ARC), (opcd == 28), "AND Immediate")
INS(andis, (rA), (rS, uimm), (AOE, ARC), (opcd == 29), "AND Immediate Shifted")
INS(cntlzw, (rA), (rS), (rc), (opcd == 31, xo1 == 26, !_16_20), "Count Leading Zeroes Word")
INS(eqv, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 284), "Equivalent")
INS(extsb, (rA), (rS), (rc), (opcd == 31, xo1 == 954, !_16_20), "Extend Sign Byte")
INS(extsh, (rA), (rS), (rc), (opcd == 31, xo1 == 922, !_16_20), "Extend Sign Half Word")
INS(nand, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 476), "NAND")
INS(nor, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 124), "NOR")
INS(or_, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 444), "OR")
INS(orc, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 412), "OR with Complement")
INS(ori, (rA), (rS, uimm), (), (opcd == 24), "OR Immediate")
INS(oris, (rA), (rS, uimm), (), (opcd == 25), "OR Immediate Shifted")
INS(xor_, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 316), "XOR")
INS(xori, (rA), (rS, uimm), (), (opcd == 26), "XOR Immediate")
INS(xoris, (rA), (rS, uimm), (), (opcd == 27), "XOR Immediate Shifted")

// Integer Rotate
INS(rlwimi, (rA), (rA, rS, sh, mb, me), (rc), (opcd == 20), "Rotate Left Word Immediate then Mask Insert")
INS(rlwinm, (rA), (rS, sh, mb, me), (rc), (opcd == 21), "Rotate Left Word Immediate then AND with Mask")
INS(rlwnm, (rA), (rS, rB, mb, me), (rc), (opcd == 23), "Rotate Left Word then AND with Mask")

// Integer Shift
INS(slw, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 24), "Shift Left Word")
INS(sraw, (rA, XERC), (rS, rB), (rc), (opcd == 31, xo1 == 792), "Shift Right Arithmetic Word")
INS(srawi, (rA, XERC), (rS, sh), (rc), (opcd == 31, xo1 == 824), "Shift Right Arithmetic Word Immediate")
INS(srw, (rA), (rS, rB), (rc), (opcd == 31, xo1 == 536), "Shift Right Word")

// Floating-Point Arithmetic
INS(fadd, (frD, FCRISI, FCRSNAN), (frA, frB), (frc), (opcd == 63, xo4 == 21), "Floating Add")
INS(fadds, (frD, FCRISI, FCRSNAN), (frA, frB), (frc), (opcd == 59, xo4 == 21), "Floating Add Single")
INS(fdiv, (frD, FCRZDZ, FCRIDI, FCRSNAN), (frA, frB), (frc), (opcd == 63, xo4 == 18), "Floating Divide")
INS(fdivs, (frD), (frA, frB), (frc), (opcd == 59, xo4 == 18), "Floating Divide Single")
INS(fmul, (frD), (frA, frC), (frc), (opcd == 63, xo4 == 25), "Floating Multiply")
INS(fmuls, (frD), (frA, frC), (frc), (opcd == 59, xo4 == 25), "Floating Multiply Single")
INS(fres, (frD), (frB), (frc), (opcd == 59, xo4 == 24), "Floating Reciprocal Estimate Single")
INS(frsqrte, (frD), (frB), (frc), (opcd == 63, xo4 == 26), "Floating Reciprocal Square Root Estimate")
INS(fsub, (frD), (frA, frB), (frc), (opcd == 63, xo4 == 20), "Floating Sub")
INS(fsubs, (frD), (frA, frB), (frc), (opcd == 59, xo4 == 20), "Floating Sub Single")
INS(fsel, (frD), (frA, frB, frC), (frc), (opcd == 63, xo4 == 23), "Floating Select")

// Floating-Point Multiply-Add
INS(fmadd, (frD), (frA, frC, frB), (rc), (opcd == 63, xo4 == 29), "Floating Multiply-Add")
INS(fmadds, (frD), (frA, frC, frB), (rc), (opcd == 59, xo4 == 29), "Floating Multiply-Add Single")
INS(fmsub, (frD), (frA, frC, frB), (rc), (opcd == 63, xo4 == 28), "Floating Multiply-Sub")
INS(fmsubs, (frD), (frA, frC, frB), (rc), (opcd == 59, xo4 == 28), "Floating Multiply-Sub Single")
INS(fnmadd, (frD), (frA, frC, frB), (rc), (opcd == 63, xo4 == 31), "Floating Negative Multiply-Add")
INS(fnmadds, (frD), (frA, frC, frB), (rc), (opcd == 59, xo4 == 31), "Floating Negative Multiply-Add Single")
INS(fnmsub, (frD), (frA, frC, frB), (rc), (opcd == 63, xo4 == 30), "Floating Negative Multiply-Sub")
INS(fnmsubs, (frD), (frA, frC, frB), (rc), (opcd == 59, xo4 == 30), "Floating Negative Multiply-Sub Single")

// Floating-Point Rounding and Conversion
INS(fctiw, (frD), (frB), (rc), (opcd == 63, xo1 == 14), "Floating Convert to Integer Word")
INS(fctiwz, (frD), (frB), (rc), (opcd == 63, xo1 == 15), "Floating Convert to Integer Word with Round toward Zero")
INS(frsp, (frD), (frB), (rc), (opcd == 63, xo1 == 12), "Floating Round to Single")

// Floating-Point Compare
INS(fcmpo, (crfD), (frA, frB), (), (opcd == 63, xo1 == 32, !_9_10, !_31), "Floating Compare Ordered")
INS(fcmpu, (crfD), (frA, frB), (), (opcd == 63, xo1 == 0, !_9_10, !_31), "Floating Compare Unordered")

// Floating-Point Status and Control Register
INS(mcrfs, (crfD), (crfS), (), (opcd == 63, xo1 == 64, !_9_10, !_14_15, !_16_20, !_31), "")
INS(mffs, (frD), (), (rc), (opcd == 63, xo1 == 583, !_11_15, !_16_20), "")
INS(mtfsb0, (), (crfD), (rc), (opcd == 63, xo1 == 70, !_11_15, !_16_20), "")
INS(mtfsb1, (), (crfD), (rc), (opcd == 63, xo1 == 38, !_11_15, !_16_20), "")
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
INS(sync, (), (), (l), (opcd == 31, xo1 == 598, !_6_9, !_11_15, !_16_20, !_31), "Synchronise")

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

// Floating-Point Move
INS(fabs, (frD), (frB), (rc), (opcd == 63, xo1 == 264, !_11_15), "Floating Absolute Value")
INS(fmr, (frD), (frB), (rc), (opcd == 63, xo1 == 72, !_11_15), "Floating Move Register")
INS(fnabs, (frD), (frB), (rc), (opcd == 63, xo1 == 136, !_11_15), "Floating Negative Absolute Value")
INS(fneg, (frD), (frB), (rc), (opcd == 63, xo1 == 40, !_11_15), "Floating Negate")

// Branch
INS(b, (), (li), (aa, lk), (opcd == 18), "Branch")
INS(bc, (bo), (bi, bd), (aa, lk), (opcd == 16), "Branch Conditional")
INS(bcctr, (bo), (bi, CTR), (lk), (opcd == 19, xo1 == 528, !_16_20), "Branch Conditional to CTR")
INS(bclr, (bo), (bi, LR), (lk), (opcd == 19, xo1 == 16, !_16_20), "Branch Conditional to LR")

// Condition Register Logical
INS(crand, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 257, !_31), "Condition Register AND")
INS(crandc, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 129, !_31), "Condition Register AND with Complement")
INS(creqv, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 289, !_31), "Condition Register Equivalent")
INS(crnand, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 225, !_31), "Condition Register NAND")
INS(crnor, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 33, !_31), "Condition Register NOR")
INS(cror, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 449, !_31), "Condition Register OR")
INS(crorc, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 417, !_31), "Condition Register OR with Complement")
INS(crxor, (crbD), (crbA, crbB), (), (opcd == 19, xo1 == 193, !_31), "Condition Register XOR")
INS(mcrf, (crfD), (crfS), (), (opcd == 19, xo1 == 0, !_9_10, !_14_15, !_16_20, !_31), "Move Condition Register Field")

// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50, !_6_10, !_11_15, !_16_20, !_31), "")
INS(kc, (), (kcn), (), (opcd == 17, _31 == 1), "krncall")  // Must come before sc for proper table setup
INS(sc, (), (), (), (opcd == 17, !_6_10, !_11_15, !_16_29, _30 == 1, !_31), "Syscall")

// Trap
INS(tw, (), (to, rA, rB), (), (opcd == 31, xo1 == 4, !_31), "")
INS(twi, (), (to, rA, simm), (), (opcd == 3), "")

// Processor Control
INS(mcrxr, (crfD), (XERO), (), (opcd == 31, xo1 == 512, !_9_10, !_11_15, !_16_20, !_31), "Move to Condition Register from XERO")
// mfcr requires bit 11 to be 0 (if 1, it's the mfocrf instruction), but the
// Espresso ignores bit 11 and treats mfocrf as mfcr.
INS(mfcr, (rD), (), (), (opcd == 31, xo1 == 19, !_20, !_31), "Move from Condition Register")
INS(mfmsr, (rD), (), (), (opcd == 31, xo1 == 83, !_11_15, !_16_20, !_31), "Move from Machine State Register")
INS(mfspr, (rD), (spr), (), (opcd == 31, xo1 == 339, !_31), "Move from Special Purpose Register")
INS(mftb, (rD), (tbr), (), (opcd == 31, xo1 == 371, !_31), "Move from Time Base Register")
// mtcrf requires bit 11 to be 0 (if 1, it's the mtocrf instruction), but the
// Espresso ignores bit 11 and treats mtocrf as mtcrf.
INS(mtcrf, (crm), (rS), (), (opcd == 31, xo1 == 144, !_20, !_31), "Move to Condition Register Fields")
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
INS(ps_add, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 21), "Paired Single Add")
INS(ps_div, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 18), "Paired Single Divide")
INS(ps_mul, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 25), "Paired Single Multiply")
INS(ps_sub, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 20), "Paired Single Subtract")
INS(ps_abs, (frD), (frB), (rc), (opcd == 4, xo1 == 264, !_11_15), "Paired Single Absolute")
INS(ps_nabs, (frD), (frB), (rc), (opcd == 4, xo1 == 136, !_11_15), "Paired Single Negate Absolute")
INS(ps_neg, (frD), (frB), (rc), (opcd == 4, xo1 == 40, !_11_15), "Paired Single Negate")
INS(ps_sel, (frD), (frA, frC, frB), (rc), (opcd == 4, xo4 == 23), "Paired Single Select")
INS(ps_res, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 24), "Paired Single Reciprocal")
INS(ps_rsqrte, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 26), "Paired Single Reciprocal Square Root Estimate")
INS(ps_msub, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 28), "Paired Single Multiply and Subtract")
INS(ps_madd, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 29), "Paired Single Multiply and Add")
INS(ps_nmsub, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 30), "Paired Single Negate Multiply and Subtract")
INS(ps_nmadd, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 31), "Paired Single Negate Multiply and Add")
INS(ps_mr, (frD), (frB), (rc), (opcd == 4, xo1 == 72, !_11_15), "Paired Single Move Register")
INS(ps_sum0, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 10), "Paired Single Sum High")
INS(ps_sum1, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 11), "Paired Single Sum Low")
INS(ps_muls0, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 12), "Paired Single Multiply Scalar High")
INS(ps_muls1, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 13), "Paired Single Multiply Scalar Low")
INS(ps_madds0, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 14), "Paired Single Multiply and Add Scalar High")
INS(ps_madds1, (frD, FPSCR), (frA, frC, frB), (rc), (opcd == 4, xo4 == 15), "Paired Single Multiply and Add Scalar Low")
INS(ps_cmpu0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 0, !_9_10, !_31), "Paired Single Compare Unordered High")
INS(ps_cmpo0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 32, !_9_10, !_31), "Paired Single Compare Ordered High")
INS(ps_cmpu1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 64, !_9_10, !_31), "Paired Single Compare Unordered Low")
INS(ps_cmpo1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 96, !_9_10, !_31), "Paired Single Compare Ordered Low")
INS(ps_merge00, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 528), "Paired Single Merge High")
INS(ps_merge01, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 560), "Paired Single Merge Direct")
INS(ps_merge10, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 592), "Paired Single Merge Swapped")
INS(ps_merge11, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 624), "Paired Single Merge Low")
