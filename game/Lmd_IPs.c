

#include "g_local.h"
#include "Lmd_IPs.h"
#include "Lmd_Arrays.h"
#include "Lmd_Commands_Core.h"
#include "Lmd_Console.h"

#ifdef _WIN32
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h> //For _beginthreadex
#pragma comment(lib, "ws2_32.lib")
#elif defined  __linux__
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define MAX_IPDATA_VALUES 10

static IP_t ip_local = {127, 0, 0, 1};

//Ufo:
int Lmd_IPs_Run_Timer = 0;

qboolean Lmd_IPs_CompareIP(IP_t primary, IP_t targ) {
	int i;
	if(primary[0] == 0)
		return qfalse; //empty ip
	for(i = 0; i < 4; i++) {
		if(primary[i] == 0)
			continue;
		if(primary[i] != targ[i])
			return qfalse;
	}
	return qtrue;
}

//http://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_host_names
qboolean Lmd_HostnameIsValid(char *str) {
	int i, len = strlen(str);
	int c = 0, dots = 0;
	if(len > 255)
		return false;
	for(i = 0; i < len; i++) {
		if(c == 0 && str[i] < 'a' && str[i] > 'z' && str[i] < 'A' && str[i] > 'Z' && str[i] < '0' && str[i] > '9')
			return qfalse;
		if(str[i] < 'a' && str[i] > 'z' && str[i] < 'A' && str[i] > 'Z' && str[i] < '0' && str[i] > '9' && str[i] != '-' && str[i] != '.')
			return qfalse;
		if(str[i] == '.') {
			dots++;
			if(c == 0)
				return qfalse;
			if(c == 63)
				return qfalse;
			c = 0;
			continue;
		}
		c++;
	}
	/* Not doing this.  Hostnames within a private network are single segments.
	//We could check for more dots than 1, but meh.
	if(dots == 0)
		return qfalse;
	*/

	return qtrue;
}

qboolean Lmd_IPs_ParseIP(char *str, IP_t ip) {
	int part;
	char data[4];
	int i;

	if(Q_stricmp(str, "localhost") == 0) {
		ip[0] = 127;
		ip[1] = 0;
		ip[2] = 0;
		ip[3] = 1;
		return qtrue;
	}

	memset(ip, 0, sizeof(IP_t));
	for(part = 0; part < 4;part++) {
		i = 0;
		while(str[0] && str[0] != '.') {
			if(str[0] < '0' || str[0] > '9')
				return qfalse;
			data[i++] = str[0];
			str++;
		}
		data[i] = 0;
		str++;

		ip[part] = atoi(data);

		if(!str[0])
			break;
	}
	return qtrue;
}

