addExtended:
   # in r1 = 1
   # in r2 = 2
   # in xer.ca = 1
   adde r3, r1, r2
   blr
   # out r3 = 4
   # out xer.ca = 0
   
addExtended2:
   # in r1 = 1
   # in r2 = 4294967295
   # in xer.ca = 1
   adde r3, r1, r2
   blr
   # out r3 = 1
   # out xer.ca = 1

addExtendedRecordWithSO:
   # in r1 = 1
   # in r2 = 2
   # in xer.so = 1
   # in xer.ca = 0
   adde. r3, r1, r2
   blr
   # out r3 = 3
   # out crf0 = Positive | SummaryOverflow
   # out xer.ca = 0

addExtendedRecordNoSO:
   # in r1 = 1
   # in r2 = 2
   # in xer.so = 0
   # in xer.ca = 0
   adde. r3, r1, r2
   blr
   # out r3 = 3
   # out crf0 = Positive
   # out xer.ca = 0
   
addoExtendedOverflow:
   # in r1 = 4294967293
   # in r2 = 4
   # in xer.so = 1
   # in xer.ca = 1
   addeo. r3, r1, r2
   blr
   # out r3 = 2
   # out crf0 = Positive | SummaryOverflow
   # out xer.ov = 1
   # out xer.ca = 1
