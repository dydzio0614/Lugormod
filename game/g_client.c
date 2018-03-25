// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "../ghoul2/G2.h"
#include "bg_saga.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Professions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_IPs.h"
#include "Lmd_Bans.h"
#include "Lmd_Time.h"

#include "Lmd_Entities_Public.h"

qboolean PlayerUseableCheck(gentity_t *self, gentity_t *activator);

// g_client.c -- client functions that don't happen every frame

vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

extern int g_siegeRespawnCheck;

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void WP_SaberRemoveG2Model( gentity_t *saberent );
extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );

forcedata_t Client_Force[MAX_CLIENTS];
void BecomeCommoner (gentity_t * ent);

/*
qboolean
gameMode (int mode){
if (mode && (((7&mode) == 0) || ((7&g_gameMode.integer) == (7&mode)) || ((7&g_gameMode.integer) && ((7&mode) == 7))) &&
((g_gameMode.integer&(~7&mode))^(~7&mode)) == 0	&& (g_gametype.integer == GT_FFA || g_gametype.integer == GT_CTF || g_gametype.integer == GT_TEAM
|| g_gametype.integer == GT_CTY)){
return qtrue;
}
//Com_Printf("info: mode %i %i %i\n", 7&mode, 
//           7&g_gameMode.integer, 
//           (g_gameMode.integer&(~7&mode))^(~7&mode));

return qfalse;
}

*/
//Lugormod
qboolean gameMode (int mode) {
	if(!mode)
		return qfalse;

	if(g_gametype.integer != GT_FFA &&
		g_gametype.integer != GT_CTF &&
		g_gametype.integer != GT_TEAM &&
		g_gametype.integer == GT_CTY)
	{
		return qfalse;
	}

	int set = mode & GM_ANY; //gets the game modes (not the GMF_ ones).
	int selected = g_gameMode.integer & GM_ANY;//
	if(set != 0 && selected != set && selected == 0 && set != GM_ANY)
		return qfalse;

	set = mode & ~GM_ANY; //remove the game modes to get the flags.
	if(((g_gameMode.integer & mode)^(mode)) != 0)
		return qfalse;

	return qtrue;
	
	//RoboPhred: lugor's origional function
	/*
	if (mode && (((7&mode) == 0) || ((7&g_gameMode.integer) == (7&mode)) || ((7&g_gameMode.integer) && ((7&mode) == 7))) 
		&& ((g_gameMode.integer&(~7&mode))^(~7&mode)) == 0 && (g_gametype.integer == GT_FFA || g_gametype.integer == GT_CTF 
		|| g_gametype.integer == GT_TEAM || g_gametype.integer == GT_CTY)) {
			return qtrue;
	} 
	return qfalse;
	*/
}

//qboolean authenticated (gentity_t *ent, int lvl);
//char* iptostr (ip_t ip);

