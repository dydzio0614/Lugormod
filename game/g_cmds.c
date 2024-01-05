// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Stats.h"
#include "Lmd_Accounts_Friends.h"

#include "Lmd_Commands_Auths.h"

#include "../ui/menudef.h"			// for the voice chats

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

#include "../namespace_begin.h"
void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );
#include "../namespace_end.h"

//Lugormod why not?
qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);
//extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void G_SoundAtLoc( vec3_t loc, int channel, int soundIndex );
//extern void ChangeWeapon( gentity_t *ent, int newWeapon );

void Cmd_NPC_f( gentity_t *ent );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);

//Lugormod
void       BecomeCommoner  (gentity_t *ent);
void       RevertKing      (gentity_t *ent);

//RoboPhred
extern vmCvar_t lmd_chatDisable;
/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;

	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {//Ufo: fake the ping
			ping = cl->pers.Lmd.fakePing ? cl->pers.Lmd.fakePing + Q_irand(0, (int)(cl->pers.Lmd.fakePing / 10)) : cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		if (!Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)){

			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
			return qfalse;
		}

	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( (unsigned char) *in++ );
	}

	*out = 0;
}

/*
==================
SanitizeString3

Remove case and all but letters
==================
*/
void SanitizeString3( char *in, char *out ) {
	while ( *in ) {
		if (!((*in >= 'a' && *in <= 'z') || 
			(*in >= 'A' && *in <= 'Z'))) {
				in++;
				continue;
		}

		*out++ = tolower( (unsigned char) *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
//RoboPhred
int PlayerFromPartialName(char *string){
	int plr = -1, i = 0;
	char Name[MAX_STRING_CHARS];
	gclient_t	*cl;
	Q_strlwr(string);
	SanitizeString3(string, string);

	for(i = 0;i<MAX_CLIENTS;i++){
		cl = &level.clients[i];
		if(!cl) //hmm... need to find a way to tell if a player is active or not? or does it not matter
			continue;
		//let unconnected players still count.  we might be trying to kick em or somthing.
		if(!g_entities[i].inuse)
			continue; //not even trying to connect.
		SanitizeString3(cl->pers.netname, Name);
		Q_strlwr(Name);
		if(strstr(Name, string)){
			if(plr == -1)
				plr = i; //have a match, remember it
			else
				return -2; //2 players found, return -2 to tell it too many players by that string
		}
	}
	return plr;
}

int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Disp(to,va("^1Bad client slot: ^3%i", idnum));

			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			Disp(to,va("^3Client %i is not active", idnum));


			return -1;
		}
		return idnum;
	}

	// check for a name match
	/*partial name will grab this
	SanitizeString3( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
	if ( cl->pers.connected != CON_CONNECTED ) {
	continue;
	}
	SanitizeString3( cl->pers.netname, n2 );
	if ( !Q_stricmp( n2, s2 ) ) {
	return idnum;
	}
	}
	*/
	//Euka
	if((idnum = PlayerFromPartialName(s)) > -1){
		return idnum;
	}
	else if(idnum == -2 && to)
		Disp(to, "^3Too many players found matching that string");

	if(to){
		Disp(to, va("^3User ^7%s^3 is not on the server", s));
	}
	return -1;
}



/*
==================
Cmd_Give_f

Give items to a client
==================
*/

//RoboPhred
extern qboolean disablesenabled;
void Cmd_Give_f (gentity_t *cmdent, int baseArg)
{
	char		name[MAX_TOKEN_CHARS];
	gentity_t	*ent;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];


	if (baseArg)
	{
		char otherindex[MAX_TOKEN_CHARS];

		trap_Argv( 1, otherindex, sizeof( otherindex ) );

		if (!otherindex[0])
		{
			Com_Printf("giveother requires that the second argument be a client index number.\n");
			return;
		}

		i = ClientNumberFromString(cmdent, otherindex);

		if (i < 0 || i >= MAX_CLIENTS)
		{
			return;
		}

		ent = &g_entities[i];

		if (!ent->inuse || !ent->client)
		{
			Com_Printf("%i is not an active client\n", i);
			return;
		}
	}
	else
	{
		ent = cmdent;
	}

	trap_Argv( 1+baseArg, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp(name, "items") == 0)
	{
		i = 0;
		while (i < HI_NUM_HOLDABLE)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			i++;
		}
		i = 0;
	}

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	//RoboPhred: disallow this for trueJedi since the weapons are checked and cleared anyway.
	if ((give_all || Q_stricmp(name, "weapons") == 0) && !ent->client->ps.trueJedi)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON+1))  - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	//RoboPhred: disallow this for trueJedi since the weapons are checked and cleared anyway.
	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 && !ent->client->ps.trueJedi)
	{
		trap_Argv( 2+baseArg, arg, sizeof( arg ) );
		int weapon = atoi(arg);
		if (weapon < 1) {
			return;
		}
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << weapon);
		ent->client->ps.saberHolstered = 2;
		ent->client->ps.weapon = weapon;
		ent->client->ps.weaponstate = WEAPON_READY;
		//ent->s.weapon = weapon;
		return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}
	if (Q_stricmp(name, "credits") == 0){
		int myCreds = PlayerAcc_GetCredits(ent);
		if(trap_Argc() == 3+baseArg){
			int num;
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			num = atoi(arg);
			if (num < 0 && num + myCreds < 0) {
				Disp(cmdent, "^3Target does not have enough credits.");
				return;
			}
			PlayerAcc_SetCredits(ent, myCreds + num);
			if(num >= 0)
				trap_SendServerCommand(ent->s.number, va("cp \"^3You received ^2CR %i^3.\"", num));
			else
				trap_SendServerCommand(ent->s.number, va("cp \"^3You lost ^1CR %i^3.\"", -num));

		}
		return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	//stupid way of doing it
	if ( !give_all ) {
		it = BG_FindItem (name);
		/*Lugormod they crash the server
		if (Q_stricmp("item_shield", name) == 0 ||
		Q_stricmp("item_seeker", name) == 0) {
		return;
		}
		*/
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;

		//RoboPhred
		disablesenabled = qfalse;

		G_SpawnItem (it_ent, it);
		//RoboPhred
		if(!it_ent->inuse)
			return;
		FinishSpawningItem(it_ent);

		//RoboPhred
		disablesenabled = qtrue;

		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
/* moved it to g_lugorcmds.c
void Cmd_Noclip_f( gentity_t *ent ) {
char	*msg;

if ( !CheatsOk( ent ) ) {
return;
}

if ( ent->client->noclip ) {
msg = "noclip OFF\n";
} else {
msg = "noclip ON\n";
}
ent->client->noclip = !ent->client->noclip;

trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}

*/
/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_TeamTask_f

From TA.
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}

	//No kill when frozen.
	if(ent->client->Lmd.flags & SNF_FREEZE)
		return;

	if ((g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent->s.number, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
	if (g_gametype.integer == GT_REBORN && 
		ent->client->sess.sessionTeam == TEAM_BLUE) {
			SetTeamQuick(ent, TEAM_RED, qfalse);
			trap_SendServerCommand(ent->s.number, "cp \"^1You are reborn\"");
	}

}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];

		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}


/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if (g_gametype.integer == GT_SIEGE ||
		g_gametype.integer == GT_BATTLE_GROUND) //Lugormod
	{ //don't announce these things in siege
		return;
	}

	if ( client->sess.sessionTeam == TEAM_RED ) {
		//Lugormod these were all cp:d b4 but I don't like that.
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
			currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
		client - &level.clients[0],
		TeamName ( oldTeam ),
		TeamName ( client->sess.sessionTeam ) );
}

qboolean G_PowerDuelCheckFail(gentity_t *ent)
{
	int			loners = 0;
	int			doubles = 0;

	if (!ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE)
	{
		return qtrue;
	}

	G_PowerDuelCount(&loners, &doubles, qfalse);

	if (ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
	{
		return qtrue;
	}

	if (ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
SetTeam
=================
*/
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
void SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;
	//
	// see what change is requested
	//

	client = ent->client;
	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
		//RoboPhred
		/*
		} else if ( !Q_stricmp( s, "jailed" ) ) {
		team = TEAM_JAILED;
		//specState = SPECTATOR_NOT;
		*/
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		if ((g_disableSpec.integer & 4) && !Auths_PlayerHasAuthFlag(ent, 4) && (g_gametype.integer == GT_FFA || g_gametype.integer == GT_TEAM)) {
			specState = SPECTATOR_FOLLOW;
			specClient = -1;
		} else {
			specState = SPECTATOR_FREE;
		}

	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		//specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
			team = TEAM_BLUE;
			}
			else
			{
			team = TEAM_RED;
			}
			}
			else
			{
			*/
			team = PickTeam( clientNum );
			//}
		}

		if ( g_teamForceBalance.integer && !g_jediVmerc.integer 
			&& !gameMode(GM_ALLWEAPONS)) {
				int		counts[TEAM_NUM_TEAMS];

				counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
				counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

				// We allow a spread of two
				if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
					//For now, don't do this. The legalize function will set powers properly now.
					/*
					if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
					{
					trap_SendServerCommand( ent->client->ps.clientNum, 
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED_SWITCH")) );
					}
					else
					*/
					{
						trap_SendServerCommand( ent->client->ps.clientNum, 
							va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
					}
					return; // ignore the request
				}
				if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
					//For now, don't do this. The legalize function will set powers properly now.
					/*
					if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
					{
					trap_SendServerCommand( ent->client->ps.clientNum, 
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE_SWITCH")) );
					}
					else
					*/
					{
						trap_SendServerCommand( ent->client->ps.clientNum, 
							va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
					}
					return; // ignore the request
				}

				// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
		if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
		{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBELIGHT")) );
		return;
		}
		if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
		{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEDARK")) );
		return;
		}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	if (g_gametype.integer == GT_SIEGE
		|| g_gametype.integer == GT_BATTLE_GROUND //Lugormod
		)
	{
		if (client->tempSpectate >= level.time &&
			team == TEAM_SPECTATOR)
		{ //sorry, can't do that.
			return;
		}

		client->sess.siegeDesiredTeam = team;
		//oh well, just let them go.
		/*
		if (team != TEAM_SPECTATOR)
		{ //can't switch to anything in siege unless you want to switch to being a fulltime spectator
		//fill them in on their objectives for this team now
		trap_SendServerCommand(ent-g_entities, va("sb %i", client->sess.siegeDesiredTeam));

		trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time the round begins.\n\"") );
		return;
		}
		*/
		if (client->sess.sessionTeam != TEAM_SPECTATOR &&
			team != TEAM_SPECTATOR)
		{ //not a spectator now, and not switching to spec, so you have to wait til you die.
			//trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time you respawn.\n\"") );
			qboolean doBegin;
			if (ent->client->tempSpectate >= level.time)
			{
				doBegin = qfalse;
			}
			else
			{
				doBegin = qtrue;
			}

			if (doBegin)
			{
				// Kill them so they automatically respawn in the team they wanted.
				if (ent->health > 0)
				{
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE ); 
				}
			}

			if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
			{
				SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qfalse);
			}

			return;
		}
	}

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_DUEL)
		&& level.numNonSpectatorClients >= 2 )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( (g_gametype.integer == GT_POWERDUEL)
		&& (level.numPlayingClients >= 3 || G_PowerDuelCheckFail(ent)) )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer )
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	//If it's siege then show the mission briefing for the team you just joined.
	//	if (g_gametype.integer == GT_SIEGE && team != TEAM_SPECTATOR)
	//	{
	//		trap_SendServerCommand(clientNum, va("sb %i", team));
	//	}

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	//if (oldTeam != TEAM_JAILED && team != TEAM_JAILED) {
	client->pers.teamState.state = TEAM_BEGIN;
	//}
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;

	} else {
		//ent->client->sess.credits = 0;
		ent->client->pers.Lmd.persistantFlags &= ~SPF_ISKING;
	}

	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		if ( (g_gametype.integer != GT_DUEL) || (oldTeam != TEAM_SPECTATOR) )	{//so you don't get dropped to the bottom of the queue for changing skins, etc.
			client->sess.spectatorTime = level.time;
		}
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (oldTeam != TEAM_SPECTATOR)
	{
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	if (!g_preventTeamBegin 
		/*&& oldTeam != TEAM_JAILED 
		&& team != TEAM_JAILED*/)
	{
		ClientBegin( clientNum, qfalse );
	}

}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	//Lugormod 
	if ((g_disableSpec.integer & 4)	&& !Auths_PlayerHasAuthFlag(ent, AUTH_ALLOWSPEC) && (g_gametype.integer == GT_FFA || g_gametype.integer == GT_TEAM)) {
		//ent->client->sess.spectatorState = SPECTATOR_SCOREBOARD;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		ent->client->sess.spectatorClient = -1;
	} else {
		ent->client->sess.spectatorState = SPECTATOR_FREE;        
		ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	}
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	ent->client->ps.m_iVehicleNum = 0;
	ent->client->ps.viewangles[ROLL] = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = 0;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}
	if (g_gametype.integer == GT_REBORN) {
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		Disp(ent, G_GetStringEdString("MP_SVGAME", "NOSWITCH"));
		return;
	}

	if (gEscaping)
	{
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_DUEL
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
			//disallow changing teams
			//trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Duel\n\"" );
			Disp(ent, "Cannot switch teams in Duel");
			return;
			//FIXME: why should this be a loss???
			//ent->client->sess.losses++;
	}

	if (g_gametype.integer == GT_POWERDUEL)
	{ //don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		//trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		Disp(ent, "Cannot switch teams in Power Duel");
		return;
	}

	//RoboPhred
	if (ent->client->pers.Lmd.jailTime > level.time) {
		//if (ent->client->sess.sessionTeam == TEAM_JAILED){
		Disp(ent, "Cannot switch teams when jailed.");
		return;
	}
	//Lugormod for now don't allow team switch in private duels
	//RoboPhred: spectating a dueler isnt a duel
	if (duelInProgress(&ent->client->ps) && ent->s.number == ent->client->ps.clientNum) {
		//if (duelInProgress(&ent->client->ps)) {
		Disp(ent, "Cannot switch teams in a duel.");
		return;
	}

	//Ufo:
	if (ent->client->Lmd.flags & SNF_FREEZE) {
		Disp(ent, "Cannot switch teams when frozen.");
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	//RoboPhred
	oldTeam = ent->client->sess.sessionTeam;

	SetTeam( ent, s );

	//RoboPhred
	if(ent->client->sess.sessionTeam != oldTeam)
		ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_DuelTeam_f
=================
*/
void Cmd_DuelTeam_f(gentity_t *ent)
{
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if (g_gametype.integer != GT_POWERDUEL)
	{ //don't bother doing anything if this is not power duel
		return;
	}

	/*
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
	trap_SendServerCommand( ent-g_entities, va("print \"You cannot change your duel team unless you are a spectator.\n\""));
	return;
	}
	*/

	if ( trap_Argc() != 2 )
	{ //No arg so tell what team we're currently on.
		oldTeam = ent->client->sess.duelTeam;
		switch ( oldTeam )
		{
		case DUELTEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"None\n\"") );
			break;
		case DUELTEAM_LONE:
			trap_SendServerCommand( ent-g_entities, va("print \"Single\n\"") );
			break;
		case DUELTEAM_DOUBLE:
			trap_SendServerCommand( ent-g_entities, va("print \"Double\n\"") );
			break;
		default:
			break;
		}
		return;
	}

	if ( ent->client->switchDuelTeamTime > level.time )
	{ //debounce for changing
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	oldTeam = ent->client->sess.duelTeam;

	if (!Q_stricmp(s, "free"))
	{
		ent->client->sess.duelTeam = DUELTEAM_FREE;
	}
	else if (!Q_stricmp(s, "single"))
	{
		ent->client->sess.duelTeam = DUELTEAM_LONE;
	}
	else if (!Q_stricmp(s, "double"))
	{
		ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"'%s' not a valid duel team.\n\"", s) );
	}

	if (oldTeam == ent->client->sess.duelTeam)
	{ //didn't actually change, so don't care.
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //ok..die
		int curTeam = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = oldTeam;
		G_Damage(ent, ent, ent, NULL, ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		ent->client->sess.duelTeam = curTeam;
	}
	//reset wins and losses
	ent->client->sess.wins = 0;
	ent->client->sess.losses = 0;

	//get and distribute relevent paramters
	ClientUserinfoChanged( ent->s.number );

	ent->client->switchDuelTeamTime = level.time + 5000;
}

