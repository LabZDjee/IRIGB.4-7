/*
--------------------------------------------------------------------------------
  Author       : G. Gauthier
  Description  : IRIG-B utility module, documentation in header
--------------------------------------------------------------------------------
*/

/*
 * Implementation of an IRIG-B receiver
 * Constraints:
 *  IRIG-B 4 to 7 (that is with year and day of year)
 *  Calculations are based on years between 2000 to 2099
 */

#include "irigb.h"

#define FALSE (0)
#define TRUE  (!0)

byte binToBcd8 (byte bin)
{
  byte bcd;

  bcd = (bin / 10) << 4;
  bcd = bcd + (bin % 10);

  return (bcd);
}


byte bcdToBin8 (byte bcd)
{
  byte bin;

  bin = (bcd >> 4) * 10;
  bin = bin + (bcd & 0x0f);

  return (bin);
}

word binToBcd16 (word bin16)
{
  word bcd16;
  int i;

  for(i=bcd16=0; i<16; i+=4) {
    bcd16 += (bin16 % 10) << i;
    bin16 /= 10;
  }
  return (bcd16);
}


word bcdToBin16 (word bcd16)
{
  word bin16=0;
  int i;

  for(i=12; i>=0; i-=4) {
    bin16 = 10*bin16 + ((bcd16>>i)&15);
  }
  return (bin16);
}

boolean isLeap(byte year)
{
  return(year%4==0);
}

byte dayOfWeek(word dayOfYear, byte year)
{
  int accumul;
  if(year>0)
  accumul = 365*year + year / 4 + 1;
  else
  accumul = 0;
  return (byte)(((dayOfYear-1)+accumul+5)% 7 + 1);
}

byte daysOfMonth(byte month, byte year)
{
  word val=0;
  switch (month) {
    case 1: val = 31; break;
    case 2: val = isLeap(year) ? 29 : 28; break;
    case 3: val = 31; break;
    case 4: val = 30; break;
    case 5: val = 31; break;
    case 6: val = 30; break;
    case 7: val = 31; break;
    case 8: val = 31; break;
    case 9: val = 30; break;
    case 10: val = 31; break;
    case 11: val = 30; break;
    case 12: val = 31; break;
  }
  return(val);
}

word monthAndDayFromDayOfYear(word dayOfYear, byte year)
{
  word month;
  word dom;
  for(month=1; month<=12; month++) {
    dom=daysOfMonth(month, year);
    if(dayOfYear<=dom)
    return (month<<8) + dayOfYear;
    dayOfYear-=dom;
  }
  return(0);
}

byte filterIrigBDelay(byte d) {
  if(d>=1 && d<=3)
   return 2;
  if(d>=4 && d<=6)
   return 5;
  if(d>=7 && d<=9)
   return 8;
  return(255);
}
//returns P, 0, 1, or ?
char decodeIrigBDelays(byte d1, byte d2) {
  if(d1==2 && d2==8)
   return('0');
  if(d1==5 && d2==5)
   return('1');
  if(d1==8 && d2==2)
   return('P');
  return('?');
}