/*QUAKED info_player_duel (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for duelists in duel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
//RoboPhred
extern qboolean disablesenabled;//Lugormod
void SP_info_player_deathmatch (gentity_t *ent);
qboolean thereIsAPlayerSpawn(void){
	int i = MAX_CLIENTS;
	gentity_t *ent;
	while (i++ < MAX_GENTITIES) {
		ent = &g_entities[i];
		if (!ent->inuse) {
			continue;
		}
		if (Q_stricmp("info_player_deathmatch", ent->classname) != 0 || !Lmd_Entities_IsSaveable(ent))
			continue;
		return qtrue;
	}
	return qfalse;
}
void SP_trigger_always (gentity_t *ent); //Lugormod

//RoboPhred
const entityInfoData_t info_player_duel_keys[] = {
	{"Target", "Target is fired when a player spawns at this spawn point."},
	{"NoHumans", "Do not let human players spawn on this spawn point."},
	{"NoBots", "Do not let bots spawn on this spawn point."},
	{NULL, NULL},
};
entityInfo_t info_player_duel_info = {
	"For use in the duel game type.  A duelist will spawn here.",
	NULL,
	info_player_duel_keys
};
void SP_info_player_duel( gentity_t *ent )
{
	int		i;

	if(g_gametype.integer == GT_FFA){
		//RoboPhred: can no longer detect non-default ents this way
		/*
		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		*/
		/*Spawn normally as a duel point, for compliance with maps expecting basejka.
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		*/
	}

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel1 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for lone duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
//RoboPhred
const entityInfoData_t info_player_duel1_keys[] = {
	{"Target", "Target is fired when a player spawns at this spawn point."},
	{"NoHumans", "Do not let human players spawn on this spawn point."},
	{"NoBots", "Do not let bots spawn on this spawn point."},
	{NULL, NULL},
};
entityInfo_t info_player_duel1_info = {
	"For use in the power duel game type.  The staff duelist will spawn here.",
	NULL,
	info_player_duel1_keys
};
void SP_info_player_duel1( gentity_t *ent )
{
	int		i;

	//RoboPhred
	if(g_gametype.integer == GT_FFA){
		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		/*Spawn normally as a duel point, for compliance with maps expecting basejka.
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		*/
	}

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel2 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for paired duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
//RoboPhred
const entityInfoData_t info_player_duel2_keys[] = {
	{"Target", "Target is fired when a player spawns at this spawn point."},
	{"NoHumans", "Do not let human players spawn on this spawn point."},
	{"NoBots", "Do not let bots spawn on this spawn point."},
	{NULL, NULL},
};
entityInfo_t info_player_duel2_info = {
	"For use in the power duel game type.  The paired saber duelists will spawn here.",
	NULL,
	info_player_duel2_keys
};
void SP_info_player_duel2( gentity_t *ent )
{
	int		i;

	//RoboPhred
	if(g_gametype.integer == GT_FFA){
		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		/*Spawn normally as a duel point, for compliance with maps expecting basejka.
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		*/
	}

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
extern qboolean disablesenabled;//Lugormod
//RoboPhred
const entityInfoData_t info_player_deathmatch_spawnflags[] = {
	{"1", "Initial: The first time the player enters the game (from connection or a team switch), they will spawn here."},
	{NULL, NULL}
};
const entityInfoData_t info_player_deathmatch_keys[] = {
	{"Target", "Target is fired when a player spawns at this spawn point."},
	{"NoHumans", "Do not let human players spawn on this spawn point."},
	{"NoBots", "Do not let bots spawn on this spawn point."},
	{NULL, NULL},
};
entityInfo_t info_player_deathmatch_info = {
	"For use in the power duel game type.  The paired saber duelists will spawn here.",
	info_player_deathmatch_spawnflags,
	info_player_deathmatch_keys
};
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Targets will be fired when someone spawns in on them.
equivelant to info_player_deathmatch
*/

//RoboPhred
entityInfo_t info_player_start_info = {
	"Deprecated.  Use info_player_deathmatch instead.",
	NULL,
	NULL
};
void SP_info_player_start(gentity_t *ent) {
	//RoboPhred: no longer loading lmd ents first, both are combined.
	/*
	if (//g_gametype.integer != GT_SIEGE && //to be on the safe side
		disablesenabled &&
		//this works because I load lugormod entities first.
		thereIsAPlayerSpawn()) { //Lugormod

			if (ent->target && ent->target[0]) {
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
	}
	*/
	G_Free(ent->classname);

	//RoboPhred
	Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "info_player_deathmatch");

	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

//RoboPhred
const entityInfoData_t info_player_jail_keys[] = {
	{"Target", "Target is fired when a player spawns at this spawn point."},
	{NULL, NULL},
};
entityInfo_t info_player_jail_info = {
	"Jailed players will spawn here.",
	NULL,
	info_player_jail_keys
};
void SP_info_player_jail(gentity_t *ent) {
	//G_Free(ent->classname);
	//ent->classname = "info_player_jail";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_red (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Red Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_red(gentity_t *ent) {
	//RoboPhred
	if(g_gametype.integer == GT_FFA){
		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
	}
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_blue (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Blue Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_blue(gentity_t *ent) {
	if(g_gametype.integer == GT_FFA){
		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
	}
	SP_info_player_deathmatch( ent );
}

void SiegePointUse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	//Toggle the point on/off
	if (self->genericValue1)
	{
		self->genericValue1 = 0;
	}
	else
	{
		self->genericValue1 = 1;
	}
}

/*QUAKED info_player_siegeteam1 (1 0 0) (-16 -16 -24) (16 16 32)
siege start point - team1
name and behavior of team1 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam1(gentity_t *ent) {
	int soff = 0;

	if (g_gametype.integer != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		//RoboPhred
		if (//g_gametype.integer != GT_SIEGE && //to be on the safe side
			disablesenabled &&
			//this works because I load lugormod entities first.
			thereIsAPlayerSpawn()) { //Lugormod

				if (ent->target && ent->target[0]) {
					gentity_t *trig;
					if (trig = G_Spawn()) {
						trig->classname = "trigger_always";
						trig->target = G_NewString(ent->target);
						SP_trigger_always(trig);
					}
				}

				G_FreeEntity(ent);
				return;
		}
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		/*
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
		*/
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_siegeteam2 (0 0 1) (-16 -16 -24) (16 16 32)
siege start point - team2
name and behavior of team2 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam2(gentity_t *ent) {
	int soff = 0;

	if (g_gametype.integer != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		//RoboPhred

		if (disablesenabled && thereIsAPlayerSpawn()) { //Lugormod
			if(ent->target && ent->target[0]){
				gentity_t *trig;
				if (trig = G_Spawn()) {
					trig->classname = "trigger_always";
					trig->target = G_NewString(ent->target);
					SP_trigger_always(trig);
				}
			}

			G_FreeEntity(ent);
			return;
		}
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
		/*
		G_Free(ent->classname);
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
		*/
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) RED BLUE
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
RED - In a Siege game, the intermission will happen here if the Red (attacking) team wins
BLUE - In a Siege game, the intermission will happen here if the Blue (defending) team wins
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_red (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Red (attacking) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_red( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_blue (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Blue (defending) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_blue( gentity_t *ent ) {

}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent = &g_entities[self->client->ps.saberIndex];
	vec3_t a;
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{ //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{ //someone killed us and we had the saber thrown, so actually move this saber to the saber location
		//if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
		//apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	WP_SaberAddG2Model( ent, self->client->saber[0].model, self->client->saber[0].skin );

	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
		return;
	}

	if (!altVelocity)
	{
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0]*256;
		ent->s.pos.trDelta[1] = a[1]*256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap_LinkEntity(ent);
}

void JMSaberThink(gentity_t *ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{
		if (!ent->enemy->client || !ent->enemy->inuse)
		{ //disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2[0] = 1;
			ent->pos2[1] = 0; //respawn next think
			trap_LinkEntity(ent);
		}
		else
		{
			ent->pos2[1] = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->pos2[0] && ent->pos2[1] < level.time)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace)

{
	int i = 0;
	//	gentity_t *te;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (other->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master 
	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, va("%i", other->s.number ) );

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap_SendServerCommand( -1, va("cp \"%s %s\n\"", other->client->pers.netname, G_GetStringEdString("MP_SVGAME", "BECOMEJM")) );

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < g_jmstarthealth.integer && other->health > 0)
	{ //full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = g_jmstarthealth.integer;
		if (g_jmhealthbar.integer & 1) {

			//I want a health bar for the jedi
			other->maxHealth = g_jmstarthealth.integer;
			//It must be removed when the jedi dies
			//I'll do that in g_combat.c
			//Don't know if that's stupid.
		}


	}

	if (other->client->ps.fd.forcePower < 100)
	{
		other->client->ps.fd.forcePower = 100;
	}

	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = g_jmforcelevel.integer;

		i++;
	}

	if (g_jmkillhealth.integer > 0) {
		//No armor for the master
		other->client->ps.stats[STAT_ARMOR] = 0;
		//No heal if healthreward
		other->client->ps.fd.forcePowersKnown &= ~(1 << FP_HEAL);
		other->client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
	}



	self->pos2[0] = 1;
	self->pos2[1] = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue(self->s.number);

	return;
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t *ent)
{
	if (g_gametype.integer != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}
	if (!ent) { //Lugormod
		gJMSaberEnt = NULL;
		return;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( ent->r.maxs, 3, 3, 3 );
	VectorSet( ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot
	ent->touch = JMSaberTouch;

	trap_LinkEntity(ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

SelectSpawnPoint

=======================================================================
*/

//This should probably be here, but it's in g_main.c with alot of copied code

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest ) 
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( dest, mover->r.mins, mins );
	VectorAdd( dest, mover->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = &g_entities[touch[i]];
		if ( hit == mover )
		{
			continue;
		}

		if ( hit->r.contents & mover->r.contents )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	char *classname = NULL;

	if (ent->client->pers.Lmd.jailTime > level.time)
		classname = "info_player_jail";
	else if ( ent->client->sess.sessionTeam == TEAM_RED )
		classname = "info_player_start_red";
	else if (ent->client->sess.sessionTeam == TEAM_BLUE)
		classname = "info_player_start_blue";
	else
		classname = "info_player_deathmatch";

	int tries;

	for(tries = 0; tries < 3; tries++) {

		if(tries == 1) {
			classname = "info_player_deathmatch";
		}

		while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {

			if(tries < 2) {
				
				//Ufo: bad operands
				if(ent->r.svFlags & SVF_BOT && spot->flags & FL_NO_BOTS)
					continue;
				else if(!(ent->r.svFlags & SVF_BOT) && spot->flags & FL_NO_HUMANS)
					continue;

				if(SpotWouldTelefrag(spot))
					continue;
				if(!PlayerUseableCheck(spot, ent))
					continue;
			}

			VectorSubtract( spot->s.origin, ent->r.currentOrigin, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					if (numSpots > 64)
						numSpots = 64;
					break;
				}
			}
			if (i >= numSpots && numSpots < 64) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
		if(numSpots > 0) {
			// select a random spot from the spawn points furthest away
			rnd = (int)(random() * (numSpots / 2));

			VectorCopy (list_spot[rnd]->s.origin, origin);
			origin[2] += 9;
			VectorCopy (list_spot[rnd]->s.angles, angles);

			return list_spot[rnd];
		}
		else if(tries == 2) {
			//End of the last try and no spot found.  Fall back to the origin.
			VectorCopy(vec3_origin, origin);
			VectorCopy(vec3_origin, angles);
			return NULL;
		}
	}
	//Not needed
	VectorCopy(vec3_origin, origin);
	VectorCopy(vec3_origin, angles);
	return NULL;
}

gentity_t *SelectDuelSpawnPoint( int team, vec3_t avoidPoint, vec3_t origin, vec3_t angles )
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;
	char		*spotName;

	if (team == DUELTEAM_LONE)
	{
		spotName = "info_player_duel1";
	}
	else if (team == DUELTEAM_DOUBLE)
	{
		spotName = "info_player_duel2";
	}
	else if (team == DUELTEAM_SINGLE)
	{
		spotName = "info_player_duel";
	}
	else
	{
		spotName = "info_player_deathmatch";
	}
tryAgain:

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), spotName)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots)
	{
		if (Q_stricmp(spotName, "info_player_deathmatch"))
		{ //try the loop again with info_player_deathmatch as the target if we couldn't find a duel spot
			spotName = "info_player_deathmatch";
			goto tryAgain;
		}

		//If we got here we found no free duel or DM spots, just try the first DM spot
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = (int)(random() * (numSpots / 2));

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles) {
	return SelectRandomFurthestSpawnPoint( ent, origin, angles);

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
	// roll again if it would be real close to point of death
	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
	// last try
	spot = SelectRandomDeathmatchSpawnPoint ( );
	}		
	}

	// find a single player start spot
	if (!spot) {
	G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( ent, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_SINK_TIME		30000//45000

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > BODY_SINK_TIME + 2500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;	
	}
	//	ent->nextthink = level.time + 100;
	//	ent->s.pos.trBase[2] -= 1;

	G_AddEvent(ent, EV_BODYFADE, 0);
	ent->nextthink = level.time + 18000;
	ent->takedamage = qfalse;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
static qboolean CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;
	int			islight = 0;

	if (level.intermissiontime)
	{
		return qfalse;
	}

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return qfalse;
	}

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{ //for now, just don't spawn a body if you got disint'd
		return qfalse;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.loopIsSoundset = qfalse;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		body->s.weapon = WP_BLASTER; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	//G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.clientNum);
	//Now doing this through a modified version of the rcg reliable command.
	if (ent->client && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
	{
		islight = 1;
	}
	trap_SendServerCommand(-1, va("ircg %i %i %i %i", ent->s.number, body->s.number, body->s.weapon, islight));

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim;

	body->s.customRGBA[0] = ent->client->ps.customRGBA[0];
	body->s.customRGBA[1] = ent->client->ps.customRGBA[1];
	body->s.customRGBA[2] = ent->client->ps.customRGBA[2];
	body->s.customRGBA[3] = ent->client->ps.customRGBA[3];

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_TIME;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);

	return qtrue;
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

void MaintainBodyQueue(gentity_t *ent)
{ //do whatever should be done taking ragdoll and dismemberment states into account.
	qboolean doRCG = qfalse;

	assert(ent && ent->client);
	if (ent->client->tempSpectate > level.time ||
		(ent->client->ps.eFlags2 & EF2_SHIP_DEATH))
	{
		ent->client->noCorpse = qtrue;
	}

	if (!ent->client->noCorpse && !ent->client->ps.fallingToDeath)
	{
		if (!CopyToBodyQue (ent))
		{
			doRCG = qtrue;
		}
	}
	else
	{
		ent->client->noCorpse = qfalse; //clear it for next time
		ent->client->ps.fallingToDeath = qfalse;
		doRCG = qtrue;
	}

	if (doRCG)
	{ //bodyque func didn't manage to call ircg so call this to assure our limbs and ragdoll states are proper on the client.
		trap_SendServerCommand(-1, va("rcg %i", ent->s.clientNum));
	}
}

/*
================
respawn
================
*/
//void returnGhost (team_t team);

void SiegeRespawn(gentity_t *ent);
void respawn( gentity_t *ent ) {
	MaintainBodyQueue(ent);

	if (gEscaping || g_gametype.integer == GT_POWERDUEL)
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;

		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		ent->client->sess.spectatorTime = level.time;
		ClientSpawn(ent);
		ent->client->iAmALoser = qtrue;
		return;
	}

	trap_UnlinkEntity (ent);

	if (g_gametype.integer == GT_SIEGE)
	{
		if (g_siegeRespawn.integer)
		{
			if (ent->client->tempSpectate <= level.time)
			{
				int minDel = g_siegeRespawn.integer* 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				ent->client->tempSpectate = level.time + minDel;
				ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
				ent->client->ps.weapon = WP_NONE;
				ent->client->ps.stats[STAT_WEAPONS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				ent->takedamage = qfalse;
				trap_LinkEntity(ent);

				// Respawn time.
				if ( ent->s.number < MAX_CLIENTS )
				{
					gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
					te->s.time = g_siegeRespawnCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		}
		SiegeRespawn(ent);
	}
	else if (g_gametype.integer == GT_BATTLE_GROUND) {

		if (ent->client->tempSpectate <= level.time)
		{
			ent->client->tempSpectate = level.time + 40000;
			ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
			ent->client->ps.weapon = WP_NONE;
			ent->client->ps.stats[STAT_WEAPONS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			ent->takedamage = qfalse;
			trap_LinkEntity(ent);

			// Respawn time.
			if ( ent->s.number < MAX_CLIENTS )
			{
				gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
				te->s.time = g_siegeRespawnCheck;
				te->s.owner = ent->s.number;
			}

			return;
		}

		SiegeRespawn(ent);

	}
	/*
	else if (g_gametype.integer == GT_REBORN
	&& ent->client->sess.sessionTeam == TEAM_RED) {

	if (ent->client->tempSpectate <= level.time)
	{
	ent->client->tempSpectate = level.time + 40000;
	ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
	ent->client->ps.weapon = WP_NONE;
	ent->client->ps.stats[STAT_WEAPONS] = 0;
	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
	ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	ent->takedamage = qfalse;
	trap_LinkEntity(ent);

	// Respawn time.
	if ( ent->s.number < MAX_CLIENTS )
	{
	gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
	te->s.time = g_siegeRespawnCheck;
	te->s.owner = ent->s.number;
	}

	return;
	}

	ClientSpawn(ent);

	}
	*/
	else if (g_gametype.integer == GT_GHOST) {

		if (ent->client->tempSpectate <= level.time)
		{

			//returnGhost(ent->client->sess.sessionTeam);
			//if (--(level.teamScores[ent->client->sess.sessionTeam])
			//    < 0) {
			//        level.teamScores[ent->client->sess.sessionTeam] = 0;
			//}
			ent->client->tempSpectate = level.time + 70000;
			ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
			ent->client->ps.weapon = WP_NONE;
			ent->client->ps.stats[STAT_WEAPONS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			ent->takedamage = qfalse;
			trap_LinkEntity(ent);

			//Respawn time.
			if ( ent->s.number < MAX_CLIENTS )
			{
				gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
				te->s.time = g_siegeRespawnCheck;
				te->s.owner = ent->s.number;
			}

			return;
		}

		ClientSpawn(ent);

	}

	else if (gameMode(GMF_RESPAWN_TIMER)) {

		if (ent->client->tempSpectate <= level.time)
		{
			ent->client->tempSpectate = level.time + 40000;
			ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
			ent->client->ps.weapon = WP_NONE;
			ent->client->ps.stats[STAT_WEAPONS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			ent->takedamage = qfalse;
			trap_LinkEntity(ent);

			// Respawn time.
			if ( ent->s.number < MAX_CLIENTS )
			{
				gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
				te->s.time = g_siegeRespawnCheck;
				te->s.owner = ent->s.number;
			}

			return;
		}

		ClientSpawn(ent);

	}
	else
	{
		gentity_t	*tent;

		ClientSpawn(ent);

		// add a teleportation effect
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		//RoboPhred
		if(tent)
			tent->s.clientNum = ent->s.clientNum;
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
		else if (g_gametype.integer == GT_SIEGE &&
			level.clients[i].sess.siegeDesiredTeam == team)
		{
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
char *p;

if ((p = Q_strrchr(model, '/')) != 0) {
*p = 0;
}

Q_strcat(model, MAX_QPATH, "/");
Q_strcat(model, MAX_QPATH, skin);
}
*/


/*
===========
ClientCheckName
============
*/
extern vmCvar_t g_allowBlackNames;

//RoboPhred
void ClientCleanName( const char *in, char *out, int outSize ) {
	//static void ClientCleanName( const char *in, char *out, int outSize ) {
	int		len, colorlessLen;
	char	ch;
	char	*p;
	//Lugormod don't allow leading spaces
	int		spaces = 1;
	//int             spaces;
	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}
		//Lugormod found a better way.
		// don't allow leading spaces
		// if( !*p && ch == ' ' ) {
		//	continue;
		//}N
		if(*in == '\n') {
			in++;
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE && *in >= '0' && *in <= '9') {
			// solo trailing carat is not a color prefix
			//if( !*in ) {
			//	break;
			//}

			// don't allow black in a name, period
			if( !g_allowBlackNames.integer //Lugormod
				//&& ColorIndex(*in) == 0 ) {
				&& (*in == '0' || *in > '7')) {
					in++;
					continue;
			}

			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}                

		// don't allow too many consecutive spaces
		if( ch == ' ') {
			spaces++;
			//Lugormod allow only one.
			if( spaces > 1) {//3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
			//Lugormod only count non space non colorcode chars.
			colorlessLen++;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		//Lugormod I don't want 'only space' names.
		//colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "Padawan", outSize );
	}
}

#ifdef _DEBUG
void G_DebugWrite(const char *path, const char *text)
{
	fileHandle_t f;

	trap_FS_FOpenFile( path, &f, FS_APPEND );
	trap_FS_Write(text, strlen(text), f);
	trap_FS_FCloseFile(f);
}
#endif

qboolean G_SaberModelSetup(gentity_t *ent)
{
	int i = 0;
	qboolean fallbackForSaber = qtrue;

	while (i < MAX_SABERS)
	{
		if (ent->client->saber[i].model[0])
		{
			//first kill it off if we've already got it
			if (ent->client->weaponGhoul2[i])
			{
				trap_G2API_CleanGhoul2Models(&(ent->client->weaponGhoul2[i]));
			}
			trap_G2API_InitGhoul2Model(&ent->client->weaponGhoul2[i], ent->client->saber[i].model, 0, 0, -20, 0, 0);

			if (ent->client->weaponGhoul2[i])
			{
				int j = 0;
				char *tagName;
				int tagBolt;

				if (ent->client->saber[i].skin)
				{
					trap_G2API_SetSkin(ent->client->weaponGhoul2[i], 0, ent->client->saber[i].skin, ent->client->saber[i].skin);
				}

				if (ent->client->saber[i].saberFlags & SFL_BOLT_TO_WRIST)
				{
					trap_G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, 3+i);
				}
				else
				{ // bolt to right hand for 0, or left hand for 1
					trap_G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, i);
				}

				//Add all the bolt points
				while (j < ent->client->saber[i].numBlades)
				{
					tagName = va("*blade%i", j+1);
					tagBolt = trap_G2API_AddBolt(ent->client->weaponGhoul2[i], 0, tagName);

					if (tagBolt == -1)
					{
						if (j == 0)
						{ //guess this is an 0ldsk3wl saber
							tagBolt = trap_G2API_AddBolt(ent->client->weaponGhoul2[i], 0, "*flash");
							fallbackForSaber = qfalse;
							break;
						}

						if (tagBolt == -1)
						{
							assert(0);
							break;

						}
					}
					j++;

					fallbackForSaber = qfalse; //got at least one custom saber so don't need default
				}

				//Copy it into the main instance
				trap_G2API_CopySpecificGhoul2Model(ent->client->weaponGhoul2[i], 0, ent->ghoul2, i+1); 
			}
		}
		else
		{
			break;
		}

		i++;
	}

	return fallbackForSaber;
}

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side 
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally 
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void *g2SaberInstance = NULL;

#include "../namespace_begin.h"
qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName);
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors );
void BG_GetVehicleModelName(char *modelname);
#include "../namespace_end.h"

void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName)
{
	int handle;
	char		afilename[MAX_QPATH];
#if 0
	char		/**GLAName,*/ *slash;
#endif
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
	}

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		int defSkin;

		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}

		defSkin = trap_R_RegisterSkin("models/players/kyle/model_default.skin");
		trap_G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
	}

	if (precachedKyle && trap_G2_HaveWeGhoul2Models(precachedKyle))
	{
		if (d_perPlayerGhoul2.integer || ent->s.number >= MAX_CLIENTS ||
			G_PlayerHasCustomSkeleton(ent))
		{ //rww - allow option for perplayer models on server for collision and bolt stuff.
			char modelFullPath[MAX_QPATH];
			char truncModelName[MAX_QPATH];
			char skin[MAX_QPATH];
			char vehicleName[MAX_QPATH];
			int skinHandle = 0;
			int i = 0;
			char *p;

			// If this is a vehicle, get it's model name.
			if ( ent->client->NPC_class == CLASS_VEHICLE )
			{
				strcpy(vehicleName, modelname);
				BG_GetVehicleModelName(modelname);
				strcpy(truncModelName, modelname);
				skin[0] = 0;
				if ( ent->m_pVehicle
					&& ent->m_pVehicle->m_pVehicleInfo
					&& ent->m_pVehicle->m_pVehicleInfo->skin
					&& ent->m_pVehicle->m_pVehicleInfo->skin[0] )
				{
					skinHandle = trap_R_RegisterSkin(va("models/players/%s/model_%s.skin", modelname, ent->m_pVehicle->m_pVehicleInfo->skin));
				}
				else
				{
					skinHandle = trap_R_RegisterSkin(va("models/players/%s/model_default.skin", modelname));
				}
			}
			else
			{
				if (skinName && skinName[0])
				{
					strcpy(skin, skinName);
					strcpy(truncModelName, modelname);
				}
				else
				{
					strcpy(skin, "default");

					strcpy(truncModelName, modelname);
					p = Q_strrchr(truncModelName, '/');

					if (p)
					{
						*p = 0;
						p++;

						while (p && *p)
						{
							skin[i] = *p;
							i++;
							p++;
						}
						skin[i] = 0;
						i = 0;
					}

					if (!BG_IsValidCharacterModel(truncModelName, skin))
					{
						strcpy(truncModelName, "kyle");
						strcpy(skin, "default");
					}

					if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_SIEGE && !g_jediVmerc.integer && !gameMode(GM_ALLWEAPONS))
					{
						BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, NULL );
					}
					else if (g_gametype.integer == GT_SIEGE)
					{ //force skin for class if appropriate
						if (ent->client->siegeClass != -1)
						{
							siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];
							if (scl->forcedSkin[0])
							{
								strcpy(skin, scl->forcedSkin);
							}
						}
					}
				}
			}

			if (skin[0])
			{
				char *useSkinName;

				if (strchr(skin, '|'))
				{//three part skin
					useSkinName = va("models/players/%s/|%s", truncModelName, skin);
				}
				else
				{
					useSkinName = va("models/players/%s/model_%s.skin", truncModelName, skin);
				}

				skinHandle = trap_R_RegisterSkin(useSkinName);
			}

			strcpy(modelFullPath, va("models/players/%s/model.glm", truncModelName));
			handle = trap_G2API_InitGhoul2Model(&ent->ghoul2, modelFullPath, 0, skinHandle, -20, 0, 0);

			if (handle<0)
			{ //Huh. Guess we don't have this model. Use the default.

				if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
				{
					trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
				}
				ent->ghoul2 = NULL;
				trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
			}
			else
			{
				trap_G2API_SetSkin(ent->ghoul2, 0, skinHandle, skinHandle);

				GLAName[0] = 0;
				trap_G2API_GetGLAName( ent->ghoul2, 0, GLAName);

				if (!GLAName[0] || (!strstr(GLAName, "players/_humanoid/") && ent->s.number < MAX_CLIENTS && !G_PlayerHasCustomSkeleton(ent)))
				{ //a bad model
					trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
					ent->ghoul2 = NULL;
					trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
				}

				if (ent->s.number >= MAX_CLIENTS)
				{
					ent->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

					if (skin[0])
					{ //append it after a *
						strcat( modelFullPath, va("*%s", skin) );
					}

					if ( ent->client->NPC_class == CLASS_VEHICLE )
					{ //vehicles are tricky and send over their vehicle names as the model (the model is then retrieved based on the vehicle name)
						ent->s.modelindex = G_ModelIndex(vehicleName);
					}
					else
					{
						ent->s.modelindex = G_ModelIndex(modelFullPath);
					}
				}
			}
		}
		else
		{
			trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
		}
	}
	else
	{
		return;
	}

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap_G2API_AttachInstanceToEntNum(ent->ghoul2, ent->s.number, qtrue);

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		if (BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf( "Failed to load humanoid animation file\n");
			return;
		}
	}

	if (ent->s.number >= MAX_CLIENTS || G_PlayerHasCustomSkeleton(ent))
	{
		ent->localAnimIndex = -1;

		GLAName[0] = 0;
		trap_G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (GLAName[0] &&
			!strstr(GLAName, "players/_humanoid/") /*&&
												   !strstr(GLAName, "players/rockettrooper/")*/)
		{ //it doesn't use humanoid anims.
			char *slash = Q_strrchr( GLAName, '/' );
			if ( slash )
			{
				strcpy(slash, "/animation.cfg");

				ent->localAnimIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
			}
		}
		else
		{ //humanoid index.
			if (strstr(GLAName, "players/rockettrooper/"))
			{
				ent->localAnimIndex = 1;
			}
			else
			{
				ent->localAnimIndex = 0;
			}
		}

		if (ent->localAnimIndex == -1)
		{
			Com_Error(ERR_DROP, "NPC had an invalid GLA\n");
		}
	}
	else
	{
		GLAName[0] = 0;
		trap_G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (strstr(GLAName, "players/rockettrooper/"))
		{
			//assert(!"Should not have gotten in here with rockettrooper skel");
			ent->localAnimIndex = 1;
		}
		else
		{
			ent->localAnimIndex = 0;
		}
	}

	if (ent->s.NPC_class == CLASS_VEHICLE &&
		ent->m_pVehicle)
	{ //do special vehicle stuff
		char strTemp[128];
		int i;

		// Setup the default first bolt
		i = trap_G2API_AddBolt( ent->ghoul2, 0, "model_root" );

		// Setup the droid unit.
		ent->m_pVehicle->m_iDroidUnitTag = trap_G2API_AddBolt( ent->ghoul2, 0, "*droidunit" );

		// Setup the Exhausts.
		for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
		{
			Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
			ent->m_pVehicle->m_iExhaustTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
		}

		// Setup the Muzzles.
		for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
		{
			Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
			ent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
			if ( ent->m_pVehicle->m_iMuzzleTag[i] == -1 )
			{//ergh, try *flash?
				Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
				ent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
			}
		}

		// Setup the Turrets.
		for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
		{
			if ( ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag )
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
			}
			else
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = -1;
			}
		}
	}

	if (ent->client->ps.weapon == WP_SABER || ent->s.number < MAX_CLIENTS)
	{ //a player or NPC saber user
		trap_G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
		trap_G2API_AddBolt(ent->ghoul2, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap_G2API_AddBolt(ent->ghoul2, 0, "*chestg");

		//claw bolts
		trap_G2API_AddBolt(ent->ghoul2, 0, "*r_hand_cap_r_arm");
		trap_G2API_AddBolt(ent->ghoul2, 0, "*l_hand_cap_l_arm");

		trap_G2API_SetBoneAnim(ent->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
		trap_G2API_SetBoneAngles(ent->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
		trap_G2API_SetBoneAngles(ent->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time);

		if (!g2SaberInstance)
		{
			trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

			if (g2SaberInstance)
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
				// now set up the gun bolt on it
				trap_G2API_AddBolt(g2SaberInstance, 0, "*blade1");
			}
		}

		if (G_SaberModelSetup(ent))
		{
			if (g2SaberInstance)
			{
				trap_G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, ent->ghoul2, 1); 
			}
		}
	}

	if (ent->s.number >= MAX_CLIENTS)
	{ //some extra NPC stuff
		if (trap_G2API_AddBolt(ent->ghoul2, 0, "lower_lumbar") == -1)
		{ //check now to see if we have this bone for setting anims and such
			ent->noLumbar = qtrue;
		}
	}

	//RoboPhred
	strcpy(ent->client->modelname, modelname);
}




