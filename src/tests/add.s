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

testCRF:
   # in r1 = 1
   # in r2 = 2
   # in r3 = 0
   # in xer.so = 0
   mtcr r3
   add. r3, r1, r2
   mfcr r4
   blr
   # out r3 = 3
   # out r4 = 0x40000000
   # out crf0 = Positive
   # out crf1 = 0
   # out crf2 = 0
   # out crf3 = 0
   # out crf4 = 0
   # out crf5 = 0
   # out crf6 = 0
   # out crf7 = 0
