cntlzw00000000:
	# in r3 = 0x00000000
	cntlzw r3, r3
	blr
	# out r3 = 32

cntlzw00000001:
	# in r3 = 0x00000001
	cntlzw r3, r3
	blr
	# out r3 = 31

cntlzw00000100:
	# in r3 = 0x00000100
	cntlzw r3, r3
	blr
	# out r3 = 23
  
cntlzw00010000:
	# in r3 = 0x00010000
	cntlzw r3, r3
	blr
	# out r3 = 15
  
cntlzw01000000:
	# in r3 = 0x01000000
	cntlzw r3, r3
	blr
	# out r3 = 7
  
cntlzw80000000:
	# in r3 = 0x80000000
	cntlzw r3, r3
	blr
	# out r3 = 0