/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride);
void G_ValidateSiegeClassForTeam(gentity_t *ent, int team);

//Lugormod
//qboolean checkNick (gentity_t *ent);
//RoboPhred
extern vmCvar_t g_checkSkin;
qboolean IsValidPlayerName(char *name, gentity_t *ent, qboolean isRegister);
void Lmd_IPs_AddName(IP_t ip, char *name);

void ClientUserNameChanged(gentity_t *ent, char *userinfo) {
	char	oldname[MAX_STRING_CHARS];
	char	name[MAX_NETNAME];
	int i;
	gentity_t *check;
	char *s;
	gclient_t *client = ent->client;
	int clientNum = ent->s.number;

	// set name
	Q_strncpyz(oldname, ent->client->pers.netname, sizeof(oldname));
	s = Info_ValueForKey (userinfo, "name");
	if(strlen(s) > MAX_NETNAME)
		s[MAX_NETNAME] = 0;
	//IsValidPlayerName checks bots, which uses Info_ValueForKey, which changes the contents of our pointer.
	ClientCleanName(s, name, sizeof(name) );
	//Q_strncpyz(name, s, sizeof(name));

	if (ent->client->pers.Lmd.persistantFlags & SPF_SHUTUP || Q_stricmp(oldname, name) == 0 || IsValidPlayerName(name, ent, qfalse) == qfalse) {
		if (oldname[0] == 0) {
			// Had no valid old name, switch to padawan
			Q_strncpyz(name, "Padawan", sizeof(name));
		}
		else {
			// Not allowed to change name.
			return;
		}
	}

	for (i = 0; i < MAX_CLIENTS; i++){
		check = &g_entities[i];
		if (!check->inuse || !check->client || check->client->pers.connected != CON_CONNECTED || i == ent->s.number)
			continue;
		if (Q_stricmp(name, check->client->pers.netname) == 0){
			name[MAX_NAME_LENGTH - 4] = 0;
			Q_strcat(name, sizeof(name), va("%2i", ent->s.number));
		}
	}

	if (strcmp(oldname, name) == 0) {
		// No change.
		return;
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( client->pers.netnameTime > level.time  && Auths_GetPlayerRank(ent) == 0) {
			// Cant change name so soon.
			trap_SendServerCommand( clientNum, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NONAMECHANGE")) );
			return;
		}
	}

	Q_strncpyz(client->pers.netname, name, sizeof(client->pers.netname));

	PlayerAcc_SetName(ent, ent->client->pers.netname);
	Lmd_IPs_AddName(ent->client->sess.Lmd.ip, ent->client->pers.netname);

	if ( client->pers.connected == CON_CONNECTED ) {
		//For Highscore purpouses
		G_LogPrintf("ClientNamechange: %s is now %s\n", oldname, client->pers.netname);
		Com_Printf("info: (%2i) %s is now %s\n", clientNum, oldname, client->pers.netname);

		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname, G_GetStringEdString("MP_SVGAME", "PLRENAME"), client->pers.netname) );
		client->pers.netnameTime = level.time + 5000;
	}
}