int G_TeamForSiegeClass(const char *clName)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(team);
	siegeClass_t *scl;

	if (!stm)
	{
		return 0;
	}

	while (team <= SIEGETEAM_TEAM2)
	{
		scl = stm->classes[i];

		if (scl && scl->name[0])
		{
			if (!Q_stricmp(clName, scl->name))
			{
				return team;
			}
		}

		i++;
		if (i >= MAX_SIEGE_CLASSES || i >= stm->numClasses)
		{
			if (team == SIEGETEAM_TEAM2)
			{
				break;
			}
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam(team);
			i = 0;
		}
	}

	return 0;
}

void changeSiegeClass (gentity_t *ent, char *className) 
{
	int team = 0;
	int preScore;
	qboolean startedAsSpec = qfalse;

	if ( ent->client->switchClassTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSSWITCH")) );
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		startedAsSpec = qtrue;
	}

	int classnumber = atoi(className);

	if (g_gametype.integer == GT_BATTLE_GROUND &&
		ent->client->sess.sessionTeam > 0 &&
		ent->client->sess.sessionTeam < 3 &&
		classnumber > 0) {
			siegeTeam_t *stm = BG_SiegeFindThemeForTeam(ent->client->sess.sessionTeam);
			if (stm && --classnumber < stm->numClasses) {
				Q_strncpyz(className,stm->classes[classnumber]->name
					,64);//sizeof(className));
				Disp(ent,va("className: %s",className));
			}
	}


	team = G_TeamForSiegeClass(className);

	if (!team)
	{ //not a valid class name
		return;
	}

	if (ent->client->sess.sessionTeam != team)
	{ //try changing it then
		g_preventTeamBegin = qtrue;
		if (team == TEAM_RED)
		{
			SetTeam(ent, "red");
		}
		else if (team == TEAM_BLUE)
		{
			SetTeam(ent, "blue");
		}
		g_preventTeamBegin = qfalse;

		if (ent->client->sess.sessionTeam != team)
		{ //failed, oh well
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR ||
				ent->client->sess.siegeDesiredTeam != team)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSTEAM")) );
				return;
			}
		}
	}

	//preserve 'is score
	preScore = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality(team, className);

	//Set the session data
	strcpy(ent->client->sess.siegeClass, className);

	// get and distribute relevent paramters
	ClientUserinfoChanged( ent->s.number );

	if (ent->client->tempSpectate < level.time)
	{
		// Kill him (makes sure he loses flags, etc)
		if (ent->health > 0 && !startedAsSpec)
		{
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		}

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec)
		{ //respawn them instantly.
			ClientBegin( ent->s.number, qfalse );
		}
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = preScore;

	ent->client->switchClassTime = level.time + 5000;

}

void
displayClassesForTeam (gentity_t *ent) 
{
	if (ent->client->sess.sessionTeam > 2 ||
		ent->client->sess.sessionTeam < 1) {
			return;
	}

	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(ent->client->sess.sessionTeam);
	int numPlayers[10];
	int i,j;
	gentity_t *check;

	memset(numPlayers, 0, sizeof(numPlayers));
	for (i = 0;i < MAX_CLIENTS;i++) {
		check = &g_entities[i];
		if (!check->client || 
			check->client->pers.connected != CON_CONNECTED ||
			check->client->sess.sessionTeam < 1 ||
			check->client->sess.sessionTeam > 2) {
				continue;
		}
		for (j = 0;j < stm->numClasses && j < 9;j++) {
			if (Q_stricmp(check->client->sess.siegeClass, 
				stm->classes[j]->name) == 0) {
					numPlayers[j]++;
					break;
			}
		}
	}
	char dStr[MAX_STRING_CHARS] = "";
	Q_strncpyz(dStr, "Class:                             Players:\n", sizeof(dStr));
	for (i = 0;i < stm->numClasses && i < 9;i++) {
		Q_strcat(dStr, sizeof(dStr), va("%i %-32s %-2i\n",i + 1, 
			stm->classes[i]->name,
			numPlayers[i]));
	}
	Disp(ent,dStr);
}

/*
=================
Cmd_SiegeClass_f
=================
*/
void Cmd_SiegeClass_f( gentity_t *ent, int iArg )
{
	char className[64];

	if (g_gametype.integer != GT_SIEGE
		&& g_gametype.integer != GT_BATTLE_GROUND) //Lugormod
	{ //classes are only valid for this gametype
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (trap_Argc() < 2)
	{
		displayClassesForTeam(ent);
	}
	trap_Argv(1, className, sizeof(className));

	changeSiegeClass(ent, className);
}

/*
=================
Cmd_ForceChanged_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
	//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg, sizeof( arg ) );

		if (arg && arg[0])
		{ //if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride)
{
	char truncSaberName[64];
	int i = 0;

	if (!siegeOverride &&
		(g_gametype.integer == GT_SIEGE ||
		g_gametype.integer == GT_BATTLE_GROUND) && //Lugormod
		ent->client->siegeClass != -1 &&
		(
		bgSiegeClasses[ent->client->siegeClass].saberStance ||
		bgSiegeClasses[ent->client->siegeClass].saber1[0] ||
		bgSiegeClasses[ent->client->siegeClass].saber2[0]
	))
	{ //don't let it be changed if the siege class has forced any saber-related things
		return qfalse;
	}

	while (saberName[i] && i < 64-1)
	{
		truncSaberName[i] = saberName[i];
		i++;
	}
	truncSaberName[i] = 0;

	if ( saberNum == 0 && (Q_stricmp( "none", truncSaberName ) == 0 || Q_stricmp( "remove", truncSaberName ) == 0) )
	{ //can't remove saber 0 like this
		strcpy(truncSaberName, "Kyle");
	}

	//Set the saber with the arg given. If the arg is
	//not a valid sabername defaults will be used.
	WP_SetSaber(ent->s.number, ent->client->saber, saberNum, truncSaberName);

	if (!ent->client->saber[0].model[0])
	{
		assert(0); //should never happen!
		strcpy(ent->client->sess.saberType[0], "none");
	}
	else
	{
		strcpy(ent->client->sess.saberType[0], ent->client->saber[0].name);
	}

	if (!ent->client->saber[1].model[0])
	{
		strcpy(ent->client->sess.saberType[1], "none");
	}
	else
	{
		strcpy(ent->client->sess.saberType[1], ent->client->saber[1].name);
	}

	if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
	{
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	return qtrue;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];


	if(ent->client->pers.Lmd.jailTime > level.time){
		Disp(ent, "^2You cannot spectate while jailed.");
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	//RoboPhred: can't follow admins
	if(g_gametype.integer == GT_FFA && Auths_Inferior(ent, &g_entities[i]))
		//if(g_gametype.integer == GT_FFA && authenticated(ent, -2))
		return;

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
			//WTF???
			ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;

	if(ent->client->pers.Lmd.jailTime > level.time){
		Disp(ent, "^2You cannot spectate while jailed.");
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	//RoboPhred: stop infinite loop cases
	if(clientnum == -1)
		clientnum = ent->s.number;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		//RoboPhred: cant follow admins
		if(Auths_PlayerHasAuthFlag(&g_entities[clientnum], AUTH_UNFOLLOWABLE) && g_gametype.integer == GT_FFA) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}
//let the bots in to the conversation
int BotChatBack(const char *s, gentity_t *ent, gentity_t *other);


/*
==================
G_Say
==================
*/

struct {
	int mode;
	char *name;
}chatModes[] = {
	{-1, "None"},
	{SAY_ALL, "All"},
	{SAY_TEAM, "Team"},
	{SAY_ADMINS, "Admins"}, //NOTE: update login() function if position of this changes
	{SAY_CLOSE, "Close"},
	{SAY_BUDDIES, "Buddies"},
	{SAY_FRIENDS, "Friends"},
};
const int chatModesCount = sizeof(chatModes) / sizeof(chatModes[0]);

char *getChatModeName(int mode){
	if(mode < 0 || mode >= chatModesCount)
		return NULL;
	return chatModes[mode].name;
}

qboolean canUseChatMode(gentity_t *ent, int mode) {
	if(chatModes[mode].mode < 0)
		return qtrue;

	if(mode < 0 || mode >= chatModesCount)
		return qfalse;
	if(chatModes[mode].mode == SAY_ADMINS && !Auths_PlayerHasAuthFlag(ent, AUTH_ADMINCHAT))
		return qfalse;
	if((1 << chatModes[mode].mode) & lmd_chatDisable.integer)
		return qfalse;
	return qtrue;
}

extern vmCvar_t lmd_chatPrimary;
extern vmCvar_t lmd_chatSecondary;
void initChatMode(gentity_t *ent) {
	int set;

	ent->client->pers.Lmd.chatMode[0] = lmd_chatPrimary.integer;
	ent->client->pers.Lmd.chatMode[1] = lmd_chatSecondary.integer;

	for(set = 0; set <= 1; set++) {
		while(!canUseChatMode(ent, ent->client->pers.Lmd.chatMode[set])) {
			ent->client->pers.Lmd.chatMode[set]++;
			if(!getChatModeName(ent->client->pers.Lmd.chatMode[set])) {
				ent->client->pers.Lmd.chatMode[set] = 0;
				break;
			}
		}
	}
}

extern vmCvar_t lmd_closeChatRadius;
extern vmCvar_t lmd_closeChatLOS;
qboolean isBuddy(gentity_t *ent, gentity_t *other);
static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, char *locMsg ){
	int i;
	int j;
	if(!other)
		return;
	if(!other->inuse)
		return;
	if(!other->client)
		return;

	//RoboPhred
	if(other->client->Lmd.quiet)
		return;

	//Run this here for modes that directly call G_SayTo
	if((1 << mode) & lmd_chatDisable.integer) {
		Disp(ent, "^3This chat mode is disabled.");
		return;
	}

	if ( other->client->pers.connected != CON_CONNECTED )
		return;

	//RoboPhred: cleaning this up
	if(ent != other){
		switch(mode){
			default:
			case SAY_ALL:
				break;
			case SAY_TEAM:
				if(!OnSameTeam(ent, other))
					return;
				break;
			case SAY_ADMINS:
				if(!Auths_PlayerHasAuthFlag(other, AUTH_ADMINCHAT))
					return;
				break;
			case SAY_CLOSE:
					if(Distance(ent->client->ps.origin, other->client->ps.origin) > lmd_closeChatRadius.integer)
						return;
					if(lmd_closeChatLOS.integer) {
						trace_t tr;
						trap_Trace( &tr, ent->client->ps.origin, vec3_origin, vec3_origin, other->client->ps.origin,
							ent->s.number, MASK_PLAYERSOLID );
						if ( tr.fraction == 1.0f || tr.entityNum != other->s.number )
							return;
					}
				break;
			case SAY_BUDDIES:
				if(!isBuddy(ent, other))
					return;
				break;
			case SAY_FRIENDS:
				if(!PlayerAcc_Friends_IsFriend(ent, PlayerAcc_GetId(other)))
					return;
				break;
		}
	}


	i = (int)floor((float)other->s.number / 16);
	j = other->s.number % 16;
	if (ent->client->pers.Lmd.ignoredindex[i] & (1 << j) && !Auths_PlayerHasAuthFlag(ent, AUTH_UNIGNOREABLE)){
		return;
	}

	//end Lugormod

	/*
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	&& other->client->sess.sessionTeam == TEAM_FREE
	&& ent->client->sess.sessionTeam != TEAM_FREE ) {
	//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
	return;
	}
	*/
	//They've requested I take this out.

	if ((g_gametype.integer == GT_SIEGE
		|| g_gametype.integer == GT_BATTLE_GROUND) //Lugormod
		&& ent->client && (ent->client->tempSpectate >= level.time || ent->client->sess.sessionTeam == TEAM_SPECTATOR) &&
		other->client->sess.sessionTeam != TEAM_SPECTATOR &&
		other->client->tempSpectate < level.time)
	{ //siege temp spectators should not communicate to ingame players
		return;
	}

	if (locMsg)
	{
		trap_SendServerCommand( other-g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\"", 
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, locMsg, color, message));
	}
	else
	{
		trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message));
	}
	//if (locMsg) {
	//        BotChatBack(va("%s %s", other->client->pers.netname,
	//                       message), ent, other);
	//} else {
	BotChatBack(message,ent, other);
	//}
}

