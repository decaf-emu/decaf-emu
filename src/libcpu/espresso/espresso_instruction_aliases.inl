INSA(li, addi, (rA == 0))
INSA(lis, addis, (rA == 0))

INSA(mflr, mfspr, (spr == 8))
INSA(mfctr, mfspr, (spr == 9))
INSA(mfxer, mfspr, (spr == 1))

INSA(mtlr, mtspr, (spr == 8))
INSA(mtctr, mtspr, (spr == 9))
INSA(mtxer, mtspr, (spr == 1))

INSA(mtcr, mtcrf, (crm == 0xFF))

INSA(mr, or_, (rS == rB))
INSA(nop, ori, (rA == 0, rS == 0, uimm == 0))
INSA(not, nor, (rS == rB))