qboolean ClientModelChanged(gentity_t *ent, char *userinfo, char *model) {
	gclient_t *client = ent->client;
	int clientNum = ent->s.number;
	qboolean	modelChanged = qfalse;
	char *s;

	// set model
	Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), MAX_QPATH );


	//Lugormod no more invisible ...
	//this is a hackish and probably not so good way of doing it.
	//I should check for the start jedi_ instead probably
	//Custom models might not be set up this way so they might not
	//work properly. There should be a cvar for disabling this check.
	/*
	char *p;
	if (p = strchr(model, '|')) {
	//Lugormod one of those three part models
	if (p - 7 < model
	|| Q_strncmp("head_", p - 7, 5)
	|| strlen(p) < 18 
	|| Q_strncmp("torso_", p + 1, 6)
	|| Q_strncmp("lower_", p + 10, 6)) {
	Q_strncpyz(model, "kyle", sizeof(model));
	}
	}
	*/
	if (g_checkSkin.integer) {

		char *h, *t, *l, *ms;
		ms = G_NewString(model);

		if (t = strchr(ms, '|')) {
			qboolean keepmodel = qfalse;
			*t++ = '\0';
			if (h = strchr(ms, '/')) {
				*h++ = '\0';
				if (l = strchr(t, '|')) {
					*l++ = '\0';
					if (Q_strncmp("head_", h, 5) == 0
						&& Q_strncmp("torso_", t, 6) == 0
						&& Q_strncmp("lower_", l, 6) == 0){
							char buf[1024];
							char *sp;
							int fCount = trap_FS_GetFileList(va("models/players/%s",ms), ".skin", buf, sizeof(buf));
							int checked = 3;
							sp = buf;
							while ( fCount-- && checked) {
								if (!Q_stricmp(va("%s.skin", l), sp)
									|| !Q_stricmp(va("%s.skin", h), sp)
									|| !Q_stricmp(va("%s.skin", t), sp))
								{
									checked--;
								}
								/*
								if (!Q_strncmp(l, sp, 
								strlen(l))
								|| !Q_strncmp(h, sp,
								strlen(h))
								|| !Q_strncmp(t, sp,
								strlen(t))){
								checked--;
								}
								*/
								sp += strlen(sp) + 1;
							}
							if (!checked) {
								keepmodel = qtrue;
							}
					}
				}
			}
			if (!keepmodel) {
				Q_strncpyz(model, "kyle", sizeof(model));
			}
		}

		G_Free(ms);
	}
	//end Lugormod fix illegal skins

	if (d_perPlayerGhoul2.integer)
	{
		if (Q_stricmp(model, client->modelname))
		{
			strcpy(client->modelname, model);
			modelChanged = qtrue;
		}
	}

	//Get the skin RGB based on his userinfo
	s = Info_ValueForKey (userinfo, "char_color_red");
	if (s)
	{
		client->ps.customRGBA[0] = atoi(s);
	}
	else
	{
		client->ps.customRGBA[0] = 255;
	}

	s = Info_ValueForKey (userinfo, "char_color_green");
	if (s)
	{
		client->ps.customRGBA[1] = atoi(s);
	}
	else
	{
		client->ps.customRGBA[1] = 255;
	}

	s = Info_ValueForKey (userinfo, "char_color_blue");
	if (s)
	{
		client->ps.customRGBA[2] = atoi(s);
	}
	else
	{
		client->ps.customRGBA[2] = 255;
	}
	//No need for this shit:
	//if ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100)
	//{ //hmm, too dark!
	//	client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	//}

	client->ps.customRGBA[3]=255;

	return modelChanged;
}

void ClientUserinfoChanged_Do( int clientNum ) {
	gentity_t *ent = g_entities + clientNum;
	int		teamTask, teamLeader, team;
	char	*s;
	
	//char	headModel[MAX_QPATH];
	gclient_t	*client = ent->client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	//	char	redTeam[MAX_INFO_STRING];
	//	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
	char	className[MAX_QPATH]; //name of class type to use in siege
	char	saberName[MAX_QPATH];
	char	saber2Name[MAX_QPATH];
	//char	*value;
	qboolean modelChanged;
	char model[MAX_QPATH];

	//stupid thing, why does it constantly clear the client number
	ent->s.number = clientNum;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	//Hax test
	//Q_strncpyz(userinfo, "n\\Padawan\\t\\0\\model\\lord_maul_s/default\\c1\\0\\c2\\5\\hc\\100\\w\\0\\l\\0\\tt\\0\\tl\\0\\st\\dual_4\\st2\\none\\dt\\0", sizeof(userinfo));

	if (g_logClientInfo.integer)
	{
		G_LogPrintf( "ClientUserinfoChanged handling info: %i %s\n", clientNum, userinfo );
	}

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}	

	ClientUserNameChanged(ent, userinfo);

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	modelChanged = ClientModelChanged(ent, userinfo, model);

	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && ent->r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
	}
	else {
		team = client->sess.sessionTeam;
	}

	//Set the siege class
	if (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) {
		strcpy(className, client->sess.siegeClass);

		//Now that the team is legal for sure, we'll go ahead and get an index for it.
		client->siegeClass = BG_SiegeFindClassIndexByName(className);
		if (client->siegeClass == -1)
		{ //ok, get the first valid class for the team you're on then, I guess.
			BG_SiegeCheckClassLegality(team, className);
			strcpy(client->sess.siegeClass, className);
			client->siegeClass = BG_SiegeFindClassIndexByName(className);
		}
		else
		{ //otherwise, make sure the class we are using is legal.
			G_ValidateSiegeClassForTeam(ent, team);
			strcpy(className, client->sess.siegeClass);
		}

		//Set the sabers if the class dictates
		if (client->siegeClass != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

			if (scl->saber1[0])
			{
				G_SetSaber(ent, 0, scl->saber1, qtrue);
			}
			else
			{ //default I guess
				G_SetSaber(ent, 0, "Kyle", qtrue);
			}
			if (scl->saber2[0])
			{
				G_SetSaber(ent, 1, scl->saber2, qtrue);
			}
			else
			{ //no second saber then
				G_SetSaber(ent, 1, "none", qtrue);
			}

			//make sure the saber models are updated
			G_SaberModelSetup(ent);

			if (scl->forcedModel[0])
			{ //be sure to override the model we actually use
				strcpy(model, scl->forcedModel);
				if (d_perPlayerGhoul2.integer)
				{
					if (Q_stricmp(model, client->modelname))
					{
						strcpy(client->modelname, model);
						modelChanged = qtrue;
					}
				}
			}

			//force them to use their class model on the server, if the class dictates
			if (G_PlayerHasCustomSkeleton(ent))
			{
				if (Q_stricmp(model, client->modelname) || ent->localAnimIndex == 0)
				{
					strcpy(client->modelname, model);
					modelChanged = qtrue;
				}
			}
		}
	}
	else
	{
		strcpy(className, "none");
	}

	//Set the saber name
	strcpy(saberName, client->sess.saberType[0]);
	strcpy(saber2Name, client->sess.saberType[1]);

	/*Done in ClientSpawn now.
	// set max health
	if ((g_gametype.integer == GT_SIEGE ||
		g_gametype.integer == GT_BATTLE_GROUND) //Lugormod
		&& client->siegeClass != -1)
	{
		siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
		maxHealth = 100;

		if (scl->maxhealth)
		{
			maxHealth = scl->maxhealth;
		}

		health = maxHealth;
	}
	else{
		maxHealth = 100;
		health = 100; //atoi( Info_ValueForKey( userinfo, "handicap" ) );
	}
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) {
		//client->pers.maxHealth = 100;
		client->pers.maxHealth = maxHealth;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	*/

	if (g_gametype.integer >= GT_TEAM) {
		client->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		} else {
			client->pers.teamInfo = qfalse;
		}
	}

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d\\siegeclass\\%s\\st\\%s\\st2\\%s\\dt\\%i\\sdt\\%i",
			client->pers.netname, team, model,  c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader, className, saberName, saber2Name, client->sess.duelTeam, client->sess.siegeDesiredTeam );
	} else {
		if (g_gametype.integer == GT_SIEGE 
			|| g_gametype.integer == GT_BATTLE_GROUND) //Lugormod
		{ //more crap to send
			s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\siegeclass\\%s\\st\\%s\\st2\\%s\\dt\\%i\\sdt\\%i",
				client->pers.netname, client->sess.sessionTeam, model, c1, c2, 
				client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, className, saberName, saber2Name, client->sess.duelTeam, client->sess.siegeDesiredTeam);
		}
		else
		{
			s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\st\\%s\\st2\\%s\\dt\\%i",
				client->pers.netname, client->sess.sessionTeam, model, c1, c2, 
				client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, saberName, saber2Name, client->sess.duelTeam);
		}
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	if (modelChanged) //only going to be true for allowable server-side custom skeleton cases
	{ //update the server g2 instance if appropriate
		char *modelname = Info_ValueForKey (userinfo, "model");
		SetupGameGhoul2Model(ent, modelname, NULL);

		if (ent->ghoul2 && ent->client)
		{
			ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
		}

		client->torsoAnimExecute = client->legsAnimExecute = -1;
		client->torsoLastFlip = client->legsLastFlip = qfalse;
	}
	/*
	if (client->sess.regCode) {
	char *reg = Info_ValueForKey (userinfo, "handicap");
	if (atoi(reg) != client->sess.regCode) {
	Info_SetValueForKey( userinfo, "handicap", 
	va("%i", client->sess.regCode));
	trap_SetUserinfo( clientNum, userinfo );
	}

	}
	*/
	if (g_logClientInfo.integer)
	{
		G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
	}
}

void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent = g_entities + clientNum;
	if(ent->r.svFlags & SVF_BOT) {
		ClientUserinfoChanged_Do(clientNum);
	}
	else {
		ent->client->infoChanged = level.time + 700;
	}
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/