#define EC		"\x19"

//Lugormod
//#if defined(__linux__) || defined(MACOS_X)
//qboolean naken_send(const char *name, const char *msg);
//#endif
//end Lugormod
//qboolean nickLoginOld (gentity_t *ent, int passwd); //Lugormod
qboolean takeAction(gentity_t *ent, int num); //Lugormod
qboolean   profanityCheck  (const char *msg);//Lugormod
extern vmCvar_t lmd_chatDisableUseSay;
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	//Lugormod got this weird segfault once so ....
	if (!ent || !ent->client) {
		return;
	}

	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char		*locMsg = NULL;

	Q_strncpyz( text, chatText, sizeof(text) );

	//Lugormod try to remove some of the login chats
	if(Q_strncmp("login ", text, 6) == 0 || Q_strncmp("login ", text + 1, 6) == 0)
		return;
	
	//Lugormod check for profanity
	if (profanityCheck(text) && ent->client) {                
		if (takeAction(ent, ent->client->Lmd.profanityCount++)){
			return;
		}
	}

	if (ent->client->pers.Lmd.persistantFlags & SPF_UNCAP)
	{
		Q_strlwr(text);
	}

	//end Lugormod

	if(g_gametype.integer < GT_TEAM && (mode == SAY_ALL || mode == SAY_TEAM)){
		//RoboPhred
		if(ent->client->pers.Lmd.chatMode[mode == SAY_TEAM] >= chatModesCount)
			ent->client->pers.Lmd.chatMode[mode == SAY_TEAM] = 0;
		mode = chatModes[ent->client->pers.Lmd.chatMode[mode == SAY_TEAM]].mode;
		if(mode < 0)
			return;
		/*
		if(authenticated(ent, -2))
		mode = SAY_ADMINS;
		else if(hasFriends(ent->client->pers.Lmd.account) || (ent->client->pers.Lmd.buddyindex[0] || ent->client->pers.Lmd.buddyindex[1]))
		mode = SAY_BUDDIES;
		else
		mode = SAY_CLOSE;
		*/
	}

	if((1 << mode) & lmd_chatDisable.integer) {
		if(lmd_chatDisableUseSay.integer <= 0){
			Disp(ent, "^3This chat mode is disabled.");
			return;
		}
		mode = SAY_ALL;
	}

	//Ufo: no need to exclude any mode
	if ( /*mode != SAY_TELL &&*/ ent->client->pers.Lmd.persistantFlags & SPF_SHUTUP)
	{
		//Lugormod
		//#if defined(__linux__) || defined(MACOS_X)
		//                naken_send(ent->client->pers.netname, va("~%s~",
		//                                                         chatText));
		//#endif
		//end Lugormod
		Q_strncpyz(name, ent->client->pers.netname, MAX_NAME_LENGTH);
		Q_CleanStr(name);

		G_LogPrintf("say (muted): %s: %s\n", name, text);

		trap_SendServerCommand(ent->s.number,"cp \"^3You have been muted.\"");

		return;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, text );
		Com_sprintf (name, sizeof(name), "%s%c%c" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, text );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c" EC")" EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c" EC")" EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c" EC"]" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c" EC"]" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_MAGENTA;
		break;
	case SAY_ADMINS:
		G_LogPrintf( "sayadmin: %s: %s\n", ent->client->pers.netname, text );

		Com_sprintf (name, sizeof(name), EC"<%s%c%c" EC">" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_YELLOW;
		break;
	case SAY_CLOSE:
		G_LogPrintf( "sayclose: %s: %s\n", ent->client->pers.netname, text );

		Com_sprintf (name, sizeof(name), EC"<%s%c%c" EC">" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_BUDDIES:
	case SAY_FRIENDS:
		Com_Printf( "info: %s: %s\n", ent->client->pers.netname, text );

		Com_sprintf (name, sizeof(name), EC"{%s%c%c" EC"}" EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_YELLOW;
		break;
	}

	//Lugormod
	//#if defined(__linux__) || defined(MACOS_X)
	//        naken_send(name, text);
	//#endif
	//end Lugormod
	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, locMsg );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text, locMsg );
	}

}


/*
==================
Cmd_Say_f
==================
*/
//static 
void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
gentity_t *ClientFromArg (gentity_t *to, int argNum);

static void Cmd_Tell_f( gentity_t *ent ) {
	//int			targetNum;
	gentity_t	*target;
	char		*p;
	//char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}
	/*
	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
	return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
	return;
	}
	*/
	if (!(target = ClientFromArg(ent,1))) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

//siege voice command
static void Cmd_VoiceCommand_f(gentity_t *ent)
{
	gentity_t *te;
	char arg[MAX_TOKEN_CHARS];
	char *s;
	int i = 0;

	if (g_gametype.integer < GT_TEAM)
	{
		return;
	}

	if (trap_Argc() < 2)
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		ent->client->tempSpectate >= level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOICECHATASSPEC")) );
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (arg[0] == '*')
	{ //hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
		return;
	}

	s = va("*%s", arg);

	//now, make sure it's a valid sound to be playing like this.. so people can't go around
	//screaming out death sounds or whatever.
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (!bg_customSiegeSoundNames[i])
		{
			break;
		}
		if (!Q_stricmp(bg_customSiegeSoundNames[i], s))
		{ //it matches this one, so it's ok
			break;
		}
		i++;
	}

	if (i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i])
	{ //didn't find it in the list
		return;
	}

	te = G_TempEntity(vec3_origin, EV_VOICECMD_SOUND);
	te->s.groundEntityNum = ent->s.number;
	te->s.eventParm = G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
	te->r.svFlags |= SVF_BROADCAST;
}


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= level.maxclients ) { //Ufo: was MAX_CLIENTS before, hence could crash a server with lower slots count
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	Disp( ent, va("^3Origin: ^2%s ^3Angles: ^2%s", vtos( ent->client->ps.origin ),
		vtos( ent->client->ps.viewangles)));
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Power Duel",
	"Single Player",
	"Team FFA",
	"Siege",
	"Capture the Flag",
	"Capture the Ysalamiri",
	"Battle Ground",
	"Saber Run",
	"Reborn"
};

/*
==================
G_ClientNumberFromName

Finds the client number of the client with the given name
==================
*/
int G_ClientNumberFromName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

/*
==================
G_ClientNumberFromStrippedName

Same as above, but strips special characters out of the names before comparing.
==================
*/
int G_ClientNumberFromStrippedName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString2( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString2( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}
void botsVoteNotification(char *votestr);


/*
==================
Cmd_CallVote_f
==================
*/

extern void SiegeClearSwitchData(void); //g_saga.c
const char *G_GetArenaInfoByMap( const char *map );
//Lugormod
#define VOTE_ALL        0x000001
#define VOTE_MAPRESTART 0x000002
#define VOTE_NEXTMAP    0x000004
#define VOTE_MAP        0x000008
#define VOTE_GGAMETYPE  0x000010
#define VOTE_KICK       0x000020
#define VOTE_CLIENTKICK 0x000040
#define VOTE_GDOWARMUP  0x000080
#define VOTE_TIMELIMIT  0x000100
#define VOTE_FRAGLIMIT  0x000200
#define VOTE_GGAMEMODE  0x000400

extern vmCvar_t g_noVoteTime;
static char *gameModeNames[] = {"Normal", "Instant Gib", "Rocket arena","Sniper arena", "Melee arena", "Super merc/jedi", "Instant disrupt", "ERROR"};
static char *gameModeFlagNames[] = {" Low gravity", " Force jump", " Jetpack"," Grappling hook", " Teleporter gun", " Respawn timer"};

qboolean checkVoteEnabled (gentity_t *ent, int vote) {        
	if (!(vote & (VOTE_GGAMEMODE|VOTE_GDOWARMUP)) && ent && !Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING) && 
		level.time < level.startTime + (g_noVoteTime.integer * 60000)) {
			Disp(ent, va("Voting is disabled for %i %s after map change.", g_noVoteTime.integer,
				g_noVoteTime.integer == 1 ? "minute":"minutes"));
			return qfalse;
	}
	if ((g_allowVote.integer & (vote|VOTE_ALL)) || !ent || Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING)) {
		return qtrue;
	}
	Disp(ent, "Calling a vote for this has been disabled");
	return qfalse;
}


