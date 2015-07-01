addSimple:
	# in r1 = 1
	# in r2 = 2
	add r3, r1, r2
	blr
	# out r3 = 3

addSimpleRecord:
	# in r1 = 1
	# in r2 = 2
	add. r3, r1, r2
	blr
	# out r3 = 3
	# out crf0 = Positive
