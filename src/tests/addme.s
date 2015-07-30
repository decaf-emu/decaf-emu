addMinusOneExtended:
   # in r1 = 654653
   # in xer.ca = 1
   addme r3, r1
   blr
   # out r3 = 654653
   # out xer.ca = 1
   
addMinusOneExtended2:
   # in r1 = 4294967295
   # in xer.ca = 0
   # in xer.so = 1
   addmeo. r3, r1
   blr
   # out r3 = 4294967294
   # out crf0 = Negative | SummaryOverflow
   # out xer.ca = 1
   # out xer.ov = 0
   
   
addZeroExtended:
   # in r1 = 654653
   # in xer.ca = 1
   addze r3, r1
   blr
   # out r3 = 654654
   # out xer.ca = 0
   
addZeroExtended2:
   # in r1 = 4294967295
   # in xer.ca = 1
   # in xer.so = 1
   addzeo. r3, r1
   blr
   # out r3 = 0
   # out crf0 = Zero | SummaryOverflow
   # out xer.ca = 1
   # out xer.ov = 0
   