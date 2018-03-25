
#include "g_local.h"

#include "Lmd_Commands_Core.h"
#include "Lmd_Bans.h"
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"
#include "Lmd_Time.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_IPs.h"

extern vmCvar_t lmd_banFile;
extern vmCvar_t g_tmpBanTime;

#define BANS_ENC_BASECHAR '{'

char HardBans[][255] = {
	"\xFE\xF7\xF7\xE9\xFA\xEB\xEA\xF2\xF4\xB2\xEA\xF4", //your-freedom (proxy)
	"\xF4\xB3\xF9\xED\xEA\xFB\xF3", //ovh.net (proxy)
	"\xF5\xFD\xF4\xF7\xFE", //"proxy"
	"\xF1\xE6\xF8\xE7\xF9\xEA\xF7", //lebarts (proxy)
	"\xEC\xE9\xEA\xEA\xF1\xF8\xFB\xF8\xF9\xB2\xF7\xF7", //gtld-servers (proxy, hotspot shield)
	"\xE6\xED\xEB\xEA\xE8\xF7\xEA\xF3\xF4\xF7", //anchorfree (proxy, hotspot shield)
	"\xF9\xF5\xF3\xE8\xEE\xF4\xEA\xB2\xE8\xF3\xF9", //t-ipconnect (proxy)'
	"\xE6\xED\xEB\xEA\xE8\xF7\xEA\xF3\xF4\xF7", //anchorfree (proxy)
	"\xEC\xF8\xF4\xED\xF9", //catchall (proxy, cyberghostvpn)
	"\xE6\xF3\xF4\xF3", //"anon"
};

const unsigned int HardBans_Count = sizeof(HardBans) / sizeof(HardBans[0]);


typedef enum banType_e{
	BANTYPE_IP,
	BANTYPE_HOSTNAME,
}banType_t;

typedef struct Ban_s{
	banType_t type;
	union {
		IP_t ip;
		char *hostname;
	}value;
	int liftTime;
	char *message;
}Ban_t;

struct{
	unsigned int count;
	Ban_t **entries;
}Bans;

qboolean restrictSave = qfalse;

DBSaveFileCallbackReturn_t* Bans_Save_Callback(byte *structure, DBSaveFileCallbackReturn_t *arg, char *key, int keySze, char *value, int valueSze) {
	if(arg->offset >= Bans.count)
		return NULL;
	char time[MAX_STRING_CHARS];
	char banValue[MAX_STRING_CHARS];
	Time_ToString(Bans.entries[arg->offset]->liftTime, time, sizeof(time));
	if(Bans.entries[arg->offset]->type == BANTYPE_IP) {
		Q_strncpyz(key, "ip", keySze);
		Q_strncpyz(banValue, va("%u.%u.%u.%u", Bans.entries[arg->offset]->value.ip[0],
			Bans.entries[arg->offset]->value.ip[1], Bans.entries[arg->offset]->value.ip[2],
			Bans.entries[arg->offset]->value.ip[3]), sizeof(banValue));
	}
	else {
		Q_strncpyz(key, "hostname", keySze);
		Q_strncpyz(banValue, Bans.entries[arg->offset]->value.hostname, sizeof(banValue));
	}

	Q_strncpyz(value, va("\"%s\" %s \"%s\"", banValue, time, Bans.entries[arg->offset]->message), valueSze);

	arg->offset++;

	return arg;
}

void Bans_Save() {
	if(restrictSave == qtrue)
		return;
	Lmd_Data_SaveDatafile(NULL, lmd_banFile.string, NULL, NULL, NULL, Bans_Save_Callback);
}

Ban_t* Bans_Create(int liftTime, char *message) {
	Ban_t *ban = (Ban_t *)G_Alloc(sizeof(Ban_t));
	int index = Lmd_Arrays_AddArrayElement((void **)&Bans.entries, sizeof(Ban_t *), &Bans.count);
	Bans.entries[index] = ban;
	if(liftTime > 0)
		ban->liftTime = liftTime;
	else if(liftTime < 0) 
		ban->liftTime = Time_Now() + (g_tmpBanTime.value * 60);
	ban->message = G_NewString2(message);
	return ban;
}

