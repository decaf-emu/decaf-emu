INSA(mr, or_, (rS == rB))

INSA(mflr, mfspr, (spr == 8))
INSA(mfctr, mfspr, (spr == 9))

INSA(mtlr, mtspr, (spr == 8))
INSA(mtctr, mtspr, (spr == 9))

INSA(blt, bc, (bo == 12, bi == 0))
INSA(ble, bc, (bo == 4, bi == 1))
INSA(beq, bc, (bo == 12, bi == 2))
INSA(bge, bc, (bo == 4, bi == 0))
INSA(bgt, bc, (bo == 12, bi == 3))
INSA(bne, bc, (bo == 4, bi == 2))
INSA(bso, bc, (bo == 12, bi == 3))
INSA(bns, bc, (bo == 4, bi == 3))