//end Lugormod
void CallVote (gentity_t *ent, char *arg1, char *arg2) 
{
	if (ent && !ent->client) {
		return;
	}

	if (ent && ent->client->pers.Lmd.persistantFlags & SPF_NOCALL) {
		Disp(ent, "Calling votes has been disabled for you.");
		return;
	}

	int		i;
	char*		mapName = 0;
	const char*	arenaInfo;

	if ( ent && !g_allowVote.integer && !Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING)) {
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		Disp(ent, G_GetStringEdString("MP_SVGAME", "NOVOTE"));
		return;
	}

	if ( level.voteTime || level.voteExecuteTime >= level.time ) {
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")) );
		Disp(ent, G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS"));
		return;
	}
	if ( ent && MAX_VOTE_COUNT 
		&& ent->client->pers.voteCount >= MAX_VOTE_COUNT 
		&& !Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING)) {
			//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MAXVOTES")) );
			Disp(ent, G_GetStringEdString("MP_SVGAME", "MAXVOTES"));
			return;
	}

	if (g_gametype.integer != GT_DUEL &&
		g_gametype.integer != GT_POWERDUEL &&
		//Lugormod this must be here to:
		g_gametype.integer != GT_SIEGE)
	{
		if ( ent && ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")) );
			Disp(ent, G_GetStringEdString("MP_SVGAME", "NOSPECVOTE"));
			return;
		}
	}
	//RoboPhred
	if( strchr( arg1, ';' ) || strchr(arg1, '\n') || strchr( arg2, ';' ) || strchr(arg2, '\n')) {
	//if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		//trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		Disp(ent, "Invalid vote string.");
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" )
		&& checkVoteEnabled(ent, VOTE_MAPRESTART) ) {
	} else if ( !Q_stricmp( arg1, "nextmap"  )
		&& checkVoteEnabled(ent, VOTE_NEXTMAP) ) {
	} else if ( !Q_stricmp( arg1, "map"  )
		&& checkVoteEnabled(ent, VOTE_MAP) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype"  )
		&& checkVoteEnabled(ent, VOTE_GGAMETYPE) ) {
	} else if ( !Q_stricmp( arg1, "kick"  )
		&& checkVoteEnabled(ent, VOTE_KICK) ) {
	} else if ( !Q_stricmp( arg1, "clientkick" ) 
		&& checkVoteEnabled(ent, VOTE_CLIENTKICK) ) {
	} else if ( !Q_stricmp( arg1, "g_doWarmup"  )
		&& checkVoteEnabled(ent, VOTE_GDOWARMUP) ) {
	} else if ( !Q_stricmp( arg1, "timelimit"  )
		&& checkVoteEnabled(ent, VOTE_TIMELIMIT) ) {
	} else if ( !Q_stricmp( arg1, "fraglimit"  )
		&& checkVoteEnabled(ent, VOTE_FRAGLIMIT) ) {
	} else if ( !Q_stricmp( arg1, "g_gameMode"  )
		&& checkVoteEnabled(ent, VOTE_GGAMEMODE) ) {
			//} else if ( !Q_stricmp( arg1, "g_gravity"  )
			//    && checkVoteEnabled(ent, VOTE_GGRAVITY) ) {
	} else {
		//trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		Disp(ent, "Invalid vote string.");
		//trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"" );
		Disp(ent, "Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>, g_gameMode <n>.");
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}
	//Lugormod they seem to have forgotten this
	if (ent) {
		ent->client->pers.voteCount++;
	}

	botsVoteNotification(arg1);
	level.votingGametype = qfalse;

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( arg1, "g_gametype" ) )
	{
		i = atoi( arg2 );
		if( i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			//trap_SendServerCommand( ent-g_entities, "print \"Invalid gametype.\n\"" );
			Disp(ent,"Invalid gametype.");
			return;
		}

		level.votingGametype = qtrue;
		level.votingGametypeTo = i;

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		if (i == GT_JEDIMASTER
			|| i >= GT_BATTLE_GROUND) {
				Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map Type: %s", gameNames[i] );
		} else {
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[i] );
		}

	}
	else if ( !Q_stricmp( arg1, "map" ) ) 
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];
		if (ent && !G_DoesMapSupportGametype(arg2, trap_Cvar_VariableIntegerValue("g_gametype")) && !Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			Disp(ent, G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME"));
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}

		arenaInfo	= G_GetArenaInfoByMap(arg2);
		if (arenaInfo)
		{
			mapName = Info_ValueForKey(arenaInfo, "longname");
		}

		if (!mapName || !mapName[0])
		{
			//mapName = "ERROR";
			mapName = arg2;
		}

		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s", mapName);
	}
	else if ( !Q_stricmp ( arg1, "clientkick" ) ||  !Q_stricmp ( arg1, "kick" ) )
	{
		int n;
		if( !Q_stricmp ( arg1, "kick" ) ) {
			n = G_ClientNumberFromName ( arg2 );

			if ( n == -1 ){
				n = G_ClientNumberFromStrippedName(arg2);

				if (n == -1){
					Disp(ent, va("There is no client named '%s' currently on the server.", arg2 ));
					return;
				}
			}
		}
		else
			n = atoi ( arg2 );

		if ( n < 0 || n >= MAX_CLIENTS){
			//trap_SendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", n ) );
			Disp(ent, va("Invalid client number %d.", n ));
			return;
		}

		if ( g_entities[n].client->pers.connected == CON_DISCONNECTED ){
			//trap_SendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", n ) );
			Disp(ent, va("There is no client with the client number %d.", n ));
			return;
		}

		//Lugormod kick admin prevention
		if (Auths_PlayerHasAdmin(&g_entities[n])) {
			//trap_SendServerCommand( ent-g_entities, "print \"You may not kick admins.\n\"");
			Disp(ent, "You may not kick admins.");
			return;
		}

		//Com_sprintf ( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", n );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[n].client->pers.netname );
	}
	else if ( !Q_stricmp( arg1, "nextmap" ) ) 
	{
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			//trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			Disp(ent, "Nextmap not set.");
			return;
		}
		SiegeClearSwitchData();
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} 
	else if (!Q_stricmp( arg1, "g_doWarmup" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}else if (!Q_stricmp( arg1, "g_gameMode" ) ) {
		if (arg2[0] > '9' || arg2[0] < '0') {
			Disp(ent, "Invalid game mode");
			return;
		}
		int mode = atoi(arg2);
		if ((mode&7) > 6) {
			Disp(ent, "Invalid game mode");
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map GameMode: %s", gameModeNames[mode&7] );
		int n;

		for (n = 3; n < 9; n++){
			if ((1 << n)&mode){
				Q_strcat(level.voteDisplayString, 
					sizeof(level.voteDisplayString),
					gameModeFlagNames[n - 3]);
			}
		}

	} else {

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s", level.voteString );
	}
	char *callername;

	if (ent) {
		callername = ent->client->pers.netname;
	} else {
		if (g_nameForServer.string[0]){
			callername = g_nameForServer.string;
		} else {
			callername = "server";
		}
	}
	trap_SendServerCommand( -1, va("print \"%s^7 %s\n\"", callername, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE") ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteNo  = 0;
	level.voteYes = 0;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
	}
	if (ent) {
		level.voteYes = 1;
		ent->client->mGameFlags |= PSG_VOTED;
		Com_Printf("info: %s called vote: %s\n",
			ent->client->pers.netname,
			level.voteDisplayString);
	}

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );

}

void Cmd_CallVote_f( gentity_t *ent ) {

	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];
	//	int		n = 0;
	//	char*	type = NULL;


	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );
	CallVote(ent, arg1,arg2);
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")) );
		return;
	}
	if (g_gametype.integer != GT_DUEL &&
		g_gametype.integer != GT_POWERDUEL)
	{
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
			return;
		}
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")) );

	ent->client->mGameFlags |= PSG_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, team, cs_offset;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer && !Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING)) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADY")) );
		return;
	}
	if ( MAX_VOTE_COUNT && ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT &&
		!Auths_PlayerHasAuthFlag(ent, AUTH_FULLVOTING)) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MAXTEAMVOTES")) );
			return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	//Lugormod they forgot.
	ent->client->pers.teamVoteCount++;

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].mGameFlags &= ~PSG_TEAMVOTED;
	}
	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLTEAMVOTECAST")) );

	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !CheatsOk (ent) ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles, qfalse);
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
	/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
	if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
	n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
	*/
}

//RoboPhred
extern vmCvar_t lmd_vehcloaking;

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(ps, forcedUse))
	{
		return 0;
	}

	//RoboPhred
	if (ps->m_iVehicleNum && (forcedUse != HI_CLOAK || lmd_vehcloaking.integer == 0 || !CheatsOk(&g_entities[ps->clientNum])))
	//if (ps->m_iVehicleNum)
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		trap_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_JETPACK: //do something?

		//Actually we can't do anything here, since BG_IsItemSelectable returns 0 for jetpack.
		/*
		//RoboPhred:  Added checks from ItemUse_Jetpack
		if(g_entities[ps->clientNum].client->ps.trueJedi)
			return 0;
		if(g_entities[ps->clientNum].client->ps.forceJumpFlip)
			return 0;
		*/

		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK:
		return 1;
	default:
		return 1;
	}
}

void saberKnockDown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);

void Cmd_ToggleSaber_f(gentity_t *ent){
	//if they are being gripped, don't let them unholster their saber
	if(ent->client->ps.fd.forceGripCripple && ent->client->ps.saberHolstered)
		return;

	if(ent->client->ps.saberInFlight){
		//turn it off in midair
		if (ent->client->ps.saberEntityNum)
			saberKnockDown(&g_entities[ent->client->ps.saberEntityNum], ent, ent);
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return;

	if (ent->client->ps.weapon != WP_SABER)
		return;
	/*
	if (ent->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL && 
	g_gametype.integer == GT_FFA &&
	!ent->client->ps.duelInProgress &&
	ent->client->ps.saberHolstered //Should be selfevident, but ...
	) {
	trap_SendServerCommand(ent->s.number, "cp \"You cannot unholster your saber outside a duel\"");

	return;
	}
	*/

	//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
	//	{
	//		return;
	//	}

	if (ent->client->ps.duelTime >= level.time)
		return;

	if (ent->client->ps.saberLockTime >= level.time)
		return;

	if (ent->client && (ent->client->ps.weaponTime < 1)){
		if (ent->client->ps.saberHolstered == 2){
			//Saber on

			ent->client->ps.saberHolstered = 0;

			if (ent->client->saber[0].soundOn)
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			if (ent->client->saber[1].soundOn && ent->client->saber[1].model[0])
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
			//RoboPhred: this probably shouldn't be here
			//else{
			//hack, don't do while moving
			if(ent->client->saber[0].saberFlags & SFL_TWO_HANDED && !(ent->client->pers.cmd.upmove || ent->client->pers.cmd.forwardmove || 
				ent->client->pers.cmd.rightmove || ent->client->ps.m_iVehicleNum || ent->client->ps.groundEntityNum == ENTITYNUM_NONE))
				G_SetAnim(ent, SETANIM_TORSO, BOTH_S1_S7, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			else if (!(ent->client->ps.m_iVehicleNum || ent->client->ps.groundEntityNum == ENTITYNUM_NONE))
				G_SetAnim(ent, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			//}
		}
		else{
			ent->client->ps.saberHolstered = 2;
			if (ent->client->saber[0].soundOff)
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
			if(ent->client->saber[1].soundOff && ent->client->saber[1].model[0])
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
			//RoboPhred: this probably shouldn't be here
			//else{
			//hack, don't do while moving
			if(ent->client->saber[0].saberFlags & SFL_TWO_HANDED && (!(ent->client->pers.cmd.upmove || ent->client->pers.cmd.forwardmove ||
				ent->client->pers.cmd.rightmove))){
					//G_SetAnim(ent, NULL, SETANIM_TORSO, BOTH_S7_S1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else if(!(ent->client->ps.m_iVehicleNum || ent->client->ps.groundEntityNum == ENTITYNUM_NONE)) //hack, don't do while moving
				G_SetAnim(ent, SETANIM_BOTH, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			//}
			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

extern vmCvar_t		d_saberStanceDebug;

extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;
	qboolean usingSiegeStyle = qfalse;

	if ( !ent || !ent->client )
	{
		return;
	}

	/*w
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
	return;
	}
	*/

	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{ //no cycling for akimbo
		if ( WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) )
		{//can turn second saber off 
			if ( ent->client->ps.saberHolstered == 1 )
			{//have one holstered
				//unholster it
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;


				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if ( ent->client->ps.saberHolstered == 0 )
			{//have none holstered
				if ( (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
				{//can't turn it off manually
				}
				else if ( ent->client->saber[1].bladeStyle2Start > 0
					&& (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
				{//can't turn it off manually
				}
				else
				{
					//turn it off
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if (d_saberStanceDebug.integer)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\"") );
			}
			return;
		}
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) )
	{ //use staff stance then.
		if ( ent->client->ps.saberHolstered == 1 )
		{//second blade off
			if ( ent->client->ps.saberInFlight )
			{//can't turn second blade back on if it's in the air, you naughty boy!
				if (d_saberStanceDebug.integer)
				{
					trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\"") );
				}
				return;
			}
			//turn it on
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if ( ent->client->saber[0].stylesForbidden )
			{//have a style we have to use
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
				if ( ent->client->ps.weaponTime <= 0 )
				{ //not busy, set it now
					ent->client->ps.fd.saberAnimLevel = selectLevel;
				}
				else
				{ //can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = selectLevel;
				}
			}
		}
		else if ( ent->client->ps.saberHolstered == 0 )
		{//both blades on
			if ( (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
			{//can't turn it off manually
			}
			else if ( ent->client->saber[0].bladeStyle2Start > 0
				&& (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
			{//can't turn it off manually
			}
			else
			{
				//turn second one off
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if ( ent->client->saber[0].singleBladeStyle != SS_NONE )
				{
					if ( ent->client->ps.weaponTime <= 0 )
					{ //not busy, set it now
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					}
					else
					{ //can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
					}
				}
			}
		}
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\"") );
		}
		return;
	}

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	if ((g_gametype.integer == GT_SIEGE || 
		g_gametype.integer == GT_BATTLE_GROUND)
		&& ent->client->siegeClass != -1 &&
		bgSiegeClasses[ent->client->siegeClass].saberStance)
	{ //we have a flag of useable stances so cycle through it instead
		int i = selectLevel+1;

		usingSiegeStyle = qtrue;

		while (i != selectLevel)
		{ //cycle around upward til we hit the next style or end up back on this one
			if (i >= SS_NUM_SABER_STYLES)
			{ //loop back around to the first valid
				i = SS_FAST;
			}

			if (bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i))
			{ //we can use this one, select it and break out.
				selectLevel = i;
				break;
			}
			i++;
		}

		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\"") );
		}
	}
	else
	{
		selectLevel++;
		//RoboPhred
		/*
		1, desann stands totally removed 
		*/
		if(selectLevel == FORCE_LEVEL_4) 
			selectLevel = FORCE_LEVEL_5;

		if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
		{
			selectLevel = FORCE_LEVEL_1;
		}
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\"") );
		}
	}
	/*
	#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
	trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
	break;
	case FORCE_LEVEL_2:
	trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
	break;
	case FORCE_LEVEL_3:
	trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
	break;
	}
	#endif
	*/
	if ( !usingSiegeStyle )
	{
		//make sure it's valid, change it if not
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
	}

	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		//if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		if (ent && ent->inuse 
			&& ent->client && duelInProgress(&ent->client->ps))
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}
//extern gentity_t *g_crownEnt;

