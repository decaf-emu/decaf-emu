// Integer Arithmetic
INS(add, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 266), "Add")
INS(addc, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 10), "Add with Carry")
INS(adde, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 138), "Add Extended")
INS(addi, (rd), (ra, simm), (), (opcd == 14), "Add Immediate")
INS(addic, (rd), (ra, simm), (), (opcd == 12), "Add Immediate with Carry")
INS(addicx, (rd), (ra, simm), (), (opcd == 13), "Add Immediate with Carry and Record")
INS(addis, (rd), (ra, simm), (), (opcd == 15), "Add Immediate Signed")
INS(addme, (rd), (ra), (oe, rc), (opcd == 31, xo2 == 234), "Add to Minus One Extended")
INS(addze, (rd), (ra), (oe, rc), (opcd == 31, xo2 == 202), "Add to Zero Extended")
INS(divw, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 491), "Divide Word")
INS(divwu, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 459), "Divide Word Unsigned")
INS(mulhw, (rd), (ra, rb), (rc), (opcd == 31, xo2 == 75), "Multiply High Word")
INS(mulhwu, (rd), (ra, rb), (rc), (opcd == 31, xo2 == 11), "Multiply High Word Unsigned")
INS(mulli, (rd), (ra, simm), (), (opcd == 7), "Multiply Low Immediate")
INS(mullw, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 235), "Multiply Low Word")
INS(neg, (rd), (ra), (oe, rc), (opcd == 31, xo2 == 104), "Negate")
INS(subf, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 40), "Subtract From")
INS(subfc, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 8), "Subtract From with Carry")
INS(subfe, (rd), (ra, rb), (oe, rc), (opcd == 31, xo2 == 136), "Subtract From Extended")
INS(subfic, (rd), (ra, simm), (), (opcd == 8), "Subtract From Immediate with Carry")
INS(subfme, (rd), (ra), (oe, rc), (opcd == 31, xo2 == 232), "Subtract From Minus One Extended")
INS(subfze, (rd), (ra), (oe, rc), (opcd == 31, xo2 == 200), "Subtract From Zero Extended")

// Integer Compare
INS(cmp, (crfd), (ra, rb), (l), (opcd == 31, xo1 == 0), "Compare")
INS(cmpi, (crfd), (ra, simm), (l), (opcd == 11), "Compare Immediate")
INS(cmpl, (crfd), (ra, rb), (l), (opcd == 31, xo1 == 32), "Compare Logical")
INS(cmpli, (crfd), (ra, uimm), (l), (opcd == 10), "Compare Logical Immediate")

// Integer Logical
INS(and, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 28), "AND")
INS(andc, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 60), "AND with Complement")
INS(andi, (ra), (rs, uimm), (), (opcd == 28), "AND Immediate")
INS(andis, (ra), (rs, uimm), (), (opcd == 29), "AND Immediate Shifted")
INS(cntlzw, (ra), (rs), (rc), (opcd == 31, xo1 == 26), "Count Leading Zeroes Word")
INS(eqv, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 284), "Equivalent")
INS(extsb, (ra), (rs), (rc), (opcd == 31, xo1 == 954), "Extend Sign Byte")
INS(extsh, (ra), (rs), (rc), (opcd == 31, xo1 == 922), "Extend Sign Half Word")
INS(nand, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 476), "NAND")
INS(nor, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 124), "NOR")
INS(or, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 444), "OR")
INS(orc, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 412), "OR with Complement")
INS(ori, (ra), (rs, uimm), (), (opcd == 24), "OR Immediate")
INS(oris, (ra), (rs, uimm), (), (opcd == 25), "OR Immediate Shifted")
INS(xor, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 316), "XOR")
INS(xori, (ra), (rs, uimm), (), (opcd == 26), "XOR Immediate")
INS(xoris, (ra), (rs, uimm), (), (opcd == 27), "XOR Immediate Shifted")

// Integer Rotate
INS(rlwimi, (ra), (rs, sh, mb, me), (rc), (opcd == 20), "Rotate Left Word Immediate then Mask Insert")
INS(rlwinm, (ra), (rs, sh, mb, me), (rc), (opcd == 21), "Rotate Left Word Immediate then AND with Mask")
INS(rlwnm, (ra), (rs, sh, mb, me), (rc), (opcd == 23), "Rotate Left Word then AND with Mask")