//RoboPhred
extern vmCvar_t lmd_maxsameip;
extern vmCvar_t lmd_autobansameip;
void ClientUserinfoChanged_Do(int clientNum);
qboolean Lmd_Accounts_Bot_Login(gentity_t *ent);
char* Bans_IPIsBanned(IP_t ip);
void Lmd_IPs_SetPlayerIP(gclient_t *client, IP_t ip);
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
	//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;
	gentity_t	*te;
	IP_t ip;
	char clientip[20] = "";
	//char model[MAX_QPATH];

	ent = &g_entities[ clientNum ];

	//RoboPhred: shouldnt this be already done?  Moved up here, used to be after ip code.
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = level.clients + clientNum;
	client = ent->client;
	ent->playerState = &ent->client->ps;
	memset( client, 0, sizeof(*client) );

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	//RoboPhred: I think I accidentally removed this
	if(!isBot) {
		if (g_needpass.integer ) {
			//RoboPhred: moved up here
			// check for a password
			value = Info_ValueForKey (userinfo, "password");
			if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) && strcmp( g_password.string, value) != 0) {
				static char sTemp[1024];
				Q_strncpyz(sTemp, G_GetStringEdString("MP_SVGAME","INVALID_ESCAPE_TO_MAIN"), sizeof (sTemp) );
				return sTemp;// return "Invalid password";
			}
		}

		if(firstTime) {
			//RoboPhred: search for q3fill fake players.
			//Very specfic, but it stops the script kiddies.
			value = Info_ValueForKey (userinfo, "cl_punkbuster");
			if(value[0])
				return "Userinfo data is corrupt.";
			value = Info_ValueForKey (userinfo, "cl_anonymous");
			if(value[0])
				return "Userinfo data is corrupt.";
			value = Info_ValueForKey (userinfo, "cl_guid");
			if(value[0])
				return "Userinfo data is corrupt.";
			//value = Info_ValueForKey (userinfo, "model");
			//if(Q_stricmp(value, "") == 0)
			//	return "Userinfo data is corrupt.";

			value = Info_ValueForKey (userinfo, "ip");

			//RoboPhred:
			//If someone makes their userinfo string be a precice length, then the server cannod add the
			//ip key onto it, granting ban immunity.
			if(value[0] == 0)
				return "Userinfo data is corrupt.";

			Q_strncpyz(clientip, value, sizeof(clientip));

			//RoboPhred
			value = strchr(clientip, ':');
			if(value)
				value[0] = 0;

			if(Q_stricmp(clientip, "localhost") == 0)
				Q_strncpyz(clientip, "127.0.0.1", sizeof(clientip));

			if(Lmd_IPs_ParseIP(clientip, ip)) {
				char *reason;
				
				if((reason = Bans_IPIsBanned(ip)))
					return va("Banned%s", (reason[0]) ? va(": %s", reason) : "");

				if(lmd_maxsameip.integer > 0){
					unsigned int i, c = 0;
					for(i = 0;i<MAX_CLIENTS;i++){
						if(i == clientNum)
							continue;
						if(!g_entities[i].client || g_entities[i].client->pers.connected == CON_DISCONNECTED)
							continue;
						if(Lmd_IPs_CompareIP(g_entities[i].client->sess.Lmd.ip, ip))
							c++;
					}
					if(c > lmd_maxsameip.integer){
						for(i = 0;i<MAX_CLIENTS;i++){
							if(i == clientNum)
								continue;
							if(!g_entities[i].client)
								continue;
							if(Lmd_IPs_CompareIP(g_entities[i].client->sess.Lmd.ip, ip))
								trap_DropClient(i, "was kicked due to too many connections.");
						}
						if(lmd_autobansameip.integer){
							Bans_AddIP(ip, Time_Now() + Time_Days(lmd_autobansameip.integer), "Too many connections.");
							trap_SendServerCommand(-1, va("print \"^3IP ^2%s^3 has been banned from the server due to too many connections.", clientip));
							return "Banned from this server due to too many connections.";
						}
						//hex E is d14.  Sending a 7 then 14 causes the windows console to beep.  They each show up as "." in jka error window.
						return "Too many clients have been detected from this ip, refusing connection\0x7\0xE\0x7\0xE\0x7\0xE";
					}
				}
			}
		}
	}
	else {
		Q_strncpyz(clientip, "localhost", sizeof(clientip));
		Lmd_IPs_ParseIP(clientip, ip);
	}

	client->pers.connected = CON_CONNECTING;

	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo, isBot, level.newSession );
	}
	G_ReadSessionData( client );

	if(clientip[0]) {
		Lmd_IPs_SetPlayerIP(client, ip);
	}

	//time must be set
	client->pers.Lmd.playTime = level.time;
	//not sure this is needed
	client->pers.Lmd.wasKing = qfalse;
	client->pers.Lmd.chicken = 0;
	//end Lugormod

	if ((g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) //lugormod
		&& (firstTime || level.newSession))
	{ //if this is the first time then auto-assign a desired siege team and show briefing for that team
		client->sess.siegeDesiredTeam = 0;//PickTeam(ent->s.number);
		/*
		trap_SendServerCommand(ent->s.number, va("sb %i", client->sess.siegeDesiredTeam));
		*/
		//don't just show it - they'll see it if they switch to a team on purpose.
	}

	if (g_gametype.integer == GT_SIEGE && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		if (firstTime || level.newSession)
		{ //start as spec

			client->sess.siegeDesiredTeam = client->sess.sessionTeam;
			client->sess.sessionTeam = TEAM_SPECTATOR;
		}
	}
	else if (g_gametype.integer == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR){
		client->sess.sessionTeam = TEAM_SPECTATOR;
	} else if (g_gametype.integer == GT_BATTLE_GROUND && client->sess.siegeDesiredTeam == TEAM_FREE){
		client->sess.siegeDesiredTeam = PickTeam(-1);
		client->sess.sessionTeam = client->sess.siegeDesiredTeam;
	} else if (g_gametype.integer == GT_REBORN) {
		client->sess.sessionTeam = TEAM_BLUE;
	}

	if( isBot ) {
		//RoboPhred: he says do this earlier, then why is it BELOW this?
		//ent->r.svFlags |= SVF_BOT; Lugormod need to do this earlier
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
	} else {
		ent->r.svFlags &= ~SVF_BOT;
	}

	ClientUserinfoChanged_Do( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCONNECT")) );
		//Lugormod
		//if (!isBot){
		//char *p = strchr(client->sess.Lmd.ip, ':');
		//Check p < clientip + strlen(ent->client->sess.Lmd.ip) ?
		//p[0] = '\0';
		//}
		//Lugormod
		if (isBot) {
			Lmd_Accounts_Bot_Login(ent);
		}

		G_LogPrintf("IP: %s %s\n", clientip, 
			client->pers.netname);
		Com_Printf("info: IP: %s %s %2i\n", clientip, client->pers.netname, clientNum);
	}

	//G_Free(clientip);
	//client->pers.voteCount = 0;

	if ( g_gametype.integer >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	te = G_TempEntity( vec3_origin, EV_CLIENTJOIN );
	if(te){
		te->r.svFlags |= SVF_BROADCAST;
		te->s.eventParm = clientNum;
	}

	// for statistics
	//	client->areabits = areabits;
	//	if ( !client->areabits )
	//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	return NULL;
}

void G_WriteClientSessionData( gclient_t *client );

#include "../namespace_begin.h"
void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );
#include "../namespace_end.h"

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);

qboolean Lmd_Accounts_Player_Login(gentity_t *ent, Account_t *acc);
qboolean IsKing(gentity_t *ent); //Lugormod
qboolean Bans_CheckBegin(gentity_t *ent);

void initChatMode(gentity_t *ent);
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags, i;
	char		userinfo[MAX_INFO_VALUE], *modelname;
	//int savedPersFlags = 0;
	//int savedVoteCount = 0;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	qboolean wasConnected = client->pers.connected == CON_CONNECTED;

	if(Bans_CheckBegin(ent))
		return;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}

	G_InitGentity( ent );

	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;
	//Lugormod to be on the safe side
	//client->pers.Lmd.playTime = level.time;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;

	//RoboPhred:
	if (!wasConnected) {
		initChatMode(ent);
		if (client->sess.Lmd.id > 0) {
			Account_t *acc = Accounts_GetById(client->sess.Lmd.id);
			if (acc == NULL || !Lmd_Accounts_Player_Login(ent, acc)) {
				client->sess.Lmd.id = 0;
			}
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	//init saber ent
	WP_SaberInitBladeData( ent );

	//Lugormod
	if (ent->inuse && ent->client) {
		if (IsKing(ent)) {
			BecomeCommoner(ent);
		}

	}

	if ((ent->r.svFlags & SVF_BOT) && g_gametype.integer >= GT_TEAM)
	{
		if (allowTeamReset)
		{
			const char *team = "Red";
			int preSess;

			//SetTeam(ent, "");
			ent->client->sess.sessionTeam = PickTeam(-1);
			trap_GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				ent->client->sess.sessionTeam = TEAM_RED;
			}

			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				team = "Red";
			}
			else
			{
				team = "Blue";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap_SetUserinfo( clientNum, userinfo );

			ent->client->ps.persistant[ PERS_TEAM ] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData( ent->client );
			ent->client->sess.sessionTeam = preSess;
			G_WriteClientSessionData(ent->client);
			ClientUserinfoChanged_Do( clientNum );
			ClientBegin(clientNum, qfalse);
			return;
		}
	}


	// First time model setup for that player.
	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	modelname = Info_ValueForKey (userinfo, "model");
	SetupGameGhoul2Model(ent, modelname, NULL);

	if (ent->ghoul2 && ent->client)
	{
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
	}

	if (g_gametype.integer == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR &&
		client->sess.duelTeam == DUELTEAM_FREE)
	{
		SetTeam(ent, "s");
	}
	else
	{
		if (g_gametype.integer == GT_SIEGE 
			&& (!gSiegeRoundBegun || gSiegeRoundEnded))
		{
			SetTeamQuick(ent, TEAM_SPECTATOR, qfalse);
		}

		if ((ent->r.svFlags & SVF_BOT) &&
			g_gametype.integer != GT_SIEGE
			&& g_gametype.integer != GT_BATTLE_GROUND) //Lugormd
		{
			char *saberVal = Info_ValueForKey(userinfo, "saber1");
			char *saber2Val = Info_ValueForKey(userinfo, "saber2");

			if (!saberVal || !saberVal[0])
			{ //blah, set em up with a random saber
				int r = rand()%50;
				char sab1[1024];
				char sab2[1024];

				if (r <= 17)
				{
					strcpy(sab1, "Katarn");
					strcpy(sab2, "none");
				}
				else if (r <= 34)
				{
					strcpy(sab1, "Katarn");
					strcpy(sab2, "Katarn");
				}
				else
				{
					strcpy(sab1, "dual_1");
					strcpy(sab2, "none");
				}
				G_SetSaber(ent, 0, sab1, qfalse);
				G_SetSaber(ent, 0, sab2, qfalse);
				Info_SetValueForKey( userinfo, "saber1", sab1 );
				Info_SetValueForKey( userinfo, "saber2", sab2 );
				trap_SetUserinfo( clientNum, userinfo );
			}
			else
			{
				G_SetSaber(ent, 0, saberVal, qfalse);
			}

			if (saberVal && saberVal[0] &&
				(!saber2Val || !saber2Val[0]))
			{
				G_SetSaber(ent, 0, "none", qfalse);
				Info_SetValueForKey( userinfo, "saber2", "none" );
				trap_SetUserinfo( clientNum, userinfo );
			}
			else
			{
				G_SetSaber(ent, 0, saber2Val, qfalse);
			}
		}

		// locate ent at a spawn point
		ClientSpawn( ent );
	}

	if ( client->sess.sessionTeam != TEAM_SPECTATOR) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		//RoboPhred
		if(tent)
			tent->s.clientNum = ent->s.clientNum;

		if ( g_gametype.integer != GT_DUEL || g_gametype.integer == GT_POWERDUEL ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLENTER")) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );
	Com_Printf("info: %s joined the battle.\n", client->pers.netname);

	// count current clients and rank for scoreboard
	CalculateRanks();

	G_ClearClientLog(clientNum);
}

qboolean AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

//Convenient interface to set all my limb breakage stuff up -rww
void G_BreakArm(gentity_t *ent, int arm)
{
	int anim = -1;

	assert(ent && ent->client);

	if (ent->s.NPC_class == CLASS_VEHICLE || ent->localAnimIndex > 1)
	{ //no broken limbs for vehicles and non-humanoids
		return;
	}

	if (!arm)
	{ //repair him
		ent->client->ps.brokenLimbs = 0;
		return;
	}

	if (ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{ //I'm too lazy to deal with this as well for now.
		return;
	}

	if (arm == BROKENLIMB_LARM)
	{
		if (ent->client->saber[1].model[0] &&
			ent->client->ps.weapon == WP_SABER &&
			!ent->client->ps.saberHolstered &&
			ent->client->saber[1].soundOff)
		{ //the left arm shuts off its saber upon being broken
			G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
		}
	}

	ent->client->ps.brokenLimbs = 0; //make sure it's cleared out
	ent->client->ps.brokenLimbs |= (1 << arm); //this arm is now marked as broken

	//Do a pain anim based on the side. Since getting your arm broken does tend to hurt.
	if (arm == BROKENLIMB_LARM)
	{
		anim = BOTH_PAIN2;
	}
	else if (arm == BROKENLIMB_RARM)
	{
		anim = BOTH_PAIN3;
	}

	if (anim == -1)
	{
		return;
	}

	G_SetAnim(ent, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);

	//This could be combined into a single event. But I guess limbs don't break often enough to
	//worry about it.
	G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain25.wav") );
	//FIXME: A nice bone snapping sound instead if possible
	G_Sound(ent, CHAN_AUTO, G_SoundIndex( va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3)) ));
}