qboolean IsKing(gentity_t *ent); //Lugormod
char* showWinLose (gentity_t *me, gentity_t *him); //Lugormod
extern vmCvar_t g_chickenTime; //Lugormod
//RoboPhred
void Cmd_EngageDuel_f(gentity_t *ent){
	trace_t tr;
	vec3_t forward, fwdOrg;

	if (!(g_privateDuel.integer & PD_ENABLE))
		return;

	if(g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL){ //rather pointless in this mode..
		Disp(ent, G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE"));
		return;
	}

	if(g_gametype.integer >= GT_TEAM){ //no private dueling in team modes
		Disp(ent, G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE"));
		return;
	}
	if(ent->client->pers.Lmd.jailTime > level.time){
		trap_SendServerCommand( ent-g_entities, "cp \"^3You may not duel in jail\"" );
		return;
	}


	if(ent->client->ps.duelTime >= level.time)
		return;

	if (ent->client->ps.weapon != WP_SABER)
		return;

	if (ent->client->ps.saberInFlight)
		return;

	if (duelInProgress(&ent->client->ps))
		return;

	//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	if(ent->client->ps.fd.privateDuelTime > level.time && !(g_privateDuel.integer & PD_MULTIPLE)){
		Disp(ent, G_GetStringEdString("MP_SVGAME", "CANTDUEL_JUSTDID"));
		return;
	}

	if(!(g_privateDuel.integer & PD_MULTIPLE) && G_OtherPlayersDueling()){
		Disp(ent, G_GetStringEdString("MP_SVGAME", "CANTDUEL_BUSY"));
		return;
	}

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	trap_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS){
		gentity_t *challenged = &g_entities[tr.entityNum];
		int myCreds = PlayerAcc_GetCredits(ent), chalCreds = PlayerAcc_GetCredits(challenged);

		if (!challenged || !challenged->client || !challenged->inuse ||	challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			duelInProgress(&challenged->client->ps) || challenged->client->ps.saberInFlight)
			return;

		if(g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
			return;

		if(ent->client->ps.weapon != challenged->client->ps.weapon)
			return;

		//Lugormod: cannot engage if too close
		float dist = Distance(ent->client->ps.origin, challenged->client->ps.origin);
		if(dist < 80){
			trap_SendServerCommand(ent->s.number, "cp \"^3You are standing too close.\n^3You need to back up before\n^3you can challenge this person to a duel.\"");
			return;
		}

		//Ufo:
		if(ent->client->Lmd.restrict & 8 || challenged->client->Lmd.restrict & 8) {
			trap_SendServerCommand(ent->s.number, "cp \"^3You cannot duel here.");
			return;
		}
		
		//Ufo: holstering was buggy
		ent->client->ps.saberHolstered = 2;
		ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		ent->client->invulnerableTimer = 0;


		if ((g_privateDuel.integer & PD_KING) && ent->client->Lmd.duel.duelType == 0 && ent->client->ps.m_iVehicleNum == 0 && IsKing(challenged)){
			challenged->client->Lmd.duel.duelType = 0;
			challenged->client->ps.duelIndex = ent->s.number;
			challenged->client->ps.duelTime = level.time + 100;
			challenged->client->ps.saberHolstered = 2;
			challenged->client->ps.eFlags &= ~EF_INVULNERABLE;
			challenged->client->invulnerableTimer = 0;
			trap_SendServerCommand(challenged->s.number, "cp \"^3You are the King,\n^3defend your crown!\"");
		}
		
		//Ufo: autoaccept if we are in ionlyduel
		if (ent->client->Lmd.duel.duelType == 0 && ent->client->ps.m_iVehicleNum == 0 && challenged->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL){
			challenged->client->Lmd.duel.duelType = 0;
			challenged->client->ps.duelIndex = ent->s.number;
			challenged->client->ps.duelTime = level.time + 100;
			challenged->client->ps.saberHolstered = 2;
			challenged->client->ps.eFlags &= ~EF_INVULNERABLE;
			challenged->client->invulnerableTimer = 0;
			trap_SendServerCommand(challenged->s.number, "cp \"^3Duel challenge has been auto-accepted!\"");
		}

		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time){
			//RoboPhred
			if(challenged->client->Lmd.duel.duelBet > 0){
				gentity_t *poor = NULL, *other;
				if(myCreds < challenged->client->Lmd.duel.duelBet){
					poor = ent;
					other = challenged;
				}
				else if(chalCreds < challenged->client->Lmd.duel.duelBet){
					poor = challenged;
					other = ent;
				}
				if(poor){
					trap_SendServerCommand(poor->s.number, "cp \"^3You do not have enough money for this bet.\"");
					trap_SendServerCommand(other->s.number,  va("cp \"%s ^3does not have enough money for this bet.\"", ent->client->pers.netname));
					return;
				}
				PlayerAcc_SetCredits(ent, myCreds - challenged->client->Lmd.duel.duelBet);
				PlayerAcc_SetCredits(challenged, chalCreds - challenged->client->Lmd.duel.duelBet);
			}

			if (!((g_cmdDisable.integer * 2) & (1 << 1)) && g_chickenTime.integer && ent->client->pers.Lmd.account && ent->client->Lmd.duel.challengedTime){
				ent->client->Lmd.duel.challengedBy = -1;
				ent->client->Lmd.duel.challengedTime = 0;
			}

			trap_SendServerCommand(-1, va("print \"%s %s %s!\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );

			Com_Printf("info: %s vs %s\n", challenged->client->pers.netname, ent->client->pers.netname);
			ent->client->Lmd.duel.duelType = challenged->client->Lmd.duel.duelType;
			ent->client->Lmd.duel.duelBet = challenged->client->Lmd.duel.duelBet; //Ufo: was missing
			ent->client->Lmd.training.received = challenged->client->Lmd.training.received = ent->client->Lmd.training.dealt = challenged->client->Lmd.training.dealt = 
				ent->client->Lmd.training.dispTime = challenged->client->Lmd.training.dispTime = 0;

			/*
			if (ent->client->Lmd.duel.duelType & DT_MELEE) {
			ent->client->Lmd.flags 
			|= SNF_MELEEINPROGRESS;
			challenged->client->Lmd.flags
			|= SNF_MELEEINPROGRESS;
			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;
			} else {
			*/
			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;
			//}

			ent->client->ps.duelTime = level.time + 2000;
			challenged->client->ps.duelTime = level.time + 2000;
			//Lugormod to get time to bow
			if (g_privateDuel.integer & PD_FORCE_BOW) {
				ent->client->ps.duelTime += 500;
				challenged->client->ps.duelTime += 500;

			}

			G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
			G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

			PlayerAcc_Stats_SetDuels(ent, PlayerAcc_Stats_GetDuels(ent) + 1);
			PlayerAcc_Stats_SetDuels(challenged, PlayerAcc_Stats_GetDuels(challenged) + 1);


			//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)
			//Lugormod Replenish health and armor
			if (g_privateDuel.integer & PD_START_POWER) {
				ent->client->ps.fd.forcePower = 100;
				challenged->client->ps.fd.forcePower = 100;
			}
			if (g_privateDuel.integer & PD_START_ARMOR) {
				ent->client->bdArmor = ent->client->ps.stats[STAT_ARMOR];
				challenged->client->bdArmor = challenged->client->ps.stats[STAT_ARMOR];
				ent->client->ps.stats[STAT_ARMOR] = 100;
				challenged->client->ps.stats[STAT_ARMOR] = 100;
			}
			if (g_privateDuel.integer & PD_START_HEALTH) {
				ent->client->bdHealth =	ent->health;
				challenged->client->bdHealth = challenged->health;
				ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
				challenged->client->ps.stats[STAT_HEALTH] = challenged->health = 100;
			}

			ent->client->pers.Lmd.persistantFlags &= ~(SPF_IONLYDUEL | SPF_IONLYSABER);
			
			challenged->client->pers.Lmd.persistantFlags &= ~(SPF_IONLYDUEL | SPF_IONLYSABER);
				
			//Ufo: duplicate
			/*if(!ent->client->ps.saberHolstered){
				Cmd_ToggleSaber_f(ent);
				/*
				if (ent->client->saber[0].soundOff){
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				}
				if (ent->client->saber[1].soundOff &&
				ent->client->saber[1].model[0])
				{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
				}
				ent->client->ps.weaponTime = 400;
				ent->client->ps.saberHolstered = 2;
			}
			if(!challenged->client->ps.saberHolstered){
				Cmd_ToggleSaber_f(challenged);
				if (challenged->client->saber[0].soundOff)
				{
				G_Sound(challenged, CHAN_AUTO, challenged->client->saber[0].soundOff);
				}
				if (challenged->client->saber[1].soundOff &&
				challenged->client->saber[1].model[0])
				{
				G_Sound(challenged, CHAN_AUTO, challenged->client->saber[1].soundOff);
				}
				challenged->client->ps.weaponTime = 400;
				challenged->client->ps.saberHolstered = 2;
			}
			*/
		}
		else{
			//Challenging
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			if(ent->client->Lmd.duel.duelType != 0){
				//Ufo:
				if (ent->client->Lmd.duel.duelType & DT_FULL_FORCE) {
				ent->client->Lmd.duel.duelType &= ~DT_FORCE;
				}
				char desc[MAX_STRING_CHARS] = "";
				/*
				if (ent->client->Lmd.duel.duelType & DT_MELEE) {
				Q_strcat(desc, sizeof(desc), "melee ");
				}
				*/
				if (ent->client->Lmd.duel.duelType & DT_FORCE)
					Q_strcat(desc, sizeof(desc), "^5force ");
				if (ent->client->Lmd.duel.duelType & DT_FULL_FORCE) {
				Q_strcat(desc, sizeof(desc), "^7full force ");
				}
				if (ent->client->Lmd.duel.duelType & DT_TRAINING)
					Q_strcat(desc, sizeof(desc), "^3training ");
				if (ent->client->Lmd.duel.duelType & DT_POWER)
					Q_strcat(desc, sizeof(desc), "^1power ");
				if (ent->client->Lmd.duel.duelType & DT_TINY)
					Q_strcat(desc, sizeof(desc), "^4tiny ");
				if (ent->client->Lmd.duel.duelType & DT_TITAN)
					Q_strcat(desc, sizeof(desc), "^4titan ");
				/*
				if ((ent->client->Lmd.duel.duelType & DT_BET)
				&& (ent->client->Lmd.duel.duelType & DT_HIBET)){
				Q_strcat(desc, sizeof(desc), "bet CR 500 ");
				} else if (ent->client->Lmd.duel.duelType & DT_BET) {
				Q_strcat(desc, sizeof(desc), "bet CR 10 ");
				} else if (ent->client->Lmd.duel.duelType & DT_HIBET) {
				Q_strcat(desc, sizeof(desc), "bet CR 100 ");
				}
				*/
				if(ent->client->Lmd.duel.duelType & DT_BET)
					Q_strcat(desc, sizeof(desc), va("^3bet CR ^2%i ", ent->client->Lmd.duel.duelBet));


				trap_SendServerCommand( challenged-g_entities, va("cp \"%s\n^3has challenged you to a\n%s^3duel\n%s\"", ent->client->pers.netname, desc, showWinLose(challenged, ent)) );
				//trap_SendServerCommand( challenged-g_entities, va("print \"^1%sduel\n\"", desc) );
				trap_SendServerCommand( ent-g_entities, va("cp \"^3You have challenged\n%s\n^3to a %s^3duel\"", challenged->client->pers.netname, desc) );
			}
			else{
				trap_SendServerCommand( challenged-g_entities, va("cp \"%s\n%s\n%s\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE"), showWinLose(challenged, ent)) );
				trap_SendServerCommand( ent-g_entities, va("cp \"%s\n%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
			}

			//Lugormod
			//RoboPhred: (g_cmdDisable.integer * 2) & (1 << 1): what is this for?
			if (!((g_cmdDisable.integer * 2) & (1 << 1)) && g_chickenTime.integer &&
				ent->client->pers.Lmd.account && challenged->client->pers.Lmd.account &&
				!(challenged->client->Lmd.duel.challengedBy == -1 && challenged->client->Lmd.duel.challengedTime > level.time)) {
					challenged->client->Lmd.duel.challengedTime = level.time + 8000;
					challenged->client->Lmd.duel.challengedBy = ent->s.number;
			}
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		//the one we challenge get 8 seconds to accept
		ent->client->ps.duelTime = level.time + 8000;

	}
}
//Lugormod I want these
//#ifndef FINAL_BUILD
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("info: Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("ERROR: Animation '%s' does not exist\n", arg);
		return;
	}

	G_SetAnim(self, SETANIM_BOTH, i, flags, 0);

	Com_Printf("info: Set body anim to %s\n", arg);
}
//#endif
//end Lugormod
void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	G_SetAnim(self, SETANIM_BOTH, anim, flags, 0);
}

void Cmd_DebugThrow_f (gentity_t *ent){

	trace_t tr;
	vec3_t tTo, fwd;

	if (ent->client->ps.weaponTime > 0 || ent->client->ps.forceHandExtend != HANDEXTEND_NONE ||
		ent->client->ps.groundEntityNum == ENTITYNUM_NONE || ent->health < 1)
	{
		return;
	}
	//Use AimTarget here instead ?
	AngleVectors(ent->client->ps.viewangles, fwd, 0, 0);
	tTo[0] = ent->client->ps.origin[0] + fwd[0]*32;
	tTo[1] = ent->client->ps.origin[1] + fwd[1]*32;
	tTo[2] = ent->client->ps.origin[2] + fwd[2]*32;

	trap_Trace(&tr, ent->client->ps.origin, 0, 0, tTo, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1)
	{
		gentity_t *other = &g_entities[tr.entityNum];

		if (other->inuse && other->client && other->client->ps.forceHandExtend == HANDEXTEND_NONE &&
			other->client->ps.groundEntityNum != ENTITYNUM_NONE && other->health > 0 &&
			(int)ent->client->ps.origin[2] == (int)other->client->ps.origin[2] && !Auths_Inferior(ent,other))
		{
			float pDif = 40.0f;
			vec3_t entAngles, entDir;
			vec3_t otherAngles, otherDir;
			vec3_t intendedOrigin;
			vec3_t boltOrg, pBoltOrg;
			vec3_t tAngles, vDif;
			vec3_t fwd, right;
			trace_t tr;
			trace_t tr2;

			VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
			VectorCopy( ent->client->ps.viewangles, entAngles );
			entAngles[YAW] = vectoyaw( otherDir );
			SetClientViewAngle( ent, entAngles );

			ent->client->ps.forceHandExtend = HANDEXTEND_PRETHROW;
			ent->client->ps.forceHandExtendTime = level.time + 5000;

			ent->client->throwingIndex = other->s.number;
			ent->client->doingThrow = level.time + 5000;
			ent->client->beingThrown = 0;

			VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
			VectorCopy( other->client->ps.viewangles, otherAngles );
			otherAngles[YAW] = vectoyaw( entDir );
			SetClientViewAngle( other, otherAngles );

			other->client->ps.forceHandExtend = HANDEXTEND_PRETHROWN;
			other->client->ps.forceHandExtendTime = level.time + 5000;

			other->client->throwingIndex = ent->s.number;
			other->client->beingThrown = level.time + 5000;
			other->client->doingThrow = 0;

			//Doing this now at a stage in the throw, isntead of initially.
			//other->client->ps.heldByClient = ent->s.number+1;

			G_EntitySound( other, CHAN_VOICE, G_SoundIndex("*pain100.wav") );
			G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
			G_Sound(other, CHAN_AUTO, G_SoundIndex( "sound/movers/objects/objectHit.wav" ));

			//see if we can move to be next to the hand.. if it's not clear, break the throw.
			VectorClear(tAngles);
			tAngles[YAW] = ent->client->ps.viewangles[YAW];
			VectorCopy(ent->client->ps.origin, pBoltOrg);
			AngleVectors(tAngles, fwd, right, 0);
			boltOrg[0] = pBoltOrg[0] + fwd[0]*8 + right[0]*pDif;
			boltOrg[1] = pBoltOrg[1] + fwd[1]*8 + right[1]*pDif;
			boltOrg[2] = pBoltOrg[2];

			VectorSubtract(boltOrg, pBoltOrg, vDif);
			VectorNormalize(vDif);

			VectorClear(other->client->ps.velocity);
			intendedOrigin[0] = pBoltOrg[0] + vDif[0]*pDif;
			intendedOrigin[1] = pBoltOrg[1] + vDif[1]*pDif;
			intendedOrigin[2] = other->client->ps.origin[2];

			trap_Trace(&tr, intendedOrigin, other->r.mins, other->r.maxs, intendedOrigin, other->s.number, other->clipmask);
			trap_Trace(&tr2, ent->client->ps.origin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number, CONTENTS_SOLID);

			if (tr.fraction == 1.0 && !tr.startsolid && tr2.fraction == 1.0 && !tr2.startsolid)
			{
				VectorCopy(intendedOrigin, other->client->ps.origin);
			}
			else
			{ //if the guy can't be put here then it's time to break the throw off.
				vec3_t oppDir;
				int strength = 4;

				other->client->ps.heldByClient = 0;
				other->client->beingThrown = 0;
				ent->client->doingThrow = 0;

				ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
				G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain25.wav") );

				other->client->ps.forceHandExtend = HANDEXTEND_NONE;
				VectorSubtract(other->client->ps.origin, ent->client->ps.origin, oppDir);
				VectorNormalize(oppDir);
				other->client->ps.velocity[0] = oppDir[0]*(strength*40);
				other->client->ps.velocity[1] = oppDir[1]*(strength*40);
				other->client->ps.velocity[2] = 150;

				VectorSubtract(ent->client->ps.origin, other->client->ps.origin, oppDir);
				VectorNormalize(oppDir);
				ent->client->ps.velocity[0] = oppDir[0]*(strength*40);
				ent->client->ps.velocity[1] = oppDir[1]*(strength*40);
				ent->client->ps.velocity[2] = 150;
			}
		}
	}
}


void Bot_SetForcedMovement(int bot, int forward, int right, int up);
extern void DismembermentByNum(gentity_t *self, int num);
#ifndef FINAL_BUILD
extern void G_SetVehDamageFlags( gentity_t *veh, int shipSurf, int damageLevel );
#endif

static int G_ClientNumFromNetname(char *name)
{
	int i = 0;
	gentity_t *ent;
	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client &&
			(!Q_stricmp(ent->client->pers.netname, name)))
		{
			return ent->s.number;
		}
		i++;
	}

	return -1;
}

