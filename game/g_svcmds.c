// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

//RoboPhred
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"
#include "Lmd_Bans.h"
#include "Lmd_IPs.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Professions.h"
#include "Lmd_Checksum.h"
#include "Lmd_EntityCore.h"


//RoboPhred
gentity_t *ClientFromArg (gentity_t *to, int argNum);

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	gentity_t	*ent;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	ent = ClientFromArg( NULL, 1 );
	if ( !ent ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( ent, str );
}
/*
#define MAPSIZEX 40
#define MAPSIZEY 20
#define MAPFRACX (WORLD_SIZE / MAPSIZEX)
#define MAPFRACY (WORLD_SIZE / MAPSIZEY)
void Svcmd_DispMap_f(void) 
{

int i, xo,yo;
gentity_t *pEnt;
char map[MAPSIZEY][MAPSIZEX + 1];
memset(map,'-',sizeof(map));
map[MAPSIZEY-1][MAPSIZEX] = '\0'; //Just in case

for (i = 0; i < MAX_CLIENTS; i++) {
pEnt = &g_entities[i];
if (!pEnt->client || pEnt->client->pers.connected != CON_CONNECTED ) {
continue;
}

xo = (int)((pEnt->r.currentOrigin[0] 
- MIN_WORLD_COORD)
/ MAPFRACX);
yo = (int)((pEnt->r.currentOrigin[1] 
- MIN_WORLD_COORD)
/ MAPFRACY);
map[yo][xo] = 'X';
//Com_Printf("%3i %3i\n", xo,yo);

}
for (i = 0; i < MAPSIZEY; i++) {
map[i][MAPSIZEX] = '\0';
Com_Printf("%s\n", map[i]);
}
}
*/

//qboolean deleteNick(char *name);

