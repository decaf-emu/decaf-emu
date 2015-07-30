addCarrying:
   # in r1 = 1
   # in r2 = 2
   addc r3, r1, r2
   blr
   # out r3 = 3
   # out xer.ca = 0

addCarryingRecordWithSO:
   # in r1 = 1
   # in r2 = 2
   # in xer.so = 1
   addc. r3, r1, r2
   blr
   # out r3 = 3
   # out crf0 = Positive | SummaryOverflow
   # out xer.ca = 0

addCarryingRecordNoSO:
   # in r1 = 1
   # in r2 = 2
   # in xer.so = 0
   addc. r3, r1, r2
   blr
   # out r3 = 3
   # out crf0 = Positive
   # out xer.ca = 0
   
addoCarryingOverflow:
   # in r1 = 4000000000
   # in r2 = 4000000000
   # in xer.so = 1
   addco. r3, r1, r2
   blr
   # out r3 = 3705032704
   # out crf0 = Negative | SummaryOverflow
   # out xer.ov = 0
   # out xer.ca = 1