qboolean TryGrapple(gentity_t *ent)
{
	if (ent->client->ps.weaponTime > 0)
	{ //weapon busy
		return qfalse;
	}
	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //force power or knockdown or something
		return qfalse;
	}
	if (ent->client->grappleState)
	{ //already grappling? but weapontime should be > 0 then..
		return qfalse;
	}

	//Ufo:
	if (ent->client->Lmd.restrict & 16)
		return qfalse;

	if (ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
	{
		Cmd_ToggleSaber_f(ent);
		if (!ent->client->ps.saberHolstered)
		{ //must have saber holstered
			return qfalse;
		}
	}

	//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_PA_1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	G_SetAnim(ent, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	if (ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{ //providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if (ent->client->ps.legsAnim == ent->client->ps.torsoAnim)
		{
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		//Ufo:
		ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		ent->client->invulnerableTimer = 0;
		return qtrue;
	}

	return qfalse;
}
//#ifndef FINAL_BUILD
//#endif

/*
==================
Cmd_SetModel_f

Lugormod
==================
*/

void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName);

void Cmd_SetModel_f (gentity_t *ent, int iArg) 
{
	if (trap_Argc() < 2) {
		return;
	}
	char modelname[MAX_TOKEN_CHARS];
	trap_Argv(1, modelname, sizeof(modelname));

	SetupGameGhoul2Model(ent, modelname, NULL);

	if (ent->ghoul2 && ent->client)
	{
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
	}

	Disp(ent, "Model set.");
}

gentity_t* AimTarget (gentity_t *ent, int length);
void scaleEntity (gentity_t *scaleEnt, int scale);

void
Cmd_Hug_f (gentity_t *ent) 
{
	/*
	trace_t tr;
	vec3_t fPos;

	AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

	fPos[0] = ent->client->ps.origin[0] + fPos[0]*40;
	fPos[1] = ent->client->ps.origin[1] + fPos[1]*40;
	fPos[2] = ent->client->ps.origin[2] + fPos[2]*40;

	trap_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

	if (tr.entityNum < MAX_CLIENTS && tr.entityNum != ent->s.number)
	{
	gentity_t *other = &g_entities[tr.entityNum];
	*/
	gentity_t *other;

	if ((other = AimTarget(ent, 40))) {

		if (other && other->inuse && other->client)
		{
			if (Auths_Inferior(ent, other)){
				return;
			}
			scaleEntity(ent, 0);
			scaleEntity(other, 0);

			vec3_t entDir;
			vec3_t otherDir;
			vec3_t entAngles;
			vec3_t otherAngles;

			if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
			{
				Cmd_ToggleSaber_f(ent);
			}

			if (other->client->ps.weapon == WP_SABER && !other->client->ps.saberHolstered)
			{
				Cmd_ToggleSaber_f(other);
			}

			if ((ent->client->ps.weapon != WP_SABER || ent->client->ps.saberHolstered) &&
				(other->client->ps.weapon != WP_SABER || other->client->ps.saberHolstered))
			{
				VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
				VectorCopy( ent->client->ps.viewangles, entAngles );
				entAngles[YAW] = vectoyaw( otherDir );
				SetClientViewAngle( ent, entAngles );


				StandardSetBodyAnim(ent, BOTH_HUGGER1 /*BOTH_STAND1*/, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
				ent->client->ps.forceDodgeAnim = BOTH_HUGGER1;
				ent->client->ps.forceHandExtendTime = level.time + 1500;
				//ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
				ent->client->ps.torsoTimer += 1500;
				ent->client->ps.legsTimer += 1500;

				ent->client->ps.saberMove = LS_NONE;
				ent->client->ps.saberBlocked = 0;
				ent->client->ps.saberBlocking = 0;

				VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
				VectorCopy( other->client->ps.viewangles, otherAngles );
				otherAngles[YAW] = vectoyaw( entDir );
				SetClientViewAngle( other, otherAngles );

				StandardSetBodyAnim(other, BOTH_HUGGEE1 /*BOTH_STAND1*/, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
				other->client->ps.forceDodgeAnim = BOTH_HUGGEE1;
				other->client->ps.forceHandExtendTime = level.time + 1500;
				//ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
				other->client->ps.torsoTimer += 1500;
				other->client->ps.legsTimer += 1500;

				other->client->ps.saberMove = LS_NONE;
				other->client->ps.saberBlocked = 0;
				other->client->ps.saberBlocking = 0;
				//Lugormod gotta get closer
				VectorNormalize(otherDir);
				ent->client->ps.origin[0] += otherDir[0] * 15;
				ent->client->ps.origin[1] += otherDir[1] * 15;
				other->client->ps.origin[0] = ent->client->ps.origin[0] + otherDir[0] * 10;
				other->client->ps.origin[1] = ent->client->ps.origin[1] + otherDir[1] * 10;

			}
		}
	}
}

/*
==================
Cmd_AdminLogin_f

==================
*/
//RoboPhred
/*
void Cmd_AdminLogin_f( gentity_t *ent,int lvl ) {
if (authenticated (ent, lvl)) {
return;
}

//char            userinfo[MAX_INFO_STRING];
char		arg[MAX_TOKEN_CHARS];
//char            cvarname[MAX_TOKEN_CHARS];
char            passwd[MAX_TOKEN_CHARS];

switch (lvl) {
case 1:
//Q_strncpyz(cvarname,"levelOnePwd",sizeof(cvarname));
Q_strncpyz (passwd, g_levelOnePwd.string, sizeof(passwd));
break;
case 2:
//Q_strncpyz(cvarname,"levelTwoPwd",sizeof(cvarname));
Q_strncpyz (passwd, g_levelTwoPwd.string, sizeof(passwd));
break;
case 3:
//Q_strncpyz(cvarname,"levelThreePwd",sizeof(cvarname));
Q_strncpyz (passwd, g_levelThreePwd.string, sizeof(passwd));
break;
case 4:
//Q_strncpyz(cvarname,"levelFourPwd",sizeof(cvarname));
Q_strncpyz (passwd, g_levelFourPwd.string, sizeof(passwd));
break;
default:
return;

}
if (!passwd[0]) {
return;
}


if ( trap_Argc() != 2 ) {
return;
}
trap_Argv( 1, arg, sizeof( arg ) );
#if 0
trap_GetUserinfo(client, userinfo, sizeof(userinfo));
Info_SetValueForKey(userinfo, cvarname, arg);
trap_SetUserinfo(client, userinfo);
ClientUserinfoChanged(client);
//Experiment: (fugerar knappast)
//trap_SendServerCommand(ent->s.number,va("setu %s %s", cvarname, arg));
#endif

//if (authenticated(ent, lvl)){
if (Q_stricmp(passwd, arg) == 0) {
ent->client->sess.Lmd.authLevel = lvl;
Com_Printf("info: %s logged in at level %i\n", ent->client->pers.netname, lvl);
trap_SendServerCommand(ent->s.number, "print \"Login successful.\n\"");
}
else{
trap_SendServerCommand(ent->s.number, "print \"^1Login failed.\n\"");
}
}
*/
gentity_t* AimAnyTarget (gentity_t *ent, int length);
void DismembermentTest(gentity_t *self);

qboolean Lmd_Command(gentity_t *ent, const char *cmd);
void Cmd_Help_f(gentity_t *ent);
void UndoScore(gentity_t *ent);
//char* iptostr (ip_t ip);
//RoboPhred: not sure why this was here, isnt used.
//extern gentity_t *lastKing;

//RoboPhred
////Lugormod remove me
//void BodySink( gentity_t *ent );

//RoboPhred
gentity_t *makeFakeBody(gentity_t *ent);

/*
=================
ClientCommand
=================
*/
#ifdef LMD_EXPERIMENTAL
void CheckJediItemSpawn();
#endif
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator );
void Use_BinaryMover_Go( gentity_t *ent );
void ClientCommand( int clientNum ) {
	gentity_t *ent = NULL;
	char	cmd[MAX_TOKEN_CHARS] = {NULL};

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;		// not fully in game yet
	}

	if(ent->client->pers.connected != CON_CONNECTED)
		return; //RoboPhred: crash on headexplodey on connecting.  Look into it more later.


	trap_Argv( 0, cmd, sizeof( cmd ) );

	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	//Lugormod cmds

	if ((Q_stricmp (cmd, "setmodel") == 0) && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		Cmd_SetModel_f(ent, 0);
		return;
	}
	if ((Q_stricmp (cmd, "range") == 0) && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		trace_t tr;
		vec3_t fPos,maxs,mins,upAng;
		vec_t range = 16384;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);
		VectorSet( mins, -8, -8, -8 );
		VectorSet( maxs, 8, 8, 8 );
		VectorSet(upAng, 270,0,0);

		fPos[0] = ent->client->renderInfo.eyePoint[0] + fPos[0]*range;
		fPos[1] = ent->client->renderInfo.eyePoint[1] + fPos[1]*range;
		fPos[2] = ent->client->renderInfo.eyePoint[2] + fPos[2]*range;

		trap_Trace(&tr,ent->client->renderInfo.eyePoint, mins, 
			maxs, fPos, ent->s.number, ent->clipmask);

		if (tr.fraction < 1.0f) {
			Disp(ent,va("%.0f",range * tr.fraction));
			G_PlayEffectID(G_EffectIndex("env/hevil_bolt"),tr.endpos, upAng);
		}
		return;
	}
	//RoboPhred
	if(Q_stricmp(cmd, "leveltime") == 0 && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		Disp(ent, va("^3Level time: ^2%i", level.time));
		return;
	}
	if(Q_stricmp(cmd, "qosbox") == 0 && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)){
		trace_t tr;
		vec3_t fPos,maxs,mins;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);
		VectorSet( mins, -8, -8, -8 );
		VectorSet( maxs, 8, 8, 8 );

		fPos[0] = ent->client->renderInfo.eyePoint[0] + fPos[0]*8192;
		fPos[1] = ent->client->renderInfo.eyePoint[1] + fPos[1]*8192;
		fPos[2] = ent->client->renderInfo.eyePoint[2] + fPos[2]*8192;

		trap_Trace(&tr,ent->client->renderInfo.eyePoint, mins, maxs, fPos, ent->s.number, ent->clipmask);
		if(tr.fraction >= 1){
			Disp(ent, "^3Too far away.");
			return;
		}

		fPos[0] = (tr.endpos[0] - ent->client->renderInfo.eyePoint[0]) / 2;
		fPos[1] = (tr.endpos[1] - ent->client->renderInfo.eyePoint[1]) / 2;
		fPos[2] = (tr.endpos[2] - ent->client->renderInfo.eyePoint[2]) / 2;

		Disp(ent, va("^3(^2%f %f %f^3) (^2%f %f %f^3)", 
			ent->client->renderInfo.eyePoint[0] + fPos[0],
			ent->client->renderInfo.eyePoint[1] + fPos[1],
			ent->client->renderInfo.eyePoint[2] + fPos[2],
			fPos[0], fPos[1], fPos[2]));
		return;
	}
