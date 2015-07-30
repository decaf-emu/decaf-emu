andImmediate:
    # in r1 = 0x13AB42FF
    # in xer.so = 1
    
    andi. r3, r1, 0
    blr
    
    # out r3 = 0
    # out crf0 = Zero | SummaryOverflow

andImmediate2:
    # in r1 = 0x13AB42FF
    # in xer.so = 0
    
    andi. r3, r1, 0xFFFF
    blr
    
    # out r3 = 0x000042FF
    # out crf0 = Positive

andImmediate3:
    # in r1 = 0x13AB42FF
    # in xer.so = 0
    
    andi. r3, r1, 0xBBF4
    blr
    
    # out r3 = 0x000002F4
    # out crf0 = Positive

    
andImmediateShifted:
    # in r1 = 0x13AB42FF
    # in xer.so = 0
    
    andi. r3, r1, 0
    blr
    
    # out r3 = 0
    # out crf0 = Zero

andImmediateShifted2:
    # in r1 = 0x13AB42FF
    # in xer.so = 0
    
    andis. r3, r1, 0xFFFF
    blr
    
    # out r3 = 0x13AB0000
    # out crf0 = Positive

andImmediateShifted3:
    # in r1 = 0x13AB42FF
    # in xer.so = 0
    
    andis. r3, r1, 0x6A40
    blr
    
    # out r3 = 0x02000000
    # out crf0 = Positive
    