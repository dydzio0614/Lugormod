
#include "g_local.h"


//years since 1900
#define TM_BASE 1900
#define YEAR_BASE 1990

//uint_max = 4294967295

int trap_RealTime( qtime_t *qtime );
unsigned int Time_Now(){      
	qtime_t now;
	trap_RealTime(&now);
	return (((now.tm_year + TM_BASE) - YEAR_BASE) * (365 * 24 * 60 * 60)) + 
		(now.tm_yday * (24 * 60 *60)) +
		(now.tm_hour * (60 * 60)) +
		(now.tm_min * 60) +
		now.tm_sec;
}

unsigned int Time_Days(unsigned int time){
	return (time / 60 / 60 / 24);
}

unsigned int daysformonth(unsigned int month, unsigned int year ){
	return (30 + (((month & 9) == 8) || ((month & 9) == 1)) - (month == 2) - (!(((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0))) && (month == 2)));
}

unsigned int Time_MonthFromDays(unsigned int *days, unsigned int *year) {
	int maxdays, month;
	for(month = 1; (*days) > (maxdays = daysformonth(month, (*year))); month++) {
		(*days) -= maxdays;
		if(month == 12) {
			(*year)++;
			month = 1;
		}
	}
	return month;
}

void Time_ToHumanString(unsigned int time, char *str, unsigned int sze) {
	str[0] = 0;

	unsigned int year, days, hours, minutes;

	year = (int)floor((float)time / (365.0f * 24.0f * 60.0f * 60.0f));
	time -= year * (365 * 24 * 60 * 60);
	year += YEAR_BASE;

	days = (int)floor((float)time / (24.0f * 60.0f * 60.0f));
	time -= days * (24 * 60 * 60);

	hours = (int)floor((float)time / (60.0f * 60.0f));
	time -= hours * (60 * 60);

	minutes = (int)floor((float)time / 60.0f);
	time -= minutes * 60;

	unsigned int month = Time_MonthFromDays(&days, &year);
	Q_strncpyz(str, va("%u-%u-%u, %u:%.2u:%.2u", month, days, year, hours, minutes, time), sze);
}

void Time_ToString(unsigned int time, char *str, unsigned int sze) {
	str[0] = 0;

	int val;
	val = (int)floor((float)time / (365.0f * 24.0f * 60.0f * 60.0f));
	time -= val * (365 * 24 * 60 * 60);
	if(val)
		Q_strcat(str, sze, va("%iy", val + YEAR_BASE));

	val = (int)floor((float)time / (24.0f * 60.0f * 60.0f));
	time -= val * (24 * 60 * 60);
	if(val)
		Q_strcat(str, sze, va("%id", val));

	val = (int)floor((float)time / (60.0f * 60.0f));
	time -= val * (60 * 60);
	if(val)
		Q_strcat(str, sze, va("%ih", val));

	val = (int)floor((float)time / 60.0f);
	time -= val * 60;
	if(val)
		Q_strcat(str, sze, va("%im", val));

	if(time)
		Q_strcat(str, sze, va("%is", time));
}

unsigned int Time_DaysToTime(unsigned int days) {
	return days * 24 * 60 * 60;
}

unsigned int Time_ParseDayValue(unsigned int value) {
	if(value < YEAR_BASE * 1000)
		return 0;
	int year = (int)floor((float)value / 1000.0f);
	int day = value - (year * 1000);
	return Time_DaysToTime(day + (365 * (year - YEAR_BASE)));
}

unsigned int Time_ParseString(char *str) {
	char *c = str;
	char buf[MAX_STRING_CHARS];
	unsigned int bufPos = 0;
	qboolean isValid = qfalse;
	unsigned int time = 0;
	while(c[0]) {
		if(c[0] == 's') {
			isValid = qtrue;
			buf[bufPos] = 0;
			bufPos = 0;
			time += atoi(buf);
		}
		else if(c[0] == 'm') {
			isValid = qtrue;
			buf[bufPos] = 0;
			bufPos = 0;
			time += atoi(buf) * 60;
		}
		else if(c[0] == 'h') {
			isValid = qtrue;
			buf[bufPos] = 0;
			bufPos = 0;
			time += atoi(buf) * (60 * 60);
		}
		else if(c[0] == 'd') {
			isValid = qtrue;
			buf[bufPos] = 0;
			bufPos = 0;
			time += Time_DaysToTime(atoi(buf));
		}
		else if(c[0] == 'y') {
			isValid = qtrue;
			buf[bufPos] = 0;
			bufPos = 0;
			time += (atoi(buf) - YEAR_BASE) * (365 * 24 * 60 * 60);
		}
		else if(c[0] < '0' || c[0] > '9')
			break;
		else {
			buf[bufPos++] = c[0];
			if(bufPos >= sizeof(buf))
				return 0;
		}
		c++;
	}

	if(!isValid) {
		buf[bufPos] = 0;
		time = Time_ParseDayValue(atoi(buf));
	}
	return time;
}

void Time_DurationString(int duration, char *str, int sze) {
	int seconds = duration;
	int minutes = (int)floor(seconds / 60.0f);
	seconds -= minutes * 60;
	int hours = (int)floor(minutes / 60.0f);
	minutes -= hours * 60;

	str[0] = 0;
	if(hours > 0) {
		Q_strcat(str, sze, va("%i", hours));
	}
	if(minutes > 0 || str[0] != 0) {
		Q_strcat(str, sze, va("%s%2i", (hours > 0) ? ":" : "", minutes));
	}
	if(seconds > 0 || str[0] != 0) {
		Q_strcat(str, sze, va("%s%2i", (minutes > 0) ? ":" : "", seconds));
	}
}