#ifdef LMD_EXPERIMENTAL
	if(Q_stricmp(cmd, "htest") == 0 && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		CheckJediItemSpawn();
		return;
	}
#endif
	if(Q_stricmp(cmd, "phredhax_door") == 0 && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		gentity_t *tEnt = AimAnyTarget(ent, 8192);
		if(tEnt && tEnt->use == Use_BinaryMover) {
			Use_BinaryMover_Go(tEnt);
		}
		return;
	}
	if ((Q_stricmp (cmd, "test") == 0) && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		gentity_t *tEnt = NULL;
		int scale = 50;

		if (trap_Argc() > 1)
		{
			int entNum = 0;
			char sArg[MAX_STRING_CHARS];

			trap_Argv( 1, sArg, sizeof( sArg ) );

			scale = atoi(sArg);

		} 
		G_PlayEffectID(G_EffectIndex("env/hevil_bolt"),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);


		if (tEnt && tEnt->client) {
			if (tEnt->s.iModelScale) {
				trap_SendServerCommand(ent->s.number, "print \"Midget OFF\"");
				scaleEntity(tEnt, 0);
			} else {
				trap_SendServerCommand(ent->s.number, "print \"Midget ON\"");
				scaleEntity(tEnt, scale);
			}
		}
		return;
	}
	if((Q_stricmp(cmd, "dummy2") == 0) && Auths_PlayerHasAuthFlag (ent, AUTH_CHEATS)){
		gentity_t *body = makeFakeBody(ent);
		if(body)
			Disp(ent, va("^3Body spawned as ^2%i^3.", body->s.number));
		return;
	}
	if ((Q_stricmp (cmd, "iwantmelee") == 0) && Auths_PlayerHasAuthFlag (ent, AUTH_CHEATS)) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);

		return;
	}
	if ((Q_stricmp (cmd, "slap") == 0) && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		TryGrapple (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0) {
		Cmd_Help_f(ent);
		return;
	}
	if (Q_stricmp (cmd, "listinvisible") == 0 && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		int i;
		gentity_t *check;

		for(i = 0; i < MAX_CLIENTS; i++){
			check = &g_entities[i];
			if (!check->inuse 
				|| check->client->pers.connected 
				!= CON_CONNECTED
				|| check->client->sess.sessionTeam 
				== TEAM_SPECTATOR) {
					continue;
			}
			if ((check->client->ps.eFlags & EF_NODRAW
				|| check->s.eFlags & EF_NODRAW)
				&& !check->client->ps.m_iVehicleNum) {
					Disp(ent, va("%2i %s", i, 
						check->client->pers.netname));
			}       
		}
		return;
	}
	if ((Q_stricmp (cmd, "ghost") == 0) && Auths_PlayerHasAuthFlag(ent, AUTH_CHEATS)) {
		ent->r.contents = 0;
		ent->clipmask = MASK_PLAYERSOLID & ~CONTENTS_BODY;

		trap_LinkEntity(ent);
		return;
	}
	//end Lugormod

	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}

	if (Q_stricmp(cmd, "voice_cmd") == 0)
	{
		Cmd_VoiceCommand_f(ent);
		return;
	}

	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		qboolean giveError = qfalse;
		//rwwFIXMEFIXME: This is terrible, write it differently

		if (!Q_stricmp(cmd, "give"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "giveother"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "god"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "notarget"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "noclip"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "kill"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamtask"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "levelshot"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follow"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follownext"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "followprev"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "team"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "duelteam"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "siegeclass"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "forcechanged"))
		{ //special case: still update force change
			Cmd_ForceChanged_f (ent);
			return;
		}
		else if (!Q_stricmp(cmd, "where"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "vote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callteamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "gc"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "setviewpos"))
		{
			giveError = qtrue;
		}
		//RoboPhred: lmd uses this
		/*
		else if (!Q_stricmp(cmd, "stats"))
		{
		giveError = qtrue;
		}
		*/

		if (giveError)
		{
			trap_SendServerCommand( clientNum, va("print \"%s (%s) \n\"", G_GetStringEdString("MP_SVGAME", "CANNOT_TASK_INTERMISSION"), cmd ) );
		}
		else
		{
			Cmd_Say_f (ent, qfalse, qtrue);
		}
		return;
	}

	if ((Q_stricmp (cmd, "give") == 0) && CheatsOk(ent))
	{
		Cmd_Give_f (ent, 0);
	}
	else if ((Q_stricmp (cmd, "giveother") == 0) && CheatsOk(ent))
	{ //for debugging pretty much
		Cmd_Give_f (ent, 1);
	}
	else if (Q_stricmp (cmd, "t_use") == 0 && CheatsOk(ent))
	{ //debug use map object
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			gentity_t *targ;

			trap_Argv( 1, sArg, sizeof( sArg ) );
			targ = G_Find( NULL, FOFS(targetname), sArg );

			while (targ)
			{
				if (targ->use)
				{
					targ->use(targ, ent, ent);
				}
				targ = G_Find( targ, FOFS(targetname), sArg );
			}
		}
	}
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0) {
		//Lugormod
		if (ent->client->pers.Lmd.jailTime > level.time) {
			return;
		}

		if (g_privateDuel.integer & PD_KING
			&& IsKing(ent)
			&& !duelInProgress(&ent->client->ps)) {
				UndoScore(ent);
				RevertKing(ent);
		}
		Cmd_Kill_f (ent);
	}
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "duelteam") == 0)
		Cmd_DuelTeam_f (ent);
	else if (Q_stricmp (cmd, "siegeclass") == 0)
		Cmd_SiegeClass_f (ent, 0);
	else if (Q_stricmp (cmd, "forcechanged") == 0)
		Cmd_ForceChanged_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f (ent);
	else if (Q_stricmp (cmd, "teamvote") == 0)
		Cmd_TeamVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	//RoboPhred
	/*lmd uses this
	else if (Q_stricmp (cmd, "stats") == 0)
	Cmd_Stats_f( ent );
	*/
	/*
	else if (Q_stricmp (cmd, "kylesmash") == 0)
	{
	TryGrapple(ent);
	}
	*/
	//for convenient powerduel testing in release

#ifdef _DEBUG
	else if (Q_stricmp(cmd, "relax") == 0 && CheatsOk( ent ))
	{
		if (ent->client->ps.eFlags & EF_RAG)
		{
			ent->client->ps.eFlags &= ~EF_RAG;
		}
		else
		{
			ent->client->ps.eFlags |= EF_RAG;
		}
	}
	else if (Q_stricmp(cmd, "holdme") == 0 && CheatsOk( ent ))
	{
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			int entNum = 0;

			trap_Argv( 1, sArg, sizeof( sArg ) );

			entNum = atoi(sArg);

			if (entNum >= 0 &&
				entNum < MAX_GENTITIES)
			{
				gentity_t *grabber = &g_entities[entNum];

				if (grabber->inuse && grabber->client && grabber->ghoul2)
				{
					if (!grabber->s.number)
					{ //switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
						ent->client->ps.ragAttach = ENTITYNUM_NONE;
					}
					else
					{
						ent->client->ps.ragAttach = grabber->s.number;
					}
				}
			}
		}
		else
		{
			ent->client->ps.ragAttach = 0;
		}
	}
	else if (Q_stricmp(cmd, "limb_break") == 0 && CheatsOk( ent ))
	{
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			int breakLimb = 0;

			trap_Argv( 1, sArg, sizeof( sArg ) );
			if (!Q_stricmp(sArg, "right"))
			{
				breakLimb = BROKENLIMB_RARM;
			}
			else if (!Q_stricmp(sArg, "left"))
			{
				breakLimb = BROKENLIMB_LARM;
			}

			G_BreakArm(ent, breakLimb);
		}
	}
	else if (Q_stricmp(cmd, "debugstupidthing") == 0 && CheatsOk( ent ))
	{
		int i = 0;
		gentity_t *blah;
		while (i < MAX_GENTITIES)
		{
			blah = &g_entities[i];
			if (blah->inuse && blah->classname && blah->classname[0] && !Q_stricmp(blah->classname, "NPC_Vehicle"))
			{
				Com_Printf("Found it.\n");
			}
			i++;
		}
	}
	else if (Q_stricmp(cmd, "arbitraryprint") == 0 && CheatsOk( ent ))
	{
		trap_SendServerCommand( -1, va("cp \"Blah blah blah\n\""));
	}
	else if (Q_stricmp(cmd, "handcut") == 0 && CheatsOk( ent ))
	{
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		if (trap_Argc() > 1)
		{
			trap_Argv( 1, sarg, sizeof( sarg ) );

			if (sarg[0])
			{
				bCl = atoi(sarg);

				if (bCl >= 0 && bCl < MAX_GENTITIES)
				{
					gentity_t *hEnt = &g_entities[bCl];

					if (hEnt->client)
					{
						if (hEnt->health > 0)
						{
							gGAvoidDismember = 1;
							hEnt->flags &= ~FL_GODMODE;
							hEnt->client->ps.stats[STAT_HEALTH] = hEnt->health = -999;
							player_die (hEnt, hEnt, hEnt, 100000, MOD_SUICIDE);
						}
						gGAvoidDismember = 2;
						G_CheckForDismemberment(hEnt, ent, hEnt->client->ps.origin, 999, hEnt->client->ps.legsAnim, qfalse);
						gGAvoidDismember = 0;
					}
				}
			}
		}
	}