// Integer Shift
INS(slw, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 24), "Shift Left Word")
INS(sraw, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 792), "Shift Right Arithmetic Word")
INS(srawi, (ra), (rs, sh), (rc), (opcd == 31, xo1 == 824), "Shift Right Arithmetic Word Immediate")
INS(srw, (ra), (rs, rb), (rc), (opcd == 31, xo1 == 536), "Shift Right Word")

// Floating-Point Arithmetic
INS(fadd, (frd), (fra, frb), (rc), (opcd == 63, xo4 == 21), "Floating Add")
INS(fadds, (frd), (fra, frb), (rc), (opcd == 59, xo4 == 21), "Floating Add Single")
INS(fdiv, (frd), (fra, frb), (rc), (opcd == 63, xo4 == 18), "Floating Divide")
INS(fdivs, (frd), (fra, frb), (rc), (opcd == 59, xo4 == 18), "Floating Divide Single")
INS(fmul, (frd), (fra, frc), (rc), (opcd == 63, xo4 == 25), "Floating Multiply")
INS(fmuls, (frd), (fra, frc), (rc), (opcd == 59, xo4 == 25), "Floating Multiply Single")
INS(fres, (frd), (frb), (rc), (opcd == 59, xo4 == 24), "Floating Reciprocal Estimate Single")
INS(frsqrte, (frd), (frb), (rc), (opcd == 63, xo4 == 26), "Floating Reciprocal Square Root Estimate")
INS(fsub, (frd), (fra, frb), (rc), (opcd == 63, xo4 == 20), "Floating Sub")
INS(fsubs, (frd), (fra, frb), (rc), (opcd == 59, xo4 == 20), "Floating Sub Single")
INS(fsel, (frd), (fra, frb, frc), (rc), (opcd == 63, xo4 == 23), "Floating Select")

// Floating-Point Multiply-Add
INS(fmadd, (frd), (fra, frb, frc), (rc), (opcd == 63, xo4 == 29), "Floating Multiply-Add")
INS(fmadds, (frd), (fra, frb, frc), (rc), (opcd == 59, xo4 == 29), "Floating Multiply-Add Single")
INS(fmsub, (frd), (fra, frb, frc), (rc), (opcd == 63, xo4 == 28), "Floating Multiply-Sub")
INS(fmsubs, (frd), (fra, frb, frc), (rc), (opcd == 59, xo4 == 28), "Floating Multiply-Sub Single")
INS(fnmadd, (frd), (fra, frb, frc), (rc), (opcd == 63, xo4 == 31), "Floating Negative Multiply-Add")
INS(fnmadds, (frd), (fra, frb, frc), (rc), (opcd == 59, xo4 == 31), "Floating Negative Multiply-Add Single")
INS(fnmsub, (frd), (fra, frb, frc), (rc), (opcd == 63, xo4 == 30), "Floating Negative Multiply-Sub")
INS(fnmsubs, (frd), (fra, frb, frc), (rc), (opcd == 59, xo4 == 30), "Floating Negative Multiply-Sub Single")

// Floating-Point Rounding and Conversion
INS(fctiw, (frd), (frb), (rc), (opcd == 63, xo1 == 14), "Floating Convert to Integer Word")
INS(fctiwz, (frd), (frb), (rc), (opcd == 63, xo1 == 15), "Floating Convert to Integer Word with Round toward Zero")
INS(frsp, (frd), (frb), (rc), (opcd == 63, xo1 == 12), "Floating Round to Single")

// Floating-Point Compare
INS(fcmpo, (crfd), (fra, frb), (), (opcd == 63, xo1 == 32), "Floating Compare Ordered")
INS(fcmpu, (crfd), (fra, frb), (), (opcd == 63, xo1 == 0), "Floating Compare Unordered")

