Implementation of an IRIG-B receiver, agnostic of positive/negative edges
(only succession of 2, 5, 8 ms delays are assessed to find frame start)

Constraints:
 *  IRIG-B 4 to 7 (that is: including two-digit year and 1-366 day of year)
 *  Calculations are based on years between 2000 to 2099 (both inclusive)

This module is only intended as a helper to ease implement a classical IRIG-B receiver
specifically on embedded hardware. A low level mechanism should feed this module
with successions of 2, 5, 8 ms delays