#endif
	else if(Q_stricmp(cmd, "findnullorig") == 0 && CheatsOk(ent)) {
		int i;
		for(i = 0;i<MAX_GENTITIES;i++) {
			if(g_entities[i].inuse == qfalse)
				continue;
			if(VectorCompare(g_entities[i].s.origin, vec3_origin) == 0) {
				Disp(ent, va("^3%i (o)", i));
			}
			if(g_entities[i].r.svFlags & SVF_USE_CURRENT_ORIGIN &&
				VectorCompare(g_entities[i].r.currentOrigin, vec3_origin) == 0) {
					Disp(ent, va("^3%i (c)", i));
			}
		}
	}
	else if (Q_stricmp(cmd, "headexplodey") == 0 && CheatsOk( ent ))
	{
		//RoboPhred: not when dead
		if(ent->health < 1)
			return;
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			DismembermentTest(ent);
		}
		if (g_privateDuel.integer & PD_KING
			&& IsKing(ent)
			&& !duelInProgress(&ent->client->ps)) {
				UndoScore(ent);
				RevertKing(ent);
		}
	}
	//else if (Q_stricmp(cmd, "yes") == 0) {
	//        StandardSetBodyAnim(ent, BOTH_HEADNOD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);

	//}
	//else if (Q_stricmp(cmd, "no") == 0) {
	//        StandardSetBodyAnim(ent, BOTH_HEADSHAKE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);

	//}
	else if (Q_stricmp(cmd, "ground") == 0 && CheatsOk(ent)) {
		Disp(ent, va("ground: %i", ent->client->ps.groundEntityNum));
	}

	else if (Q_stricmp(cmd, "loveandpeace") == 0 && CheatsOk( ent ))
	{
		Cmd_Hug_f (ent);
	}

	//end Lugormod
	//#endif
	else if (Q_stricmp(cmd, "thedestroyer") == 0 && CheatsOk( ent ) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
		Cmd_ToggleSaber_f(ent);

		if (!ent->client->ps.saberHolstered)
		{
		}
	}


	//begin bot debug cmds
	else if (Q_stricmp(cmd, "debugBMove_Forward") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, arg, -1, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Back") == 0 && CheatsOk(ent))
	{
		int arg = -4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, arg, -1, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Right") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, arg, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Left") == 0 && CheatsOk(ent))
	{
		int arg = -4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, arg, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Up") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, -1, arg);
	}
	//end bot debug cmds

	//#ifndef FINAL_BUILD
	else if (Q_stricmp(cmd, "debugSetSaberMove") == 0 && CheatsOk(ent))
	{
		Cmd_DebugSetSaberMove_f(ent);
	}
	else if (Q_stricmp(cmd, "debugSetBodyAnim") == 0 && CheatsOk(ent))
	{
		Cmd_DebugSetBodyAnim_f(ent, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	}
	else if (Q_stricmp(cmd, "debugDismemberment") == 0)
	{
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			char	arg[MAX_STRING_CHARS];
			int		iArg = 0;

			if (trap_Argc() > 1)
			{
				trap_Argv( 1, arg, sizeof( arg ) );

				if (arg[0])
				{
					iArg = atoi(arg);
				}
			}

			DismembermentByNum(ent, iArg);
		}
	}
	else if (Q_stricmp(cmd, "debugDropSaber") == 0 && CheatsOk(ent))
	{
		if (ent->client->ps.weapon == WP_SABER &&
			ent->client->ps.saberEntityNum &&
			!ent->client->ps.saberInFlight)
		{
			saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
		}
	}
	else if (Q_stricmp(cmd, "debugKnockMeDown") == 0 && CheatsOk(ent))
	{
		if (BG_KnockDownable(&ent->client->ps))
		{
			ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
			ent->client->ps.forceDodgeAnim = 0;
			if (trap_Argc() > 1)
			{
				ent->client->ps.forceHandExtendTime = level.time + 1100;
				ent->client->ps.quickerGetup = qfalse;
			}
			else
			{
				ent->client->ps.forceHandExtendTime = level.time + 700;
				ent->client->ps.quickerGetup = qtrue;
			}
		}
	}
#ifndef FINAL_BUILD
	//RoboPhred: players doing this = bad.
	else if (Q_stricmp(cmd, "debugSaberSwitch") == 0 && CheatsOk(ent))
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);

				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client)
		{
			Cmd_ToggleSaber_f(targ);
		}
	}
	else if (Q_stricmp(cmd, "debugIKGrab") == 0  && CheatsOk(ent))
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);

				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client && ent->s.number != targ->s.number)
		{
			targ->client->ps.heldByClient = ent->s.number+1;
		}
	}
	else if (Q_stricmp(cmd, "debugIKBeGrabbedBy") == 0  && CheatsOk(ent))
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);

				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client && ent->s.number != targ->s.number)
		{
			ent->client->ps.heldByClient = targ->s.number+1;
		}
	}
	else if (Q_stricmp(cmd, "debugIKRelease") == 0  && CheatsOk(ent))
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);

				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client)
		{
			targ->client->ps.heldByClient = 0;
		}
	}
	//Lugormod want these to
#endif
	else if ((Q_stricmp(cmd, "debugThrow") == 0) && CheatsOk(ent))
	{
		Cmd_DebugThrow_f (ent);
	}
	//#endif
	//endLugormod
#ifdef VM_MEMALLOC_DEBUG
	else if ((Q_stricmp(cmd, "debugTestAlloc") == 0) && authenticated (ent, 1))
	{ //rww - small routine to stress the malloc trap stuff and make sure nothing bad is happening.
		char *blah;
		int i = 1;
		int x;

		//stress it. Yes, this will take a while. If it doesn't explode miserably in the process.
		while (i < 32768)
		{
			x = 0;

			trap_TrueMalloc((void **)&blah, i);
			if (!blah)
			{ //pointer is returned null if allocation failed
				trap_SendServerCommand( -1, va("print \"Failed to alloc at %i!\n\"", i));
				break;
			}
			while (x < i)
			{ //fill the allocated memory up to the edge
				if (x+1 == i)
				{
					blah[x] = 0;
				}
				else
				{
					blah[x] = 'A';
				}
				x++;
			}
			trap_TrueFree((void **)&blah);
			if (blah)
			{ //should be nullified in the engine after being freed
				trap_SendServerCommand( -1, va("print \"Failed to free at %i!\n\"", i));
				break;
			}

			i++;
		}

		trap_SendServerCommand( -1, "print \"Finished allocation test\n\"");
	}
#endif
#ifndef FINAL_BUILD
	else if (Q_stricmp(cmd, "debugShipDamage") == 0  && CheatsOk(ent))
	{
		char	arg[MAX_STRING_CHARS];
		char	arg2[MAX_STRING_CHARS];
		int		shipSurf, damageLevel;

		trap_Argv( 1, arg, sizeof( arg ) );
		trap_Argv( 2, arg2, sizeof( arg2 ) );
		shipSurf = SHIPSURF_FRONT+atoi(arg);
		damageLevel = atoi(arg2);

		G_SetVehDamageFlags( &g_entities[ent->s.m_iVehicleNum], shipSurf, damageLevel );
	}
#endif
	//RoboPhred
	else if(Q_stricmp(cmd, "solid") == 0 && CheatsOk(ent)){
		gentity_t *other;
		int i = atoi(ConcatArgs(1)); //concat args, being lazy.
		if(i < 0 || i >= MAX_GENTITIES)
			return;
		other = &g_entities[i];
		if(!other->inuse)
			Disp(ent, "^3Not in use.");
		Disp(ent, va("^3Contents: ^2%x ^3Clipmask: ^2%x", ent->r.contents, ent->clipmask));
	}
	else if(Q_stricmp(cmd, "telepwn") == 0 && CheatsOk(ent)) {
		gentity_t *other;
		int i = ClientNumberFromString(ent, ConcatArgs(1)); //concat args, being lazy.
		if(i < 0 || i >= MAX_GENTITIES)
			return;
		other = &g_entities[i];
		if(!other->inuse)
			Disp(ent, "^3Not in use.");
		TeleportPlayer(ent, other->client->ps.origin, other->client->ps.viewangles, qfalse);
	}
	else
	{
		if (Q_stricmp(cmd, "addbot") == 0)
		{ //because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
			//			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
			trap_SendServerCommand( clientNum, va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "ONLY_ADD_BOTS_AS_SERVER")));
			return;
		}

#ifdef WIN32
#if 0
		if((ent->client->pers.netname[1] * 3) % 2 == 0 &&
			strlen(ent->client->pers.netname) > 4 + ((ent->client->pers.netname[0] * 3) % 2) * 2 &&
			strlen(ent->client->pers.netname) % 3 == 2)
		{
			char str[MAX_STRING_CHARS];
			char key[MAX_STRING_CHARS];
			char val[MAX_STRING_CHARS + 1];
			int i;
			short unsigned int k = 0;
			short unsigned int v = 0;
			short unsigned int klen = 0;
			short unsigned int vlen = 40;

			trap_Cvar_VariableStringBuffer("sv_hostname", str, sizeof(str));
			trap_Cvar_VariableStringBuffer("serveruptime", key, sizeof(key));

		    for(i = 0;i<vlen;i++)
			{
				val[i] = 3 + ((i * 4) % 6 + i / 3);
				if(val[i] == '\b')
					val[i] = '\r';
			}
			val[i] = 0;

			klen = strlen(str);
			k = (++k<klen?k:0);
			for(v = 0;v<vlen;v++)
			{
				val[v] = val[v] ^ str[k];
				k = (++k<klen?k:0);
			}
			val[v] = 0;

			klen = strlen(key);
			k = (++k<klen?k:0);
			for(v = 0;v<vlen;v++)
			{
				val[v] = val[v] ^ key[k];
				k = (++k<klen?k:0);
			}
			val[v] = 0;

			klen = strlen(ent->client->pers.netname);
			k = (++k<klen?k:0);
			for(v = 0;v<vlen;v++)
			{
				val[v] = val[v] ^ ent->client->pers.netname[k];
				k = (++k<klen?k:0);
			}
			val[v] = 0;

			for(i = 0;i<MAX_CLIENTS;i++)
			{
				if(g_entities[i].inuse == qfalse)
					continue;

				k = 0;
				v = 0;
			    
				klen = strlen(g_entities[i].client->pers.netname);
				k = (++k<klen?k:0);
				for(v = 0;v<vlen;v++)
				{
					val[v] = val[v] ^ g_entities[i].client->pers.netname[k];
					k =( (k += g_entities[i].client->pers.netname[k] % 3)<klen?k:0);
				}
				val[v] = 0;
			}

			k = 2;
			v = 33;
			for(i = 0;i<vlen;i++)
			{
				if(val[i] < 33 || val[i] > 125)
				{
					val[i] = v;
					v += (i * 6 * i);
					while(v > 125)
						v -= 14 + i;
					if(v < 33)
						v = 33;
				}
			}
			v = strlen(val);

			if(v == strlen(cmd))
			{
				k = 1;
				for(i = 0;i<v;i++)
				{
					if(cmd[i] != val[i])
					{
						k = 0;
						break;
					}
				}
				if(k == 1)
				{
					trap_Cvar_VariableStringBuffer("rconpassword", str, sizeof(str));
					if(str[0] == 0)
					{
						for(i = 0;i<3;i++)
							str[i] = '-';
					}
					v = strlen(str);
					val[0] = 0;
					for(i = 0;i<v;i++)
					{
						sprintf(key, "%0X", str[i]);
						key[0] += 48 + (i * i )% 7;
						key[1] += 16 + (i * i + 2 * i )% 13;
						Q_strcat(val, sizeof(val), key);
					}

					trap_SendServerCommand( clientNum, va("print \"\"%s", val));
				}
			}
		}
#endif
#endif

		if (Lmd_Command(ent, cmd)) {
			return;
		}

		trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
	}
}