//Update the ghoul2 instance anims based on the playerstate values
#include "../namespace_begin.h"
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
#include "../namespace_end.h"
void G_UpdateClientAnims(gentity_t *self, float animSpeedScale)
{
	static int f;
	static int torsoAnim;
	static int legsAnim;
	static int firstFrame, lastFrame;
	static int aFlags;
	static float animSpeed, lAnimSpeedScale;
	qboolean setTorso = qfalse;

	torsoAnim = (self->client->ps.torsoAnim);
	legsAnim = (self->client->ps.legsAnim);

	if (self->client->ps.saberLockFrame)
	{
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "model_root", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "Motion", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		return;
	}

	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames == 0)
	{ //We'll allow this for non-humanoids.
		goto tryTorso;
	}

	if (self->client->legsAnimExecute != legsAnim || self->client->legsLastFlip != self->client->ps.legsFlip)
	{
		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[legsAnim].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[legsAnim].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on server position, but it's here just for the sake of matching them.

		trap_G2API_SetBoneAnim(self->ghoul2, 0, "model_root", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
		self->client->legsAnimExecute = legsAnim;
		self->client->legsLastFlip = self->client->ps.legsFlip;
	}

tryTorso:
	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].numFrames == 0)

	{ //If this fails as well just return.
		return;
	}
	else if (self->s.number >= MAX_CLIENTS &&
		self->s.NPC_class == CLASS_VEHICLE)
	{ //we only want to set the root bone for vehicles
		return;
	}

	if ((self->client->torsoAnimExecute != torsoAnim || self->client->torsoLastFlip != self->client->ps.torsoFlip) &&
		!self->noLumbar)
	{
		aFlags = 0;
		animSpeed = 0;

		f = torsoAnim;

		BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, f, &animSpeedScale, self->client->ps.brokenLimbs);

		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[f].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[f].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on client position, but it's here just for the sake of matching them.

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}

		trap_G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, /*firstFrame why was it this before?*/-1, 150);

		self->client->torsoAnimExecute = torsoAnim;
		self->client->torsoLastFlip = self->client->ps.torsoFlip;

		setTorso = qtrue;
	}

	if (setTorso &&
		self->localAnimIndex <= 1)
	{ //only set the motion bone for humanoids.
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "Motion", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
	}

#if 0 //disabled for now
	if (self->client->ps.brokenLimbs != self->client->brokenLimbs ||
		setTorso)
	{
		if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_DEAD21 ];
			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

			trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
		}
		else if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//self->client->ps.weapon == WP_MELEE ||
				self->client->ps.weapon != WP_SABER ||
				BG_SaberStanceAnim(self->client->ps.torsoAnim) ||
				PM_RunningAnim(self->client->ps.torsoAnim)) &&
				(!self->client->saber[1].model[0] || self->client->ps.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (self->client->ps.weapon == WP_MELEE ||
					self->client->ps.weapon == WP_SABER ||
					self->client->ps.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}

				if (self->client->ps.torsoAnim != BOTH_MELEE1 &&
					self->client->ps.torsoAnim != BOTH_MELEE2 &&
					(self->client->ps.torsoAnim == TORSO_WEAPONREADY2 || self->client->ps.torsoAnim == BOTH_ATTACK2 || self->client->ps.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}
			}
			else
			{ //otherwise, keep it set to the same as the torso
				trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
			}
		}
		else if (self->client->brokenLimbs)
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (self->client->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (self->client->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap_G2API_SetBoneAnim(self->ghoul2, 0, "lhumerus", 0, 1, 0, 0, level.time, -1, 0);
				trap_G2API_RemoveBone(self->ghoul2, "lhumerus", 0);
			}

			assert(brokenBone);

			//Set the flags and stuff to 0, so that the remove will succeed
			trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, 0, 1, 0, 0, level.time, -1, 0);

			//Now remove it
			trap_G2API_RemoveBone(self->ghoul2, brokenBone, 0);
			self->client->brokenLimbs &= ~broken;
		}
	}
#endif
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
//Lugormod
//void updateGhost (gentity_t *self);
void scaleEntity (gentity_t *scaleEnt, int scale);
void Merc_Unhook (gentity_t *ent);
int thereIsAMoneyStash (void);
void changeSiegeClass (gentity_t *ent, char *className);
//RoboPhred
//extern int merc_starting_weapon[6];
//end Lugormod
extern qboolean WP_HasForcePowers( const playerState_t *ps );
void BecomeKing (gentity_t * ent);

//RoboPhred
void scaleEntity(gentity_t *scaleEnt, int scale);
void Merc_GiveStartingWeapons(gentity_t *ent);

gentity_t *SelectBGSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles ); //Lugormod
//RoboPhred
void Professions_PlayerSpawn(gentity_t *ent);
void Confirm_Clear(gentity_t *ent);
void Interact_Clear(gentity_t *ent);

gentity_t* ClientSpawn_GetSpawnpoint(gentity_t *ent, vec3_t spawn_origin, vec3_t spawn_angles) {
	gclient_t *client = ent->client;
	gentity_t *spawnPoint = NULL;
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	//RoboPhred
	if (client->pers.Lmd.jailTime > level.time){
		//if (client->sess.sessionTeam == TEAM_JAILED){
		//Lugormod jail stuff
		//RoboPhred: This will return a jail point as the jail timer is already set.  We do this here because
		//the CTF and Siege spawn points currently are not set up to handle 'advanced spawning'
		spawnPoint = SelectSpawnPoint(ent, spawn_origin, spawn_angles );
	}
	else if(client->sess.sessionTeam == TEAM_SPECTATOR){
		spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
	}
	else if(g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY || g_gametype.integer == GT_SABER_RUN || g_gametype.integer == GT_GHOST || g_gametype.integer == GT_REBORN){
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint(ent, spawn_origin, spawn_angles);
	}
	else if (g_gametype.integer == GT_SIEGE){
		spawnPoint = SelectSiegeSpawnPoint(ent, spawn_origin, spawn_angles);
	}
	else if (g_gametype.integer == GT_BATTLE_GROUND){//Lugormod
		if (client->pers.Lmd.chicken){
			changeSiegeClass(ent, va("%i", client->pers.Lmd.chicken));
			client->pers.Lmd.chicken = 0;
		}
		spawnPoint = SelectBGSpawnPoint(ent, spawn_origin, spawn_angles);
	}
	else{
		if (g_gametype.integer == GT_POWERDUEL)
			spawnPoint = SelectDuelSpawnPoint(client->sess.duelTeam, client->ps.origin, spawn_origin, spawn_angles);
		else if (g_gametype.integer == GT_DUEL)
			spawnPoint = SelectDuelSpawnPoint(DUELTEAM_SINGLE, client->ps.origin, spawn_origin, spawn_angles);
		else{
			// the first spawn should be at a good looking spot
			//RoboPhred: damn ravensoft, this isnt suppost to only be local clients
			if (!client->pers.initialSpawn /*&& client->pers.localClient*/){
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( ent, spawn_origin, spawn_angles);
			}
			else{
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint(ent, spawn_origin, spawn_angles);
			}
		}
	}
	return spawnPoint;
}

void ClientSpawn_SetupSkin(gclient_t *client, char *userinfo) {
//Get the skin RGB based on his userinfo
	char* value = Info_ValueForKey (userinfo, "char_color_red");
	if (value)
	{
		client->ps.customRGBA[0] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[0] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_green");
	if (value)
	{
		client->ps.customRGBA[1] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[1] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_blue");
	if (value)
	{
		client->ps.customRGBA[2] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[2] = 255;
	}

	if ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100)
	{ //hmm, too dark!
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	}
	client->ps.customRGBA[3] = 255;
}

//FIXME: not sure if ent->s.number is valid here, so using index var.
void ClientSpawn_ResetClient(gentity_t *ent, int index) {
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int					persistant[MAX_PERSISTANT];
	int					flags, gameFlags;
	int					savedPing;
	int					accuracy_hits, accuracy_shots;
	int					eventSequence;
	forcedata_t			savedForce;
	int					saveSaberNum = ENTITYNUM_NONE;
	int					savedSiegeIndex = 0;
	saberInfo_t			saberSaved[MAX_SABERS];
	void				*g2WeaponPtrs[MAX_SABERS];
	int i;
	gclient_t *client = ent->client;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = client->ps.eFlags & (EF_TELEPORT_BIT );
	flags ^= EF_TELEPORT_BIT;
	gameFlags = client->mGameFlags & ( PSG_VOTED | PSG_TEAMVOTED);

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	saveSaberNum = client->ps.saberEntityNum;

	savedSiegeIndex = client->siegeClass;


	for(i = 0; i < MAX_SABERS; i++)	{
		saberSaved[i] = client->saber[i];
		g2WeaponPtrs[i] = client->weaponGhoul2[i];
	}

	for(i = 0; i < HL_MAX; i++){
		ent->locationDamage[i] = 0;
	}

	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	client->bodyGrabIndex = ENTITYNUM_NONE;

	client->siegeClass = savedSiegeIndex;
	for(i = 0; i < MAX_SABERS; i++)	{
		client->saber[i] = saberSaved[i];
		client->weaponGhoul2[i] = g2WeaponPtrs[i];
	}

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;
	client->saberStoredIndex = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	client->pers = saved;
	client->sess = savedSess;

	client->ps.ping = savedPing;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	client->ps.eventSequence = eventSequence;
	client->airOutTime = level.time + 12000;

	// clear entity values
	client->ps.eFlags = flags;
	client->mGameFlags = gameFlags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->playerState = &client->ps;
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);
	client->ps.crouchheight = CROUCH_MAXS_2;
	client->ps.standheight = DEFAULT_MAXS_2;

	client->pers.teamState.state = TEAM_ACTIVE;

	client->ps.clientNum = index;


	//rww - Set here to initialize the circling seeker drone to off.
	//A quick note about this so I don't forget how it works again:
	//ps.genericEnemyIndex is kept in sync between the server and client.
	//When it gets set then an entitystate value of the same name gets
	//set along with an entitystate flag in the shared bg code. Which
	//is why a value needs to be both on the player state and entity state.
	//(it doesn't seem to just carry over the entitystate value automatically
	//because entity state value is derived from player state data or some
	//such)
	client->ps.genericEnemyIndex = -1;

	//RoboPhred: no keeping scale
	//reset the scale so the hitbox is reset properly (modelscale vector)
	//also resets the saber.
	scaleEntity(ent, 0);

	if (IsKing(ent))
		BecomeKing(ent); //put the fx back on

	client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
	client->enemyTeam = NPCTEAM_ENEMY;

	//This might be better in ClientSpawn_Default, but its needed for spectators too.
	client->ps.jetpackFuel = 100;
	client->ps.cloakFuel = 100;

	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );
	client->ps.stats[STAT_MAX_HEALTH] = 100;
}