// Floating-Point Status and Control Register
INS(mcrfs, (crfd), (crfs), (), (opcd == 63, xo1 == 64), "")
INS(mffs, (frd), (), (rc), (opcd == 63, xo1 == 583), "")
INS(mffsb0, (crbd), (), (rc), (opcd == 63, xo1 == 70), "")
INS(mffsb1, (crbd), (), (rc), (opcd == 63, xo1 == 38), "")
INS(mtfsf, (crbd), (fm, frb), (rc), (opcd == 63, xo1 == 711), "")
INS(mtfsfi, (crfd), (), (rc, imm), (opcd == 63, xo1 == 134), "")

// Integer Load
INS(lbz, (rd), (ra, d), (), (opcd == 34), "Load Byte and Zero")
INS(lbzu, (rd), (ra, d), (), (opcd == 35), "Load Byte and Zero with Update")
INS(lbzx, (rd), (ra, rb), (), (opcd == 31, xo1 == 87), "Load Byte and Zero Indexed")
INS(lbzux, (rd), (ra, rb), (), (opcd == 31, xo1 == 119), "Load Byte and Zero with Update Indexed")
INS(lha, (rd), (ra, d), (), (opcd == 42), "Load Half Word Algebraic")
INS(lhau, (rd), (ra, d), (), (opcd == 43), "Load Half Word Algebraic with Update")
INS(lhax, (rd), (ra, rb), (), (opcd == 31, xo1 == 343), "Load Half Word Algebraic Indexed")
INS(lhaux, (rd), (ra, rb), (), (opcd == 31, xo1 == 375), "Load Half Word Algebraic with Update Indexed")
INS(lhz, (rd), (ra, d), (), (opcd == 40), "Load Half Word and Zero")
INS(lhzu, (rd), (ra, d), (), (opcd == 41), "Load Half Word and Zero with Update")
INS(lhzx, (rd), (ra, rb), (), (opcd == 31, xo1 == 279), "Load Half Word and Zero Indexed")
INS(lhzux, (rd), (ra, rb), (), (opcd == 31, xo1 == 311), "Load Half Word and Zero with Update Indexed")
INS(lwz, (rd), (ra, d), (), (opcd == 32), "Load Word and Zero")
INS(lwzu, (rd), (ra, d), (), (opcd == 33), "Load Word and Zero with Update")
INS(lwzx, (rd), (ra, rb), (), (opcd == 31, xo1 == 23), "Load Word and Zero Indexed")
INS(lwzux, (rd), (ra, rb), (), (opcd == 31, xo1 == 55), "Load Word and Zero with Update Indexed")

// Integer Store
INS(stb, (), (rs, ra, d), (), (opcd == 38), "Store Byte")
INS(stbu, (), (rs, ra, d), (), (opcd == 39), "Store Byte with Update")
INS(stbx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 215), "Store Byte Indexed")
INS(stbux, (), (rs, ra, rb), (), (opcd == 31, xo1 == 247), "Store Byte with Update Indexed")
INS(sth, (), (rs, ra, d), (), (opcd == 44), "Store Half Word")
INS(sthu, (), (rs, ra, d), (), (opcd == 45), "Store Half Word with Update")
INS(sthx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 407), "Store Half Word Indexed")
INS(sthux, (), (rs, ra, rb), (), (opcd == 31, xo1 == 439), "Store Half Word with Update Indexed")
INS(stw, (), (rs, ra, d), (), (opcd == 36), "Store Word")
INS(stwu, (), (rs, ra, d), (), (opcd == 37), "Store Word with Update")
INS(stwx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 151), "Store Word Indexed")
INS(stwux, (), (rs, ra, rb), (), (opcd == 31, xo1 == 183), "Store Word with Update Indexed")

// Integer Load and Store with Byte Reverse
INS(lhbrx, (rd), (ra, rb), (), (opcd == 31, xo1 == 790), "Load Half Word Byte-Reverse Indexed")
INS(lwbrx, (rd), (ra, rb), (), (opcd == 31, xo1 == 534), "Load Word Byte-Reverse Indexed")
INS(sthbrx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 918), "Store Half Word Byte-Reverse Indexed")
INS(stwbrx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 662), "Store Word Byte-Reverse Indexed")

// Integer Load and Store Multiple
INS(lmw, (rd), (ra, d), (), (opcd == 46), "Load Multiple Words")
INS(stmw, (), (rs, ra, d), (), (opcd == 47), "Store Multiple Words")