void Svcmd_DeleteNick_f (void){
	char arg[MAX_TOKEN_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	Account_t *acc;
	if(trap_Argc() < 2) {
		Com_Printf("Usage:\ndeletenick <name>\n");
		return;
	}
	acc = Accounts_GetById(atoi(arg));
	if(!acc)
		acc = Accounts_GetByUsername(arg);
	if(!acc){
		Com_Printf("No such nick name registered.\n");
		return;
	}
	Accounts_Delete(acc);
	Com_Printf("Account %s is now deleted.\n", arg);
}

//RoboPhred
extern vmCvar_t g_log;
extern vmCvar_t g_logSync;
void Svcmd_CopyLog_f(){
	if(trap_Argc() < 2) {
		Com_Printf("Usage:\ncopylog <file name>\n");
		return;
	}
	if(!level.logFile)
	{
		Com_Printf("Log file not enabled.\n");
		return;
	}

	char arg[MAX_STRING_CHARS + 1];
	trap_Argv(1, arg, sizeof(arg));

	trap_FS_FCloseFile(level.logFile);

	fileHandle_t newFile, logFile;
	trap_FS_FOpenFile(arg, &newFile, FS_WRITE);
	int len = trap_FS_FOpenFile(g_log.string, &logFile, FS_READ);
	int i;
	for(i = 0;i<len;i += MAX_STRING_CHARS)
	{
		memset(arg, 0, sizeof(arg));
		trap_FS_Read(arg, MAX_STRING_CHARS, logFile);
		trap_FS_Write(arg, strlen(arg), newFile);
	}
	trap_FS_FCloseFile(logFile);
	trap_FS_FCloseFile(newFile);

	if ( g_logSync.integer ) {
		trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
	} else {
		trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
	}
};

void       Cmd_Give_f             (gentity_t  *cmdent, int   baseArg);
void       Cmd_Strip_f            (gentity_t  *cmdent, int   iArg);
void       Cmd_talkingBot_f       (void);
void       Cmd_NPC_f              (gentity_t  *ent);
void       Cmd_botStatus_f        (void);
void       Cmd_Items_f            (gentity_t  *ent,    int   iArg);
void       Cmd_KillOther_f        (gentity_t  *ent,    int   iArg);
qboolean   ThereIsAKing           (void);
gentity_t *GetKing                (void);
void       Cmd_Scale_f            (gentity_t  *ent,    int   iArg);
void       Cmd_ToggleSPF_f        (gentity_t  *ent,    int   iArg);
void       Cmd_Jail_f             (gentity_t  *ent,    int   iArg);
int        ClientNumberFromString ( gentity_t *to,     char *s );
void       Cmd_CallVote_f         (gentity_t  *ent );
void       Cmd_Playerlist_f       (gentity_t  *ent,    int   iArg);
void       listAdmins              (gentity_t  *ent);
void       clearAccounts          (void);
//void       Cmd_ClearLevelSelect_f (void); //RoboPhred: replaced with clearSkills
//void       checkLevelSelect       (void);
void       clearLevels            (void);
void       clearCash              (void);
void       clearScore             (void);
void       clearTime              (void);
void       HiScore                (gentity_t  *ent, int field);
//RoboPhred: replaced by Cmd_AccountInfo_f
//void       GetStats               (gentity_t  *ent, char *who);
//int        saveNicks              (void);
void       ClearMap               (void);
void       Cmd_Position_f         (gentity_t *ent, int iArg);
void       Cmd_Announce_f         (gentity_t *ent, int iArg);
void       make_money_stash       (void);
int        thereIsAMoneyStash     (void);
void       Cmd_Trace_f            (gentity_t *ent, int iArg);

//RoboPhred
void clearSkills(void);
void Accounts_SaveAll(qboolean full);
void Factions_Save(qboolean full);
void Cmd_AccountEdit_f(gentity_t *ent, int iArg);
void Cmd_AccountInfo_f(gentity_t *ent, int iArg);
void Cmd_GrantAdmin_f (gentity_t *ent, int iArg);
void Cmd_GrantTempAdmin_f(gentity_t *ent, int iArg);
void Cmd_AdminInfo_f (gentity_t *ent, int iArg);

void Cmd_PlayerIPs_f(gentity_t *ent, int iArg);
void Cmd_BanIP_f(gentity_t *ent, int iArg);
void Cmd_BanHost_f(gentity_t *ent, int iArg);
void Cmd_RemoveBan_f(gentity_t *ent, int iArg);
void Cmd_ListBans_f(gentity_t *ent, int iArg);
void Cmd_ListIPs_f(gentity_t *ent, int iArg);
void Cmd_FindIP_f(gentity_t *ent, int iArg);
void resetStash();
extern gentity_t *g_bestKing;
extern int        g_bestKingScore;
gentity_t *ClientFromArg (gentity_t *to, int argNum);
#ifdef LMD_EXPERIMENTAL
void GenerateCvarDocs();
void GenerateInvDocs();
void Cmd_Entityinfo_t(gentity_t *ent, int iArg);
#endif
void Accounts_ClearSeccode(Account_t *acc);
void Inventory_ClearAccounts();
void Accounts_Custom_ClearAll();
void Lmd_IPs_SetPlayerIP(gclient_t *client, IP_t ip);
void Cmd_Entitylist_f(gentity_t *ent, int iArg);
/*
=================
ConsoleCommand

=================
*/


qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );
#ifdef LMD_EXPERIMENTAL
	//RoboPhred
	if(Q_stricmp(cmd, "generatecmddocs") == 0) {
		Com_Printf("TODO: generatecmddocs\n");
		return qtrue;
	}
	if(Q_stricmp(cmd, "generatecvardocs") == 0) {
		GenerateCvarDocs();
		return qtrue;
	}
	if(Q_stricmp(cmd, "generateentdocs") == 0) {
		Cmd_Entityinfo_t(NULL, 0);
		return qtrue;
	}
	else if(Q_stricmp(cmd, "generateinvdocs") == 0){
		GenerateInvDocs();
		return qtrue;
	}

	else if(Q_stricmp(cmd, "enttest") == 0) {
		Disp(NULL, va("First normal ent: %s\n"
			"First logical ent: %s", GetEnt(0)->classname, GetEnt(MAX_GENTITIES)->classname));
		gentity_t *check = NULL;
		while(check = IterateEnts(check)) {
			Disp(NULL, va("Ent %i: %s", check->s.number, check->classname));
		}
	}