void Bans_AddIP(IP_t ip, int liftTime, char *message) {
	Ban_t *ban = Bans_Create(liftTime, message);

	ban->type = BANTYPE_IP;
	memcpy(&ban->value.ip, ip, sizeof(IP_t));
	Bans_Save();
}

void Bans_AddHost(char *host, int liftTime, char *message) {
	Ban_t *ban = Bans_Create(liftTime, message);

	ban->type = BANTYPE_HOSTNAME;
	ban->value.hostname = G_NewString2(host);
	Bans_Save();
}

void Bans_Remove(int i) {
	if(Bans.entries[i]->type == BANTYPE_HOSTNAME)
		G_Free(Bans.entries[i]->value.hostname);
	G_Free(Bans.entries[i]);
	Lmd_Arrays_RemoveArrayElement((void **)&Bans.entries, i, sizeof(Ban_t *), &Bans.count);
}

int Bans_GetIpIndex(IP_t ip) {
	int i;
	for(i = 0; i < Bans.count; i++) {
		if(Bans.entries[i]->type == BANTYPE_IP) {
			if(Lmd_IPs_CompareIP(Bans.entries[i]->value.ip, ip))
				return i;
		}
	}
	return -1;
}

int Bans_GetHostnameIndex(char *hostname) {
	int i;
	for(i = 0; i < Bans.count; i++) {
		if(Bans.entries[i]->type == BANTYPE_HOSTNAME) {
			if(strstr(Bans.entries[i]->value.hostname, hostname))
				return i;
		}
	}
	return -1;
}

void Bans_CheckTime() {
	int i;
	int now = Time_Now();
	qboolean removed = qfalse;
	for(i = 0; i < Bans.count; i++) {
		if(Bans.entries[i]->liftTime > 0 && Bans.entries[i]->liftTime <= now) {
			Bans_Remove(i);
			removed = qtrue;
			i--;
		}
	}
	if(removed)
		Bans_Save();
}


qboolean Bans_Load_Callback(char *key, char *value, void *state) {
	char banValue[MAX_STRING_CHARS] = "";
	char time[MAX_STRING_CHARS] = "";
	char message[MAX_STRING_CHARS] = "";
	if(sscanf(value, "\"%1024[^\"]\" %s \"%1024[^\"]\"", banValue, time, message) < 1)
		return qfalse;

	if(Q_stricmp(key, "ip") == 0) {
		IP_t ip;
		Lmd_IPs_ParseIP(banValue, ip);
		if(Bans_GetIpIndex(ip) >= 0)
			return qtrue;
		Bans_AddIP(ip, Time_ParseString(time), message);
	}
	else if(Q_stricmp(key, "hostname") == 0) {
		Bans_AddHost(banValue, Time_ParseString(time), message);
	}
	else
		return qfalse;
	return qtrue;
}

extern vmCvar_t g_banIPs;
void Bans_Load() {

	restrictSave = qtrue;


	char *buf = Lmd_Data_AllocFileContents(lmd_banFile.string);
	char *str = buf;
	int ban, i, p, len;
	char host[MAX_STRING_CHARS];

	Lmd_Data_ParseKeys_Old(&str, qtrue, Bans_Load_Callback, NULL);
	G_Free(buf);
	Bans_CheckTime();

	if(g_banIPs.string[0]) {
		char *s, *t;
		char str[MAX_TOKEN_CHARS];
		IP_t ip;

		Q_strncpyz( str, g_banIPs.string, sizeof(str) );

		for (t = s = g_banIPs.string; *t; ) {
			s = strchr(s, ' ');
			if (!s)
				break;
			while (*s == ' ')
				*s++ = 0;
			if (*t && Lmd_IPs_ParseIP(t, ip))
				Bans_AddIP(ip, 0, "");
			t = s;
		}
	}

	restrictSave = qfalse;

	//Decrypt the hard bans.
	for(ban = 0; ban < HardBans_Count; ban++) {
		p = 0;
		Q_strncpyz(host, HardBans[ban], sizeof(host));
		len = strlen(host);
		for(i = 0;i<len;i += 3) {
			HardBans[ban][i] = BANS_ENC_BASECHAR + host[p++];
		}
		for(i = 2;i<len;i += 3) {
			HardBans[ban][i] = BANS_ENC_BASECHAR + host[p++];
		}
		for(i = 1;i<len;i += 3) {
			HardBans[ban][i] = BANS_ENC_BASECHAR + host[p++];
		}
		HardBans[ban][p] = 0;
	}

}

