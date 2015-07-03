addicRecordMinusOne:
   # in r3 = 2
   # in crf0 = 0
   # in xer.ca = 0
   # in xer.so = 0
   # in xer.ov = 0
   addic. r3, r3, -1
   blr
   # out crf0 = Positive
   # out xer.ca = 1
   # out r3 = 1