// Integer Load and Store String
INS(lswi, (rd), (ra, nb), (), (opcd == 31, xo1 == 597), "Load String Word Immediate")
INS(lswx, (rd), (ra, rb), (), (opcd == 31, xo1 == 533), "Load String Word Indexed")
INS(stswi, (), (rs, ra, nb), (), (opcd == 31, xo1 == 725), "Store String Word Immediate")
INS(stswx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 661), "Store String Word Indexed")

// Memory Synchronisation
INS(eieio, (), (), (), (opcd == 31, xo1 == 854), "Enforce In-Order Execution of I/O")
INS(isync, (), (), (), (opcd == 19, xo1 == 150), "Instruction Synchronise")
INS(lwarx, (rd), (ra, rb), (), (opcd == 31, xo1 == 20), "Load Word and Reserve Indexed")
INS(stwcx, (), (rs, ra, rb), (), (opcd == 31, xo1 == 150), "Store Word Conditional Indexed")
INS(sync, (), (), (), (opcd == 31, xo1 == 598), "Synchronise")

// Floating-Point Load
INS(lfd, (frd), (ra, d), (), (opcd == 50), "Load Floating-Point Double")
INS(lfdu, (frd), (ra, d), (), (opcd == 51), "Load Floating-Point Double with Update")
INS(lfdx, (frd), (ra, rb), (), (opcd == 31, xo1 == 599), "Load Floating-Point Double Indexed")
INS(lfdux, (frd), (ra, rb), (), (opcd == 31, xo1 == 631), "Load Floating-Point Double with Update Indexed")
INS(lfs, (frd), (fra, d), (), (opcd == 48), "Load Floating-Point Single")
INS(lfsu, (frd), (fra, d), (), (opcd == 49), "Load Floating-Point Single with Update")
INS(lfsx, (frd), (ra, rb), (), (opcd == 31, xo1 == 535), "Load Floating-Point Single Indexed")
INS(lfsux, (frd), (ra, rb), (), (opcd == 31, xo1 == 567), "Load Floating-Point Single with Update Indexed")

// Floating-Point Store
INS(stfd, (), (frs, ra, d), (), (opcd == 54), "Store Floating-Point Double")
INS(stfdu, (), (frs, ra, d), (), (opcd == 55), "Store Floating-Point Double with Update")
INS(stfdx, (), (frs, ra, rb), (), (opcd == 31, xo1 == 727), "Store Floating-Point Double Indexed")
INS(stfdux, (), (frs, ra, rb), (), (opcd == 31, xo1 == 759), "Store Floating-Point Double with Update Indexed")
INS(stfiwx, (), (frs, ra, rb), (), (opcd == 31, xo1 == 983), "Store Floating-Point as Integer Word Indexed")
INS(stfs, (), (frs, ra, d), (), (opcd == 52), "Store Floating-Point Single")
INS(stfsu, (), (frs, ra, d), (), (opcd == 53), "Store Floating-Point Single with Update")
INS(stfsx, (), (frs, ra, rb), (), (opcd == 31, xo1 == 663), "Store Floating-Point Single Indexed")
INS(stfsux, (), (frs, ra, rb), (), (opcd == 31, xo1 == 695), "Store Floating-Point Single with Update Indexed")

// Floating-Point Move
INS(fabs, (frd), (frb), (rc), (opcd == 63, xo1 == 264), "Floating Absolute Value")
INS(fmr, (frd), (frb), (rc), (opcd == 63, xo1 == 72), "Floating Move Register")
INS(fnabs, (frd), (frb), (rc), (opcd == 63, xo1 == 136), "Floating Negative Absolute Value")
INS(fneg, (frd), (frb), (rc), (opcd == 63, xo1 == 40), "Floating Negate")

// Branch
INS(b, (), (li), (aa, lk), (opcd == 18), "Branch")
INS(bc, (), (bo, bi, bd), (aa, lk), (opcd == 16), "Branch Conditional")
INS(bcctr, (), (bo, bi), (lk), (opcd == 19, xo1 == 528), "Branch Conditional to CTR")
INS(bclr, (), (bo, bi), (lk), (opcd == 19, xo1 == 16), "Branch Conditional to LR")

