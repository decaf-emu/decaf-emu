andTest:
    # in r1 = 12312312
    # in r2 = 0
    and r3, r1, r2
    # out r3 = 0

andTest2:
    # in r1 = 12312312
    # in r2 = 4294967295
    and r3, r1, r2
    # out r3 = 12312312

andTest3:
    # in r1 = 12312312
    # in r2 = 49875685
    and r3, r1, r2
    # out r3 = 12126944

andComplementTest:
    # in r1 = 12312312
    # in r2 = 0
    andc r3, r1, r2
    # out r3 = 12312312

andComplementTest2:
    # in r1 = 12312312
    # in r2 = 4294967295
    andc r3, r1, r2
    # out r3 = 0

andComplementTest3:
    # in r1 = 12312312
    # in r2 = 49875685
    andc r3, r1, r2
    # out r3 = 185368
    