#endif
	if(Q_stricmp(cmd, "resetstash") == 0) {
		resetStash();
		Com_Printf("Stash reset\n");
		return qtrue;
	}
	if(Q_stricmp(cmd, "clearseccode") == 0) {
		char arg[MAX_STRING_CHARS];
		Account_t *acc;
		if (trap_Argc() < 2) {
			return qtrue;
		}
		trap_Argv(1, arg, sizeof(arg));
		acc = Accounts_GetById(atoi(arg));
		if(!acc)
			acc = Accounts_GetByUsername(arg);
		if(!acc){
			Com_Printf("Cannot find account.\n");
			return qtrue;
		}
		Accounts_ClearSeccode(acc);
		Com_Printf("Security code cleared.\n");
		return qtrue;
	}
	if(Q_stricmp(cmd, "copylog") == 0)
	{
		Svcmd_CopyLog_f();
		Com_Printf("Log copied.\n");
		return qtrue;
	}
	//Lugormod
	if ((Q_stricmp (cmd, "chksum") == 0)){
		if (trap_Argc() < 2) {
			return qtrue;
		}
		char arg[MAX_STRING_CHARS];
		trap_Argv(1, arg, sizeof(arg));
		Com_Printf("%06X\n", Checksum(arg));
		return qtrue;
	}
	if ((Q_stricmp (cmd, "chpasswd") == 0)){
		char arg[MAX_STRING_CHARS];
		char *passwd;
		Account_t *acc;
		if (trap_Argc() < 3) {
			return qtrue;
		}
		passwd = ConcatArgs(2);     
		trap_Argv(1, arg, sizeof(arg));
		acc = Accounts_GetById(atoi(arg));
		if(!acc)
			acc = Accounts_GetByUsername(arg);
		if(!acc){
			Com_Printf("Cannot find account.\n");
			return qtrue;
		}
		Accounts_SetPassword(acc, passwd);
		Com_Printf("Password changed.\n");
		return qtrue;
	}
	if ((Q_stricmp (cmd, "ambient") == 0)){
		if (trap_Argc() < 2) {
			return qtrue;
		}
		char arg[MAX_STRING_CHARS];
		trap_Argv(1, arg, sizeof(arg));
		int i = atoi(arg);
		return qtrue;
	}

	if ((Q_stricmp (cmd, "profpoints") == 0)) {
		if (trap_Argc() < 2) {
			return qtrue;
		}
		char arg[MAX_STRING_CHARS];
		trap_Argv(1, arg, sizeof(arg));
		int prof = atoi(arg);
		if(prof < 0 || prof >= NUM_PROFESSIONS)
			return qtrue;
		trap_Argv(2, arg, sizeof(arg));
		int i = atoi(arg);
		Com_Printf("Points for level %i: %i.\nCost: CR %i\n", i, Professions_TotalSkillPoints(prof, i),
			(i * (i+1)) * 50);
		return qtrue;
	}
	if((Q_stricmp (cmd, "trace") == 0)) {
		Cmd_Trace_f(NULL, 0);
		return qtrue;
	}
	if(Q_stricmp(cmd, "accountedit") == 0){
		Cmd_AccountEdit_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "makestash") == 0)) {
		if (thereIsAMoneyStash() == -1) {
			make_money_stash();
		}
		return qtrue;
	}
	if ((Q_stricmp (cmd, "location") == 0)) {
		Cmd_Position_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "uptime") == 0)){
		Com_Printf("uptime: %i:%02i:%02i\n",
			level.time /3600000,
			(level.time/60000) % 60,
			(level.time/1000) % 60);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearmap") == 0)){
		ClearMap();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "accountinfo") == 0)){
		char		arg[MAX_TOKEN_CHARS];

		if (trap_Argc() < 2) {
			return qtrue;
		}
		trap_Argv( 1, arg, sizeof( arg ) );        
		//RoboPhred
		//GetStats(NULL, arg);
		Cmd_AccountInfo_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "saveaccounts") == 0)){
		Accounts_SaveAll(qtrue);
		Factions_Save(qtrue);
		Com_Printf("^2Accounts and factions saved.\n");
		return qtrue;
	}
	if ((Q_stricmp (cmd, "hiscore") == 0)){
		HiScore(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "hitime") == 0)){
		HiScore(NULL, 1);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "hicredits") == 0)){
		HiScore(NULL, 2);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "hilevel") == 0)){
		HiScore(NULL, 3);
		return qtrue;
	}
	if(Q_stricmp(cmd, "hikills") == 0) {
		HiScore(NULL, 4);
		return qtrue;
	}
	if(Q_stricmp(cmd, "histashes") == 0) {
		HiScore(NULL, 5);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearscore") == 0)){
		clearScore();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearlevels") == 0)){
		clearLevels();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "cleartime") == 0)){
		clearTime();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearcredits") == 0)){
		clearCash();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearaccounts") == 0)){
		clearAccounts();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "clearskills") == 0)){
		//Cmd_ClearLevelSelect_f();
		clearSkills();
		return qtrue;
	}
	if(Q_stricmp(cmd, "clearcustomskills") == 0) {
		Accounts_Custom_ClearAll();
	}
	if ((Q_stricmp (cmd, "clearinventory") == 0)) {
		Inventory_ClearAccounts();
		return qtrue;
	}
	//RoboPhred: noone will use this anyway
	/*
	if ((Q_stricmp (cmd, "checklevelselect") == 0)){
	checkLevelSelect();
	return qtrue;
	}
	*/
	if ((Q_stricmp (cmd, "grantadmin") == 0)){
		Cmd_GrantAdmin_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "removeadmin") == 0)){
		Cmd_GrantAdmin_f(NULL, 1);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "granttempadmin") == 0)){
		Cmd_GrantTempAdmin_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "removetempadmin") == 0)){
		Cmd_GrantTempAdmin_f(NULL, 1);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "deletenick") == 0)){
		Svcmd_DeleteNick_f();
		return qtrue;
	}
	if ((Q_stricmp (cmd, "listadmins") == 0)){
		listAdmins(NULL);
		return qtrue;
	}
	//RoboPhred
	if ((Q_stricmp (cmd, "playerlist") == 0)){
		Cmd_Playerlist_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "playerinfo") == 0)){
		//RoboPhred
		//Cmd_Playerlist_f(NULL, 0);
		Disp(NULL, "^3Use /playerlist\n");
		return qtrue;
	}
	if ((Q_stricmp (cmd, "callvote") == 0)){
		Cmd_CallVote_f(NULL);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "jail") == 0)){
		if (trap_Argc() < 2) {
			return qfalse;
		}
		Cmd_Jail_f(NULL, 0);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "shutup") == 0)){
		if (trap_Argc() < 2) {
			return qfalse;
		}
		Cmd_ToggleSPF_f(NULL, SPF_SHUTUP);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "nocaps") == 0)){
		if (trap_Argc() < 2) {
			return qfalse;
		}
		Cmd_ToggleSPF_f(NULL, SPF_UNCAP);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "novote") == 0)){
		if (trap_Argc() < 2) {
			return qfalse;
		}
		Cmd_ToggleSPF_f(NULL, SPF_NOCALL);
		return qtrue;
	}
	if ((Q_stricmp (cmd, "scale") == 0)){
		Cmd_Scale_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "listinvisible") == 0) {
		int i;
		gentity_t *check;

		for(i = 0; i < MAX_CLIENTS; i++){
			check = &g_entities[i];
			if(!check->inuse || check->client->pers.connected != CON_CONNECTED || check->client->sess.sessionTeam == TEAM_SPECTATOR)
				continue;
			if((check->client->ps.eFlags & EF_NODRAW || check->s.eFlags & EF_NODRAW) && !check->client->ps.m_iVehicleNum){
				Com_Printf("%2i %s", i, check->client->pers.netname);
			}       
		}
		return qtrue;
	}

	if ((Q_stricmp (cmd, "king") == 0)){
		if (g_gametype.integer == GT_JEDIMASTER ||
			!(g_privateDuel.integer & PD_KING) ||
			!ThereIsAKing()) {
				Com_Printf("There is no King.\n");
				return qtrue;
		}
		gentity_t *kingent = GetKing();

		//Com_Printf("%s is the King, and has won %i duels.", kingent->client->pers.netname, kingent->client->ps.persistant[PERS_KING_SCORE]);
		Com_Printf("%s is the King, and has won %i duels.", kingent->client->pers.netname, kingent->client->pers.Lmd.kingScore);
		if (g_bestKing 
			&& g_bestKing->client) {
				Com_Printf(" %s is the best King with %i duels won.", g_bestKing->client->pers.netname, g_bestKingScore);
		}
		Com_Printf("\n");
		return qtrue;
	}
	if ( Q_stricmp (cmd, "sayasbot") == 0 ) {
		Cmd_talkingBot_f();
		return qtrue;
	}
	if ( Q_stricmp (cmd, "admins") == 0 ) {
		Cmd_AdminInfo_f(NULL, 0);
		return qtrue;
	}

	if (Q_stricmp (cmd, "giveother") == 0)
	{ 
		Cmd_Give_f (NULL, 1);
		return qtrue;
	}
	if (Q_stricmp (cmd, "strip") == 0)
	{
		Cmd_Strip_f (NULL, 0);
		return qtrue;
	}

	if (Q_stricmp (cmd, "npc") == 0)
	{
		Cmd_NPC_f( NULL );
		return qtrue;
	}

	if (Q_stricmp (cmd, "botstatus") == 0)
	{
		Cmd_botStatus_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "getname") == 0)
	{
		char		arg[MAX_TOKEN_CHARS];
		int             client;
		gentity_t      *ent;
		char            numerical[4 * MAX_NAME_LENGTH + 1];
		int             i = 0;
		unsigned char            ch;

		if (trap_Argc() != 2) {
			return qtrue;
		}
		trap_Argv( 1, arg, sizeof( arg ) );        
		client = ClientNumberFromString( NULL, arg );
		if ( client < 0 || client >= level.maxclients ) {
			return qtrue;
		}

		ent = &g_entities[client];
		if ( !ent || !ent->inuse || !ent->client ) {
			return qtrue;
		}
		Com_Printf("%s\n",ent->client->pers.netname);
		numerical[0] = 0;
		while (ch = ent->client->pers.netname[i++]) {
			Q_strcat(numerical, sizeof(numerical), va("%i ", ch));
		}
		Com_Printf("%s\n", numerical);
		return qtrue;
	}
	if (Q_stricmp (cmd, "getstrippedname") == 0)
	{
		char            name[MAX_TOKEN_CHARS];
		char		arg[MAX_TOKEN_CHARS];
		int             client;
		gentity_t      *ent;


		if (trap_Argc() != 2) {
			return qtrue;
		}
		trap_Argv( 1, arg, sizeof( arg ) );        
		client = ClientNumberFromString( NULL, arg );
		if ( client < 0 || client >= level.maxclients ) {
			return qtrue;
		}

		ent = &g_entities[client];
		if ( !ent || !ent->inuse || !ent->client ) {
			return qtrue;
		}
		Q_strncpyz(name,ent->client->pers.netname,sizeof(name) );
		Com_Printf("%s\n",name);
		Q_StripTags(name);
		Com_Printf("%s\n",name);


		return qtrue;
	}
	if ( Q_stricmp (cmd, "killother") == 0 ) {
		if (trap_Argc() < 2) {
			return qfalse;
		}
		Cmd_KillOther_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		int client;
		gentity_t *other;
		char *msg;
		char arg[MAX_TOKEN_CHARS];
		if (trap_Argc() < 3) {
			return qtrue;
		}
		trap_Argv( 1, arg, sizeof( arg ) );        
		client = ClientNumberFromString( NULL, arg );
		if ( client < 0 || client >= level.maxclients ) {
			Com_Printf("No such client.\n");
			return qtrue;
		}

		other = &g_entities[client];
		if ( !other || !other->inuse || !other->client ) {
			Com_Printf("No such client.\n");
			return qtrue;
		}
		msg = ConcatArgs(2);

		if (other 
			&& other->client) {
				trap_SendServerCommand( client, 
					va("chat \"[%s%c%c]: %c%c%s\"", 
					g_nameForServer.string,
					Q_COLOR_ESCAPE, 
					COLOR_WHITE, 
					Q_COLOR_ESCAPE, 
					COLOR_MAGENTA, 
					msg));

		}
		return qtrue;
	}
	if (Q_stricmp (cmd, "say_admins") == 0) {
		int j, n = 0;
		gentity_t *other;
		char *msg;

		if (g_dedicated.integer) {
			n = 1;
		}
		msg = ConcatArgs(n);
		for (j = 0; j < level.maxclients; j++) {
			other = &g_entities[j];
			if (!other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			if(Auths_PlayerHasAuthFlag(other, AUTH_ADMINCHAT)) {
				trap_SendServerCommand( j, va("chat \"<%s%c%c>: %c%c%s\"", g_nameForServer.string,
						Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_YELLOW, msg));
			}
		}
		return qtrue;
	}
	if ( Q_stricmp (cmd, "grantadmin") == 0 ) {
		Cmd_GrantAdmin_f(NULL, 0);
		return qtrue;
	}
	if ( Q_stricmp (cmd, "crash") == 0 ) {
		gentity_t *evilguy;
		evilguy = NULL;
		evilguy->client->Lmd.training.dispTime++;
		return qtrue;
	}
	if ( Q_stricmp (cmd, "crash2") == 0 ) {
		int yarg = 0;
		int blarg = 3 / yarg;
		return qtrue;
	}
	if ( Q_stricmp (cmd, "error") == 0 ) {
		G_Error("Evil error!!");
		return qtrue;
	}
	if ( Q_stricmp (cmd, "announce") == 0 ) {
		Cmd_Announce_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "saveit") == 0)	{
		char arg[MAX_TOKEN_CHARS];
		if (trap_Argc() < 2) {
			return qtrue;
		}
		trap_Argv( 1, arg, sizeof( arg ) );
		SaveEntitiesData(arg);
		Com_Printf("^2Entities saved.\n");
		return qtrue;
	}
	if (Q_stricmp (cmd, "loadit") == 0){
		qboolean nodefaults = qfalse;
		char arg[MAX_TOKEN_CHARS];
		//RoboPhred
		if(trap_Argc() < 2)
			return qtrue;
		trap_Argv(2, arg, sizeof(arg));        
		if(Q_stricmp(arg, "nodefaults") == 0)
			nodefaults = qtrue;
		trap_Argv(1, arg, sizeof(arg));        
		LoadEntitiesData(arg, nodefaults);
		Com_Printf("^2Entities loaded\n");
		return qtrue;
	}
	/* Stupid cmd
	if ( Q_stricmp (cmd, "dispmap") == 0 ) {
	Svcmd_DispMap_f();
	return qtrue;
	}
	*/       

	//end Lugormod

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Cmd_Entitylist_f(NULL, 0);
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "botlist") == 0) {
		Svcmd_BotList_f();
		return qtrue;
	}

	/*	if (Q_stricmp (cmd, "abort_podium") == 0) {
	Svcmd_AbortPodium_f();
	return qtrue;
	}
	*/

	//RoboPhred
	if (Q_stricmp (cmd, "playerips") == 0) {
		Cmd_PlayerIPs_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "banip") == 0) {
		Cmd_BanIP_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "banhost") == 0) {
		Cmd_BanHost_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "listbans") == 0) {
		Cmd_ListBans_f(NULL, 0);
		return qtrue;
	}
	if(Q_stricmp(cmd, "listips") == 0) {
		Cmd_ListIPs_f(NULL, 0);
		return qtrue;
	}
	if(Q_stricmp(cmd, "findip") == 0) {
		Cmd_FindIP_f(NULL, 0);
		return qtrue;
	}
	if (Q_stricmp (cmd, "removeban") == 0) {
		Cmd_RemoveBan_f(NULL, 0);
		return qtrue;
	}
	if(Q_stricmp(cmd, "banclient") == 0){
		gentity_t *targ = ClientFromArg(NULL, 1);
		if(!targ || !targ->client->sess.Lmd.ip || targ->client->sess.Lmd.ip[0] == 0)
			return qtrue;
		Bans_AddIP(targ->client->sess.Lmd.ip, 0, "");
		//guess its best to use targ-g_entities, in case the client isnt fully joined and the damn game resets targ->s.number again
		trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i\n", targ-g_entities));
		return qtrue;
	}
	if(Q_stricmp(cmd, "fakeip") == 0) {
		gentity_t *targ = ClientFromArg(NULL, 1);
		if(!targ)
			return qtrue;
		char *ipstr = ConcatArgs(2);
		IP_t ip;
		if(Lmd_IPs_ParseIP(ipstr, ip))
			Lmd_IPs_SetPlayerIP(targ->client, ip);
		return qtrue;
	}
	if(Q_stricmp(cmd, "settimescale") == 0) {
		trap_Cvar_Set("timescale", ConcatArgs(1));
		return qtrue;
	}

	if (Q_stricmp (cmd, "say") == 0) {
		int j, n = 0;
		gentity_t *other;
		char *msg;

		if (g_dedicated.integer) {
			n = 1;
		}
		msg = ConcatArgs(n);

		for (j = 0; j < level.maxclients; j++) {
			other = &g_entities[j];
			if (other 
				&& other->client) {
					trap_SendServerCommand( j, 
						va("chat \"%s%c%c: %c%c%s\"",
						g_nameForServer.string,
						Q_COLOR_ESCAPE, 
						COLOR_WHITE, 
						Q_COLOR_ESCAPE, 
						COLOR_GREEN, 
						msg));

					//trap_SendServerCommand( j, va("print \"[server]:^3 %s\n\"", ConcatArgs(n) ) );
			}
		}
		//Com_Printf("info: server: %s\n", msg);
		return qtrue;
	}
	if (g_dedicated.integer) {        
		Com_Printf("No such svcmd\n");
	}

	return qfalse;
}