void feedIrigbFrame(sIrigbFrame* irigbFrame, byte newPulse)
{
  char val;
  byte posId;
  byte localIndex;
  if(newPulse!=2 && newPulse!=5 && newPulse!=8) {
    goto _reset_chase;
  }
  switch(irigbFrame->frameState) {
    default:
    case eChaseP0_8:
      if(newPulse==8)
       irigbFrame->frameState=eChaseP0_2;
      else
       goto _reset_chase;
      break;
    case eChaseP0_2:
      switch(newPulse) {
       case 2: irigbFrame->frameState=eChasePr_8; // fall-through: no break on purpose
       case 8: break;
       default: goto _reset_chase;
      }
      break;
    case eChasePr_8:
      if(newPulse==8)
       irigbFrame->frameState=eChasePr_2;
      else
       goto _reset_chase;
      break;
    case eChasePr_2:
      switch(newPulse) {
       case 2: irigbFrame->frameState=eChase_2_or_5; break;
       case 8: irigbFrame->frameState=eChaseP0_2; break;
       default: goto _reset_chase;
      }
      break;
    case eChase_2_or_5:
      if(newPulse==2 || newPulse==5) {
       irigbFrame->frameState=eAcquiringFirstFrame;
       irigbFrame->leadingPulse=newPulse;
       irigbFrame->frameBitNum=1;
       irigbFrame->scratchAcc=0;
      } else {
       goto _reset_chase;
    }
    break;
    case eAcquiringFirstFrame:
    case eLocked:
      if(irigbFrame->leadingPulse==0) {
       irigbFrame->leadingPulse=newPulse;
      } else {
       val = decodeIrigBDelays(irigbFrame->leadingPulse, newPulse);
       if(val == '?' ||
          (val == 'P' && irigbFrame->frameBitNum!=0 && irigbFrame->frameBitNum%10!=9)) {
          _reset_chase:
           irigbFrame->frameState=eChaseP0_8;
           return;
       }
       posId = (irigbFrame->frameBitNum+1)/10;
       localIndex = irigbFrame->frameBitNum%10;
       if(posId==0)
        localIndex--;
       if(val=='P') {
        switch(posId) {
          case 1:
           irigbFrame->scratchSec = irigbFrame->scratchAcc & 0x7f;
           break;
          case 2:
           irigbFrame->scratchMin = irigbFrame->scratchAcc & 0x7f;
           break;
          case 3:
           irigbFrame->scratchHour = irigbFrame->scratchAcc & 0x3f;
           break;
          case 4:
           irigbFrame->scracthDayOfYear = irigbFrame->scratchAcc;
           break;
          case 5:
           irigbFrame->scracthDayOfYear += (word)(irigbFrame->scratchAcc&3) << 8;
           break;
          case 6:
           irigbFrame->scratchYear = irigbFrame->scratchAcc;
           break;
         }
         irigbFrame->scratchAcc=0;
       } else if (val=='1'){
        if(localIndex<4)
         irigbFrame->scratchAcc |= 1 << localIndex;
        else if (localIndex>4)
         irigbFrame->scratchAcc |= 1 << (localIndex-1);
       }
      irigbFrame->frameBitNum++;
      if(irigbFrame->frameBitNum>99) {
        irigbFrame->frameState = eLocked;
        irigbFrame->frameBitNum=0;
      }
      irigbFrame->leadingPulse=0;
    }
    break;
  }
}

boolean irigBFrameStateToTimeDate(const sIrigbFrame* pIrigFrame, sTimeDateType* pTimeDate)
{
  word wordVal, dayOfYear;
  byte byteVal, year;
  if(pIrigFrame->frameState != eLocked) {
    pTimeDate->second =
    pTimeDate->minute =
    pTimeDate->hour =
    pTimeDate->dayOfWeek =
    pTimeDate->dayOfMonth =
    pTimeDate->month =
    pTimeDate->year = 0xff;
    return FALSE;
  }
  pTimeDate->second = pIrigFrame->scratchSec;
  pTimeDate->minute = pIrigFrame->scratchMin;
  pTimeDate->hour = pIrigFrame->scratchHour;
  dayOfYear = bcdToBin16(pIrigFrame->scracthDayOfYear);
  year = bcdToBin8(pIrigFrame->scratchYear);
  byteVal = dayOfWeek(dayOfYear, year);
  pTimeDate->dayOfWeek = binToBcd8(byteVal);
  wordVal = monthAndDayFromDayOfYear(dayOfYear, year);
  byteVal = (byte)(wordVal & 255);
  pTimeDate->dayOfMonth = binToBcd8(byteVal);
  byteVal = (byte)(wordVal >> 8);
  pTimeDate->month = binToBcd8(byteVal);
  pTimeDate->year = pIrigFrame->scratchYear;
  return TRUE;
}

