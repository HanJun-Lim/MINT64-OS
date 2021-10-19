#include "RTC.h"


// read Current Time from CMOS Memory (stored by RTC Controller)
void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond)
{
	BYTE bData;


	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbHour = RTC_BCDTOBINARY(bData);

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMinute = RTC_BCDTOBINARY(bData);

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbSecond = RTC_BCDTOBINARY(bData);
}


// read Current Date from CMOS Memory (stored by RTC Controller)
void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek)
{
	BYTE bData;
	

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pwYear = RTC_BCDTOBINARY(bData) + 2000;

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMonth = RTC_BCDTOBINARY(bData);

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfMonth = RTC_BCDTOBINARY(bData);

	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfWeek = RTC_BCDTOBINARY(bData);
}


// return Day String using day of week value
char* kConvertDayOfWeekToString(BYTE bDayOfWeek)
{
	static char* vpcDayOfWeekString[8] = { "Error", "Sunday", "Monday", "Tuesday",
		"Wednesday", "Thursday", "Friday", "Saturday" };

	if(bDayOfWeek >= 8)
		return vpcDayOfWeekString[0];

	return vpcDayOfWeekString[bDayOfWeek];
}


