/*
--------------------------------------------------------------------------------
  Author       : G. Gauthier
  Description  : IRIG-B utility module header
--------------------------------------------------------------------------------
*/

/*
 * Implementation of an IRIG-B receiver, agnostic of positive/negative edges
 * (only succession of 2, 5, 8 ms delays are assessed to find frame start)
 * Constraints:
 *  IRIG-B 4 to 7 (that is: including two-digit year and 1-366 day of year)
 *  Calculations are based on years between 2000 to 2099 (both inclusive)
 */

#ifndef IRIGB_H_
#define IRIGB_H_

typedef unsigned char   byte;
typedef unsigned char   boolean;
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned long   dword;
typedef unsigned short  word;

// date and time structure such as implemented in RTC chips such as M41Txx or DS1307
// all in BCD (Binary Coded Decimal)
typedef struct
{
  byte second;     // 0 to 59
  byte minute;     // 0 to 59
  byte hour;       // 0 to 23
  byte dayOfWeek;  // 1 (Monday) to 7 (Sunday)
  byte dayOfMonth; // 1 to 31
  byte month;      // 1 to 12
  byte year;       // 0 to 99
} sTimeDateType;

// defines states of a state machine implemented in method 'feedIrigbFrame'
// which takes pulses one by one (e.g.: 8, 2, 8, 2, 5, 5, 2, 8...) and populates date/time data
typedef enum
{
  // initial state when sync with frame is not done or was lost
  eChaseP0_8, // P0 (end of a frame, offset 99)
  eChaseP0_2,
  eChasePr_8, // Pr (start of frame, offset 0)
  eChasePr_2,
  eChase_2_or_5, // got Pr: expect first data bit
  // initial data acquiring in progress
  eAcquiringFirstFrame,
  // data are ready for consumption
  eLocked
} eIrigbFrameState;

// struct which memorizes states and values in order to capture IRIG-B data
typedef struct _sIrigbFrame
{
  eIrigbFrameState frameState; // 'machine state'
  byte frameBitNum; // captured frame offset, 0 to 99
  byte leadingPulse; // value of leading pulse (0 if pulse couple just processed)
  // scratch values in BCD
  byte scratchAcc; // temporary to store value, bit by bit
  byte scratchSec; // seconds, zero based
  byte scratchMin; // minutes, zero based
  byte scratchHour; // hours, zero based
  word scracthDayOfYear; // one-based
  byte scratchYear; // two-digit year, zero-based
} sIrigbFrame;

////////////////// Low Level Stuff //////////////////

// simple filter which accepts delays around 2, 5, and 8 seconds
// returns: 2, 5, 8, or 
byte filterIrigBDelay(byte d);

//returns char 'P', '0', '1', if sequence d1 and d2 is valid (e.g. d1==2 and d2==8)
// or '?' if sequence is invalid
char decodeIrigBDelays(byte d1, byte d2);

// bin to BCD both ways translators
byte binToBcd8 (byte hex);
byte bcdToBin8 (byte bcd);
word binToBcd16 (word hex16);
word bcdToBin16 (word bcd16);

// return TRUE is year is a leap year
boolean isLeap(byte year);

// return day of week: 1 (Monday) to 7 (Sunday)
// day of year is one based
byte dayOfWeek(word dayOfYear, byte year);

// returns number of days of a given month (one-based) and year (0-99)
byte daysOfMonth(byte month, byte year);

////////////////// High Level Stuff //////////////////

// return one-based dayOfMonth (low byte) and one-based month (high byte)
word monthAndDayFromDayOfYear(word dayOfYear, byte year);

// feeds sIrigbFrame 'irigbFrame' each time a new pulse 'newPulse' is received
// newPulse: 2, 5, 8, or anything to re-init irigbFrame
void feedIrigbFrame(sIrigbFrame* irigbFrame, byte newPulse);

// transfer time/date data from an 'eIrigbFrameState' structure to
// a BCD 'TimeDateType' structure
// if field 'frameState' of input structure is not 'eLocked', all output
// fields are set to 0xff
// return true if transfer is successful
boolean irigBFrameStateToTimeDate(const sIrigbFrame* pIrigFrame, sTimeDateType* pTimeDate);

#endif /* IRIGB_H_ */