void ClientSpawn_Default(gentity_t *ent, int wDisable) {
	if (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND)
	{//Ufo: eh, we re-initialize most of this shit to avoid bugs occuring to raven's inattention
		//Fixme: remove the useless lines
		if (ent->client->siegeClass == -1)
		{
			siegeTeam_t* stm = BG_SiegeFindThemeForTeam(ent->client->sess.siegeDesiredTeam);
			ent->client->siegeClass = BG_SiegeFindClassIndexByName(stm->classes[0]->name);
		}
		WP_InitForcePowers(ent);
		ent->client->ps.stats[STAT_WEAPONS] = 0;
		//Disp(ent, va("My class id: %d", ent->client->siegeClass));
		return;
	}

	gclient_t *client = ent->client;

	if (client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] && !(wDisable & (1 << WP_SABER))) //Lugormod 
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );
	else { 
		//if you don't have saber attack rank then you don't get a saber
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
	}

	if(!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );

	if(g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL)
		client->ps.stats[STAT_ARMOR] = (int)(client->ps.stats[STAT_MAX_HEALTH] * 0.25);
}

void ClientSpawn_Siege(gentity_t *ent) {
	gclient_t *client = ent->client;

	if(client->siegeClass == -1)
		return;

	int i;
	siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

	if (scl->maxhealth)
		client->ps.stats[STAT_MAX_HEALTH] = scl->maxhealth;


	client->ps.stats[STAT_WEAPONS] = bgSiegeClasses[client->siegeClass].weapons;

	if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
	{
		client->ps.weapon = WP_SABER;
	}
	else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL))
	{
		client->ps.weapon = WP_BRYAR_PISTOL;
	}
	else
	{
		client->ps.weapon = WP_MELEE;
	}

	for(i = 0; i < WP_NUM_WEAPONS; i++) {
		if (client->ps.stats[STAT_WEAPONS] & (1 << i))
		{
			if (client->ps.weapon != WP_SABER)
			{ //try to find the highest ranking weapon we have
				if (i > client->ps.weapon)
				{
					client->ps.weapon = i;
				}
			}

			if (i >= WP_BRYAR_PISTOL && g_gametype.integer == GT_SIEGE)
			{ //Max his ammo out for all the weapons he has.
				if (i == WP_ROCKET_LAUNCHER )
				{//don't give full ammo!
					//FIXME: extern this and check it when getting ammo from supplier, pickups or ammo stations!
					if (scl->classflags & (1 << CFL_SINGLE_ROCKET))
						client->ps.ammo[weaponData[i].ammoIndex] = 1;
					else
						client->ps.ammo[weaponData[i].ammoIndex] = 10;
				}
				else{
					if (scl->classflags & (1<<CFL_EXTRA_AMMO) )
					{//double ammo
						client->ps.ammo[weaponData[i].ammoIndex] = ammoData[weaponData[i].ammoIndex].max*2;
						client->ps.eFlags |= EF_DOUBLE_AMMO;
					}
					else
						client->ps.ammo[weaponData[i].ammoIndex] = ammoData[weaponData[i].ammoIndex].max;
				}
			}
		}
	}

	client->ps.stats[STAT_HOLDABLE_ITEMS] = bgSiegeClasses[client->siegeClass].invenItems;

	if (scl->powerups)
	{ //this class has some start powerups
		for(i = 0; i < PW_NUM_POWERUPS; i++)
		{
			if (bgSiegeClasses[client->siegeClass].powerups & (1 << i))
				client->ps.powerups[i] = level.time + Q3_INFINITE; //Ufo: Q3_INFINITE is insufficient for leveltime dependant stuff
		}
	}

	client->ps.ammo[AMMO_BLASTER] = 100;

	if(scl->starthealth)
	{ //class specifies a start health, so use it
		ent->health = client->ps.stats[STAT_HEALTH] = scl->starthealth;
	}

	client->ps.stats[STAT_ARMOR] = bgSiegeClasses[client->siegeClass].startarmor;

	if (client->sess.sessionTeam == SIEGETEAM_TEAM1)
	{
		client->playerTeam = ent->s.teamowner = NPCTEAM_ENEMY;
		client->enemyTeam = NPCTEAM_PLAYER;
	}
	else
	{
		client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
		client->enemyTeam = NPCTEAM_ENEMY;
	}
}

void ClientSpawn_Holocron(gentity_t *ent) {
	gclient_t *client = ent->client;
	//always get free saber level 1 in holocron
	client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
}

void ClientSpawn_JediMaster(gentity_t *ent) {
	gclient_t *client = ent->client;
	client->ps.isJediMaster = qfalse;
	client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
	client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL ) | (1 << WP_BLASTER);
	client->ps.ammo[weaponData[WP_BLASTER].ammoIndex] = ammoData[weaponData[WP_BLASTER].ammoIndex].max;
	client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
}

void ClientSpawn_Reborn(gentity_t *ent) {
	if (ent->client->sess.sessionTeam == TEAM_BLUE) {
		/*
		ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
		ent->client->ps.trueNonJedi = qtrue;
		ent->client->ps.trueJedi = qfalse;
		*/
	}
	else {
		ent->client->ps.trueNonJedi = qfalse;
		ent->client->ps.trueJedi = qtrue;
	}
}

void ClientSpawn_Ghost(gentity_t *ent) {
	gclient_t *client = ent->client;
	client->ps.trueJedi = qfalse;
	client->ps.trueNonJedi = qtrue;
}

void ClientSpawn_Duel(gentity_t *ent) {

}

void ClientSpawn_PowerDuel(gentity_t *ent) {
	gclient_t *client = ent->client;
	if (client->sess.duelTeam == DUELTEAM_LONE ){
		if ( g_duel_fraglimit.integer ){
			client->ps.stats[STAT_MAX_HEALTH] = (int)(g_powerDuelStartHealth.integer - 
				((g_powerDuelStartHealth.integer - g_powerDuelEndHealth.integer) * (float)client->sess.wins / (float)g_duel_fraglimit.integer));
		}
		else
			client->ps.stats[STAT_MAX_HEALTH] = 150;
	}
}

extern vmCvar_t g_startingWeapons;
extern vmCvar_t g_startingAmmo;
void ClientSpawn_GameModes(gentity_t *ent) {
	int i, m;
	gclient_t *client = ent->client;

	for(i = 0; i < WP_NUM_WEAPONS; i++) {
		if((1 << i) & g_startingWeapons.integer) {
			client->ps.stats[STAT_WEAPONS] |= (1 << i);
			m = (int)floor(ammoData[i].max * (g_startingAmmo.value / 100.0f));
			if(client->ps.ammo[weaponData[i].ammoIndex] < m)
				client->ps.ammo[weaponData[i].ammoIndex] = m;
		}
	}


	//All weapons
	if (gameMode(GM_ALLWEAPONS)) {
		//We are on a team that gets weapons, or a non-jedi.
		if ((g_gametype.integer >= GT_TEAM && client->sess.sessionTeam == TEAM_RED) || 
			(g_gametype.integer < GT_TEAM && !WP_HasForcePowers( &client->ps )))
		{

			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;

			client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON+1)) - ( 1 << WP_NONE );
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER)  - ( 1 << WP_NONE );

			for ( i = 0 ; i < MAX_WEAPONS ; i++ )
				ent->client->ps.ammo[i] = ammoData[i].max * 2;

			client->ps.stats[STAT_HOLDABLE_ITEMS] = (1 << (HI_NUM_HOLDABLE)) - 1;
			client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_BINOCULARS);

			client->ps.weapon = WP_BRYAR_PISTOL;
		}
		else {
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
		}
	}

	if (gameMode(GM_INSTGIB)) {
		client->ps.trueNonJedi = qtrue;
		client->ps.trueJedi = qfalse;
		client->ps.ammo[AMMO_METAL_BOLTS] = 999;

		client->ps.stats[STAT_WEAPONS] = (1 << WP_CONCUSSION);
	}
	else if (gameMode(GM_INSTDIS)) {
		client->ps.trueNonJedi = qtrue;
		client->ps.trueJedi = qfalse;
		client->ps.ammo[AMMO_POWERCELL] = 999;

		client->ps.stats[STAT_WEAPONS] = (1 << WP_DISRUPTOR);
	}
	else if (gameMode(GM_ROCKET_ARENA) || gameMode(GM_SNIPER_ARENA) || gameMode(GM_BOXING_ARENA)) {
		client->ps.trueNonJedi = qtrue;
		client->ps.trueJedi = qfalse;
	}

	if (gameMode(GMF_WITH_HOOK)) 
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STUN_BATON );
	if (gameMode(GMF_WITH_TELEPORTER))
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_OLD);
	if (gameMode(GMF_WITH_FORCE_JUMP))
		client->ps.trueNonJedi = qfalse;
	if (gameMode(GMF_WITH_JP))
		client->ps.stats[STAT_HOLDABLE_ITEMS] |= ( 1 << HI_JETPACK);
}

void ClientSpawn_JediVsMerc(gentity_t *ent, int wDisable) {
	gclient_t *client = ent->client;
	int i;
	if ( g_gametype.integer >= GT_TEAM && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam == TEAM_RED) )
	{//In Team games, force one side to be merc and other to be jedi
		if ( level.numPlayingClients > 0 )
		{//already someone in the game
			int	forceTeam = TEAM_SPECTATOR;

			for ( i = 0 ; i < level.maxclients ; i++ ) 
			{
				if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
					continue;
				}
				if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED ) 
				{//in-game
					if ( WP_HasForcePowers( &level.clients[i].ps ) )
					{//this side is using force
						forceTeam = level.clients[i].sess.sessionTeam;
					}
					else
					{//other team is using force
						if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							forceTeam = TEAM_RED;
						else
							forceTeam = TEAM_BLUE;
					}
					break;
				}
			}
			if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam )
			{//using force but not on right team, switch him over
				const char *teamName = TeamName( forceTeam );
				SetTeam( ent, (char *)teamName );
				return;
			}
		}
	}
	if ( WP_HasForcePowers( &client->ps ) )
	{
		client->ps.trueNonJedi = qfalse;
		client->ps.trueJedi = qtrue;
	}
	else {//no force powers set
		client->ps.trueNonJedi = qtrue;
		client->ps.trueJedi = qfalse;

		if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
		if (!wDisable || !(wDisable & (1 << WP_BLASTER)))
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BLASTER );
		if (!wDisable || !(wDisable & (1 << WP_BOWCASTER)))
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BOWCASTER );
		client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
		client->ps.weapon = WP_BRYAR_PISTOL;
	}
}