// Condition Register Logical
INS(crand, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 257), "Condition Register AND")
INS(crandc, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 129), "Condition Register AND with Complement")
INS(creqv, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 289), "Condition Register Equivalent")
INS(crnand, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 225), "Condition Register NAND")
INS(crnor, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 33), "Condition Register NOR")
INS(cror, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 449), "Condition Register OR")
INS(crorc, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 417), "Condition Register OR with Complement")
INS(crxor, (crbd), (crba, crbb), (), (opcd == 19, xo1 == 193), "Condition Register XOR")
INS(mcrf, (crfd), (crfs), (), (opcd == 19, xo1 == 0), "Move Condition Register Field")

// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50), "")
INS(sc, (), (), (), (opcd == 17), "Syscall")
INS(kc, (), (li), (aa), (opcd == 1), "krncall")

// Trap
INS(tw, (), (to, ra, rb), (), (opcd == 31, xo1 == 4), "")
INS(twi, (), (to, ra, simm), (), (opcd == 3), "")

// Processor Control
INS(mcrxr, (), (crfs), (), (opcd == 31, xo1 == 512), "Move to Condition Register from XER")
INS(mfcr, (rd), (), (), (opcd == 31, xo1 == 19), "Move from Condition Register")
INS(mfmsr, (rd), (), (), (opcd == 31, xo1 == 83), "Move from Machine State Register")
INS(mfspr, (rd), (spr), (), (opcd == 31, xo1 == 339), "Move from Special Purpose Register")
INS(mftb, (rd), (tbr), (), (opcd == 31, xo1 == 371), "Move from Time Base Register")
INS(mtcrf, (rd), (crm), (), (opcd == 31, xo1 == 144), "Move to Condition Register Fields")
INS(mtmsr, (rd), (), (), (opcd == 31, xo1 == 146), "Move to Machine State Register")
INS(mtspr, (rd), (spr), (), (opcd == 31, xo1 == 467), "Move to Special Purpose Register")

// Cache Management
INS(dcbf, (), (ra, rb), (), (opcd == 31, xo1 == 86), "")
INS(dcbi, (), (ra, rb), (), (opcd == 31, xo1 == 470), "")
INS(dcbst, (), (ra, rb), (), (opcd == 31, xo1 == 54), "")
INS(dcbt, (), (ra, rb), (), (opcd == 31, xo1 == 278), "")
INS(dcbtst, (), (ra, rb), (), (opcd == 31, xo1 == 246), "")
INS(dcbz, (), (ra, rb), (), (opcd == 31, xo1 == 1014), "")
INS(icbi, (), (ra, rb), (), (opcd == 31, xo1 == 982), "")
INS(dcbz_l, (), (ra, rb), (), (opcd == 4, xo1 == 1014), "")

// Segment Register Manipulation
INS(mfsr, (rd), (sr), (), (opcd == 31, xo1 == 595), "Move from Segment Register")
INS(mfsrin, (rd), (rb), (), (opcd == 31, xo1 == 659), "Move from Segment Register Indirect")
INS(mtsr, (), (rd, sr), (), (opcd == 31, xo1 == 210), "Move to Segment Register")
INS(mtsrin, (), (rd, rb), (), (opcd == 31, xo1 == 242), "Move to Segment Register Indirect")

// Lookaside Buffer Management
INS(tlbie, (), (rb), (), (opcd == 31, xo1 == 306), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566), "")

// External Control
INS(eciwx, (rd), (ra, rb), (), (opcd == 31, xo1 == 310), "")
INS(ecowx, (rd), (ra, rb), (), (opcd == 31, xo1 == 438), "")

