
#include "g_local.h"
#include "Lmd_Console.h"

#ifdef LMD_DYNAMICTHEME

typedef struct consoleTheme_s{
	char base;
	char replace;
}consoleTheme_t;

consoleTheme_t theme[] = {
	{CT_B[1], '3'},		//base
	{CT_AR[1], '2'},	//argument required
	{CT_AO[1], '5'},	//argument optional
	{CT_V[1], '2'},		//value
	{CT_PB[1], '5'},	//prof bot
	{CT_PF[1], '4'},	//prof forceuser
	{CT_PM[1], '1'},	//prof merc
	{CT_PG[1], '6'},	//prof god
	{CT_E[1], '1'},
};
const unsigned int themeCount = sizeof(theme) / sizeof(consoleTheme_t);

void Console_ThemeString(char *str) {
	int i, t, len = strlen(str);
	//NOTE: this could be further optimized by having all CT_ numbersr be linear, and just using that as the index.
	for(i = 0; i < len; i++) {
		if(str[i] >= 32)
			continue;
		for(t = 0; t < themeCount; t++) {
			if(str[i] == theme[t].base) {
				str[i] = theme[t].replace;
				break;
			}
		}
	}
}

#endif


#if FALSE

#define TARGET_PLAYER 0
#define TARGET_ENTITY 1
#define TARGET_SERVER 2

// "You have <activator:credits> credits"
// "This terminal requires <self:usereq.credits> credits to use."

typedef struct ConsoleEscapeSeq_s {
	char *pattern;
	int target;
	int ofs;
}ConsoleEscapeSeq_t;

ConsoleEscapeSeq_t consoleEscapes[] = {
	{"plname", 
};

#endif

void Disp (gentity_t *ent, const char *msg) {
	const int bufLen = SERVERCOMMAND_MAX - 9; // -10 print "\n"

	unsigned int len = strlen(msg);
	char buf[MAX_STRING_CHARS];
	const char *str = msg;
	if(len == 0) {
		trap_SendServerCommand(ent->s.number, "print \"\n\"");
		return;
	}
	while(len > 0) {
		Q_strncpyz(buf, str, sizeof(buf));
		buf[bufLen - 1] = 0;
#ifdef LMD_DYNAMICTHEME
		Console_ThemeString(buf);
#endif
		if (ent) {
			trap_SendServerCommand(ent->s.number, va("print \"%s\n\"", buf));
		}
		else {
			Com_Printf("%s\n",msg);
		}
		if(len < bufLen)
			return;
		len -= bufLen;
		str += bufLen;
	}
}

void DispContiguous(gentity_t *ent, const char *msg) {
	const int contigLength = SERVERCOMMAND_MAX - 9; // -9 print ""
	static char buf[contigLength];
	static int len = 0;
	// +1 linefeed
	if(msg == NULL || len + strlen(msg) + 1 > contigLength) {
		//Already have linefeed at the end.
		if (ent)
			trap_SendServerCommand(ent->s.number, va("print \"%s\"", buf));
		else
			Com_Printf("%s",buf);
		buf[0] = 0;
		len = 0;
	}

	if(msg) {
		Q_strcat(buf, sizeof(buf), va("%s\n", msg));
		len += strlen(msg) + 1;
	}
}

#if 0

void Euka_StringUtils_ListItems(gentity_t *ent, byte *structArray, int structSize, int bitOffset, int numEntries, char *arg,
								qboolean (*Callback)(char *input, int index, char *output, int outputSze)){
	int start = 0;
	int count = 0;
	int i;
	char String[MAX_STRING_CHARS];
	char Output[MAX_STRING_CHARS];
	if(arg)
		start = atoi(arg);
	if(arg && arg[0] && (start == 0 && !(arg[0] == '0' && arg[1] == '\0'))){
		char mid[MAX_STRING_CHARS];
		char *pStart = NULL;
		int len;
		Q_strncpyz(Output, arg, sizeof(Output));
		Q_CleanStr(Output);
		Q_strlwr(Output);
		len = strlen(Output);
		Disp(ent, "\n^3Containing the string: ^2%s\n^5===========================", Output);
		for(i = 0;i<numEntries;i++){
			Q_strncpyz(String, *(char **) (*(byte **)(structArray + (i * structSize)) + bitOffset), sizeof(String));
			Q_CleanStr(String);
			Q_strlwr(String);
			if(!(pStart = strstr(String, Output)))
				continue;
			count++;
			if(count > 20)
				continue;
			Q_strncpyz(mid, pStart, sizeof(mid));
			pStart[0] = 0; //cut out the start of the match from 'string'
			mid[len] = 0;
			pStart += len;
			Q_strncpyz(String, va("^2%s^6%s^2%s", String, mid, pStart), sizeof(String));
			if(Callback){
				if(!Callback(String, i, Output, sizeof(Output)))
					continue;
				Q_strncpyz(Output, String, sizeof(Output));
			}
			Disp(ent, "%s", Output);
		}
	}
	else{
		int end;
		if(start > numEntries-1)
			start = numEntries-1;
		end = start + 20;
		if(end > numEntries)
			end = numEntries;
		count = numEntries;
		Disp(ent, "\n^2%i ^3to ^2%i ^3(of ^2%i^3):\n^5===========================", start, end, numEntries);
		for(i = start;i<end && i < end;i++){
			Q_strncpyz(String, *(char **) (*(byte **)(structArray + (i * structSize)) + bitOffset), sizeof(String));
			if(Callback){
				if(!Callback(String, i, Output, sizeof(Output)))
					continue;
				Q_strncpyz(Output, String, sizeof(Output));
			}
			Disp(ent, "%s", Output);
		}
	}
	Disp(ent, "^5===========================");
}

char *Euka_StringUtils_ReplaceInstance(char *string, char *instance, char *replace){
	char *s = (string-1), *last = string;
	int instanceLen = strlen(instance);
	static char buf[MAX_STRING_CHARS];
	buf[0] = 0;
	while((s = strstr(s+1, instance))){
		//Q_strncpyz((buf + bufPos), last, s - last);
		//Q_strcat(buf, s - last + 1, last);
		Q_strncpyz(buf + strlen(buf), last, (s - last + 1));	//+1 because we are using max size as length count, 
																//and max size subtracts 1 to leave room for null char
		Q_strcat(buf, sizeof(buf), replace);
		last = s + instanceLen;
	}
	Q_strcat(buf, sizeof(buf), last);
	return buf;
}

#endif