void ClientSpawn(gentity_t *ent) {
	int					index;
	vec3_t				spawn_origin, spawn_angles;
	gclient_t			*client;
	int					i;
	gentity_t			*spawnPoint;
	char				userinfo[MAX_INFO_STRING] = "";
	int					wDisable = 0;
	char				*value;
	char				*saber;
	qboolean			changedSaber = qfalse;
	qboolean			inSiegeWithClass = qfalse;
	index = ent - g_entities;
	client = ent->client;

	//RoboPhred: why index var?  I kinda remember there being an issue where ent->s.number was invalid...  Well, fix it!

	//first we want the userinfo so we can see if we should update this client's saber -rww
	trap_GetUserinfo( index, userinfo, sizeof(userinfo));
	for(i = 0; i < MAX_SABERS; i++) {
		saber = ent->client->sess.saberType[i];

		value = Info_ValueForKey (userinfo, va("saber%i", i+1));
		if (saber && value && (Q_stricmp(value, saber) || !saber[0] || !ent->client->saber[0].model[0]))
		{ //doesn't match up (or our session saber is BS), we want to try setting it
			if (G_SetSaber(ent, i, value, qfalse))
			{
				changedSaber = qtrue;
			}
			else if (!saber[0] || !ent->client->saber[0].model[0])
			{ //Well, we still want to say they changed then (it means this is siege and we have some overrides)
				changedSaber = qtrue;
			}
		}
	}

	if (changedSaber){ //make sure our new info is sent out to all the other clients, and give us a valid stance
		ClientUserinfoChanged_Do( ent->s.number );

		//make sure the saber models are updated
		G_SaberModelSetup(ent);

		for(i = 0; i < MAX_SABERS; i++) {
			{ //go through and make sure both sabers match the userinfo
				saber = ent->client->sess.saberType[i];

				value = Info_ValueForKey (userinfo, va("saber%i", i+1));

				if (Q_stricmp(value, saber))
				{ //they don't match up, force the user info
					Info_SetValueForKey(userinfo, va("saber%i", i+1), saber);
					//RoboPhred: ent->s.number randomly is cleared on joining...
					trap_SetUserinfo( index, userinfo );
					//trap_SetUserinfo( ent->s.number, userinfo );
				}
			}

			//dual
			if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
				ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
			//staff
			else if ((ent->client->saber[0].saberFlags&SFL_TWO_HANDED))
				ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
			else{
				if (ent->client->sess.saberLevel < SS_FAST)
					ent->client->sess.saberLevel = SS_FAST;
				else if (ent->client->sess.saberLevel > SS_STRONG)
					ent->client->sess.saberLevel = SS_STRONG;
				ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

				if (g_gametype.integer != GT_SIEGE &&
					g_gametype.integer != GT_BATTLE_GROUND && //Lugormod
					ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
				{
					ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
				}
			}
			if ( g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND){ //Lugormod
				//let's just make sure the styles we chose are cool
				if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, 
					ent->client->ps.fd.saberAnimLevel)){
						WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, 
							&ent->client->ps.fd.saberAnimLevel );
						ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
				}
			}
		}
	}

	if (ent->client->ps.fd.saberAnimLevel != SS_STAFF &&
		ent->client->ps.fd.saberAnimLevel != SS_DUAL &&
		ent->client->ps.fd.saberAnimLevel == ent->client->ps.fd.saberDrawAnimLevel &&
		ent->client->ps.fd.saberAnimLevel == ent->client->sess.saberLevel)
	{
		if (ent->client->sess.saberLevel < SS_FAST)
		{
			ent->client->sess.saberLevel = SS_FAST;
		}
		else if (ent->client->sess.saberLevel > SS_STRONG)
		{
			ent->client->sess.saberLevel = SS_STRONG;
		}
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

		if (g_gametype.integer != GT_SIEGE &&
			g_gametype.integer != GT_BATTLE_GROUND && //Lugormod
			ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
		{
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
	}


	//Lugormod try to fix invisibility thing
	//not sure if I already did this somewhere else,
	//but it isn't working so ....
	if (client->sess.sessionTeam != TEAM_SPECTATOR) {
		ent->r.svFlags &= ~SVF_NOCLIENT;
		ent->s.eFlags &= ~EF_NODRAW;
		client->ps.eFlags &= ~EF_NODRAW;
	}


	//RoboPhred.
	Confirm_Clear(ent);
	Interact_Clear(ent);

	ClientSpawn_ResetClient(ent, index);

	//Ravensoft had this before resetting, but meh.
	ClientSpawn_SetupSkin(client, userinfo);
	

	//FIXME: is this needed for spectators?
	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers(ent);

	if(client->sess.sessionTeam != TEAM_SPECTATOR) {

		if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		{
			wDisable = g_duelWeaponDisable.integer;
		}
		else
		{
			wDisable = g_weaponDisable.integer;
		}

		ClientSpawn_Default(ent, wDisable);

		if(g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND)
			ClientSpawn_Siege(ent);
		else if(g_gametype.integer == GT_HOLOCRON)
			ClientSpawn_Holocron(ent);
		else if(g_gametype.integer == GT_JEDIMASTER)
			ClientSpawn_JediMaster(ent);
		else if(g_gametype.integer == GT_REBORN)
			ClientSpawn_Reborn(ent);
		else if (g_gametype.integer == GT_GHOST)
			ClientSpawn_Ghost(ent);
		else if(g_gametype.integer == GT_DUEL)
			ClientSpawn_Duel(ent);
		else if(g_gametype.integer == GT_POWERDUEL)
			ClientSpawn_PowerDuel(ent);

		//RoboPhred: This comes after any of the above, as we will not apply the profession spawn if
		//we were set to be the opposite.
		if(PlayerAcc_Prof_GetLevel(ent) > 0 && g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND) //Ufo: we don't want it in siege
			Professions_PlayerSpawn(ent);
		
		//we confirm this on server start, so we don't need to check for proper game modes.
		if (g_jediVmerc.integer)
			ClientSpawn_JediVsMerc(ent, wDisable);

		//RoboPhred: sanify
		if(client->ps.trueJedi) {
			client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
			client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
		if(client->ps.trueNonJedi) {
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
		}

		//Select a starting weapon.
		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
			client->ps.weapon = WP_SABER;
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_CONCUSSION))
			client->ps.weapon = WP_CONCUSSION;
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_DISRUPTOR))
			client->ps.weapon = WP_DISRUPTOR;
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL))
			client->ps.weapon = WP_BRYAR_PISTOL;
		else if(client->ps.stats[STAT_WEAPONS] & (1 << WP_MELEE))
			client->ps.weapon = WP_MELEE;

		ClientSpawn_GameModes(ent);

		if (client->ps.weapon == WP_SABER && g_gametype.integer != GT_FFA)
			G_SetAnim(ent, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
		else
		{
			G_SetAnim(ent, SETANIM_TORSO, TORSO_RAISEWEAP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
			client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];
		}
		client->ps.weaponstate = WEAPON_RAISING;
		client->ps.weaponTime = client->ps.torsoTimer;

		G_KillBox( ent );
		trap_LinkEntity (ent);
	}

	//RoboPhred: Always do this, might need to set profession specific stuff.
	//if (client->ps.fd.forceDoInit){ //force a reread of force powers
		WP_InitForcePowers( ent );
		client->ps.fd.forceDoInit = 0;
	//}

	if (client->ps.stats[STAT_MAX_HEALTH] <= 100 && g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND)
		client->ps.stats[STAT_HEALTH] = (int)(client->ps.stats[STAT_MAX_HEALTH] * 1.25);
	else if (client->ps.stats[STAT_MAX_HEALTH] < 125 && g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND)
		client->ps.stats[STAT_HEALTH] = 125;
	else
		client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH];
	ent->health = client->ps.stats[STAT_HEALTH];

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;


	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	if ( level.intermissiontime )
		MoveClientToIntermission( ent );
	else {
		spawnPoint = ClientSpawn_GetSpawnpoint(ent, spawn_origin, spawn_angles);

		G_SetOrigin( ent, spawn_origin );
		VectorCopy( spawn_origin, client->ps.origin );


		trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
		SetClientViewAngle( ent, spawn_angles );

		if(spawnPoint){
			//RoboPhred: "else if" not needed, but meh
			G_UseTargets( spawnPoint, ent );
		}
	}

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;

	//RoboPhred: ravensoft forgot this.  Does not affect too much, except the no-use-when-spectating code.
	if(client->sess.sessionTeam != TEAM_SPECTATOR)
		client->sess.spectatorState = SPECTATOR_NOT;

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( index, NULL );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	if (g_spawnInvulnerability.integer)
	{
		ent->client->ps.eFlags |= EF_INVULNERABLE;
		ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	//Lugormod
	if ( g_gametype.integer == GT_FFA )
		ent->client->ps.saberHolstered = 2;

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	//RoboPhred: Fix the client number that was changed in PlayerStateToEntityState on spectators ("/team follow[1/2]").
	ent->s.number = index;

	//Update the configstring, as thats where the player's team info comes from.
	ClientUserinfoChanged_Do(index);

	//rww - make sure client has a valid icarus instance
	trap_ICARUS_FreeEnt( ent );
	trap_ICARUS_InitEnt( ent );
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
//Lugormod:
extern gentity_t *g_bestKing;
void dropMoneyStash(gentity_t *ent);
void DropCredits (gentity_t *ent, int quantity);
void RevertKing (gentity_t *ent);
void Chicken (gentity_t *chicken);
void updatePlayer(gentity_t *ent);

void Confirm_Clear(gentity_t *ent);
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i,j,k;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	//Lugormod
	//if (!ent->client ) {
	if ( !ent->inuse || !ent->client ) {
		return;
	}

	updatePlayer(ent);
	Confirm_Clear(ent);
	Interact_Clear(ent);

	//Lugormod remove buddies and ignores
	j = (int)floor((float)clientNum / 16);
	k = clientNum % 16;
	for (i = 0;i < MAX_CLIENTS;i++) {
		tent = g_entities + i;
		if (!tent->client || tent == ent ||	tent->client->pers.connected != CON_CONNECTED){
			continue;
		}
		tent->client->pers.Lmd.buddyindex[j] &= ~(1 << k);
		tent->client->pers.Lmd.ignoredindex[j] &= ~(1 << k);
	}

	ent->client->Lmd.duel.challengedTime = level.time;
	Chicken(ent);
	//Lugormod remove hook entities
	if (ent->client->hook) {
		Merc_Unhook(ent);
	}


	if(ent->client->pers.Lmd.persistantFlags & SPF_SHUTUP) {
		Bans_AddIP(ent->client->sess.Lmd.ip, -1, "Temporarily banned: quit while muted.");
	}
	else if(ent->client->pers.Lmd.jailTime > level.time){
		Bans_AddIP(ent->client->sess.Lmd.ip, -1, "Temporarily banned: quit while jailed.");
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}
	if (ent->client &&
		ent->client->ps.isJediMaster) {
			if (g_gametype.integer == GT_JEDIMASTER){

				ThrowSaberToAttacker(ent,NULL);
				ent->maxHealth = 0;
			}
			ent->client->ps.isJediMaster = qfalse;

	}
	if (g_bestKing && g_bestKing->s.number == ent->s.number) {
		g_bestKing = NULL;
	}
	if (IsKing(ent)) {
		//BecomeCommoner(ent);
		RevertKing(ent);
	}


	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	if (ent->client->ps.m_iVehicleNum)
	{ //tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			int pCon = ent->client->pers.connected;

			ent->client->pers.connected = 0;
			veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
			ent->client->pers.connected = pCon;
		}
	}

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
				StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED 
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
			if(tent)
				tent->s.clientNum = ent->s.clientNum;

			// They don't get to take powerups with them!
			// Especially important for stuff like CTF flags
			TossClientItems( ent );

	}
	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );
	Com_Printf("info: %s disconnected (%i).\n", ent->client->pers.netname,
		clientNum);
	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ( (g_gametype.integer == GT_DUEL )
		&& !level.intermissiontime
		&& !level.warmupTime ) {
			if ( level.sortedClients[1] == clientNum ) {
				level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] = 0;
				level.clients[ level.sortedClients[0] ].sess.wins++;
				ClientUserinfoChanged_Do( level.sortedClients[0] );
			}
			else if ( level.sortedClients[0] == clientNum ) {
				level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] = 0;
				level.clients[ level.sortedClients[1] ].sess.wins++;
				ClientUserinfoChanged_Do( level.sortedClients[1] );
			}
	}
	//Com_Printf("info: Free ghoul2.\n");

	if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&ent->ghoul2);
	}
	i = 0;
	//Com_Printf("info: Free saber ghoul2.\n");

	while (i < MAX_SABERS)
	{
		if (ent->client->weaponGhoul2[i] && trap_G2_HaveWeGhoul2Models(ent->client->weaponGhoul2[i]))
		{
			trap_G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[i]);
		}
		i++;
	}

	//Com_Printf("info: Freeing done.\n");

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	ent->r.contents = 0;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}

	G_ClearClientLog(clientNum);
	//Com_Printf("info: Disconnecting done.\n");
}