// Paired-Single Load and Store
INS(psq_l, (frd), (ra, d), (w, i), (opcd == 56), "Paired Single Load")
INS(psq_lu, (frd), (ra, d), (w, i), (opcd == 57), "Paired Single Load with Update")
INS(psq_lx, (frd), (ra, rb), (qw, qi), (opcd == 4, xo3 == 6), "Paired Single Load Indexed")
INS(psq_lux, (frd), (ra, rb), (qw, qi), (opcd == 4, xo3 == 38), "Paired Single Load with Update Indexed")
INS(psq_st, (frd), (ra, d), (w, i), (opcd == 60), "Paired Single Store")
INS(psq_stu, (frd), (ra, d), (w, i), (opcd == 61), "Paired Single Store with Update")
INS(psq_stx, (frs), (ra, rb), (qw, qi), (opcd == 4, xo3 == 7), "Paired Single Store Indexed")
INS(psq_stux, (frs), (ra, rb), (qw, qi), (opcd == 4, xo3 == 39), "Paired Single Store with Update Indexed")

// Paired-Single Floating Point Arithmetic
INS(ps_add, (frd), (fra, frb), (rc), (opcd == 4, xo4 == 21), "Paired Single Add")
INS(ps_div, (frd), (fra, frb), (rc), (opcd == 4, xo4 == 18), "Paired Single Divide")
INS(ps_mul, (frd), (fra, frc), (rc), (opcd == 4, xo4 == 25), "Paired Single Multiply")
INS(ps_sub, (frd), (fra, frb), (rc), (opcd == 4, xo4 == 20), "Paired Single Subtract")
INS(ps_abs, (frd), (frb), (rc), (opcd == 4, xo1 == 264), "Paired Single Absolute")
INS(ps_nabs, (frd), (frb), (rc), (opcd == 4, xo1 == 136), "Paired Single Negate Absolute")
INS(ps_neg, (frd), (frb), (rc), (opcd == 4, xo1 == 40), "Paired Single Negate")
INS(ps_sel, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 23), "Paired Single Select")
INS(ps_res, (frd), (frb), (rc), (opcd == 4, xo4 == 24), "Paired Single Reciprocal")
INS(ps_rsqrte, (frd), (frb), (rc), (opcd == 4, xo4 == 26), "Paired Single Reciprocal Square Root Estimate")
INS(ps_msub, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 28), "Paired Single Multiply and Subtract")
INS(ps_madd, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 29), "Paired Single Multiply and Add")
INS(ps_nmsub, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 30), "Paired Single Negate Multiply and Subtract")
INS(ps_nmadd, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 31), "Paired Single Negate Multiply and Add")
INS(ps_mr, (frd), (frb), (rc), (opcd == 4, xo1 == 72), "Paired Single Move Register")
INS(ps_sum0, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 10), "Paired Single Sum High")
INS(ps_sum1, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 11), "Paired Single Sum Low")
INS(ps_muls0, (frd), (fra, frc), (rc), (opcd == 4, xo4 == 12), "Paired Single Multiply Scalar High")
INS(ps_muls1, (frd), (fra, frc), (rc), (opcd == 4, xo4 == 13), "Paired Single Multiply Scalar Low")
INS(ps_madds0, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 14), "Paired Single Multiply and Add Scalar High")
INS(ps_madds1, (frd), (fra, frb, frc), (rc), (opcd == 4, xo4 == 15), "Paired Single Multiply and Add Scalar Low")
INS(ps_cmpu0, (crfd), (fra, frb), (), (opcd == 4, xo1 == 0), "Paired Single Compare Unordered High")
INS(ps_cmpo0, (crfd), (fra, frb), (), (opcd == 4, xo1 == 32), "Paired Single Compare Ordered High")
INS(ps_cmpu1, (crfd), (fra, frb), (), (opcd == 4, xo1 == 64), "Paired Single Compare Unordered Low")
INS(ps_cmpo1, (crfd), (fra, frb), (), (opcd == 4, xo1 == 96), "Paired Single Compare Ordered Low")
INS(ps_merge00, (frd), (fra, frb), (rc), (opcd == 4, xo1 == 528), "Paired Single Merge High")
INS(ps_merge01, (frd), (fra, frb), (rc), (opcd == 4, xo1 == 560), "Paired Single Merge Direct")
INS(ps_merge10, (frd), (fra, frb), (rc), (opcd == 4, xo1 == 592), "Paired Single Merge Swapped")
INS(ps_merge11, (frd), (fra, frb), (rc), (opcd == 4, xo1 == 624), "Paired Single Merge Low")
