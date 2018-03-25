// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "Lmd_IPs.h"

//Lugormod
//#define MAX_SESS_CHARS 2048

/*
=======================================================================

SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/

void Auths_WriteTempAdminSess(gclient_t *client);
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;
	const char	*ls;
	const char	*lvar;
	int			i = 0;
	char		siegeClass[64];
	char		saberType[64];
	char		saber2Type[64];

	strcpy(siegeClass, client->sess.siegeClass);

	while (siegeClass[i])
	{ //sort of a hack.. we don't want spaces by siege class names have spaces so convert them all to unused chars
		if (siegeClass[i] == ' ')
		{
			siegeClass[i] = 1;
		}

		i++;
	}

	if (!siegeClass[0])
	{ //make sure there's at least something
		strcpy(siegeClass, "none");
	}

	//Do the same for the saber
	strcpy(saberType, client->sess.saberType[0]);

	i = 0;
	while (saberType[i])
	{
		if (saberType[i] == ' ')
		{
			saberType[i] = 1;
		}

		i++;
	}

	strcpy(saber2Type, client->sess.saberType[1]);

	i = 0;
	while (saber2Type[i])
	{
		if (saber2Type[i] == ' ')
		{
			saber2Type[i] = 1;
		}

		i++;
	}

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s", 
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.setForce,
		client->sess.saberLevel,
		client->sess.selectedFP,
		client->sess.duelTeam,
		client->sess.siegeDesiredTeam,
		siegeClass,
		saberType,
		saber2Type);


	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );

	//Lugormod
	ls = va("%i %u.%u.%u.%u", client->sess.Lmd.id, client->sess.Lmd.ip[0], client->sess.Lmd.ip[1], 
		client->sess.Lmd.ip[2], client->sess.Lmd.ip[3]);
	lvar = va( "lsession%i", client - level.clients );
	trap_Cvar_Set( lvar, ls );

	//RoboPhred
	Auths_WriteTempAdminSess(client);

	//end Lugormod
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/

void Lmd_IPs_SetPlayerIP(gclient_t *client, IP_t ip);
void Auths_ReadTempAdminSess(gclient_t *client);
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	char	ls[MAX_STRING_CHARS];
	IP_t ip;

	const char *var;
	int i, j;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	//Lugormod
	var = va( "lsession%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, ls, sizeof(ls) );
	sscanf( ls, "%i %s", &client->sess.Lmd.id, s);
	if(Lmd_IPs_ParseIP(s, ip)) {
		Lmd_IPs_SetPlayerIP(client, ip);
	}

	//RoboPhred
	Auths_ReadTempAdminSess(client);

	var = va( "session%i", client - level.clients );

	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorTime,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,                   // bk010221 - format
		&client->sess.setForce,
		&client->sess.saberLevel,
		&client->sess.selectedFP,
		&client->sess.duelTeam,
		&client->sess.siegeDesiredTeam,
		&client->sess.siegeClass,
		&client->sess.saberType[0],
		&client->sess.saberType[1]);

	i = 0;
	while (client->sess.siegeClass[i])
	{ //convert back to spaces from unused chars, as session data is written that way.
		if (client->sess.siegeClass[i] == 1)
		{
			client->sess.siegeClass[i] = ' ';
		}

		i++;
	}

	for(i = 0; i < MAX_SABERS; i++) {
		j = 0;
		//And do the same for the saber type
		while (client->sess.saberType[i][j])
		{
			if (client->sess.saberType[i][j] == 1)
			{
				client->sess.saberType[i][j] = ' ';
			}

			j++;
		}
	}

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.saberDrawAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/

void Auths_AllocTempAdmin(clientSession_t *sess);
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot, qboolean newSession ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	//RoboPhred
	//If this is a new player (not a new session on an existing one), clear all the old data.
	if(!newSession)
		memset(sess, 0, sizeof(clientSession_t));

	client->sess.siegeDesiredTeam = TEAM_FREE;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer 
			|| g_gametype.integer == GT_BATTLE_GROUND) {
				sess->sessionTeam = PickTeam( -1 );
				BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			if (!isBot)
			{
				sess->sessionTeam = TEAM_SPECTATOR;	
			}
			else
			{ //Bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if (value[0] == 'r' || value[0] == 'R')
				{
					sess->sessionTeam = TEAM_RED;
				}
				else if (value[0] == 'b' || value[0] == 'B')
				{
					sess->sessionTeam = TEAM_BLUE;
				}
				else
				{
					sess->sessionTeam = PickTeam( -1 );
				}
				BroadcastTeamChange( client, -1 );
			}
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
						sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_DUEL:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_POWERDUEL:
				//sess->duelTeam = DUELTEAM_LONE; //default
				{
					int loners = 0;
					int doubles = 0;

					G_PowerDuelCount(&loners, &doubles, qtrue);

					if (!doubles || loners > (doubles/2))
					{
						sess->duelTeam = DUELTEAM_DOUBLE;
					}
					else
					{
						sess->duelTeam = DUELTEAM_LONE;
					}
				}
				sess->sessionTeam = TEAM_SPECTATOR;
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	sess->siegeClass[0] = 0;
	sess->saberType[0][0] = 0;
	sess->saberType[1][0] = 0;

	//RoboPhred
	Auths_AllocTempAdmin(sess);

	//end Lugormod

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
