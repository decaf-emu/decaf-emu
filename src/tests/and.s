andTest:
    # in r1 = 0x13AB42FF
    # in r2 = 0
    and r3, r1, r2
    blr
    # out r3 = 0

andTest2:
    # in r1 = 0x13AB42FF
    # in r2 = 0xFFFFFFFF
    and r3, r1, r2
    blr
    # out r3 = 0x13AB42FF

andTest3:
    # in r1 = 0x13AB42FF
    # in r2 = 0x6A40BBF4
    and r3, r1, r2
    blr
    # out r3 = 0x020002F4

andComplementTest:
    # in r1 = 0x13AB42FF
    # in r2 = 0
    andc r3, r1, r2
    blr
    # out r3 = 0x13AB42FF

andComplementTest2:
    # in r1 = 0x13AB42FF
    # in r2 = 0xFFFFFFFF
    andc r3, r1, r2
    blr
    # out r3 = 0

andComplementTest3:
    # in r1 = 0x13AB42FF
    # in r2 = 0x95BF440B
    andc r3, r1, r2
    blr
    # out r3 = 0x020002F4
    