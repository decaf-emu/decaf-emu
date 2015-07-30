branching:
    # in r1 = 1
    # in r2 = 2
    # in r3 = 0

    b endProg
    add r3, r1, r2
    blr
    
    # our r3 = 50
   
endProg:
    # in r3 = 3

    blr
    # out r3 = 3
    
branching2:
    # in r1 = 1
    # in r2 = 2
    # in r3 = 0

    mflr r3
    bl subRoutine
    mtlr r3
    add r3, r1, r2
        
    blr
    
    # out r3 = 3
   
subRoutine:
    # in r3 = 5

    blr
    # out r3 = 5
    