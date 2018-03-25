
unsigned int Time_Now();
unsigned int Time_Days(unsigned int time);

unsigned int Time_ParseDayValue(unsigned int value);
unsigned int Time_ParseString(char *str);

void Time_ToString(unsigned int time, char *str, unsigned int sze);
void Time_ToHumanString(unsigned int time, char *str, unsigned int sze);

void Time_DurationString(int duration, char *str, int sze);

unsigned int Time_DaysToTime(unsigned int days);