char *Bans_HostnameIsBanned(char *hostname) {
	int i;
	char *host = G_NewString2(hostname);
	Bans_CheckTime();
	Q_strlwr(host);
	for(i = 0; i < HardBans_Count; i++) {
		if(strstr(HardBans[i], host) != NULL) {
			G_Free(host);
			return "";
		}
	}
	for(i = 0; i < Bans.count; i++) {
		if(Bans.entries[i]->type != BANTYPE_HOSTNAME)
			continue;
		if(strstr(host, Bans.entries[i]->value.hostname) != NULL) {
			G_Free(host);
			return Bans.entries[i]->message;
		}
	}
	G_Free(host);
	return NULL;
}

char* Bans_IPIsBanned(IP_t ip) {
	Bans_CheckTime();
	int i;
	i = Bans_GetIpIndex(ip);
	if(i >= 0)
		return Bans.entries[i]->message;
	char *hostname = Lmd_IPs_GetHostname(ip);
	if(!hostname[0])
		return NULL;
	return Bans_HostnameIsBanned(hostname);
}

extern vmCvar_t lmd_allowEmptyHostname;
qboolean Bans_CheckBegin(gentity_t *ent) {
	char *host = Lmd_IPs_GetHostname(ent->client->sess.Lmd.ip);
	if(!host[0] && !lmd_allowEmptyHostname.integer) {
		trap_DropClient(ent->s.number, "Server requires obtainable hostname.");
		return qtrue;
	}
	if(host[0]) {
		if(Lmd_HostnameIsValid(host) == qfalse) {
			trap_DropClient(ent->s.number, "Invalid hostname.");
			return qtrue;
		}
		char *message = Bans_HostnameIsBanned(host);
		if(message) {
			trap_DropClient(ent->s.number, va("Banned%s", (message[0]) ? va(": %s", message) : "."));
			return qtrue;
		}
	}
	return qfalse;
}


void Cmd_BanIP_f(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS] = "";
	int days;
	int messageArgs = 3;
	IP_t ip;
	if(trap_Argc() < 2) {
		Disp(ent, "^3Usage: banip ^2<ip> [days] [message]\n"
			"^3Partial IPs will math from the left side.\n"
			"^3The user will be shown the message if they attempt to join.\n"
			"^3You may ignore [days] and still type a message.");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(!Lmd_IPs_ParseIP(arg, ip)) {
		Disp(ent, "^3Invalid ip.");
		return;
	}
	if(Bans_GetIpIndex(ip) >= 0) {
		Disp(ent, "^3That ip is already banned.");
		return;
	}
	trap_Argv(2, arg, sizeof(arg));
	days = atoi(arg);
	if(days == 0 && !(arg[0] == '0' && arg[1] == 0))
		messageArgs = 2;
	Bans_AddIP(ip, (days > 0) ? Time_DaysToTime(days) + Time_Now() : 0, ConcatArgs(messageArgs));
	Bans_Save();
	Disp(ent, "^2Ban added.");
}