char* Lmd_IPs_IPToString(IP_t ip) {
	if(ip[0] == 0)
		return "Unknown";
	else if(Lmd_IPs_CompareIP(ip_local, ip))
		return "localhost";
	return va("%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}


typedef struct IPData_s{
	char *names[MAX_IPDATA_VALUES];
	int accounts[MAX_IPDATA_VALUES];
	IP_t ip;
	struct {
		char *host;
		int attempts;
		qboolean valid; //Is the obtained hostname valid?
		qboolean busy; //Are we currently obtaining the hostname?
		qboolean updated; //Do we have new hostname data since the last ip run?
	}Hostname;
}IPData_t;

struct {
	IPData_t **data;
	unsigned int count;
}IPList;


int Lmd_IPs_GetIPIndex(IP_t ip) {
	unsigned int i;
	for(i = 0; i < IPList.count; i++) {
		if(Lmd_IPs_CompareIP(ip, IPList.data[i]->ip))
			return i;
	}
	return -1;
}

IPData_t* Lmd_IPs_CreateIPData(IP_t ip) {
	int i = Lmd_Arrays_AddArrayElement((void **)&IPList.data, sizeof(IPData_t *), &IPList.count);
	IPList.data[i] = (IPData_t *)G_Alloc(sizeof(IPData_t));
	memcpy(IPList.data[i]->ip, ip, sizeof(IP_t));
	return IPList.data[i];
}

IPData_t* Lmd_IPs_GetIPData(IP_t ip) {
	int i = Lmd_IPs_GetIPIndex(ip);
	if(i == -1)
		return Lmd_IPs_CreateIPData(ip);
	return IPList.data[i];
}

void Lmd_IPs_AddName(IP_t ip, char *name) {
	IPData_t *data = Lmd_IPs_GetIPData(ip);
	int i;
	int index = -1;

	if(Q_stricmp(name, "Padawan") == 0)
		return;

	for(i = 0; i < MAX_IPDATA_VALUES; i++) {
		if(Q_stricmp(data->names[i], name) == 0) {
			if(i == 0) {
				//Already at the front
				return;
			}
			//Set the name to the end of the list, as its the most recent
			name = data->names[i];
			Lmd_Arrays_Shift((void *)data->names, sizeof(char *), i + 1, 0, 1);
			data->names[0] = name;
			return;
		}
		if(data->names[i] == NULL) {
			index = i;
			break;
		}
	}
	if(index == -1) {
		G_Free(data->names[MAX_IPDATA_VALUES - 1]);
		index = 0;
		Lmd_Arrays_Shift((void *)data->names, sizeof(char *), MAX_IPDATA_VALUES, 0, 1);
	}
	data->names[index] = G_NewString2(name);
}

void Lmd_IPs_AddAccount(IP_t ip, int id) {
	IPData_t *data = Lmd_IPs_GetIPData(ip);
	int i;
	int index = -1;

	if(id == 0)
		return;

	for(i = 0; i < MAX_IPDATA_VALUES; i++) {
		if(data->accounts[i] == id) {
			if(i == 0) {
				//Already at the front
				return;
			}
			//Set the name to the end of the list, as its the most recent
			Lmd_Arrays_Shift((void *)data->names, sizeof(int), i + 1, 0, 1);
			data->accounts[0] = id;
			return;
		}
		if(data->accounts[i] == 0) {
			index = i;
			break;
		}
	}
	if(index == -1) {
		index = 0;
		Lmd_Arrays_Shift((void *)data->accounts, sizeof(int), MAX_IPDATA_VALUES, 0, 1);
	}
	data->accounts[index] = id;
}

char *Lmd_IPs_GetHostname(IP_t ip) {
	int i = Lmd_IPs_GetIPIndex(ip);

	if(i < 0)
		return "";

	if(!IPList.data[i]->Hostname.host)
		return "";
	
	return IPList.data[i]->Hostname.host;
}

static qboolean networkInit = qfalse;
void Lmd_IPs_Init() {
#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return;
    }
#endif
	networkInit = qtrue;
}

char* Lmd_IPs_QueryHostname(IP_t ip) {
	if(!networkInit)
		return "";
	struct hostent *remoteHost;
    struct in_addr addr;

	//Might be able to directly set it to the contents of IP_t...
    addr.s_addr = inet_addr(va("%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]));
    if (addr.s_addr == INADDR_NONE) 
		return "";

	remoteHost = gethostbyaddr((char *) &addr, 4, AF_INET);
	if(!remoteHost)
		return "";
	return remoteHost->h_name;
}

unsigned int Lmd_IPs_IdentifyHostnameCall(IPData_t* data) {

	data->Hostname.host = G_NewString2(Lmd_IPs_QueryHostname(data->ip));
	data->Hostname.valid = Lmd_HostnameIsValid(data->Hostname.host);

	data->Hostname.busy = qfalse;

	return 0;
}

void Lmd_IPs_IdentifyHostnameByData(IPData_t *data) {
	if(data->Hostname.host || data->Hostname.busy)
		return;

	data->Hostname.busy = qtrue;
	data->Hostname.attempts++;

	//CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Lmd_IPs_IdentifyHostnameCall, data, 0, NULL);
#ifdef _WIN32
	_beginthreadex(NULL, 0, (unsigned int (__stdcall *)(void *))Lmd_IPs_IdentifyHostnameCall, data, 0, NULL);
#elif defined __linux__
	pthread_t tid;
	pthread_create(&tid, NULL, (unsigned int (*)(void *))Lmd_IPs_IdentifyHostnameCall, data);
#endif
}

void Lmd_IPs_IdentifyHostname(IP_t ip) {

	if(ip[0] == 0) {
		return;
	}

	Lmd_IPs_IdentifyHostnameByData(Lmd_IPs_GetIPData(ip));
}

char *Bans_HostnameIsBanned(char *hostname);
void Lmd_IPs_CheckUpdatedHostnames() {
	char *message;
	int i, j;
	//Check updates from previous runs, we might have new info that may lead to a ban.
	for(i = 0; i < IPList.count; i++) {
		if(IPList.data[i]->Hostname.updated == qtrue && IPList.data[i]->Hostname.host != NULL && IPList.data[i]->Hostname.busy == qfalse) {
			IPList.data[i]->Hostname.updated = qfalse;
			for(j = 0; j < MAX_CLIENTS; j++) {

				//NOTE: CompareIP considers an entry of 0 in the primary ip as a wildcard.  Might be an issue?
				//IP segment of 0 is invalid, but it /might/ come up.
				if(Lmd_IPs_CompareIP(level.clients[j].sess.Lmd.ip, IPList.data[i]->ip)) {

					if(IPList.data[i]->Hostname.valid == qfalse) {
						trap_DropClient(g_entities[j].s.number, "Invalid hostname.");
						continue;
					}

					message = Bans_HostnameIsBanned(IPList.data[i]->Hostname.host);
					if(message)
						trap_DropClient(g_entities[j].s.number, va("Banned%s", (message[0]) ? va(": %s", message) : "."));
				}
			}
		}
	}
}

char *Bans_HostnameIsBanned(char *hostname);
void Lmd_IPs_Run() {

	//Ufo: we don't need to run this every server frame
	if (Lmd_IPs_Run_Timer < level.time) {
		int i;
	
		Lmd_IPs_CheckUpdatedHostnames();
	
		for(i = 0; i < IPList.count; i++) {
			if(IPList.data[i]->Hostname.host == NULL && IPList.data[i]->Hostname.busy == qfalse) {
				IPList.data[i]->Hostname.updated = qtrue;
				Lmd_IPs_IdentifyHostnameByData(IPList.data[i]);
			}
		}

		Lmd_IPs_Run_Timer = level.time + 10000;
	}
}



void Lmd_IPs_SetPlayerIP(gclient_t *client, IP_t ip) {
	if(Lmd_IPs_CompareIP(ip_local, ip)) {
		client->pers.localClient = qtrue;
		memcpy(client->sess.Lmd.ip, ip_local, sizeof(IP_t));
		return;
	}
	memcpy(client->sess.Lmd.ip, ip, sizeof(IP_t));
	Lmd_IPs_IdentifyHostname(ip);
}


void Cmd_PlayerIPs_f(gentity_t *ent, int iArg) {
	int i;
	qboolean shade = qfalse;
	char name[MAX_NAME_LENGTH];
	char dstr[MAX_TOKEN_CHARS] = "";
	gentity_t *pEnt;
	Disp(ent,	"^3--------------------------------------------------------------\n"
				"   ^2Player                       ^1IP              ^5Hostname");

	for (i = 0; i < MAX_CLIENTS; i++) {
		pEnt = &g_entities[i];
		if(!pEnt->inuse || !pEnt->client)
			continue;
		if(pEnt->r.svFlags & SVF_BOT)
			continue;

		dstr[0] = 0;

		Q_strcat(dstr, sizeof(dstr), va("%-2i ", i));

		if(shade)
			Q_strcat(dstr, sizeof(dstr),"^3");
		else
			Q_strcat(dstr, sizeof(dstr),"^7");
		shade = !shade;
		
		Q_strncpyz(name, pEnt->client->pers.netname, sizeof(name));
		Q_CleanStr(name);
		Q_strcat(dstr, sizeof(dstr), va("%-28s ", name));

		//xxx.xxx.xxx.xxx
		Q_strcat(dstr, sizeof(dstr), va("%-15s ", Lmd_IPs_IPToString(pEnt->client->sess.Lmd.ip)));
		//Max legal hostname is 255, and illegal hostnames are not accepted, so this wont overflow.
		Q_strcat(dstr, sizeof(dstr), Lmd_IPs_GetHostname(pEnt->client->sess.Lmd.ip));

		DispContiguous(ent, dstr);
	}
	DispContiguous(ent, NULL);
}

void Cmd_FindIP_ListNames(gentity_t *ent, IP_t ip) {
	int i;
	char *host;
	int index = Lmd_IPs_GetIPIndex(ip);
	if(index < 0) {
		Disp(ent, "^3That ip is not in the logs.");
		return;
	}
	
	host = IPList.data[index]->Hostname.host;
	if(!host)
		host = "Unknown host";
	Disp(ent, va("^3Last %i names from ip ^2%s ^3(^2%s^3)\n^3-----------------------", MAX_IPDATA_VALUES, Lmd_IPs_IPToString(ip), host));
	for(i = 0; i < MAX_IPDATA_VALUES; i++) {
		if(IPList.data[index]->names[i] == NULL)
			break;
		Disp(ent, IPList.data[index]->names[i]);
	}
	Disp(ent, "^3-----------------------");
}

void Cmd_FindIP_FindName(gentity_t *ent, char *name) {
	int i, j;
	char *host;
	char cleanName[MAX_STRING_CHARS];
	char cleanArg[MAX_STRING_CHARS];

	Q_strncpyz(cleanArg, name, sizeof(cleanArg));
	Q_CleanStr2(cleanArg);
	Q_strlwr(cleanArg);

	Disp(ent, va("^3IPs matching the partial name \'^7%s^3\'\n^3--------------------------------------------------------------", name));
	for(i = 0; i < IPList.count; i++) {
		for(j = 0; j < MAX_IPDATA_VALUES; j++) {
			if(IPList.data[i]->names[j] == NULL)
				break;
			Q_strncpyz(cleanName, IPList.data[i]->names[j], sizeof(cleanName));
			Q_CleanStr2(cleanName);
			Q_strlwr(cleanName);

			if(strstr(cleanName, cleanArg)) {
				host = IPList.data[i]->Hostname.host;
				if(!host)
					host = "Unknown host";
				Disp(ent, va("%-25s ^2%-15s ^3%s", IPList.data[i]->names[j], Lmd_IPs_IPToString(IPList.data[i]->ip), host));
			}
		}
	}
}

void Cmd_FindIP_f(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));

	if(arg[0]) {
		IP_t ip;
		if(Lmd_IPs_ParseIP(arg, ip))
			Cmd_FindIP_ListNames(ent, ip);
		else
			Cmd_FindIP_FindName(ent, arg);
	}
	else
		Disp(ent, CT_B"Usage: /findip "CT_AR"<ip or partial name>");
}

void Cmd_ListIPs_f(gentity_t *ent, int iArg) {
	int i, ofs, c = 0;
	char dstr[MAX_TOKEN_CHARS] = "";
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	ofs = atoi(arg);
	if(ofs > IPList.count - 25)
		ofs = IPList.count - 25;
	else if(ofs < 0)
		ofs = 0;
	
	DispContiguous(ent,	"^3--------------------------------------------------------------\n"
				"^2IP              ^3Hostname");
	for(i = ofs ; i < IPList.count; i++) {
		dstr[0] = 0;

		Q_strcat(dstr, sizeof(dstr), va("^3%-15s ", Lmd_IPs_IPToString(IPList.data[i]->ip)));

		if(IPList.data[i]->Hostname.host == NULL) {
			if(IPList.data[i]->Hostname.busy)
				Q_strcat(dstr, sizeof(dstr), va("^3Obtaining... (attempt ^2%i^3)", IPList.data[i]->Hostname.attempts));
			else
				Q_strcat(dstr, sizeof(dstr), "^1Unknown");
		}
		else {
			Q_strcat(dstr, sizeof(dstr), "^2");
			Q_strcat(dstr, sizeof(dstr), IPList.data[i]->Hostname.host);
		}

		DispContiguous(ent, dstr);

		c++;

		if(c == 25)
			break;
	}
	DispContiguous(ent, va("^3--------------------------------------------------------------\n^3Showing ^2%i^3 to ^2%i^3 of ^2%i^3 IPs.", ofs + 1, (ofs + 25 > IPList.count) ? ofs + 25 : IPList.count, IPList.count));
	DispContiguous(ent, NULL);
}

cmdEntry_t IPCommandEntries[] = {
	{"PlayerIPs", "^3View all players and their ips.", Cmd_PlayerIPs_f, 0, qtrue, 1, 0, 0},
	{"ListIPs", "^3View all IPs that have connected since the last restart.", Cmd_ListIPs_f, 0, qtrue, 1, 0, 0},
	{"FindIP", "^3Find players who have used an IP, or search players by ip names.\nOnly works for the last 10 names on IPs that have connected since the last restart.", Cmd_FindIP_f, 0, qtrue, 1, 0, 0},
	{NULL},

};
