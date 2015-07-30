conditionalBranching:
    # in r1 = 0
    # in r4 = 10
    # in xer.so = 0
    
    li r3, 1
    mflr r1
    bl factorial
    mtlr r1
    
    li r1, 0
    
    blr
    
    # out r3 = 3628800
    # out r4 = 0
    # out crf0 = Zero

factorial:
    # in r4 = 1
    # in xer.so = 0
    
    mullw r3, r3, r4
    subi r4, r4, 1
    cmpwi r4, 0
    bne factorial
    
    blr
    
    # out crf0 = Zero
    # out r4 = 0
    
    
    