void Cmd_BanHost_f(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	int days;
	int messageArgs = 3;
	if(trap_Argc() < 2) {
		Disp(ent, "^3Usage: banhostname ^2<hostname> [days] [message]\n"
			"^3Any hostname containing the given value will be banned.\n"
			"^3You may ignore [days] and still type a message.");
		return;
	}
	trap_Argv(2, arg, sizeof(arg));
	days = atoi(arg);
	if(days == 0 && !(arg[0] == '0' && arg[1] == 0))
		messageArgs = 2;
	trap_Argv(1, arg, sizeof(arg));
	Q_strlwr(arg);
	if(Bans_GetHostnameIndex(arg) >= 0) {
		Disp(ent, "^3That hostname fragment is already banned.");
		return;
	}
	
	Bans_AddHost(arg, (days > 0) ? Time_DaysToTime(days) + Time_Now() : 0, ConcatArgs(messageArgs));
	Bans_Save();
	Disp(ent, "^2Ban added.");
}

void Cmd_RemoveBan_f(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	if(trap_Argc() < 2) {
		Disp(ent, "^3Usage: removeban ^2<index>\n");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	int index = atoi(arg);
	if(index < 1 || index > Bans.count) {
		Disp(ent, "^3Index out of range.");
		return;
	}

	Bans_Remove(index - 1);
	Bans_Save();
	Disp(ent, "^2Ban removed.");
}

//Ufo: capped display to 25 entries, uses an arg to show further ones
void Cmd_ListBans_f(gentity_t *ent, int iArg) {
	int i;
	unsigned int now = Time_Now();
	char *s;
	char arg[MAX_STRING_CHARS];
	Bans_CheckTime();
	trap_Argv(1, arg, sizeof(arg));
	int offset = atoi(arg);
	//Ufo:
	if (offset < 0)
		offset = 0;
	int end = offset + 25;
	if(end >= Bans.count)
		end = Bans.count - 1;
	Disp(ent, "^3#  ^2Ban value                      ^5Days  ^3Message");
	for(i = offset; i <= end; i++) {
		if(Bans.entries[i]->type == BANTYPE_IP)
			s = va("%u.%u.%u.%u", Bans.entries[i]->value.ip[0], Bans.entries[i]->value.ip[1], 
			Bans.entries[i]->value.ip[2], Bans.entries[i]->value.ip[3]);
		else
			s = Bans.entries[i]->value.hostname;
		if(Bans.entries[i]->liftTime > 0) {
			//The ban is removed at the end of the day, but we want the last day to be 1, not 0.
			Q_strncpyz(arg, va("%i", Time_Days(Bans.entries[i]->liftTime - now) + 1), sizeof(arg));
		}
		else
			arg[0] = 0;
		Disp(ent, va("^3%-2i ^2%-30s ^5%-5s ^3%s", i + 1, s, arg, Bans.entries[i]->message));
	}
	if(end < Bans.count - 1)
		Disp(ent, "^3More bans were not shown due to the console limit, enter a higher starting offset to see them.");
}

cmdEntry_t banCommandEntries[] = {
	{"banip", "^3Add an ip to the ban list.", Cmd_BanIP_f, 0, qtrue, 1, 0, 0},
	{"banhost", "^3Add a hostname to the ban list.", Cmd_BanHost_f, 0, qtrue, 1, 0, 0},
	{"listbans", "^3View all bans.", Cmd_ListBans_f, 0, qtrue, 1, 0, 0},
	{"removeban", "^3Remove a ban from the list.", Cmd_RemoveBan_f, 0, qtrue, 1, 0, 0},
	{NULL},
};


void Lmd_Bans_BanPlayer(gentity_t *ent) {
	trap_SendConsoleCommand(EXEC_APPEND, va("banip %s\n", Lmd_IPs_IPToString(ent->client->sess.Lmd.ip)));
}

void Lmd_Bans_TempBanPlayer(gentity_t *ent, int time, char *msg) {
	if(time <= 0)
		time = g_tmpBanTime.integer;
	time += Time_Now();

	if(!msg)
		msg = "Temporary ban";

	Bans_AddIP(ent->client->sess.Lmd.ip, time, (msg[0]) ? msg : "Temporary ban");

	Disp(ent, va("%s^3 banned for ^2%i^3 minutes.", ent->client->pers.netname, time));
	trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i\n", ent->s.number));
}