addSimple:
	# in r1 = 1
	# in r2 = 2
	add r3, r1, r2
	blr
	# out r3 = 3

addSimpleRecordWithSO:
	# in r1 = 1
	# in r2 = 2
  # in xer.so = 1
	add. r3, r1, r2
	blr
	# out r3 = 3
	# out crf0 = Positive | SummaryOverflow

addSimpleRecordNoSO:
	# in r1 = 1
	# in r2 = 2
  # in xer.so = 0
	add. r3, r1, r2
	blr
	# out r3 = 3
	# out crf0 = Positive
  