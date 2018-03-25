//#include "g_local.h"
#include "b_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include "../ghoul2/G2.h"

#include "Lmd_Accounts_Friends.h"
#include "Lmd_Professions.h"
#include "Lmd_Prof_Merc.h"
#include "Lmd_Commands_Auths.h"

extern void G_Knockdown( gentity_t *victim, int duration );

int Jedi_GetForceRegenDebounce(gentity_t *ent);

#define METROID_JUMP 1

//RoboPhred
qboolean isBuddy(gentity_t *ent,gentity_t *other);

//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void NPC_PlayConfusionSound( gentity_t *self );
extern void NPC_Jedi_PlayConfusionSound( gentity_t *self );
extern void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone );
//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void Jedi_Decloak( gentity_t *self );

extern vmCvar_t		g_saberRestrictForce;

#include "../namespace_begin.h"
extern qboolean BG_FullBodyTauntAnim( int anim );
#include "../namespace_end.h"

extern bot_state_t *botstates[MAX_CLIENTS];

int speedLoopSound = 0;

int rageLoopSound = 0;

int protectLoopSound = 0;

int absorbLoopSound = 0;

int seeLoopSound = 0;

int	ysalamiriLoopSound = 0;

#define FORCE_VELOCITY_DAMAGE 0

#ifndef LMD_NEW_FORCEPOWERS
int ForceShootDrain( gentity_t *self );
#endif

gentity_t *G_PreDefSound(vec3_t org, int pdSound)
{
	gentity_t	*te;

	te = G_TempEntity( org, EV_PREDEFSOUND );
	if(te){
		te->s.eventParm = pdSound;
		VectorCopy(org, te->s.origin);
	}

	return te;
}

const int forcePowerMinRank[NUM_FORCE_POWER_LEVELS + 2][NUM_FORCE_POWERS] = //0 == neutral
{
	{
		999,//FP_HEAL,//instant
			999,//FP_LEVITATION,//hold/duration
			999,//FP_SPEED,//duration
			999,//FP_PUSH,//hold/duration
			999,//FP_PULL,//hold/duration
			999,//FP_TELEPATHY,//instant
			999,//FP_GRIP,//hold/duration
			999,//FP_LIGHTNING,//hold/duration
			999,//FP_RAGE,//duration
			999,//FP_PROTECT,//duration
			999,//FP_ABSORB,//duration
			999,//FP_TEAM_HEAL,//instant
			999,//FP_TEAM_FORCE,//instant
			999,//FP_DRAIN,//hold/duration
			999,//FP_SEE,//duration
			999,//FP_SABER_OFFENSE,
			999,//FP_SABER_DEFENSE,
			999//FP_SABERTHROW,
			//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
			0,//FP_LEVITATION,//hold/duration
			0,//FP_SPEED,//duration
			0,//FP_PUSH,//hold/duration
			0,//FP_PULL,//hold/duration
			10,//FP_TELEPATHY,//instant
			15,//FP_GRIP,//hold/duration
			10,//FP_LIGHTNING,//hold/duration
			15,//FP_RAGE,//duration
			15,//FP_PROTECT,//duration
			15,//FP_ABSORB,//duration
			10,//FP_TEAM_HEAL,//instant
			10,//FP_TEAM_FORCE,//instant
			10,//FP_DRAIN,//hold/duration
			5,//FP_SEE,//duration
			0,//FP_SABER_OFFENSE,
			0,//FP_SABER_DEFENSE,
			0//FP_SABERTHROW,
			//NUM_FORCE_POWERS
		},
		{
			10,//FP_HEAL,//instant
				0,//FP_LEVITATION,//hold/duration
				0,//FP_SPEED,//duration
				0,//FP_PUSH,//hold/duration
				0,//FP_PULL,//hold/duration
				10,//FP_TELEPATHY,//instant
				15,//FP_GRIP,//hold/duration
				10,//FP_LIGHTNING,//hold/duration
				15,//FP_RAGE,//duration
				15,//FP_PROTECT,//duration
				15,//FP_ABSORB,//duration
				10,//FP_TEAM_HEAL,//instant
				10,//FP_TEAM_FORCE,//instant
				10,//FP_DRAIN,//hold/duration
				5,//FP_SEE,//duration
				5,//FP_SABER_OFFENSE,
				5,//FP_SABER_DEFENSE,
				5//FP_SABERTHROW,
				//NUM_FORCE_POWERS
		},
		{
			10,//FP_HEAL,//instant
				0,//FP_LEVITATION,//hold/duration
				0,//FP_SPEED,//duration
				0,//FP_PUSH,//hold/duration
				0,//FP_PULL,//hold/duration
				10,//FP_TELEPATHY,//instant
				15,//FP_GRIP,//hold/duration
				10,//FP_LIGHTNING,//hold/duration
				15,//FP_RAGE,//duration
				15,//FP_PROTECT,//duration
				15,//FP_ABSORB,//duration
				10,//FP_TEAM_HEAL,//instant
				10,//FP_TEAM_FORCE,//instant
				10,//FP_DRAIN,//hold/duration
				5,//FP_SEE,//duration
				10,//FP_SABER_OFFENSE,
				10,//FP_SABER_DEFENSE,
				10//FP_SABERTHROW,
				//NUM_FORCE_POWERS
			},
			{
				10,//FP_HEAL,//instant
					0,//FP_LEVITATION,//hold/duration
					0,//FP_SPEED,//duration
					0,//FP_PUSH,//hold/duration
					0,//FP_PULL,//hold/duration
					10,//FP_TELEPATHY,//instant
					15,//FP_GRIP,//hold/duration
					10,//FP_LIGHTNING,//hold/duration
					15,//FP_RAGE,//duration
					15,//FP_PROTECT,//duration
					15,//FP_ABSORB,//duration
					10,//FP_TEAM_HEAL,//instant
					10,//FP_TEAM_FORCE,//instant
					10,//FP_DRAIN,//hold/duration
					5,//FP_SEE,//duration
					15,//FP_SABER_OFFENSE,
					15,//FP_SABER_DEFENSE,
					15//FP_SABERTHROW,
					//NUM_FORCE_POWERS
			},
			{
				10,//FP_HEAL,//instant
					0,//FP_LEVITATION,//hold/duration
					0,//FP_SPEED,//duration
					0,//FP_PUSH,//hold/duration
					0,//FP_PULL,//hold/duration
					10,//FP_TELEPATHY,//instant
					15,//FP_GRIP,//hold/duration
					10,//FP_LIGHTNING,//hold/duration
					15,//FP_RAGE,//duration
					15,//FP_PROTECT,//duration
					15,//FP_ABSORB,//duration
					10,//FP_TEAM_HEAL,//instant
					10,//FP_TEAM_FORCE,//instant
					10,//FP_DRAIN,//hold/duration
					5,//FP_SEE,//duration
					20,//FP_SABER_OFFENSE,
					20,//FP_SABER_DEFENSE,
					20//FP_SABERTHROW,
					//NUM_FORCE_POWERS
				}
};

int mindTrickTime[NUM_FORCE_POWER_LEVELS + 2] =
{
	0,//none
	5000,
	10000,
	15000,
	20000,
	30000,
};

int RebornForcePowerLevels[NUM_FORCE_POWERS] = {	
	0, //FP_HEAL            L
	4, //FP_LEVITATION      N
	2, //FP_SPEED           N
	3, //FP_PUSH            N
	0, //FP_PULL            N
	0, //FP_TELEPATHY       L
	2, //FP_GRIP            D
	2, //FP_LIGHTNING       D
	0, //FP_RAGE            D
	0, //FP_PROTECT         L
	0, //FP_ABSORB          L
	0, //FP_TEAM_HEAL       L
	0, //FP_TEAM_FORCE      D
	1, //FP_DRAIN           D
	3, //FP_SEE             N
	3, //FP_SABER_OFFENSE   S
	2, //FP_SABER_DEFENSE   S
	2  //FP_SABERTHROW      S
};


#ifdef LMD_NEW_FORCEPOWERS
//Temporary glue functions
qboolean Force_UsePower(gentity_t *ent, int power);
void ForceHeal(gentity_t *ent) {
	Force_UsePower(ent, FP_HEAL);
}
void ForceJump(gentity_t *ent, usercmd_t *unused) {
	Force_UsePower(ent, FP_LEVITATION);
}
void ForceSpeed(gentity_t *ent, int unused) {
	Force_UsePower(ent, FP_SPEED);
}
void ForceThrow(gentity_t *ent, qboolean pull) {
	Force_UsePower(ent, (pull)?FP_PULL:FP_PUSH);
}
void ForceTelepathy(gentity_t *ent) {
	Force_UsePower(ent, FP_TELEPATHY);
}
void ForceGrip(gentity_t *ent) {
	Force_UsePower(ent, FP_GRIP);
}
void ForceLightning(gentity_t *ent) {
	Force_UsePower(ent, FP_LIGHTNING);
}
void ForceRage(gentity_t *ent) {
	Force_UsePower(ent, FP_RAGE);
}
void ForceProtect(gentity_t *ent) {
	Force_UsePower(ent, FP_PROTECT);
}
void ForceAbsorb(gentity_t *ent) {
	Force_UsePower(ent, FP_ABSORB);
}
void ForceTeamHeal(gentity_t *ent) {
	Force_UsePower(ent, FP_TEAM_HEAL);
}
void ForceTeamForceReplenish(gentity_t *ent) {
	Force_UsePower(ent, FP_TEAM_FORCE);
}
void ForceDrain(gentity_t *ent) {
	Force_UsePower(ent, FP_DRAIN);
}
void ForceSeeing(gentity_t *ent) {
	Force_UsePower(ent, FP_SEE);
}
#else
void ForceThrow_Old( gentity_t *self, qboolean pull );
void ForceThrow(gentity_t *ent, qboolean pull) {
	ForceThrow_Old(ent, pull);
}
#endif




// if the force string is incorrect, this one will be used
#define GENERIC_FORCE "7-1-033330000000000333"
// masks: no values outside these boundaries will be accepted
#define FORCE_LOWER "0-1-000000000000000000"
#define FORCE_UPPER "7-2-333333333333333333"

extern vmCvar_t g_regForceRank; //Lugormod
qboolean WP_HasForcePowers( const playerState_t *ps );//Lugormod
//RoboPhred
void Jedi_SetupForce(gentity_t *ent);
void WP_InitForcePowers( gentity_t *ent )
{
	int i;
	int i_r;
	int maxRank = g_maxForceRank.integer;
	qboolean warnClient = qfalse;
	qboolean warnClientLimit = qfalse;
	char userinfo[MAX_INFO_STRING];
	char forcePowers[256];
	char readBuf[256];
	int lastFPKnown = -1;
	qboolean didEvent = qfalse;

	int prof = 0;
	if(PlayerAcc_Prof_CanUseProfession(ent))
		prof = PlayerAcc_Prof_GetProfession(ent);
	
	if (!maxRank)
	{ //if server has no max rank, default to max (50)
		maxRank = FORCE_MASTERY_JEDI_MASTER;
	}
	else if (maxRank >= NUM_FORCE_MASTERY_LEVELS)
	{//ack, prevent user from being dumb
		maxRank = FORCE_MASTERY_JEDI_MASTER;
		trap_Cvar_Set( "g_maxForceRank", va("%i", maxRank) );
	}
	else if (maxRank < 0)
	{
		maxRank = 0;
		trap_Cvar_Set("g_maxForceRank", "0");
	}

	/*
	if (g_forcePowerDisable.integer)
	{
	maxRank = FORCE_MASTERY_UNINITIATED;
	}
	*/
	//rww - don't do this

	if ( !ent || !ent->client )
	{
		return;
	}

	ent->client->ps.fd.saberAnimLevel = ent->client->sess.saberLevel;

	if (ent->client->ps.fd.saberAnimLevel < FORCE_LEVEL_1 ||
		ent->client->ps.fd.saberAnimLevel > FORCE_LEVEL_3)
	{
		ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}

	if (!speedLoopSound)
	{ //so that the client configstring is already modified with this when we need it
		speedLoopSound = G_SoundIndex("sound/weapons/force/speedloop.wav");
	}

	if (!rageLoopSound)
	{
		rageLoopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
	}

	if (!absorbLoopSound)
	{
		absorbLoopSound = G_SoundIndex("sound/weapons/force/absorbloop.wav");
	}

	if (!protectLoopSound)
	{
		protectLoopSound = G_SoundIndex("sound/weapons/force/protectloop.wav");
	}

	if (!seeLoopSound)
	{
		seeLoopSound = G_SoundIndex("sound/weapons/force/seeloop.wav");
	}

	if (!ysalamiriLoopSound)
	{
		ysalamiriLoopSound = G_SoundIndex("sound/player/nullifyloop.wav");
	}

	if (ent->s.eType == ET_NPC)
	{ //just stop here then.
		return;
	}        

	if(prof && prof != PROF_BOT){
		maxRank = 1;
	}

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerLevel[i] = 0;
		ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		i++;
	}

	ent->client->ps.fd.forcePowerSelected = -1;

	ent->client->ps.fd.forceSide = 0;

	//RoboPhred: mercs dont get any powers
	if(PlayerAcc_Prof_GetProfession(ent) == PROF_MERC)
		return;

	if ((g_gametype.integer == GT_SIEGE ||
		g_gametype.integer == GT_BATTLE_GROUND) &&
		ent->client->siegeClass != -1)
	{ //Then use the powers for this class, and skip all this nonsense.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}

		if (!ent->client->sess.setForce 
			&& g_gametype.integer != GT_BATTLE_GROUND)
		{
			//bring up the class selection menu
			trap_SendServerCommand(ent-g_entities, "scl");
		}
		ent->client->sess.setForce = qtrue;

		return;
	}
	//RoboPhred: Can use accounts or skill select.
	/*
	else if (g_gametype.integer == GT_REBORN) {
		i = 0;

		//Otherwise, give default powers.
		while (i < NUM_FORCE_POWERS)
		{
			if (ent->client->sess.sessionTeam == TEAM_RED) {
				ent->client->ps.fd.forcePowerLevel[i] = RebornForcePowerLevels[i];
			} else {
				ent->client->ps.fd.forcePowerLevel[i] = 0;
			}

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
		return;
	}
	*/

	if (ent->s.eType == ET_NPC && ent->s.number >= MAX_CLIENTS)
	{ //rwwFIXMEFIXME: Temp
		strcpy(userinfo, "forcepowers\\7-1-333003000313003120");
	}
	else
	{
		trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
	}

	if ( (ent->r.svFlags & SVF_BOT) && botstates[ent->s.number] )
	{ //if it's a bot just copy the info directly from its personality
		Com_sprintf(forcePowers, sizeof(forcePowers), "%s\0", botstates[ent->s.number]->forceinfo);
	}
	else {
		char *p = forcePowers, *pu = FORCE_UPPER, *pl = FORCE_LOWER;
		Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof(forcePowers));
		if (!p || strlen(p) != 22) {
			Q_strncpyz( forcePowers, GENERIC_FORCE, sizeof( forcePowers ) );
		}
		else {
			while(*p) {
				if (*p > *pu++ || *p++ < *pl++) {
					Q_strncpyz( forcePowers, GENERIC_FORCE, sizeof( forcePowers ) );
				}
			}
		}
	}



	if(prof == PROF_JEDI){
		Jedi_SetupForce(ent);
	}
	else if(prof == PROF_ADMIN){ 
		i = 0;
		while (i < NUM_FORCE_POWERS){
			ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_5;
			ent->client->ps.fd.forcePowersKnown |= (1 << i);
			if (i == FP_SEE && ent->client->ps.fd.forcePowerLevel[i] > FORCE_LEVEL_3)
				ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3; //Lugormod for now

			i++;
		}
		ent->client->ps.fd.forceSide = 0;
	}
	else if(prof == PROF_MERC){
		// RoboPhred: Was part of indexed skills, but not using that right now
#if 0
		i = 0;
		while(i < SK_MERC_NUM_SKILLS && i < NUM_FORCE_POWERS){
			ent->client->ps.fd.forcePowerLevel[i] = PlayerAcc_Prof_GetSkill(ent, PROF_MERC, i);
			i++;
		}
#else
		i = 0;
		while(i < NUM_FORCE_POWERS) {
			ent->client->ps.fd.forcePowerLevel[i] = 0;
		}
#endif
		//RoboPhred
		ent->client->ps.fd.forcePowersKnown = 0;
		return;
	}
	else {
		//RoboPhred: only do this if we dont have other force powers set already.

		//rww - parse through the string manually and eat out all the appropriate data
		i = 0;

		//RoboPhred: also reborn
		if (g_forceBasedTeams.integer || g_gametype.integer == GT_REBORN)
		{
			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), FORCE_DARKSIDE, g_gametype.integer, g_forcePowerDisable.integer));
			}
			else if (ent->client->sess.sessionTeam == TEAM_BLUE)
			{
				warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), FORCE_LIGHTSIDE, g_gametype.integer, g_forcePowerDisable.integer));
			}
			else
			{
				warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), 0, g_gametype.integer, g_forcePowerDisable.integer));
			}
		}
		else
		{
			warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), 0, g_gametype.integer, g_forcePowerDisable.integer));
		}

		i_r = 0;
		while (forcePowers[i] && forcePowers[i] != '-')
		{
			readBuf[i_r] = forcePowers[i];
			i_r++;
			i++;
		}
		readBuf[i_r] = 0;
		//THE RANK
		ent->client->ps.fd.forceRank = atoi(readBuf);
		i++;

		i_r = 0;
		while (forcePowers[i] && forcePowers[i] != '-')
		{
			readBuf[i_r] = forcePowers[i];
			i_r++;
			i++;
		}
		readBuf[i_r] = 0;
		//THE SIDE
		ent->client->ps.fd.forceSide = atoi(readBuf);
		i++;

		//THE POWERS
		qboolean lock = qfalse; //punish morons.
		i_r = 0;
		//RoboPhred:
		qboolean hasSidePower = qfalse;
		while (forcePowers[i] && forcePowers[i] != '\n' && i_r < NUM_FORCE_POWERS){
			readBuf[0] = forcePowers[i];
			readBuf[1] = 0;
			ent->client->ps.fd.forcePowerLevel[i_r] = atoi(readBuf);
			if (ent->client->ps.fd.forcePowerLevel[i_r] < FORCE_LEVEL_0 || ent->client->ps.fd.forcePowerLevel[i_r] > FORCE_LEVEL_5) {
				lock = qtrue;
				break;
			}
			//RoboPhred
			//if servers dont want this, then they should lower their force rank.
			//if(prof == PROF_NONE && ent->client->ps.fd.forcePowerLevel[i_r] > 3)
			//	ent->client->ps.fd.forcePowerLevel[i_r] = 3;
			if (ent->client->ps.fd.forcePowerLevel[i_r]){
				ent->client->ps.fd.forcePowersKnown |= (1 << i_r);
				if(forcePowerDarkLight[i_r] == ent->client->ps.fd.forceSide)
					hasSidePower = qtrue;
			}
			else{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i_r);
			}
			i++;
			i_r++;
		}

		if(lock) {
			ent->client->ps.fd.forcePowersKnown = 0;
			for(i = 0; i < NUM_FORCE_POWERS; i++) {
				ent->client->ps.fd.forcePowerLevel[i] = 0;
			}
		}

		//RoboPhred
		if(g_gametype.integer >= GT_TEAM && ent->client->ps.fd.forceSide != 0 && !hasSidePower)
			ent->client->sess.setForce = qfalse;
	}



	if (ent->s.eType != ET_NPC)
	{
		if (HasSetSaberOnly())
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FREE_SABER );
			//RoboPhred
			if(te) {
				te->r.svFlags |= SVF_BROADCAST;
				te->s.eventParm = 1;
			}
		}
		else
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FREE_SABER );
			//RoboPhred
			if(te){
				te->r.svFlags |= SVF_BROADCAST;
				te->s.eventParm = 0;
			}
		}

		if (g_forcePowerDisable.integer)
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FORCE_DISABLE );
			//RoboPhred
			if(te){
				te->r.svFlags |= SVF_BROADCAST;
				te->s.eventParm = 1;
			}
		}
		else
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FORCE_DISABLE );
			//RoboPhred
			if(te){
				te->r.svFlags |= SVF_BROADCAST;
				te->s.eventParm = 0;
			}
		}
	}

	if (ent->s.eType == ET_NPC)
	{
		ent->client->sess.setForce = qtrue;
	}
	else if (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) //Lugormod
	{
		if (!ent->client->sess.setForce)
		{
			ent->client->sess.setForce = qtrue;
			//bring up the class selection menu
			trap_SendServerCommand(ent-g_entities, "scl");
		}
	}
	else
	{
		if (warnClient || !ent->client->sess.setForce)
		{ //the client's rank is too high for the server and has been autocapped, so tell them
			//RoboPhred: in reborn, bring up the screen straight away
			if(g_gametype.integer == GT_REBORN) {
				trap_SendServerCommand(ent-g_entities, "spc");	// Fire up the profile menu
			}
			else if (g_gametype.integer != GT_HOLOCRON && g_gametype.integer != GT_JEDIMASTER 
				&& g_gametype.integer != GT_REBORN)
			{
#ifdef EVENT_FORCE_RANK
				gentity_t *te = G_TempEntity( vec3_origin, EV_GIVE_NEW_RANK );

				te->r.svFlags |= SVF_BROADCAST;
				te->s.trickedentindex = ent->s.number;
				te->s.eventParm = maxRank;
				te->s.bolt1 = 0;
#endif
				didEvent = qtrue;

				if (!(ent->r.svFlags & SVF_BOT) && ent->s.eType != ET_NPC){
					if (!g_teamAutoJoin.integer && g_gametype.integer != GT_BATTLE_GROUND) {
						//Make them a spectator so they can set their powerups up without being bothered.
						ent->client->sess.sessionTeam = TEAM_SPECTATOR;
						ent->client->sess.spectatorState = SPECTATOR_FREE;
						ent->client->sess.spectatorClient = 0;

						ent->client->pers.teamState.state = TEAM_BEGIN;
						trap_SendServerCommand(ent-g_entities, "spc");	// Fire up the profile menu
					}
				}

#ifdef EVENT_FORCE_RANK
				te->s.bolt2 = ent->client->sess.sessionTeam;
#else
				//Event isn't very reliable, I made it a string. This way I can send it to just one
				//client also, as opposed to making a broadcast event.
				//Lugormod changed second argument to 0
				trap_SendServerCommand(ent->s.number, va("nfr %i %i %i", maxRank, 0, ent->client->sess.sessionTeam));
				//Arg1 is new max rank, arg2 is non-0 if force menu should be shown, arg3 is the current team
#endif
			}
			ent->client->sess.setForce = qtrue;
		}

		if (!didEvent)
		{
#ifdef EVENT_FORCE_RANK
			gentity_t *te = G_TempEntity( vec3_origin, EV_GIVE_NEW_RANK );

			te->r.svFlags |= SVF_BROADCAST;
			te->s.trickedentindex = ent->s.number;
			te->s.eventParm = maxRank;
			te->s.bolt1 = 1;
			te->s.bolt2 = ent->client->sess.sessionTeam;
#else
			trap_SendServerCommand(ent->s.number, va("nfr %i %i %i", maxRank, 0, ent->client->sess.sessionTeam));
#endif
		}

		if (warnClientLimit)
		{ //the server has one or more force powers disabled and the client is using them in his config
			//trap_SendServerCommand(ent-g_entities, va("print \"The server has one or more force powers that you have chosen disabled.\nYou will not be able to use the disable force power(s) while playing on this server.\n\""));
		}
	}

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((ent->client->ps.fd.forcePowersKnown & (1 << i)) &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{ //err..
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}
		else
		{
			if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW)
			{
				lastFPKnown = i;
			}
		}

		i++;
	}

	if (ent->client->ps.fd.forcePowersKnown & ent->client->sess.selectedFP)
	{
		ent->client->ps.fd.forcePowerSelected = ent->client->sess.selectedFP;
	}

	if (!(ent->client->ps.fd.forcePowersKnown & (1 << ent->client->ps.fd.forcePowerSelected)))
	{
		if (lastFPKnown != -1)
		{
			ent->client->ps.fd.forcePowerSelected = lastFPKnown;
		}
		else
		{
			ent->client->ps.fd.forcePowerSelected = 0;
		}
	}

	if (gameMode(GM_ALLWEAPONS)) {
		if ((g_gametype.integer < GT_TEAM && WP_HasForcePowers(&ent->client->ps)) || (g_gametype.integer >= GT_TEAM	&&
			ent->client->sess.sessionTeam == TEAM_BLUE)) {
				i = 0;
				while (i < NUM_FORCE_POWERS){
					if (g_gametype.integer >= GT_TEAM || (i != FP_DRAIN && i != FP_ABSORB)){
						ent->client->ps.fd.forcePowerLevel[i] = 3;
						ent->client->ps.fd.forcePowersKnown |=(1 << i);
					}
					i++;
				}
		}
		else{
			while (i < NUM_FORCE_POWERS)
			{
				ent->client->ps.fd.forcePowerLevel[i] = 0;
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
				i++;
			}
		}
	}
	if (gameMode(GMF_WITH_FORCE_JUMP)){
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 3;
		ent->client->ps.fd.forcePowersKnown |=(1 << FP_LEVITATION);
	}

	//Ufo:
	if (ent->client->Lmd.restrict & 32) {
		ent->client->Lmd.backupJumpLevel = ent->client->ps.fd.forcePowerLevel[FP_LEVITATION];
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 0;
	}

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerBaseLevel[i] = ent->client->ps.fd.forcePowerLevel[i];
		i++;
	}
	ent->client->ps.fd.forceUsingAdded = 0;
}

void WP_SpawnInitForcePowers( gentity_t *ent )
{
	int i = 0;

	ent->client->ps.saberAttackChainCount = 0;

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}

		i++;
	}

	ent->client->ps.fd.forceDeactivateAll = 0;

	//RoboPhred
	if(PlayerAcc_Prof_GetProfession(ent) < PROF_JEDI)
		ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax = FORCE_POWER_MAX;

	ent->client->ps.fd.forcePowerRegenDebounceTime = 0;
	ent->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceMindtrickTargetIndex = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex2 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex3 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex4 = 0;

	ent->client->ps.holocronBits = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.holocronsCarried[i] = 0;
		i++;
	}

	if (g_gametype.integer == GT_HOLOCRON)
	{
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			i++;
		}

		if (HasSetSaberOnly())
		{
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			}
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			}
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerDebounce[i] = 0;
		ent->client->ps.fd.forcePowerDuration[i] = 0;

		i++;
	}

	ent->client->ps.fd.forcePowerRegenDebounceTime = 0;
	ent->client->ps.fd.forceJumpZStart = 0;
	ent->client->ps.fd.forceJumpCharge = 0;
	ent->client->ps.fd.forceJumpSound = 0;
	ent->client->ps.fd.forceGripDamageDebounceTime = 0;
	ent->client->ps.fd.forceGripBeingGripped = 0;
	ent->client->ps.fd.forceGripCripple = 0;
	ent->client->ps.fd.forceGripUseTime = 0;
	ent->client->ps.fd.forceGripSoundTime = 0;
	ent->client->ps.fd.forceGripStarted = 0;
	ent->client->ps.fd.forceHealTime = 0;
	ent->client->ps.fd.forceHealAmount = 0;
	ent->client->ps.fd.forceRageRecoveryTime = 0;
	ent->client->ps.fd.forceDrainEntNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceDrainTime = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((ent->client->ps.fd.forcePowersKnown & (1 << i)) &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{ //make sure all known powers are cleared if we have level 0 in them
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}

		i++;
	}

	if ((g_gametype.integer == GT_SIEGE 
		|| g_gametype.integer == GT_BATTLE_GROUND)
		&& ent->client->siegeClass != -1)
	{ //Then use the powers for this class.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
	} else if (g_gametype.integer == GT_REBORN) {

		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if (ent->client->sess.sessionTeam == TEAM_RED) {
				ent->client->ps.fd.forcePowerLevel[i] = RebornForcePowerLevels[i];
			} else {
				ent->client->ps.fd.forcePowerLevel[i] = 0;
			}

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
	}
}

#include "../namespace_begin.h"
extern qboolean BG_InKnockDown( int anim ); //bg_pmove.c
#include "../namespace_end.h"

qboolean meditateProtect (gentity_t *ent); //Lugormod

int ForcePowerUsableOn(gentity_t *attacker, gentity_t *other, forcePowers_t forcePower){

	if (other && other->client && BG_HasYsalamiri(g_gametype.integer, &other->client->ps))
		return 0;

	if (attacker && attacker->client && !BG_CanUseFPNow(g_gametype.integer, &attacker->client->ps, level.time, forcePower))
		return 0;

	//Dueling fighters cannot use force powers on others, with the exception of force push when locked with each other
	if (attacker && attacker->client && duelInProgress(&attacker->client->ps)
		&& (attacker->client->Lmd.duel.duelType & (DT_FORCE|DT_FULL_FORCE)) == 0)
		return 0;
	//Lugormod Meditate protect
	if(other && other->client){
		if(meditateProtect(other))
			return 0;
		//Lugormod godmode protects.
		//Ufo:
		if(other->flags & FL_GODMODE) 
			return 0;
		if(other->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)
			return 0;

		//Ufo:
		if(other->client->pers.Lmd.persistantFlags & SPF_IONLYSABER && !other->client->ps.m_iVehicleNum && forcePower != FP_DRAIN)
			return 0;

		//Lugormod Only use force on opponent in duel.
		if(duelInProgress(&other->client->ps) && other->client->ps.duelIndex != attacker->s.number)
			return 0;
		if(duelInProgress(&attacker->client->ps) && attacker->client->ps.duelIndex != other->s.number){
			return 0;
		}
		//Ufo:
		if (other->client->Lmd.restrict & 2)
			return 0;
		if (other->client->invulnerableTimer > level.time && forcePower != FP_DRAIN && forcePower != FP_TELEPATHY)
			return 0;
	}

	if (forcePower == FP_GRIP){
		if (other && other->client && (other->client->ps.fd.forcePowersActive & (1<<FP_ABSORB))){ //don't allow gripping to begin with if they are absorbing
			//play sound indicating that attack was absorbed
			if (other->client->forcePowerSoundDebounce < level.time){
				gentity_t *abSound = G_PreDefSound(other->client->ps.origin, PDSOUND_ABSORBHIT);
				abSound->s.trickedentindex = other->s.number;
				other->client->forcePowerSoundDebounce = level.time + 400;
			}
			return 0;
		}
		else if (other && other->client && other->client->ps.weapon == WP_SABER && BG_SaberInSpecial(other->client->ps.saberMove)){
			//don't grip person while they are in a special or some really bad things can happen.
			return 0;
		}
	}

	if (other && other->client && (forcePower == FP_PUSH || forcePower == FP_PULL) && BG_InKnockDown(other->client->ps.legsAnim))
	{
		//Ufo: it's optional, so why not
		if (!(g_fixForce.integer & (1 << forcePower)) || !duelInProgress(&attacker->client->ps) || !(attacker->client->Lmd.duel.duelType & DT_FULL_FORCE))
			return 0;
	}

	if (other && other->client && other->s.eType == ET_NPC && other->s.NPC_class == CLASS_VEHICLE){
		//can't use the force on vehicles.. except lightning
		//Lugormod hate this, removing it
		if (forcePower == FP_LIGHTNING && (g_gametype.integer != GT_FFA || attacker->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_5))
			return 1;
		else if ((forcePower == FP_PUSH || forcePower == FP_PULL) && attacker->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_5)
			return 1;
		else
			return 0;
	}
	if ((forcePower == FP_GRIP || g_gametype.integer == GT_FFA) //Lugormod
		&& attacker->client->ps.fd.forcePowerLevel[forcePower] < FORCE_LEVEL_5 // Lugormod
		&& other && other->client && other->s.eType == ET_NPC && (Q_stricmp(other->NPC_type, "rancor") == 0	||
		Q_stricmp(other->NPC_type, "mutant_rancor") == 0))
		return 0;


	//can't use powers at all on npc's normally in siege...
	if (other && other->client && other->s.eType == ET_NPC && (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND))
		return 0;

	return 1;
}

//RoboPhred
// Never found out what caused this, shutting off log since its growing too large.
// Suspecting malacious clients sending unfiltered values, or general data corruption.
//	Reports are that this was logged often for random clients, so probably corruption.
//	Did have someone apparently abusing this though.
//void WP_LogWeirdStuff(char *error) {
//	fileHandle_t f;
//	G_LogPrintf("Weird force power stuff: %s\n", error);
//	trap_FS_FOpenFile("log_forcepowererrors.txt", &f, FS_APPEND_SYNC);
//	trap_FS_Write(error, strlen(error), f);
//	trap_FS_FCloseFile(f);
//}

qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	// RoboPhred: Check for invalid force powers.
	if (forcePower < 0 || forcePower >= NUM_FORCE_POWERS) {
		//WP_LogWeirdStuff(va("Client %s tried to get available power for invalid power %i\n", self->client->pers.netname, forcePower));
		return qfalse;
	}

	// RoboPhred: Check for invalid force levels.
	int level = self->client->ps.fd.forcePowerLevel[forcePower];
	if (level < FORCE_LEVEL_0 || level > FORCE_LEVEL_5) {
		//WP_LogWeirdStuff(va("Client %s tried to get available power for invalid level %i on power %i\n", self->client->pers.netname, level, forcePower));
		return qfalse;
	}

	int	drain = overrideAmt ? overrideAmt :	forcePowerNeeded[level][forcePower];

	if (meditateProtect(self))
	{
		//trap_SendServerCommand(self->s.number, "cp \"Force Powers are not available while meditating.\"");
		return qfalse;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << forcePower))
	{ //we're probably going to deactivate it..
		return qtrue;
	}
	if ( forcePower == FP_LEVITATION )
	{
		return qtrue;
	}

	//Ufo:
	if (self->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL) {
		return qfalse;
	}

	if (self->client->Lmd.restrict & 2
		&& forcePower != FP_SABER_OFFENSE
		&& forcePower != FP_SABER_DEFENSE)
	{
		return qfalse;
	}

	if (self->client->pers.Lmd.persistantFlags & SPF_IONLYSABER
		&& forcePower != FP_SABER_OFFENSE
		&& forcePower != FP_SABER_DEFENSE
		&& forcePower != FP_DRAIN
		&& forcePower != FP_HEAL)
	{
		return qfalse;
	}

	if ( !drain )
	{
		return qtrue;
	}
	if ((forcePower == FP_DRAIN || forcePower == FP_LIGHTNING) &&
		self->client->ps.fd.forcePower >= 25)
	{ //it's ok then, drain/lightning are actually duration
		return qtrue;
	}
	if ( self->client->ps.fd.forcePower < drain )
	{
		return qfalse;
	}
	return qtrue;
}

qboolean WP_ForcePowerInUse( gentity_t *self, forcePowers_t forcePower )
{
	if ( (self->client->ps.fd.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		return qtrue;
	}

	return qfalse;
}

qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower )
{
	if (BG_HasYsalamiri(g_gametype.integer, &self->client->ps))
	{
		return qfalse;
	}

	if (self->health <= 0 || self->client->ps.stats[STAT_HEALTH] <= 0 ||
		(self->client->ps.eFlags & EF_DEAD))
	{
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //specs can't use powers through people
		return qfalse;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	if (self->client->tempSpectate >= level.time)
	{
		return qfalse;
	}
	//Ufo: duplicate
	/*
	if ((forcePower == FP_TEAM_HEAL || forcePower == FP_TEAM_FORCE) &&
		(self->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)) {
			return qfalse;
	}
	*/

	if (!BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, forcePower))
	{
		return qfalse;
	}

	if ( !(self->client->ps.fd.forcePowersKnown & ( 1 << forcePower )) )
	{//don't know this power
		return qfalse;
	}

	if ( (self->client->ps.fd.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		if (forcePower != FP_LEVITATION)
		{
			return qfalse;
		}
	}

	if (forcePower == FP_LEVITATION && self->client->fjDidJump)
	{
		return qfalse;
	}

	if (!self->client->ps.fd.forcePowerLevel[forcePower])
	{
		return qfalse;
	}

	if ( g_debugMelee.integer )
	{
		if ( (self->client->ps.pm_flags&PMF_STUCK_TO_WALL) )
		{//no offensive force powers when stuck to wall
			switch ( forcePower )
			{
			case FP_GRIP:
			case FP_LIGHTNING:
			case FP_DRAIN:
			case FP_SABER_OFFENSE:
			case FP_SABER_DEFENSE:
			case FP_SABERTHROW:
				return qfalse;
				break;
			}
		}
	}

	if ( !self->client->ps.saberHolstered )
	{
		if ( (self->client->saber[0].saberFlags&SFL_TWO_HANDED) )
		{
			if ( g_saberRestrictForce.integer )
			{
				switch ( forcePower )
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
					break;
				}
			}
		}

		if ( (self->client->saber[0].saberFlags&SFL_TWO_HANDED)
			|| (self->client->saber[0].model && self->client->saber[0].model[0]) )
		{//this saber requires the use of two hands OR our other hand is using an active saber too
			if ( (self->client->saber[0].forceRestrictions&(1<<forcePower)) )
			{//this power is verboten when using this saber
				return qfalse;
			}
		}

		if ( self->client->saber[0].model 
			&& self->client->saber[0].model[0] )
		{//both sabers on
			if ( g_saberRestrictForce.integer )
			{
				switch ( forcePower )
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
					break;
				}
			}
			if ( (self->client->saber[1].forceRestrictions&(1<<forcePower)) )
			{//this power is verboten when using this saber
				return qfalse;
			}
		}
	}
	return WP_ForcePowerAvailable( self, forcePower, 0 );	// OVERRIDEFIXME
}

int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent)
{
	int getLevel = 0;
	int addTot = 0;
	gentity_t *abSound;

	if (atPower != FP_LIGHTNING &&
		atPower != FP_DRAIN &&
		atPower != FP_GRIP &&
		atPower != FP_PUSH &&
		atPower != FP_PULL)
	{ //Only these powers can be absorbed
		return -1;
	}


	if (!atdAbsLevel)
	{ //looks like attacker doesn't have any absorb power
		return -1;
	}

	// RoboPhred: Passive absorb if lvl 4 or 5.
	qboolean absorbActive = (attacked->client->ps.fd.forcePowersActive & (1 << FP_ABSORB));
	// RoboPhred: Not now
	//if (!absorbActive && atdAbsLevel < 4)
	//{ //absorb is not active
	//	return -1;
	//}
	// else
	if (!absorbActive) {
		// Passive, use lvl 1
		//atdAbsLevel = 1;
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	getLevel = atPowerLevel;
	getLevel -= atdAbsLevel;

	if (getLevel < 0)
	{
		getLevel = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	addTot = (atForceSpent/3)*attacked->client->ps.fd.forcePowerLevel[FP_ABSORB];

	if (addTot < 1 && atForceSpent >= 1)
	{
		addTot = 1;
	}
	attacked->client->ps.fd.forcePower += addTot;
	if (attacked->client->ps.fd.forcePower > 100)
	{
		attacked->client->ps.fd.forcePower = 100;
	}

	//play sound indicating that attack was absorbed
	if (attacked->client->forcePowerSoundDebounce < level.time)
	{
		abSound = G_PreDefSound(attacked->client->ps.origin, PDSOUND_ABSORBHIT);
		abSound->s.trickedentindex = attacked->s.number;

		attacked->client->forcePowerSoundDebounce = level.time + 400;
	}

	return getLevel;
}
extern vmCvar_t g_meditateExtraForce;//Lugormod
int Jedi_GetForceIncrease(gentity_t *ent);

void WP_ForcePowerRegenerate( gentity_t *self, int overrideAmt )
{ //called on a regular interval to regenerate force power.
	if ( !self->client )
	{
		return;
	}
	int extraForce = g_meditateExtraForce.integer;
	if(extraForce < 0)
		extraForce = 0;
	else if(extraForce > 100)
		extraForce = 100;

	int fpmax = self->client->ps.fd.forcePowerMax;

	if(g_meditateExtraForce.integer && g_gametype.integer == GT_FFA && self->client->ps.legsAnim == BOTH_MEDITATE
		&& self->client->ps.torsoAnim == BOTH_MEDITATE && PlayerAcc_Prof_GetProfession(self) <= PROF_JEDI)
		fpmax += extraForce;

	if(PlayerAcc_Prof_GetProfession(self) == PROF_JEDI) {
		fpmax += Jedi_GetForceIncrease(self);
	}

	if ( self->client->ps.fd.forcePower >= fpmax)
	{ //cap it off at the max (default 100)
		//self->client->ps.fd.forcePower = fpmax;
		return;
	}

	if ( overrideAmt )
	{ //custom regen amount
		self->client->ps.fd.forcePower += overrideAmt;
	}
	else
	{ //otherwise, just 1
		self->client->ps.fd.forcePower++;
	}
	if ( self->client->ps.fd.forcePower >= fpmax)
	{ //cap it off at the max (default 100)
		self->client->ps.fd.forcePower = fpmax;
	}
}

void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{ //activate the given force power
	int	duration = 0;
	qboolean hearable = qfalse;
	float hearDist = 0;


	if (!WP_ForcePowerAvailable( self, forcePower, overrideAmt ))
	{
		return;
	}

	if ( BG_FullBodyTauntAnim( self->client->ps.legsAnim ) )
	{//stop taunt
		self->client->ps.legsTimer = 0;
	}
	if ( BG_FullBodyTauntAnim( self->client->ps.torsoAnim ) )
	{//stop taunt
		self->client->ps.torsoTimer = 0;
	}
	//hearable and hearDist are merely for the benefit of bots, and not related to if a sound is actually played.
	//If duration is set, the force power will assume to be timer-based.
	switch( (int)forcePower )
	{
	case FP_HEAL:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_LEVITATION:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_SPEED:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2)
		{
			duration = 15000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_4)
		{
			duration = 25000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_5)
		{
			duration = 35000;
		}
		else //shouldn't get here
		{
			break;
		}

		if (overrideAmt)
		{
			duration = overrideAmt;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_PUSH:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
#endif
		break;
	case FP_PULL:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
#endif
		break;
	case FP_TELEPATHY:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
		{
			duration = 25000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_4)
		{
			duration = 40000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_5)
		{
			duration = 50000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_GRIP:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 60000;
#endif
		break;
	case FP_LIGHTNING:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 512;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
#endif
		break;
	case FP_RAGE:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
		{
			duration = 8000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
		{
			duration = 14000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_4)
		{
			duration = 25000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_5)
		{
			duration = 30000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_PROTECT:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_ABSORB:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_TEAM_HEAL:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_TEAM_FORCE:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_DRAIN:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		//self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN];
#endif
		break;
	case FP_SEE:
#ifndef LMD_NEW_FORCEPOWERS
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_4)
		{
			duration = 40000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_5)
		{
			duration = 60000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
#endif
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	if ( duration )
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = 0;
	}

	if (hearable)
	{
		self->client->ps.otherSoundLen = hearDist;
		self->client->ps.otherSoundTime = level.time + 100;
	}

	self->client->ps.fd.forcePowerDebounce[forcePower] = 0;

#ifndef LMD_NEW_FORCEPOWERS
	if ((int)forcePower == FP_SPEED && overrideAmt)
	{
		BG_ForcePowerDrain( &self->client->ps, forcePower, (int)(overrideAmt*0.025) );
	}
	else if ((int)forcePower != FP_GRIP && (int)forcePower != FP_DRAIN)
	{ //grip and drain drain as damage is done
		BG_ForcePowerDrain( &self->client->ps, forcePower, overrideAmt );
	}
#endif
}

#ifndef LMD_NEW_FORCEPOWERS
void ForceHeal( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_HEAL ) )
	{
		return;
	}

	if ( self->health >= self->client->ps.stats[STAT_MAX_HEALTH])
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_5)
	{
		self->health += 50;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_4)
	{
		self->health += 35;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_3)
	{
		self->health += 25; //This was 50, but that angered the Balance God.

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2)
	{
		self->health += 10;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
	}
	else
	{
		self->health += 5;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
	}
	/*
	else
	{
	WP_ForcePowerStart( self, FP_HEAL, 0 );
	}
	*/
	//NOTE: Decided to make all levels instant.

	G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav") );
}
#endif

void WP_AddToClientBitflags(gentity_t *ent, int entNum)
{
	if (!ent)
	{
		return;
	}

	if (entNum > 47)
	{
		ent->s.trickedentindex4 |= (1 << (entNum-48));
	}
	else if (entNum > 31)
	{
		ent->s.trickedentindex3 |= (1 << (entNum-32));
	}
	else if (entNum > 15)
	{
		ent->s.trickedentindex2 |= (1 << (entNum-16));
	}
	else
	{
		ent->s.trickedentindex |= (1 << entNum);
	}
}

#ifndef LMD_NEW_FORCEPOWERS
void ForceTeamHeal( gentity_t *self ){
	float radius = 256;
	int i = 0;
	gentity_t *ent;
	vec3_t a;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int healthadd = 0;
	gentity_t *te = NULL;

	if ( self->health <= 0 )
		return;

	if ( !WP_ForcePowerUsable( self, FP_TEAM_HEAL ) )
		return;

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] >= level.time)
		return;

	if(self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
		radius *= 1.5;
	else if(self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
		radius *= 2;
	else if(self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] >= FORCE_LEVEL_4)
		radius *= 3;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];
		//RoboPhred: cleaning this up, using isBuddy function
		/*
		int j,k;
		if (g_gametype.integer == GT_FFA) { //Lugormod
		j = (int)floor((float)i / 16);
		k = i % 16;
		}
		*/

		if(ent && ent->client && self != ent && (OnSameTeam(self, ent) || 
			(g_gametype.integer == GT_FFA && ent->client->sess.sessionTeam == TEAM_FREE &&
			//we dont have buddies or friends, or they are a buddy or friend
			((!(self->client->pers.Lmd.buddyindex[0] || self->client->pers.Lmd.buddyindex[1]) && !PlayerAcc_Friends_Count(self))
			|| isBuddy(self, ent) || PlayerAcc_Friends_IsFriend(self, ent->client->sess.Lmd.id)))) && ent->client->ps.stats[STAT_HEALTH] < ent->client->ps.stats[STAT_MAX_HEALTH] && ent->client->ps.stats[STAT_HEALTH] > 0 
			&& ForcePowerUsableOn(self, ent, FP_TEAM_HEAL) && trap_InPVS(self->client->ps.origin, ent->client->ps.origin)){
				VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

				if (VectorLength(a) <= radius)
				{
					pl[numpl] = i;
					numpl++;
				}
		}

		i++;
	}

	if (numpl < 1)
		return;

	if (numpl == 1)
		healthadd = 50;
	else if (numpl == 2)
		healthadd = 33;
	else
	{
		healthadd = 25;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_5)
	{
		healthadd *= 2;
	}

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] = level.time + 2000;
	i = 0;

	while (i < numpl)
	{
		if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > 0 &&
			g_entities[pl[i]].health > 0)
		{
			g_entities[pl[i]].client->ps.stats[STAT_HEALTH] += healthadd;
			if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH])
			{
				g_entities[pl[i]].client->ps.stats[STAT_HEALTH] = g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH];
			}

			g_entities[pl[i]].health = g_entities[pl[i]].client->ps.stats[STAT_HEALTH];

			//At this point we know we got one, so add him into the collective event client bitflag
			if (!te)
			{
				te = G_TempEntity( self->client->ps.origin, EV_TEAM_POWER);
				te->s.eventParm = 1; //eventParm 1 is heal, eventParm 2 is force regen

				//since we had an extra check above, do the drain now because we got at least one guy
				BG_ForcePowerDrain( &self->client->ps, FP_TEAM_HEAL, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL] );
			}

			WP_AddToClientBitflags(te, pl[i]);
			//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.
		}
		i++;
	}
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceTeamForceReplenish( gentity_t *self ){
	float radius = 256;
	int i = 0;
	gentity_t *ent;
	vec3_t a;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int poweradd = 0;
	gentity_t *te = NULL;

	if ( self->health <= 0 )
		return;

	if ( !WP_ForcePowerUsable( self, FP_TEAM_FORCE ) )
		return;

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] >= level.time)
		return;

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
		radius *= 1.5;
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
		radius *= 2;
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] >= FORCE_LEVEL_4)
		radius *= 3;

	while (i < MAX_CLIENTS){
		ent = &g_entities[i];

		if (ent && ent->client && self != ent && (OnSameTeam(self, ent) || (g_gametype.integer == GT_FFA && ent->client->sess.sessionTeam == TEAM_FREE &&
			//we dont have buddies or friends, or they are a buddy or friend
			((!(self->client->pers.Lmd.buddyindex[0] || self->client->pers.Lmd.buddyindex[1]) && !PlayerAcc_Friends_Count(self))
			|| isBuddy(self, ent)))) && ent->client->ps.fd.forcePower < ent->client->ps.fd.forcePowerMax && ForcePowerUsableOn(self, ent, FP_TEAM_FORCE) &&
			trap_InPVS(self->client->ps.origin, ent->client->ps.origin)){
				VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

				if (VectorLength(a) <= radius){
					pl[numpl] = i;
					numpl++;
				}
		}

		i++;
	}

	if (numpl < 1)
		return;

	if (numpl == 1)
		poweradd = 50;
	else if (numpl == 2)
		poweradd = 33;
	else
		poweradd = 25;
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_5)
		poweradd *= 2;

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 2000;

	BG_ForcePowerDrain( &self->client->ps, FP_TEAM_FORCE, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE] );

	i = 0;

	while (i < numpl)
	{
		g_entities[pl[i]].client->ps.fd.forcePower += poweradd;
		if (g_entities[pl[i]].client->ps.fd.forcePower > 100)
		{
			g_entities[pl[i]].client->ps.fd.forcePower = 100;
		}

		//At this point we know we got one, so add him into the collective event client bitflag
		if (!te)
		{
			te = G_TempEntity( self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 2; //eventParm 1 is heal, eventParm 2 is force regen
		}

		WP_AddToClientBitflags(te, pl[i]);
		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

		i++;
	}
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceGrip( gentity_t *self )
{
	trace_t tr;
	vec3_t tfrom, tto, fwd;

	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.fd.forceGripUseTime > level.time)
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_GRIP ) )
	{
		return;
	}
	int gripdist = MAX_GRIP_DISTANCE;
	if (PlayerAcc_Prof_GetProfession(self) == PROF_ADMIN) {
		gripdist = 4096;
	} else if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_4)
	{
		gripdist *= 2;
	} 
	else if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_5)
	{
		gripdist *= 4;
	}

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*gripdist;
	tto[1] = tfrom[1] + fwd[1]*gripdist;
	tto[2] = tfrom[2] + fwd[2]*gripdist;

	trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 &&
		tr.entityNum != ENTITYNUM_NONE &&
		g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time &&
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum])) ) //don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS && g_entities[tr.entityNum].client->ps.m_iVehicleNum)
		{ //a player on a vehicle
			gentity_t *vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
			{
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{ //push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)&g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;
		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;
	}
	else
	{
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
		return;
	}
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceSpeed( gentity_t *self, int forceDuration )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)) )
	{
		WP_ForcePowerStop( self, FP_SPEED );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_SPEED ) )
	{
		return;
	}

	if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
		&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD )
	{//holding Siege item
		if ( g_entities[self->client->holdingObjectiveItem].genericValue15 )
		{//disables force powers
			return;
		}
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_SPEED, forceDuration );
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
	G_Sound( self, TRACK_CHANNEL_2, speedLoopSound );
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceSeeing( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)) )
	{
		WP_ForcePowerStop( self, FP_SEE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_SEE ) )
	{
		return;
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_SEE, 0 );

	G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/see.wav") );
	G_Sound( self, TRACK_CHANNEL_5, seeLoopSound );
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceProtect( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_PROTECT ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_PROTECT, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_PROTECT);
	G_Sound( self, TRACK_CHANNEL_3, protectLoopSound );
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceAbsorb( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_ABSORB ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
	}
	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)
		//Lugormod absorb + protect
		&& !(self->client->ps.fd.forcePowerLevel[FP_ABSORB] >= FORCE_LEVEL_5)
		&& !(g_fixForce.integer & (1 << FP_ABSORB)))
	{
		WP_ForcePowerStop( self, FP_PROTECT );
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_ABSORB, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_ABSORB);
	G_Sound( self, TRACK_CHANNEL_3, absorbLoopSound );
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceRage( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_RAGE ) )
	{
		return;
	}

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	if (self->health < 20 
		&& g_fixForce.integer & (1 << FP_RAGE) 
		&& g_gametype.integer != GT_SIEGE 
		&& g_gametype.integer != GT_BATTLE_GROUND
		&& g_gametype.integer != GT_JEDIMASTER) {
			//Lugormod
			return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
	}
	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_RAGE, 0 );

	G_Sound( self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/rage.wav") );
	G_Sound( self, TRACK_CHANNEL_3, rageLoopSound );
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void ForceLightning( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	{
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/lightning") );

	WP_ForcePowerStart( self, FP_LIGHTNING, 500 );
}

void ForceLightningDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		//if (!traceEnt->client &&
		//    g_gametype.integer == GT_REBORN) {
		//}

		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if ( traceEnt->client )
		{//an enemy or object
			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > 100)
				{
					traceEnt->client->ps.fd.forcePower = 100;
				}
				return;
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_LIGHTNING))
			{
				int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );

				int modPowerLevel = -1;

				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_LIGHTNING, self->client->ps.fd.forcePowerLevel[FP_LIGHTNING], 1);
				}

				if (modPowerLevel != -1)
				{
					if (!modPowerLevel)
					{
						dmg = 0;
						traceEnt->client->noLightningTime = level.time + 400;
					}
					else if (modPowerLevel == 1)
					{
						dmg = 1;
						traceEnt->client->noLightningTime = level.time + 300;
					}
					else if (modPowerLevel == 2)
					{
						dmg = 1;
						traceEnt->client->noLightningTime = level.time + 100;
					}
				}

				//RoboPhred: no.
				/*
				if ( self->client->ps.weapon == WP_MELEE
				&& self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
				{//2-handed lightning
				//jackin' 'em up, Palpatine-style
				dmg *= 2;
				}
				*/
				if (g_gametype.integer == GT_REBORN ) {
					dmg *=3;
				}


				if (dmg)
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
						G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

					if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						traceEnt->client->ps.electrifyTime = level.time + 800;
					}
					if ( traceEnt->client->ps.powerups[PW_CLOAKED] )
					{//disable cloak temporarily
						Jedi_Decloak( traceEnt );
						traceEnt->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
					}
				}
			}
		}

	}
}

void ForceShootLightning( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	if ( self->health <= 0 )
	{
		return;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = FORCE_LIGHTNING_RADIUS, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		//Lugormod
		if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_4) {
			radius *= 1.5;
		}else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_5) {
			radius *= 2;
		}
		//end Lugormod
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}

			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceLightningDamage( self, traceEnt, dir, ent_org );
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );

		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceLightningDamage( self, traceEnt, forward, tr.endpos );
	}
}
#endif

#ifndef LMD_NEW_FORCEPOWERS
//Lugormod
void BotForceNotification (gclient_t *bot, gentity_t *attacker);

void ForceDrain( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_DRAIN ) )
	{
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_DRAIN] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	//	self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
	//	self->client->ps.forceHandExtendTime = level.time + 1000;
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/drain.wav") );

	WP_ForcePowerStart( self, FP_DRAIN, 500 );
}

void ForceDrainDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	gentity_t *tent;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		if ( traceEnt->client && (!OnSameTeam(self, traceEnt) || g_friendlyFire.integer) && self->client->ps.fd.forceDrainTime < level.time && traceEnt->client->ps.fd.forcePower )
		{//an enemy or object
			if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
			{ //g2animent
				if (traceEnt->s.genericenemyindex < level.time)
				{
					traceEnt->s.genericenemyindex = level.time + 2000;
				}
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_DRAIN))
			{
				int modPowerLevel = -1;
				int	dmg = 0; //Q_irand( 1, 3 );
				int powerLevel = self->client->ps.fd.forcePowerLevel[FP_DRAIN];

				// RoboPhred: Changing logic to handle higher levels.
				if (traceEnt->client) {
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.fd.forcePowerLevel[FP_DRAIN], 1);
					BotForceNotification(traceEnt->client, self);
				}

				if (modPowerLevel != -1) {
					powerLevel = modPowerLevel;
				}

				if (powerLevel == FORCE_LEVEL_1)
				{
					dmg = 2; //because it's one-shot
				}
				else if (powerLevel == FORCE_LEVEL_2)
				{
					dmg = 3;
				}
				else if (powerLevel == FORCE_LEVEL_3)
				{
					dmg = 4;
				}
				else if (powerLevel == FORCE_LEVEL_4)
				{
					dmg = 5;
				}
				else if (powerLevel == FORCE_LEVEL_5)
				{
					dmg = 6;
				}

				if (modPowerLevel != -1)
				{
					// RoboPhred: Follow the old logic and use one less than normal dmg if we got powerlevel'd.
					if (dmg > 0) {
						dmg -= 1;
					}
				}

				traceEnt->client->ps.fd.forcePower -= dmg;

				if (traceEnt->client->ps.fd.forcePower < 0 && dmg > 0)
				{
					// Not right now
					//// RoboPhred: Move into health drain.
					//G_Damage(traceEnt, traceEnt, traceEnt,
					//	vec3_origin, traceEnt->s.origin,
					//	1, // Only do 1 damage.
					//	DAMAGE_NO_ARMOR | DAMAGE_NO_DISMEMBER | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC, MOD_FORCE_DARK);

					traceEnt->client->ps.fd.forcePower = 0;
				}

				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += dmg;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}

					self->client->ps.stats[STAT_HEALTH] = self->health;
				}

				traceEnt->client->ps.fd.forcePowerRegenDebounceTime = level.time + 800; //don't let the client being drained get force power back right away
				//Drain really sucks big time!!!
				//Drain the standard amount since we just drained someone else

				/*
				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1)
				{
				BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, 0 );
				}
				else
				{
				BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN]/5 );
				}

				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1)
				{
				self->client->ps.fd.forceDrainTime = level.time + 100;
				}
				else
				{
				self->client->ps.fd.forceDrainTime = level.time + 20;
				}

				if ( traceEnt->client )
				{
				if ( !Q_irand( 0, 2 ) )
				{
				//G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( "sound/weapons/force/lightninghit.wav" ) );
				}
				//	traceEnt->s.powerups |= ( 1 << PW_DISINT_1 );

				//	traceEnt->client->ps.powerups[PW_DISINT_1] = level.time + 500;
				}
				*/

				if (traceEnt->client->forcePowerSoundDebounce < level.time)
				{
					tent = G_TempEntity( impactPoint, EV_FORCE_DRAINED);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;

					traceEnt->client->forcePowerSoundDebounce = level.time + 400;
				}
			}
		}
	}
}

int ForceShootDrain( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	int			gotOneOrMore = 0;

	if ( self->health <= 0 )
	{
		return 0;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = MAX_DRAIN_DISTANCE, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !traceEnt->client )
				continue;
			if ( !traceEnt->client->ps.fd.forcePower )
				continue;
			if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}

			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceDrainDamage( self, traceEnt, dir, ent_org );
			gotOneOrMore = 1;
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );

		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid || !g_entities[tr.entityNum].client || !g_entities[tr.entityNum].inuse )
		{
			//Lugormod Shouldn't be able to just go on and on ..
			BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, 5 ); //used to be 1, but this did, too, anger the God of Balance.
			return 0;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceDrainDamage( self, traceEnt, forward, tr.endpos );
		gotOneOrMore = 1;
	}

	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN] + FORCE_LEVEL_3;

	BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, 5 ); //used to be 1, but this did, too, anger the God of Balance.

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return gotOneOrMore;
}
#endif

void ForceJumpCharge( gentity_t *self, usercmd_t *ucmd )
{ //I guess this is unused now. Was used for the "charge" jump type.
	float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	if ( self->health <= 0 )
	{
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION])
	{
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.fd.forceJumpAddTime = 0;
	}

	if (self->client->ps.fd.forceJumpAddTime >= level.time)
	{
		return;
	}

	//need to play sound
	if ( !self->client->ps.fd.forceJumpCharge )
	{
		G_Sound( self, TRACK_CHANNEL_1, G_SoundIndex("sound/weapons/force/jumpbuild.wav") );
	}

	//Increment
	if (self->client->ps.fd.forceJumpAddTime < level.time)
	{
		self->client->ps.fd.forceJumpCharge += forceJumpChargeInterval*50;
		self->client->ps.fd.forceJumpAddTime = level.time + 500;
	}

	//clamp to max strength for current level
	if ( self->client->ps.fd.forceJumpCharge > forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]] )
	{
		self->client->ps.fd.forceJumpCharge = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]];
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
	}

	//clamp to max available force power
	if ( self->client->ps.fd.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION] > self->client->ps.fd.forcePower )
	{//can't use more than you have
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
		self->client->ps.fd.forceJumpCharge = self->client->ps.fd.forcePower*forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);
	}

	//G_Printf("%f\n", self->client->ps.fd.forceJumpCharge);
}

#ifndef LMD_NEW_FORCEPOWERS

int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd )
{
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy( self->client->ps.viewangles, view );
	view[0] = 0;
	AngleVectors( view, forward, right, NULL );
	if ( ucmd->forwardmove && ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if ( ucmd->rightmove > 0 )
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if ( ucmd->forwardmove || ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 100;
		}
		else if ( ucmd->forwardmove < 0 )
		{
			pushFwd = -100;
		}
		else if ( ucmd->rightmove > 0 )
		{
			pushRt = 100;
		}
		else if ( ucmd->rightmove < 0 )
		{
			pushRt = -100;
		}
	}

	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);

	G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);

	if (self->client->ps.fd.forceJumpCharge < JUMP_VELOCITY+40)
	{ //give him at least a tiny boost from just a tap
		self->client->ps.fd.forceJumpCharge = JUMP_VELOCITY+400;
	}

	if (self->client->ps.velocity[2] < -30)
	{ //so that we can get a good boost when force jumping in a fall
		self->client->ps.velocity[2] = -30;
	}

	VectorMA( self->client->ps.velocity, pushFwd, forward, jumpVel );
	VectorMA( self->client->ps.velocity, pushRt, right, jumpVel );
	jumpVel[2] += self->client->ps.fd.forceJumpCharge;

	if ( pushFwd > 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_FORWARD;
	}
	else if ( pushFwd < 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_BACKWARD;
	}
	else if ( pushRt > 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_RIGHT;
	}
	else if ( pushRt < 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_LEFT;
	}
	else
	{
		return FJ_UP;
	}
}

void ForceJump( gentity_t *self, usercmd_t *ucmd )
{
	float forceJumpChargeInterval;
	vec3_t	jumpVel;

	if ( self->client->ps.fd.forcePowerDuration[FP_LEVITATION] > level.time )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_LEVITATION ) )
	{
		return;
	}
	if ( self->s.groundEntityNum == ENTITYNUM_NONE 
		&& (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] //Lugormod
	< FORCE_LEVEL_5
		|| self->client->ps.fd.forcePower < 1))
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}

	self->client->fjDidJump = qtrue;

	forceJumpChargeInterval = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]]/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	WP_GetVelocityForForceJump( self, jumpVel, ucmd );

	//FIXME: sound effect
	self->client->ps.fd.forceJumpZStart = self->client->ps.origin[2];//remember this for when we land
	VectorCopy( jumpVel, self->client->ps.velocity );
	//wasn't allowing them to attack when jumping, but that was annoying
	//self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

	WP_ForcePowerStart( self, FP_LEVITATION, (int)(self->client->ps.fd.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION]));
	//self->client->ps.fd.forcePowerDuration[FP_LEVITATION] = level.time + self->client->ps.weaponTime;
	self->client->ps.fd.forceJumpCharge = 0;
	self->client->ps.forceJumpFlip = qtrue;
}
#endif

#ifndef LMD_NEW_FORCEPOWERS

void WP_AddAsMindtricked(forcedata_t *fd, int entNum)
{
	if (!fd)
	{
		return;
	}

	if (entNum > 47)
	{
		fd->forceMindtrickTargetIndex4 |= (1 << (entNum-48));
	}
	else if (entNum > 31)
	{
		fd->forceMindtrickTargetIndex3 |= (1 << (entNum-32));
	}
	else if (entNum > 15)
	{
		fd->forceMindtrickTargetIndex2 |= (1 << (entNum-16));
	}
	else
	{
		fd->forceMindtrickTargetIndex |= (1 << entNum);
	}
}

qboolean ForceTelepathyCheckDirectNPCTarget( gentity_t *self, trace_t *tr, qboolean *tookPower )
{
	gentity_t	*traceEnt;
	qboolean	targetLive = qfalse, mindTrickDone = qfalse;
	vec3_t		tfrom, tto, fwd;
	float		radius = MAX_TRICK_DISTANCE;

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*radius/2;
	tto[1] = tfrom[1] + fwd[1]*radius/2;
	tto[2] = tfrom[2] + fwd[2]*radius/2;

	trap_Trace( tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID );

	if ( tr->entityNum == ENTITYNUM_NONE 
		|| tr->fraction == 1.0f
		|| tr->allsolid 
		|| tr->startsolid )
	{
		return qfalse;
	}

	traceEnt = &g_entities[tr->entityNum];

	if( traceEnt->NPC 
		&& traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
	{
		return qfalse;
	}

	if ( traceEnt && traceEnt->client  )
	{
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
		case CLASS_ATST://much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_BOBAFETT:
		case CLASS_RANCOR:
		case CLASS_SAND_CREATURE:
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}

	if ( traceEnt->s.number < MAX_CLIENTS )
	{//a regular client
		return qfalse;
	}

	if ( targetLive && traceEnt->NPC )
	{//hit an organic non-player
		vec3_t	eyeDir;
		if ( G_ActivateBehavior( traceEnt, BSET_MINDTRICK ) )
		{//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		else if ( (self->NPC && traceEnt->client->playerTeam != self->client->playerTeam)
			|| (!self->NPC && traceEnt->client->playerTeam != self->client->sess.sessionTeam) )
		{//an enemy
			int override = 0;
			if ( (traceEnt->NPC->scriptFlags&SCF_NO_MIND_TRICK) )
			{
			}
			else if ( traceEnt->s.weapon != WP_SABER 
				&& traceEnt->client->NPC_class != CLASS_REBORN )
			{//haha!  Jedi aren't easily confused!
				if ( self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 )
				{//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					if ( traceEnt->s.weapon != WP_NONE )
					{//don't charm people who aren't capable of fighting... like ugnaughts and droids
						int newPlayerTeam, newEnemyTeam;

						if ( traceEnt->enemy )
						{
							G_ClearEnemy( traceEnt );
						}
						if ( traceEnt->NPC )
						{
							//traceEnt->NPC->tempBehavior = BS_FOLLOW_LEADER;
							traceEnt->client->leader = self;
						}
						//FIXME: maybe pick an enemy right here?
						if ( self->NPC )
						{//NPC
							newPlayerTeam = self->client->playerTeam;
							newEnemyTeam = self->client->enemyTeam;
						}
						else
						{//client/bot
							if ( self->client->sess.sessionTeam == TEAM_BLUE )
							{//rebel
								newPlayerTeam = NPCTEAM_PLAYER;
								newEnemyTeam = NPCTEAM_ENEMY;
							}
							else if ( self->client->sess.sessionTeam == TEAM_RED )
							{//imperial
								newPlayerTeam = NPCTEAM_ENEMY;
								newEnemyTeam = NPCTEAM_PLAYER;
							}
							else
							{//neutral - wan't attack anyone
								newPlayerTeam = NPCTEAM_NEUTRAL;
								newEnemyTeam = NPCTEAM_NEUTRAL;
							}
						}
						//store these for retrieval later
						traceEnt->genericValue1 = traceEnt->client->playerTeam;
						traceEnt->genericValue2 = traceEnt->client->enemyTeam;
						traceEnt->genericValue3 = traceEnt->s.teamowner;
						//set the new values
						traceEnt->client->playerTeam = newPlayerTeam;
						traceEnt->client->enemyTeam = newEnemyTeam;
						traceEnt->s.teamowner = newPlayerTeam;
						//FIXME: need a *charmed* timer on this...?  Or do TEAM_PLAYERS assume that "confusion" means they should switch to team_enemy when done?
						traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];
					}
				}
				else
				{//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds
					NPC_PlayConfusionSound( traceEnt );
					if ( traceEnt->enemy )
					{
						G_ClearEnemy( traceEnt );
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound( traceEnt );
			}
			WP_ForcePowerStart( self, FP_TELEPATHY, override );
		}
		else if ( traceEnt->client->playerTeam == self->client->playerTeam )
		{//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if ( traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC!=NULL && !(traceEnt->NPC->scriptFlags&SCF_NO_RESPONSE) )
			{
				NPC_UseResponse( traceEnt, self, qfalse );
				WP_ForcePowerStart( self, FP_TELEPATHY, 1 );
			}
		}//NOTE: no effect on TEAM_NEUTRAL?
		AngleVectors( traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL );
		VectorNormalize( eyeDir );
		G_PlayEffectID( G_EffectIndex( "force/force_touch" ), traceEnt->client->renderInfo.eyePoint, eyeDir );

		//make sure this plays and that you cannot press fire for about 1 second after this
		//FIXME: BOTH_FORCEMINDTRICK or BOTH_FORCEDISTRACT
		//NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
		//FIXME: build-up or delay this until in proper part of anim
		mindTrickDone = qtrue;
	}
	else 
	{
		if ( self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && tr->fraction * 2048 > 64 )
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffectID( G_EffectIndex( "force/force_touch" ), tr->endpos, tr->plane.normal );
			//FIXME: these events don't seem to always be picked up...?
			AddSoundEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue );//, qtrue );
			AddSightEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, 50 );
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			*tookPower = qtrue;
		}
		//NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
	}
	//self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	/*
	if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
	{
	self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
	*/
	return qtrue;
}

void ForceTelepathy(gentity_t *self)
{
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_TRICK_DISTANCE;
	qboolean	tookPower = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG])
	{ //can't mindtrick while carrying the flag
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) )
	{
		WP_ForcePowerStop( self, FP_TELEPATHY );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TELEPATHY ) )
	{
		return;
	}

	if ( ForceTelepathyCheckDirectNPCTarget( self, &tr, &tookPower ) )
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		radius = MAX_TRICK_DISTANCE*2.0f;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_4)
	{
		visionArc = 360;
		radius = MAX_TRICK_DISTANCE*3.0f;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_5)
	{
		visionArc = 360;
		radius = MAX_TRICK_DISTANCE*4.0f;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
	{
		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE &&
			g_entities[tr.entityNum].inuse &&
			g_entities[tr.entityNum].client &&
			g_entities[tr.entityNum].client->pers.connected &&
			g_entities[tr.entityNum].client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			WP_AddAsMindtricked(&self->client->ps.fd, tr.entityNum);
			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;

			return;
		}
		else
		{
			return;
		}
	}
	else	//level 2 & 3
	{
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{ //not in the arc, don't consider it
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (!ent->client)
				{
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (!InFieldOfVision(self->client->ps.viewangles, visionArc, a))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
				{
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (OnSameTeam(self, ent))
				{
					entityList[e] = ENTITYNUM_NONE;
				}
			}
			ent = &g_entities[entityList[e]];
			if (ent && ent != self && ent->client)
			{
				gotatleastone = qtrue;
				WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);
			}
			e++;
		}

		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
	}

}
#endif

#ifndef LMD_NEW_FORCEPOWERS
void GEntity_UseFunc( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	GlobalUse(self, other, activator);
}
qboolean CanCounterThrow(gentity_t *self, gentity_t *thrower, qboolean pull)
{
	int powerUse = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return 0;
	}

	if ( self->health <= 0 )
	{
		return 0;
	}

	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
	{
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_CHARGING ||
		self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}

	if ((g_gametype.integer == GT_SIEGE ||
		g_gametype.integer == GT_BATTLE_GROUND) &&
		pull &&
		thrower && thrower->client)
	{ //in siege, pull will affect people if they are not facing you, so they can't run away so much
		vec3_t d;
		float a;

		VectorSubtract(thrower->client->ps.origin, self->client->ps.origin, d);
		vectoangles(d, d);

		a = AngleSubtract(d[YAW], self->client->ps.viewangles[YAW]);

		if (a > 60.0f || a < -60.0f)
		{ //if facing more than 60 degrees away they cannot defend
			return 0;
		}
	}

	if (pull)
	{
		powerUse = FP_PULL;
	}
	else
	{
		powerUse = FP_PUSH;
	}

	if ( !WP_ForcePowerUsable( self, powerUse ) )
	{
		return 0;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{ //you cannot counter a push/pull if you're in the air
		return 0;
	}

	return 1;
}
#endif

qboolean G_InGetUpAnim(playerState_t *ps)
{
	switch( (ps->legsAnim) )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	switch( (ps->torsoAnim) )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	return qfalse;
}

void G_LetGoOfWall( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	if ( BG_InReboundJump( ent->client->ps.legsAnim ) 
		|| BG_InReboundHold( ent->client->ps.legsAnim ) )
	{
		ent->client->ps.legsTimer = 0;
	}
	if ( BG_InReboundJump( ent->client->ps.torsoAnim ) 
		|| BG_InReboundHold( ent->client->ps.torsoAnim ) )
	{
		ent->client->ps.torsoTimer = 0;
	}
}

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS + 2] =
{
	0,//none
	384,//256,
	448,//384,
	512,
	768,
	1024
};
//rwwFIXMEFIXME: incorporate this into the below function? Currently it's only being used by jedi AI

#ifndef LMD_NEW_FORCEPOWERS

//Lugormod
//RoboPhred
extern vmCvar_t lmd_stashrate;
qboolean IsThrowable_Old (gentity_t *ent){
	if ( Q_stricmp( "lightsaber", ent->classname ) == 0 )
	{//a lightsaber 
		return qtrue;
	}

	//RoboPhred

	if ((Q_stricmp("func_door", ent->classname) == 0 || Q_stricmp("lmd_door", ent->classname) == 0) && (ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
		//if ( Q_stricmp( "func_door", ent->classname ) == 0 && (ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
	{//a force-usable door
		if ( ent->moverState != MOVER_POS1 
			&& ent->moverState != MOVER_POS2 )
		{//not at rest
			return qfalse;
		}
		return qtrue;
	}

	if ( Q_stricmp( "func_static", ent->classname ) == 0 
		&& ((ent->spawnflags&1/*F_PUSH*/) 
		|| (ent->spawnflags&2/*F_PULL*/)) )
	{//a force-usable func_static

		return qtrue;
	}

	if ( Q_stricmp( "limb", ent->classname ) == 0 )
	{//a limb
		return qtrue;
	}
	if ( Q_stricmp( "credits", ent->classname) == 0) {
		return qtrue;
	}
	if ( Q_stricmp( "teleporter", ent->classname) == 0) {
		return qtrue;
	}
	if ( Q_stricmp( "misc_model_breakable", ent->classname) == 0 && 
		ent->flags&FL_DROPPED_ITEM) {
			return qtrue;
	}
	if ( Q_stricmp( "money_stash", ent->classname) == 0 
		//&& ent->flags&FL_DROPPED_ITEM
		) {
			//G_SetOrigin(ent, ent->s.origin); 
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
			ent->physicsObject = qtrue;
			ent->bounceCount = 8;
			ent->flags = FL_DROPPED_ITEM|FL_BOUNCE;
			//RoboPhred
			//ent->think = stash_remove_and_spawn;
			ent->timestamp = level.time + 300000; //five minutes
			trap_LinkEntity(ent);
			return qtrue;
	}

	return qfalse;
}

qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );//Lugormod

qboolean CanCounterThrow_Old(gentity_t *self, gentity_t *thrower, qboolean pull) {
	int powerUse = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;

	if (self->client->ps.weaponTime > 0)
		return qfalse;

	if ( self->health <= 0 )
		return qfalse;

	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
		return qfalse;

	if (self->client->ps.weaponstate == WEAPON_CHARGING || self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return qfalse;
	}

	if ((g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) && pull && thrower && thrower->client)
	{ //in siege, pull will affect people if they are not facing you, so they can't run away so much
		vec3_t d;
		float a;

		VectorSubtract(thrower->client->ps.origin, self->client->ps.origin, d);
		vectoangles(d, d);

		a = AngleSubtract(d[YAW], self->client->ps.viewangles[YAW]);

		if (a > 60.0f || a < -60.0f)
		{ //if facing more than 60 degrees away they cannot defend
			return qfalse;
		}
	}

	if (pull)
		powerUse = FP_PULL;
	else
		powerUse = FP_PUSH;

	int overrideAmt = 0;
	int resistSkill = PlayerProf_Merc_GetForceResistanceSkill(self);
	if (resistSkill > 0) {
		overrideAmt = 25;
	}

	if ( !WP_ForcePowerAvailable( self, powerUse, overrideAmt) )
		return qfalse;

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		return qfalse;

	return qtrue;
}
extern void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace );
void ForceThrow_Old( gentity_t *self, qboolean pull )
{
	//shove things in front of you away
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius = 1024; //since it's view-based now. //350;
	int			powerLevel;
	int			visionArc;
	int			pushPower;
	int			pushPowerMod;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	float		dot1;
	trace_t		tr;
	int			x;
	vec3_t		pushDir;
	vec3_t		thispush_org;
	vec3_t		tfrom, tto, fwd, a;
	int			powerUse = 0;
	//RoboPhred
	int debounceTime = 3000;

	visionArc = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE &&
		self->client->ps.forceHandExtend != HANDEXTEND_FORCEPULL &&
		self->client->ps.forceHandExtend != HANDEXTEND_FORCEPUSH &&
		(self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN || !G_InGetUpAnim(&self->client->ps)))
	{
		return;
	}


	if (!g_useWhileThrowing.integer && self->client->ps.saberInFlight)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if ( self->health <= 0 )
	{
		return;
	}

	if (pull)
	{
		powerUse = FP_PULL;
	}
	else
	{
		powerUse = FP_PUSH;
	}

	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
	{
		return;
	}

	//RoboPhred: using a debounce now
	if(self->client->ps.fd.forcePowerDebounce[powerUse] > level.time) {
		return;
	}


	if ( !WP_ForcePowerUsable( self, powerUse ) )
	{
		return;
	}



	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame)
	{
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;

		self->client->ps.saberLockHits += self->client->ps.fd.forcePowerLevel[FP_PUSH]*2;

		WP_ForcePowerStart( self, FP_PUSH, 0 );
		return;
	}

	WP_ForcePowerStart( self, powerUse, 0 );

	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( pull )
	{
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			if ( (g_gametype.integer == GT_SIEGE 
				|| g_gametype.integer == GT_BATTLE_GROUND) 
				&& self->client->ps.weapon == WP_SABER )
			{//hold less so can attack right after a pull
				self->client->ps.forceHandExtendTime = level.time + 200;
			}
			else
			{
				self->client->ps.forceHandExtendTime = level.time + 400;
			}
		}
		self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else
	{
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps))
		{
			if (self->client->ps.forceDodgeAnim > 4)
			{
				self->client->ps.forceDodgeAnim -= 8;
			}
			self->client->ps.forceDodgeAnim += 8; //special case, play push on upper torso, but keep playing current knockdown anim on legs
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}


	powerLevel = self->client->ps.fd.forcePowerLevel[powerUse];
	pushPower = 256 * powerLevel;

	if (!powerLevel)
	{ //Shouldn't have made it here..
		return;
	}

	if (powerLevel == FORCE_LEVEL_2)
	{
		visionArc = 60;
		debounceTime = 2500;
	}
	else if (powerLevel == FORCE_LEVEL_3)
	{
		visionArc = 180;
		//RoboPhred
		debounceTime = 2000;
	}
	else if (powerLevel == FORCE_LEVEL_4)
	{
		visionArc = 180;
		//RoboPhred
		//pushPower *= 1.5;
		debounceTime = 1700;
	}
	else if (powerLevel == FORCE_LEVEL_5)
	{
		visionArc = 180;
		//RoboPhred
		//pushPower *= 2;
		debounceTime = 1400;
	}

	//RoboPhred
	self->client->ps.fd.forcePowerDebounce[powerUse] = level.time + debounceTime;


	if (powerLevel == FORCE_LEVEL_1)
	{ //can only push/pull targeted things at level 1
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0]*radius/2;
		tto[1] = tfrom[1] + fwd[1]*radius/2;
		tto[2] = tfrom[2] + fwd[2]*radius/2;

		trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
			{ //g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}

			numListedEntities = 0;
			entityList[numListedEntities] = tr.entityNum;

			if (pull)
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PULL))
				{
					return;
				}
			}
			else
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PUSH))
				{
					return;
				}
			}
			numListedEntities++;
		}
		else
		{
			//didn't get anything, so just
			return;
		}
	}
	else
	{
		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		e = 0;

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (!ent->client && ent->s.eType == ET_NPC)
			{ //g2animent
				if (ent->s.genericenemyindex < level.time)
				{
					ent->s.genericenemyindex = level.time + 2000;
				}
			}

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) &&
					ForcePowerUsableOn(self, ent, powerUse))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (pull)
					{
						if (!ForcePowerUsableOn(self, ent, FP_PULL))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
					else
					{
						if (!ForcePowerUsableOn(self, ent, FP_PUSH))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
				}

			}
			e++;
		}
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		if (entityList[e] != ENTITYNUM_NONE &&
			entityList[e] >= 0 &&
			entityList[e] < MAX_GENTITIES)
		{
			ent = &g_entities[entityList[e]];
		}
		else
		{
			ent = NULL;
		}

		if (!ent)
			continue;
		if (ent == self)
			continue;
		if (ent->client && OnSameTeam(ent, self))
		{
			continue;
		}
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( ent->s.eType != ET_ITEM )
			{
				//FIXME: need pushable objects
				if ( Q_stricmp( "func_button", ent->classname ) == 0 )
				{//we might push it
					if ( pull || !(ent->spawnflags&SPF_BUTTON_FPUSHABLE) )
					{//not force-pushable, never pullable
						continue;
					}
				}
				else
				{
					if ( ent->s.eFlags & EF_NODRAW )
					{
						continue;
					}
					if ( !ent->client )
						//Lugormod this was stupid and ugly
					{
						if (!IsThrowable_Old(ent)) {
							continue;
						}

					}
					else if ( ent->client->NPC_class == CLASS_GALAKMECH 
						|| ent->client->NPC_class == CLASS_ATST
						//RoboPhred: at push/pull lvl 4, let it affect rancors
						|| (ent->client->NPC_class == CLASS_RANCOR && powerLevel < FORCE_LEVEL_4)
						//|| ent->client->NPC_class == CLASS_RANCOR 
						|| ent->client->NPC_class == CLASS_SAND_CREATURE)
					{//can't push ATST or Galak or Rancor or Sand creature
						continue;
					}
				}
			}
		}
		else
		{
			if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
			{//can't force-push/pull stuck missiles (detpacks, tripmines)
				continue;
			}
			if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
			{//only thermal detonators can be pushed once stopped
				continue;
			}
		}

		//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( center[i] < ent->r.absmin[i] ) 
			{
				v[i] = ent->r.absmin[i] - center[i];
			} else if ( center[i] > ent->r.absmax[i] ) 
			{
				v[i] = center[i] - ent->r.absmax[i];
			} else 
			{
				v[i] = 0;
			}
		}

		VectorSubtract( ent->r.absmax, ent->r.absmin, size );
		VectorMA( ent->r.absmin, 0.5, size, ent_org );

		VectorSubtract( ent_org, center, dir );
		VectorNormalize( dir );
		if ( (dot1 = DotProduct( dir, forward )) < 0.6 )
			continue;

		dist = VectorLength( v );

		//Now check and see if we can actually deflect it
		//method1
		//if within a certain range, deflect it
		if ( dist >= radius ) 
		{
			continue;
		}

		//in PVS?
		if ( !ent->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
		{//must be in PVS
			continue;
		}

		//really should have a clear LOS to this thing...
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
		if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
		{//must have clear LOS
			//try from eyes too before you give up
			vec3_t eyePoint;
			VectorCopy(self->client->ps.origin, eyePoint);
			eyePoint[2] += self->client->ps.viewheight;
			trap_Trace( &tr, eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );

			if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
			{
				//Lugormod these things are placed
				//inside other func_static
				//sometimes
				gentity_t *hitEnt = &g_entities[tr.entityNum];

				if (!hitEnt || !hitEnt->inuse || Q_stricmp("func_static", ent->classname) || Q_stricmp("func_static", hitEnt->classname) || 
					ent->spawnflags & 3 || !G_PointInBounds(ent->s.origin, hitEnt->r.absmin, 	hitEnt->r.absmax)) {
						continue;
				}
			}
		}

		// ok, we are within the radius, add us to the incoming list
		push_list[ent_count] = ent;
		ent_count++;
	}

	if ( ent_count )
	{
		//method1:
		for ( x = 0; x < ent_count; x++ )
		{
			int modPowerLevel = powerLevel;


			if (push_list[x]->client)
			{
				modPowerLevel = WP_AbsorbConversion(
					push_list[x],
					push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB],
					self,
					powerUse,
					powerLevel,
					forcePowerNeeded[self->client->ps.fd.forcePowerLevel[powerUse]][powerUse]);
				if (modPowerLevel == -1)
				{
					modPowerLevel = powerLevel;
				}
			}

			pushPower = 256*modPowerLevel;

			if (push_list[x]->client)
			{
				VectorCopy(push_list[x]->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(push_list[x]->s.origin, thispush_org);
			}

			if ( push_list[x]->client )
			{//FIXME: make enemy jedi able to hunker down and resist this?
				int otherPushForcePower = push_list[x]->client->ps.fd.forcePowerLevel[powerUse];
				int otherResistanceSkill = PlayerProf_Merc_GetForceResistanceSkill(push_list[x]);

				int otherPushResist = otherPushForcePower;
				if (otherResistanceSkill > otherPushResist)
				{
					otherPushResist = otherResistanceSkill;
					otherPushForcePower = 0;
				}
				else
				{
					otherResistanceSkill = 0;
				}

				qboolean canPullWeapon = qtrue;
				float dirLen = 0;

				if ( g_debugMelee.integer )
				{
					if ( (push_list[x]->client->ps.pm_flags & PMF_STUCK_TO_WALL) )
					{//no resistance if stuck to wall
						//push/pull them off the wall
						otherPushResist = 0;
						G_LetGoOfWall( push_list[x] );
					}
				}

				pushPowerMod = pushPower;

				//RoboPhred: if rancor, nerf the power
				if(push_list[x]->client->NPC_class == CLASS_RANCOR)
					pushPowerMod /= 2;


				if (push_list[x]->client->pers.cmd.forwardmove ||
					push_list[x]->client->pers.cmd.rightmove)
				{ //if you are moving, you get one less level of defense
					otherPushResist--;

					if (otherPushResist < 0)
					{
						otherPushResist = 0;
					}
				}

				if (otherPushResist && CanCounterThrow_Old(push_list[x], self, pull))
				{
					// RoboPhred: only jedi have the effect.
					if (otherPushForcePower) {
						if ( pull )
						{
							G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
							push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
							push_list[x]->client->ps.forceHandExtendTime = level.time + 400;
						}
						else
						{
							G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
							push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
							push_list[x]->client->ps.forceHandExtendTime = level.time + 1000;
						}

						push_list[x]->client->ps.powerups[PW_DISINT_4] = push_list[x]->client->ps.forceHandExtendTime + 200;

						if (pull)
						{
							push_list[x]->client->ps.powerups[PW_PULL] = push_list[x]->client->ps.powerups[PW_DISINT_4];
						}
						else
						{
							push_list[x]->client->ps.powerups[PW_PULL] = 0;
						}
					}
					else if (otherResistanceSkill) {
						BG_ForcePowerDrain( &push_list[x]->client->ps, powerUse, 25 );
					}

					//Make a counter-throw effect

					if (otherPushResist >= modPowerLevel)
					{
						pushPowerMod = 0;
						canPullWeapon = qfalse;
					}
					else
					{
						int powerDif = (modPowerLevel - otherPushResist);

						if (powerDif >= 3)
						{
							pushPowerMod -= (int)(pushPowerMod*0.2);
						}
						else if (powerDif == 2)
						{
							pushPowerMod -= (int)(pushPowerMod*0.4);
						}
						else if (powerDif == 1)
						{
							pushPowerMod -= (int)(pushPowerMod*0.8);
						}

						if (pushPowerMod < 0)
						{
							pushPowerMod = 0;
						}
					}
				}

				dirLen = VectorLength(pushDir);

				//shove them
				if ( pull )
				{
					VectorSubtract( self->client->ps.origin, thispush_org, pushDir );

					if (canPullWeapon && push_list[x]->client && dirLen <= 256 && !push_list[x]->m_pVehicle && !OnSameTeam(self, push_list[x]))
					{
						int randfact = 0;

						// RoboPhred: Numbers from ufooooo
						// 10/20/30/40/50
						randfact = modPowerLevel * 10;

						if (Q_irand(0, 100) <= randfact)
						{
							vec3_t uorg, vecnorm;

							VectorCopy(self->client->ps.origin, uorg);
							uorg[2] += 64;

							VectorSubtract(uorg, thispush_org, vecnorm);
							VectorNormalize(vecnorm);

							TossClientWeapon(push_list[x], vecnorm, 500);
						}
					}
				}
				else
				{
					VectorSubtract( thispush_org, self->client->ps.origin, pushDir );
				}

				if ((modPowerLevel > otherPushResist || push_list[x]->client->ps.m_iVehicleNum) && push_list[x]->client)
				{
					//Lugormod
					BotForceNotification (push_list[x]->client, self);

					if (modPowerLevel >= FORCE_LEVEL_3 &&
						push_list[x]->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{

						//RoboPhred
						if (push_list[x]->s.number < MAX_CLIENTS)
						{
							if (push_list[x]->client->ps.m_iVehicleNum && dirLen <= 128.0f)
							{ //a player on a vehicle
								gentity_t *vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];
								if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
								{
									if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
										vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
									{ //push the guy off
										vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *) push_list[x], qfalse);
									}
								}
							}
							// RoboPhred
							// Removed for now: otherResistanceSkill < FORCE_LEVEL_4 && 
							else if (dirLen <= 64 * ((modPowerLevel - otherPushResist) - 1))
							{
								G_Knockdown(push_list[x], 600);
							}
						}
					}
				}


				VectorNormalize(pushDir);

				if (push_list[x]->client)
				{
					//escape a force grip if we're in one
					if (self->client->ps.fd.forceGripBeingGripped > level.time)
					{ //force the enemy to stop gripping me if I managed to push him
						if (push_list[x]->client->ps.fd.forceGripEntityNum == self->s.number)
						{
							if (modPowerLevel >= push_list[x]->client->ps.fd.forcePowerLevel[FP_GRIP])
							{ //only break the grip if our push/pull level is >= their grip level
								WP_ForcePowerStop(push_list[x], FP_GRIP);
								self->client->ps.fd.forceGripBeingGripped = 0;
								push_list[x]->client->ps.fd.forceGripUseTime = level.time + 1000; //since we just broke out of it..
							}
						}
					}

					push_list[x]->client->ps.otherKiller = self->s.number;
					push_list[x]->client->ps.otherKillerTime = level.time + 5000;
					push_list[x]->client->ps.otherKillerDebounceTime = level.time + 100;

					pushPowerMod -= (int)(dirLen*0.7);
					if (pushPowerMod < 16)
					{
						pushPowerMod = 16;
					}

					//fullbody push effect
					push_list[x]->client->pushEffectTime = level.time + 600;

					push_list[x]->client->ps.velocity[0] = pushDir[0]*pushPowerMod;
					push_list[x]->client->ps.velocity[1] = pushDir[1]*pushPowerMod;

					if ((int)push_list[x]->client->ps.velocity[2] == 0)
					{ //if not going anywhere vertically, boost them up a bit
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod;

						if (push_list[x]->client->ps.velocity[2] < 128)
						{
							push_list[x]->client->ps.velocity[2] = 128;
						}
					}
					else
					{
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod;
					}
				}
			}
			else if ( push_list[x]->s.eType == ET_MISSILE && push_list[x]->s.pos.trType != TR_STATIONARY && (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				if ( pull )
				{//deflect rather than reflect?
				}
				else 
				{
					G_ReflectMissile( self, push_list[x], forward );
				}
			}
			else if ( !Q_stricmp( "func_static", push_list[x]->classname ) )
			{//force-usable func_static
				if ( !pull && (push_list[x]->spawnflags&1/*F_PUSH*/) )
				{
					GlobalUse( push_list[x], self, self );
				}
				else if ( pull && (push_list[x]->spawnflags&2/*F_PULL*/) )
				{
					GlobalUse( push_list[x], self, self );
				}
			}
			//RoboPhred
			else if((!Q_stricmp("func_door", push_list[x]->classname) || !Q_stricmp("lmd_door", push_list[x]->classname)) && (push_list[x]->spawnflags&2))
			{//push/pull the door
				vec3_t	pos1, pos2;
				vec3_t	trFrom;

				VectorCopy(self->client->ps.origin, trFrom);
				trFrom[2] += self->client->ps.viewheight;

				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( trFrom, radius, forward, end );
				trap_Trace( &tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}

				if ( VectorCompare( vec3_origin, push_list[x]->s.origin ) )
				{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract( push_list[x]->r.absmax, push_list[x]->r.absmin, size );
					VectorMA( push_list[x]->r.absmin, 0.5, size, center );
					if ( (push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS1 )
					{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos1, center );
					}
					else if ( !(push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS2 )
					{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos2, center );
					}
					VectorAdd( center, push_list[x]->pos1, pos1 );
					VectorAdd( center, push_list[x]->pos2, pos2 );
				}
				else
				{//actually has an origin, pos1 and pos2 are absolute
					VectorCopy( push_list[x]->r.currentOrigin, center );
					VectorCopy( push_list[x]->pos1, pos1 );
					VectorCopy( push_list[x]->pos2, pos2 );
				}

				if ( Distance( pos1, trFrom ) < Distance( pos2, trFrom ) )
				{//pos1 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{//pos2 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GlobalUse( push_list[x], self, self );
			}
			else if ( Q_stricmp( "func_button", push_list[x]->classname ) == 0 )
			{//pretend you pushed it
				Touch_Button( push_list[x], self, NULL );
				continue;
			} else if (Q_stricmp( "lightsaber", push_list[x]->classname ) == 0){

			} else if (push_list[x]->flags & FL_DROPPED_ITEM) { //Lugormod other stuff
				VectorCopy(push_list[x]->r.currentOrigin, thispush_org);
				//get size
				vec3_t size;
				vec_t weight;

				VectorSubtract (push_list[x]->r.maxs, push_list[x]->r.mins, size);
				//weight = size[0] * size[1] * size [2];
				weight = VectorLength(size);

				pushPower *= 64/weight;
				if (pushPower > 1536) {
					pushPower = 1536;
				}
				if (pushPower <= 1) {
					continue;
				}


				//shove it :)
				if ( pull )
				{
					VectorSubtract( self->client->ps.origin, thispush_org, pushDir );
				}
				else
				{
					VectorSubtract( thispush_org, self->client->ps.origin, pushDir );
				}
				VectorNormalize(pushDir);
				//VectorMA(push_list[x]->s.pos.trDelta,
				//         pushPower,
				//         pushDir,
				//         push_list[x]->s.pos.trDelta);
				VectorScale(pushDir,
					pushPower,
					push_list[x]->s.pos.trDelta);
				if (push_list[x]->s.pos.trDelta[2] < pushPower /10) {
					push_list[x]->s.pos.trDelta[2] = pushPower/10;
				}
				//push_list[x]->s.pos.trDelta[2] += pushPower / 10;

				//trap_SendServerCommand(self-g_entities,
				//                       va("print \"Throwing %s\n\"", vtos(thispush_org)));

				//push_list[x]->flags |= FL_BOUNCE_HALF;
				push_list[x]->s.pos.trType = TR_GRAVITY;
				push_list[x]->s.pos.trTime = level.time;

				//trap_LinkEntity(push_list[x]);
			} /*else if ((Q_stricmp("misc_model_breakable", 
			  push_list[x]->classname) == 0)
			  && push_list[x]->spawnflags&64
			  //&& push_list[x]->use
			  ) {
			  //useit
			  //Disp(self, "Push/pull switch");
			  GEntity_UseFunc( push_list[x], self, self );
			  }*/
		}
	}

	//attempt to break any leftover grips
	//if we're still in a current grip that wasn't broken by the push, it will still remain
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}
}
#endif

#ifdef LMD_NEW_FORCEPOWERS
void Force_StopPower(gentity_t *ent, int power);
#endif
void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower )
{

#ifdef LMD_NEW_FORCEPOWERS
	Force_StopPower(self, forcePower);
#endif

	int wasActive = self->client->ps.fd.forcePowersActive;

	self->client->ps.fd.forcePowersActive &= ~( 1 << forcePower );

	switch( (int)forcePower )
	{
	case FP_HEAL:
#ifndef LMD_NEW_FORCEPOWERS
		self->client->ps.fd.forceHealAmount = 0;
		self->client->ps.fd.forceHealTime = 0;
#endif
		break;
	case FP_SPEED:
#ifndef LMD_NEW_FORCEPOWERS
		if (wasActive & (1 << FP_SPEED))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_2-50], CHAN_VOICE);
		}
#endif
		break;
	case FP_TELEPATHY:
#ifndef LMD_NEW_FORCEPOWERS
		if (wasActive & (1 << FP_TELEPATHY))
		{
			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav") );
		}
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
#endif
		break;
	case FP_SEE:
#ifndef LMD_NEW_FORCEPOWERS
		if (wasActive & (1 << FP_SEE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_5-50], CHAN_VOICE);
		}
#endif
		break;
	case FP_GRIP:
#ifndef LMD_NEW_FORCEPOWERS
		self->client->ps.fd.forceGripUseTime = level.time + 3000;
		if (self->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].client &&
			g_entities[self->client->ps.fd.forceGripEntityNum].health > 0 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse &&
			(level.time - g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.fd.forceGripStarted) > 500)
		{ //if we had our throat crushed in for more than half a second, gasp for air when we're let go
			if (wasActive & (1 << FP_GRIP))
			{
				G_EntitySound( &g_entities[self->client->ps.fd.forceGripEntityNum], CHAN_VOICE, G_SoundIndex("*gasp.wav") );
			}
		}

		if (g_entities[self->client->ps.fd.forceGripEntityNum].client &&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse)
		{

			g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.forceGripChangeMovetype = PM_NORMAL;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0;
		}

		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		self->client->ps.powerups[PW_DISINT_4] = 0;
#endif
		break;
	case FP_LIGHTNING:
#ifndef LMD_NEW_FORCEPOWERS
		if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 1500;
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
#endif
		break;
	case FP_RAGE:
#ifndef LMD_NEW_FORCEPOWERS
		self->client->ps.fd.forceRageRecoveryTime = level.time + 10000;
		if (wasActive & (1 << FP_RAGE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
#endif
		break;
	case FP_ABSORB:
#ifndef LMD_NEW_FORCEPOWERS
		if (wasActive & (1 << FP_ABSORB))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
#endif
		break;
	case FP_PROTECT:
#ifndef LMD_NEW_FORCEPOWERS
		if (wasActive & (1 << FP_PROTECT))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
#endif
		break;
	case FP_DRAIN:
#ifndef LMD_NEW_FORCEPOWERS
		if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum...
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 1500;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
#endif
	default:
		break;
	}
}

#ifndef LMD_NEW_FORCEPOWERS
void DoGripAction(gentity_t *self, forcePowers_t forcePower)
{
	gentity_t *gripEnt;
	int gripLevel = 0;
	trace_t tr;
	vec3_t a;
	vec3_t fwd, fwd_o, start_o, nvel;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gripEnt = &g_entities[self->client->ps.fd.forceGripEntityNum];

	if (!gripEnt || !gripEnt->client || !gripEnt->inuse || gripEnt->health < 1 || !ForcePowerUsableOn(self, gripEnt, FP_GRIP))
	{
		WP_ForcePowerStop(self, forcePower);
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		if (gripEnt && gripEnt->client && gripEnt->inuse)
		{
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}
		return;
	}

	VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);

	trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID);

	gripLevel = WP_AbsorbConversion(gripEnt, gripEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.fd.forcePowerLevel[FP_GRIP], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP]);

	if (gripLevel == -1)
	{
		gripLevel = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	}

	if (!gripLevel)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}
	int gripdist = MAX_GRIP_DISTANCE;
	if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_4)
	{
		gripdist *= 2;
	} 
	else if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_5)
	{
		gripdist *= 4;
	}

	if (VectorLength(a) > gripdist)//MAX_GRIP_DISTANCE)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if ( !InFront( gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) &&
		gripLevel < FORCE_LEVEL_3)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (tr.fraction != 1.0f &&
		tr.entityNum != gripEnt->s.number /*&&
										  gripLevel < FORCE_LEVEL_3*/)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time)
	{ //2 damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		G_Damage(gripEnt, self, self, NULL, NULL, 2, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
	}

#ifndef LMD_NEW_JETPACK
	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.
#endif

	if (gripLevel == FORCE_LEVEL_1)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (gripLevel == FORCE_LEVEL_2)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			gripEnt->client->ps.velocity[2] = 30;

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 3000 && !self->client->ps.fd.forceGripDamageDebounceTime)
		{ //if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 20, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
			{ //choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (gripLevel >= FORCE_LEVEL_3) //Lugormod was ==
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			float nvLen = 0;

			VectorCopy(gripEnt->client->ps.origin, start_o);
			AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
			fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
			fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
			fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
			fwd_o[2] += 16;
			VectorSubtract(fwd_o, start_o, nvel);

			nvLen = VectorLength(nvel);

			if (nvLen < 16)
			{ //within x units of desired spot
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*8;
				gripEnt->client->ps.velocity[1] = nvel[1]*8;
				gripEnt->client->ps.velocity[2] = nvel[2]*8;
			}
			else if (nvLen < 64)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*128;
				gripEnt->client->ps.velocity[1] = nvel[1]*128;
				gripEnt->client->ps.velocity[2] = nvel[2]*128;
			}
			else if (nvLen < 128)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*256;
				gripEnt->client->ps.velocity[1] = nvel[1]*256;
				gripEnt->client->ps.velocity[2] = nvel[2]*256;
			}
			else if (nvLen < 200)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*512;
				gripEnt->client->ps.velocity[1] = nvel[1]*512;
				gripEnt->client->ps.velocity[2] = nvel[2]*512;
			}
			else
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*700;
				gripEnt->client->ps.velocity[1] = nvel[1]*700;
				gripEnt->client->ps.velocity[2] = nvel[2]*700;
			}

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 3000 && !self->client->ps.fd.forceGripDamageDebounceTime)
		{ //if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 40, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
			{ //choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}
}
#endif

qboolean G_IsMindTricked(forcedata_t *fd, int client)
{
	int checkIn;
	int trickIndex1, trickIndex2, trickIndex3, trickIndex4;
	int sub = 0;

	if (!fd)
	{
		return qfalse;
	}

	trickIndex1 = fd->forceMindtrickTargetIndex;
	trickIndex2 = fd->forceMindtrickTargetIndex2;
	trickIndex3 = fd->forceMindtrickTargetIndex3;
	trickIndex4 = fd->forceMindtrickTargetIndex4;

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn & (1 << (client-sub)))
	{
		return qtrue;
	}

	return qfalse;
}

static void RemoveTrickedEnt(forcedata_t *fd, int client)
{
	if (!fd)
	{
		return;
	}

	if (client > 47)
	{
		fd->forceMindtrickTargetIndex4 &= ~(1 << (client-48));
	}
	else if (client > 31)
	{
		fd->forceMindtrickTargetIndex3 &= ~(1 << (client-32));
	}
	else if (client > 15)
	{
		fd->forceMindtrickTargetIndex2 &= ~(1 << (client-16));
	}
	else
	{
		fd->forceMindtrickTargetIndex &= ~(1 << client);
	}
}

extern int g_LastFrameTime;
extern int g_TimeSinceLastFrame;

static void WP_UpdateMindtrickEnts(gentity_t *self)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (G_IsMindTricked(&self->client->ps.fd, i))
		{
			gentity_t *ent = &g_entities[i];

			if ( !ent || !ent->client || !ent->inuse || ent->health < 1 ||
				(ent->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
			{
				if (!(g_fixForce.integer & (1 << FP_SEE)
					|| g_gametype.integer == GT_SIEGE
					|| g_gametype.integer == GT_BATTLE_GROUND
					|| g_gametype.integer == GT_JEDIMASTER)
					//|| self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]

					//<= ent->client->ps.fd.forcePowerLevel[FP_SEE] 
					) { //Lugormod
						RemoveTrickedEnt(&self->client->ps.fd, i);
				}

			}
			else if ((level.time - self->client->dangerTime) < g_TimeSinceLastFrame*4)
			{ //Untrick this entity if the tricker (self) fires while in his fov
				if (trap_InPVS(ent->client->ps.origin, self->client->ps.origin) &&
					OrgVisible(ent->client->ps.origin, self->client->ps.origin, ent->s.number))
				{
					RemoveTrickedEnt(&self->client->ps.fd, i);
				}
			}
			else if (BG_HasYsalamiri(g_gametype.integer, &ent->client->ps))
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);
			}
		}

		i++;
	}

	if (!self->client->ps.fd.forceMindtrickTargetIndex &&
		!self->client->ps.fd.forceMindtrickTargetIndex2 &&
		!self->client->ps.fd.forceMindtrickTargetIndex3 &&
		!self->client->ps.fd.forceMindtrickTargetIndex4)
	{ //everyone who we had tricked is no longer tricked, so stop the power
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
	//Ufo:
	else if (g_gametype.integer != GT_FFA &&
		(self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG]) )
	{
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
}

static void WP_ForcePowerRun( gentity_t *self, forcePowers_t forcePower, usercmd_t *cmd )
{
	extern usercmd_t	ucmd;

	switch( (int)forcePower )
	{
	case FP_HEAL:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1)
		{
			if (self->client->ps.velocity[0] || self->client->ps.velocity[1] || self->client->ps.velocity[2])
			{
				WP_ForcePowerStop( self, forcePower );
				break;
			}
		}

		if (self->health < 1 || self->client->ps.stats[STAT_HEALTH] < 1)
		{
			WP_ForcePowerStop( self, forcePower );
			break;
		}

		if (self->client->ps.fd.forceHealTime > level.time)
		{
			break;
		}
		if ( self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{ //rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
			WP_ForcePowerStop( self, forcePower );
			break;
		}
		self->client->ps.fd.forceHealTime = level.time + 1000;
		self->health++;
		self->client->ps.fd.forceHealAmount++;

		if ( self->health > self->client->ps.stats[STAT_MAX_HEALTH])	// Past max health
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
			WP_ForcePowerStop( self, forcePower );
		}

		if ( (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount >= 25) ||
			(self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount >= 33))
		{
			WP_ForcePowerStop( self, forcePower );
		}
#endif
		break;
	case FP_SPEED:
		//This is handled in PM_WalkMove and PM_StepSlideMove
		if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD )
		{
			if ( g_entities[self->client->holdingObjectiveItem].genericValue15 )
			{//disables force powers
				WP_ForcePowerStop( self, forcePower );
			}
		}
		/*
		if ( self->client->ps.powerups[PW_REDFLAG]
		|| self->client->ps.powerups[PW_BLUEFLAG]
		|| self->client->ps.powerups[PW_NEUTRALFLAG] )
		{//no force speed when carrying flag
		WP_ForcePowerStop( self, forcePower );
		}
		*/
		break;
	case FP_GRIP:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		if (self->client->ps.fd.forcePowerDebounce[FP_PULL] < level.time)
		{ //This is sort of not ideal. Using the debounce value reserved for pull for this because pull doesn't need it.
			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 );
			self->client->ps.fd.forcePowerDebounce[FP_PULL] = level.time + 100;
		}

		if (self->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		DoGripAction(self, forcePower);
#endif
		break;
	case FP_LEVITATION:
#ifndef LMD_NEW_FORCEPOWERS
		if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.fd.forceJumpZStart )
		{//done with jump
			WP_ForcePowerStop( self, forcePower );
		}
#endif
		break;
	case FP_RAGE:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime = 400;

			//self->health -= 2; Lugormod moved it down

			if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 150;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 300;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 450;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_4)
			{
				addTime = 600;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_5)
			{
				addTime = 750;
			}
			if ((g_fixForce.integer & (1 << FP_RAGE)
				&& g_gametype.integer != GT_SIEGE
				&& g_gametype.integer != GT_JEDIMASTER
				&& g_gametype.integer != GT_BATTLE_GROUND)
				|| self->client->ps.fd.forcePowerLevel[FP_RAGE] > FORCE_LEVEL_3) {
					self->health -= 1;
					//addTime *= 2; don't over do this
			} else {
				self->health -= 2;
			}
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}

		if (self->health < 1)
		{
			self->health = 1;
			//if ((g_fixForce.integer & (1 << FP_RAGE) 
			//     && g_gametype.integer != GT_SIEGE
			//     && g_gametype.integer != GT_JEDIMASTER
			//     && g_gametype.integer != GT_BATTLE_GROUND
			//     && !g_gameMode(GM_ALLWEAPONS))
			//    || self->client->ps.fd.forcePowerLevel[FP_RAGE] >
			//    FORCE_LEVEL_3) { //Lugormod
			//} else {
			WP_ForcePowerStop(self, forcePower);
			//}
		}

		self->client->ps.stats[STAT_HEALTH] = self->health;
#endif
		break;
	case FP_DRAIN:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( (cmd->buttons & BUTTON_FORCE_DRAIN) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_DRAIN) )
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
			}
		}
		// OVERRIDEFIXME
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) || self->client->ps.fd.forcePowerDuration[FP_DRAIN] < level.time ||
			self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			ForceShootDrain( self );
		}
#endif
		break;
	case FP_LIGHTNING:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{ //Animation for hand extend doesn't end with hand out, so we have to limit lightning intervals by animation intervals (once hand starts to go in in animation, lightning should stop)
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( (cmd->buttons & BUTTON_FORCE_LIGHTNING) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_LIGHTNING) )
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
			}
		}
		// OVERRIDEFIXME
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) || self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] < level.time ||
			self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			ForceShootLightning( self );
			BG_ForcePowerDrain( &self->client->ps, forcePower, 0 );
		}
#endif
		break;
	case FP_TELEPATHY:
		if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD
			&& g_entities[self->client->holdingObjectiveItem].genericValue15 )
		{ //if force hindered can't mindtrick whilst carrying a siege item
			WP_ForcePowerStop( self, FP_TELEPATHY );
		}
		else
		{
			WP_UpdateMindtrickEnts(self);
		}
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	case FP_PROTECT:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 );
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 300;
		}
#endif
		break;
	case FP_ABSORB:
#ifndef LMD_NEW_FORCEPOWERS
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 );
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 600;
		}
#endif
		break;
	default:
		break;
	}
}

int WP_DoSpecificPower( gentity_t *self, usercmd_t *ucmd, forcePowers_t forcepower)
{
	int powerSucceeded;

	powerSucceeded = 1;

	//RoboPhred
	if(self->client->ps.pm_type == PM_FREEZE)
		return 0;

	// OVERRIDEFIXME
	if ( !WP_ForcePowerAvailable( self, forcepower, 0 ) )
	{
		return 0;
	}
	switch(forcepower)
	{
	case FP_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_LEVITATION:
		//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.

		if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE 
			&& self->client->ps.fd.forcePowerLevel[FP_LEVITATION] <
			FORCE_LEVEL_5)
		{
			self->client->ps.fd.forceJumpCharge = 0;
			G_MuteSound( self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE );
			//This only happens if the groundEntityNum == ENTITYNUM_NONE when the button is actually released
		}
		else
		{//still on ground, so jump
			ForceJump( self, ucmd );
		}
		break;
	case FP_SPEED:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceSpeed(self, 0);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_GRIP:
		if (self->client->ps.fd.forceGripEntityNum == ENTITYNUM_NONE)
		{
			ForceGrip( self );
		}

		if (self->client->ps.fd.forceGripEntityNum != ENTITYNUM_NONE)
		{
			if (!(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP)))
			{
				WP_ForcePowerStart( self, FP_GRIP, 0 );
				BG_ForcePowerDrain( &self->client->ps, FP_GRIP, GRIP_DRAIN_AMOUNT );
			}
		}
		else
		{
			powerSucceeded = 0;
		}
		break;
	case FP_LIGHTNING:
		ForceLightning(self);
		//RoboPhred: Stupid, the ForceX commands should really return a boolean.
		//Anyway, the using forcepower check was returning true when lightning failed, so regen was not occuring with
		//my sanity fixes.
		if(!(self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING)))
			powerSucceeded = 0;
		break;
	case FP_PUSH:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease && !(self->r.svFlags & SVF_BOT))
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qfalse);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PULL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qtrue);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TELEPATHY:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTelepathy(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_RAGE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceRage(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PROTECT:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceProtect(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_ABSORB:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceAbsorb(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTeamHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_FORCE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTeamForceReplenish(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_DRAIN:
		ForceDrain(self);
		//RoboPhred: Stupid, the ForceX commands should really return a boolean.
		//Anyway, the using forcepower check was returning true when lightning failed, so regen was not occuring with
		//my sanity fixes.
		if(!(self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN)))
			powerSucceeded = 0;
		break;
	case FP_SEE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceSeeing(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	return powerSucceeded;
}

void FindGenericEnemyIndex(gentity_t *self)
{ //Find another client that would be considered a threat.
	int i = 0;
	float tlen;
	gentity_t *ent;
	gentity_t *besten = NULL;
	float blen = 99999999;
	vec3_t a;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->s.number != self->s.number && ent->health > 0 && !OnSameTeam(self, ent) && ent->client->ps.pm_type != PM_INTERMISSION && ent->client->ps.pm_type != PM_SPECTATOR)
		{
			VectorSubtract(ent->client->ps.origin, self->client->ps.origin, a);
			tlen = VectorLength(a);

			if (tlen < blen &&
				InFront(ent->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f ) &&
				OrgVisible(self->client->ps.origin, ent->client->ps.origin, self->s.number))
			{
				blen = tlen;
				besten = ent;
			}
		}

		i++;
	}

	if (!besten)
	{
		return;
	}

	self->client->ps.genericEnemyIndex = besten->s.number;
}

void SeekerDroneUpdate(gentity_t *self)
{
	vec3_t org, elevated, dir, a, endir;
	gentity_t *en;
	float angle;
	float prefig = 0;
	trace_t tr;

	if (!(self->client->ps.eFlags & EF_SEEKERDRONE))
	{
		self->client->ps.genericEnemyIndex = -1;
		return;
	}

	if (self->health < 1)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.droneExistTime >= level.time && 
		self->client->ps.droneExistTime < (level.time+5000))
	{
		self->client->ps.genericEnemyIndex = 1024+(int)self->client->ps.droneExistTime;
		if (self->client->ps.droneFireTime < level.time)
		{
			G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/laser_trap/warning.wav") );
			self->client->ps.droneFireTime = level.time + 100;
		}
		return;
	}
	else if (self->client->ps.droneExistTime < level.time)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		prefig = (self->client->ps.droneExistTime-level.time)/80;

		if (prefig > 55)
		{
			prefig = 55;
		}
		else if (prefig < 1)
		{
			prefig = 1;
		}

		elevated[2] -= 55-prefig;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.genericEnemyIndex == -1)
	{
		self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		if (!en || !en->client)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->s.number == self->s.number)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->health < 1)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (OnSameTeam(self, en))
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else
		{
			if (!InFront(en->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f ))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
			else if (!OrgVisible(self->client->ps.origin, en->client->ps.origin, self->s.number))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
		}
	}

	if (self->client->ps.genericEnemyIndex == ENTITYNUM_NONE || self->client->ps.genericEnemyIndex == -1)
	{
		FindGenericEnemyIndex(self);
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		//org is now where the thing should be client-side because it uses the same time-based offset
		if (self->client->ps.droneFireTime < level.time)
		{
			trap_Trace(&tr, org, NULL, NULL, en->client->ps.origin, -1, MASK_SOLID);

			if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
			{
				VectorSubtract(en->client->ps.origin, org, endir);
				VectorNormalize(endir);

				WP_FireGenericBlasterMissile(self, org, endir, 0, 15, 2000, MOD_BLASTER);
				G_SoundAtLoc( org, CHAN_WEAPON, G_SoundIndex("sound/weapons/bryar/fire.wav") );

				self->client->ps.droneFireTime = level.time + Q_irand(400, 700);
			}
		}
	}
}

void HolocronUpdate(gentity_t *self)
{ //keep holocron status updated in holocron mode
	int i = 0;
	//RoboPhred: what the fuck?
	int noHRank = 0;

	if (noHRank < FORCE_LEVEL_0)
	{
		noHRank = FORCE_LEVEL_0;
	}
	if (noHRank > FORCE_LEVEL_3)
	{
		noHRank = FORCE_LEVEL_3;
	}

	trap_Cvar_Update(&g_MaxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.holocronsCarried[i])
		{ //carrying it, make sure we have the power
			self->client->ps.holocronBits |= (1 << i);
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			//Ufo: since it's Lugormod let's use level 5
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_5;
		}
		else
		{ //otherwise, make sure the power is cleared from us
			self->client->ps.fd.forcePowerLevel[i] = 0;
			if (self->client->ps.holocronBits & (1 << i))
			{
				self->client->ps.holocronBits -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}
			}
			else if (i == FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown |= (1 << i);

				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}

	if (HasSetSaberOnly())
	{ //if saberonly, we get these powers no matter what (still need the holocrons for level 3)
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
		}
	}
}

//Need this for jedimasterupdate
//The idiots that wrote this doesn't seem to know how to use
//.h-files.
qboolean G_ThereIsAMaster(void);

void JediMasterUpdate(gentity_t *self)
{ //keep jedi master status updated for JM gametype
	int i = 0;

	trap_Cvar_Update(&g_MaxHolocronCarry);
	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.isJediMaster)
		{
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			self->client->ps.fd.forcePowerLevel[i] = g_jmforcelevel.integer;    

			if (i == FP_TEAM_HEAL || i == FP_TEAM_FORCE ||
				i == FP_DRAIN || i == FP_ABSORB ||
				(i == FP_HEAL && g_jmkillhealth.integer > 0))
			{ //team powers are useless in JM, absorb is too because no one else has powers to absorb. Drain is just
				//relatively useless in comparison, because its main intent is not to heal, but rather to cripple others
				//by draining their force at the same time. And no one needs force in JM except the JM himself.
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
				self->client->ps.fd.forcePowerLevel[i] = 0;
			}

			if (i == FP_TELEPATHY && g_jmkillhealth.integer == 0)
			{ //this decision was made because level 3 mindtrick allows the JM to just hide too much, and no one else has force
				//sight to counteract it. Since the JM himself is the focus of gameplay in this mode, having him hidden for large
				//durations is indeed a bad thing.
				//But that is ok if his health counts down.
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_2;
			}
		}
		else
		{
			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) && i != FP_LEVITATION)
			{
				//Not sure, but this seems wrong
				//self->client->ps.fd.forcePowersKnown -= (1 << i);
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				//in mp/ffa3 some places where the saber
				//can be placed is unreachable with
				//levitate level 1. (in some other maps to)
				//Let's see if I can do it this way ...
				//Hmmm... doesn't work when the master kills
				//himself.
				if (G_ThereIsAMaster()) {
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				} else {
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
				}       
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}
}
qboolean WP_HasForcePowers( const playerState_t *ps )
{
	int i;
	if ( ps )
	{
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( i == FP_LEVITATION )
			{
				if ( ps->fd.forcePowerLevel[i] > FORCE_LEVEL_1 )
				{
					return qtrue;
				}
			}
			else if ( ps->fd.forcePowerLevel[i] > FORCE_LEVEL_0 )
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

//try a special roll getup move
qboolean G_SpecialRollGetup(gentity_t *self)
{ //fixme: currently no knockdown will actually land you on your front... so froll's are pretty useless at the moment.
	qboolean rolled = qfalse;

	/*
	if (self->client->ps.weapon != WP_SABER &&
	self->client->ps.weapon != WP_MELEE)
	{ //can't do acrobatics without saber selected
	return qfalse;
	}
	*/

	if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove > 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, SETANIM_BOTH, BOTH_GETUP_BROLL_R, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove < 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, SETANIM_BOTH, BOTH_GETUP_BROLL_L, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove > 0)
	{
		G_SetAnim(self, SETANIM_BOTH, BOTH_GETUP_BROLL_F, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, SETANIM_BOTH, BOTH_GETUP_BROLL_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (self->client->pers.cmd.upmove)
	{
		G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		self->client->ps.forceDodgeAnim = 2;
		self->client->ps.forceHandExtendTime = level.time + 500;

		//self->client->ps.velocity[2] = 300;
	}

	if (rolled)
	{
		G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
	}

	return rolled;
}

//RoboPhred
void Prof_Merc_Flame(gentity_t *ent);
#ifdef LMD_NEW_FORCEPOWERS
void Force_Update(gentity_t *ent);
#endif
void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd ){
	qboolean	usingForce = qfalse;
	int			i, holo, holoregen;
	int			prepower = 0;
	int prof = PlayerAcc_Prof_GetProfession(self);

	if (g_gametype.integer == GT_GHOST) {
		return;
	}

	//see if any force powers are running
	if ( !self )
	{
		return;
	}

	if ( !self->client )
	{
		return;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //not a "real" game client, it's a spectator following someone
		return;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	/*
	if (self->client->ps.fd.saberAnimLevel > self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{
	self->client->ps.fd.saberAnimLevel = self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	else if (!self->client->ps.fd.saberAnimLevel)
	{
	self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	*/
	//The stance in relation to power level is no longer applicable with the crazy new akimbo/staff stances.
	if (!self->client->ps.fd.saberAnimLevel)
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}

	if (g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND)
	{
		if (!(self->client->ps.fd.forcePowersKnown & (1 << FP_LEVITATION)))
		{
			self->client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		}

		//RoboPhred: zero means zero
		/*
		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
		{
		self->client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
		}
		*/
	}

	if (self->client->ps.fd.forcePowerSelected < 0)
	{ //bad
		self->client->ps.fd.forcePowerSelected = 0;
	}

	if ( ((self->client->sess.selectedFP != self->client->ps.fd.forcePowerSelected) ||
		(self->client->sess.saberLevel != self->client->ps.fd.saberAnimLevel)) &&
		!(self->r.svFlags & SVF_BOT) )
	{
		if (self->client->sess.updateUITime < level.time)
		{ //a bit hackish, but we don't want the client to flood with userinfo updates if they rapidly cycle
			//through their force powers or saber attack levels

			self->client->sess.selectedFP = self->client->ps.fd.forcePowerSelected;
			self->client->sess.saberLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (!g_LastFrameTime)
	{
		g_LastFrameTime = level.time;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		self->client->ps.zoomFov = 0;
		self->client->ps.zoomMode = 0;
		self->client->ps.zoomLocked = qfalse;
		self->client->ps.zoomTime = 0;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
		self->client->ps.forceHandExtendTime >= level.time)
	{
		self->client->ps.saberMove = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.saberBlocked = 0;
		self->client->ps.weaponTime = 0;
		self->client->ps.weaponstate = WEAPON_READY;
	}
	else if (self->client->ps.forceHandExtend != HANDEXTEND_NONE &&
		self->client->ps.forceHandExtendTime < level.time)
	{
		if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
			!self->client->ps.forceDodgeAnim)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (G_SpecialRollGetup(self))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else
			{ //hmm.. ok.. no more getting up on your own, you've gotta push something, unless..
				if ((level.time-self->client->ps.forceHandExtendTime) > 4000)
				{ //4 seconds elapsed, I guess they're too dumb to push something to get up!
					if (self->client->pers.cmd.upmove &&
						self->client->ps.fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
					{ //force getup
						G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
						self->client->ps.forceDodgeAnim = 2;
						self->client->ps.forceHandExtendTime = level.time + 500;

						//self->client->ps.velocity[2] = 400;
					}
					else if (self->client->ps.quickerGetup)
					{
						G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
						self->client->ps.forceDodgeAnim = 3;
						self->client->ps.forceHandExtendTime = level.time + 500;
						self->client->ps.velocity[2] = 300;
					}
					else
					{
						self->client->ps.forceDodgeAnim = 1;
						self->client->ps.forceHandExtendTime = level.time + 1000;
					}
				}
			}
			self->client->ps.quickerGetup = qfalse;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_POSTTHROWN)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceDodgeAnim = 1;
				self->client->ps.forceHandExtendTime = level.time + 1000;
				G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
				self->client->ps.velocity[2] = 100;

			}
			else if (!self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceHandExtendTime = level.time + 100;
			}
			else
			{
				self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
			}
		}
		else
		{
			self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
		}
	}

	if (g_gametype.integer == GT_HOLOCRON)
	{
		HolocronUpdate(self);
	}
	if (g_gametype.integer == GT_JEDIMASTER)
	{
		JediMasterUpdate(self);
	}
	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL){
		self->client->ps.fd.forcePowersKnown &= ~g_duelForcePowerDisable.integer;
	}

	SeekerDroneUpdate(self);

	if (self->client->ps.powerups[PW_FORCE_BOON])
	{
		prepower = self->client->ps.fd.forcePower;
	}

	if (self && self->client && (BG_HasYsalamiri(g_gametype.integer, &self->client->ps) ||
		self->client->ps.fd.forceDeactivateAll || self->client->tempSpectate >= level.time))
	{ //has ysalamiri.. or we want to forcefully stop all his active powers
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}

		if (self->client->tempSpectate >= level.time)
		{
			self->client->ps.fd.forcePower = 100;
			self->client->ps.fd.forceRageRecoveryTime = 0;
		}

		self->client->ps.fd.forceDeactivateAll = 0;

		if (self->client->ps.fd.forceJumpCharge)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}
	else
	{ //otherwise just do a check through them all to see if they need to be stopped for any reason.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION &&
				!BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, i))
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}
	}

	i = 0;

	//if (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] || self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] || (self->client->ps.duelInProgress && (self->client->Lmd.duel.duelType & DT_FULL_FORCE)))
	if (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] || self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] || (duelInProgress(&self->client->ps) && (self->client->Lmd.duel.duelType & DT_FULL_FORCE)))
	{ //enlightenment and full force duel
		if (!self->client->ps.fd.forceUsingAdded)
		{
			i = 0;

			while (i < NUM_FORCE_POWERS)
			{
				self->client->ps.fd.forcePowerBaseLevel[i] = self->client->ps.fd.forcePowerLevel[i];

				if (!forcePowerDarkLight[i] ||
					self->client->ps.fd.forceSide == forcePowerDarkLight[i])
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
					self->client->ps.fd.forcePowersKnown |= (1 << i);
				}

				i++;
			}

			self->client->ps.fd.forceUsingAdded = 1;
		}
	}
	else if (self->client->ps.fd.forceUsingAdded)
	{ //we don't have enlightenment but we're still using enlightened powers, so clear them back to how they should be.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			self->client->ps.fd.forcePowerLevel[i] = self->client->ps.fd.forcePowerBaseLevel[i];
			if (!self->client->ps.fd.forcePowerLevel[i])
			{
				if (self->client->ps.fd.forcePowersActive & (1 << i))
				{
					WP_ForcePowerStop(self, i);
				}
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}

			i++;
		}

		self->client->ps.fd.forceUsingAdded = 0;
	}

	i = 0;

	if (!(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)))
	{ //clear the mindtrick index values
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
	}

	if (self->health < 1)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripCripple = 1;

		//keep the saber off during this period
		if (self->client->ps.weapon == WP_SABER && !self->client->ps.saberHolstered)
		{
			Cmd_ToggleSaber_f(self);
		}
	}
	else
	{
		self->client->ps.fd.forceGripCripple = 0;
	}

	if (self->client->ps.fd.forceJumpSound)
	{
		G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		self->client->ps.fd.forceJumpSound = 0;
	}

	if (self->client->ps.fd.forceGripCripple)
	{
		if (self->client->ps.fd.forceGripSoundTime < level.time)
		{
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEGRIP);
			self->client->ps.fd.forceGripSoundTime = level.time + 1000;
		}
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{
		self->client->ps.powerups[PW_SPEED] = level.time + 100;
	}

	if ( self->health <= 0 )
	{//if dead, deactivate any active force powers
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( self->client->ps.fd.forcePowerDuration[i] || (self->client->ps.fd.forcePowersActive&( 1 << i )) )
			{
				WP_ForcePowerStop( self, (forcePowers_t)i );
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		goto powersetcheck;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		self->client->fjDidJump = qfalse;
	}

	if (self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->fjDidJump)
	{ //this was for the "charge" jump method... I guess
		if (ucmd->upmove < 10 && (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LEVITATION))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}

#ifndef METROID_JUMP
	else if ( (ucmd->upmove > 10) && (self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.groundTime && (level.time - self->client->ps.groundTime) > 150 && !BG_HasYsalamiri(g_gametype.integer, &self->client->ps) && BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, FP_LEVITATION) )
	{//just charging up
		ForceJumpCharge( self, ucmd );
		usingForce = qtrue;
	}
	else if (ucmd->upmove < 10 && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.pm_flags &= ~(PMF_JUMP_HELD);
	}
#endif

	if (!(self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.fd.forceJumpCharge)
	{
		if (!(ucmd->buttons & BUTTON_FORCEPOWER) ||
			self->client->ps.fd.forcePowerSelected != FP_LEVITATION)
		{
			if (WP_DoSpecificPower( self, ucmd, FP_LEVITATION ))
			{
				usingForce = qtrue;
			}
		}
	}

	if ( ucmd->buttons & BUTTON_FORCEGRIP )
	{ //grip is one of the powers with its own button.. if it's held, call the specific grip power function.
		if (WP_DoSpecificPower( self, ucmd, FP_GRIP ))
		{
			usingForce = qtrue;
		}
		else
		{ //don't let recharge even if the grip misses if the player still has the button down
			//Ufo:
			//usingForce = qtrue;
		}
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_GRIP)
			{
				WP_ForcePowerStop(self, FP_GRIP);
			}
		}
	}

	if ( ucmd->buttons & BUTTON_FORCE_LIGHTNING )
	{ //lightning
		//RoboPhred
		if(PlayerAcc_Prof_GetProfession(self) == PROF_MERC)
			Prof_Merc_Flame(self);
		else{
			WP_DoSpecificPower(self, ucmd, FP_LIGHTNING);
			usingForce = qtrue;
		}
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LIGHTNING)
			{
				WP_ForcePowerStop(self, FP_LIGHTNING);
			}
		}
	}

	if ( ucmd->buttons & BUTTON_FORCE_DRAIN )
	{ //drain
		WP_DoSpecificPower(self, ucmd, FP_DRAIN);
		usingForce = qtrue;
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_DRAIN)
			{
				WP_ForcePowerStop(self, FP_DRAIN);
			}
		}
	}

	if ( (ucmd->buttons & BUTTON_FORCEPOWER) &&
		BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, self->client->ps.fd.forcePowerSelected))
	{
		if (self->client->ps.fd.forcePowerSelected == FP_LEVITATION)
		{
			// RoboPhred: ufooooo says modded clients can end up using this odd code path.
			/*ForceJumpCharge( self, ucmd );
			usingForce = qtrue;*/
		}
		else if (WP_DoSpecificPower( self, ucmd, self->client->ps.fd.forcePowerSelected ))
		{
			usingForce = qtrue;
		}
		//Ufo:
		/*
		else if (self->client->ps.fd.forcePowerSelected == FP_GRIP)
		{
			usingForce = qtrue;
		}
		*/
	}
	else
		self->client->ps.fd.forceButtonNeedRelease = 0;

#ifdef LMD_NEW_FORCEPOWERS
	Force_Update(self);
#endif
	for ( i = 0; i < NUM_FORCE_POWERS; i++ )
	{
		if ( self->client->ps.fd.forcePowerDuration[i] )
		{
			if ( self->client->ps.fd.forcePowerDuration[i] < level.time )
			{
				if ( (self->client->ps.fd.forcePowersActive&( 1 << i )) )
				{//turn it off
					WP_ForcePowerStop( self, (forcePowers_t)i );
				}
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		if ( (self->client->ps.fd.forcePowersActive&( 1 << i )) )
		{
			usingForce = qtrue;
			WP_ForcePowerRun( self, (forcePowers_t)i, ucmd );
		}
	}

	if ( self->client->ps.saberInFlight && self->client->ps.saberEntityNum )
	{//don't regen force power while throwing saber

		//RoboPhred: REALLY dont regen force power while throwing saber
		usingForce = qtrue;
		/*
		if ( self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0 )//player is 0
		{//
		if ( &g_entities[self->client->ps.saberEntityNum] != NULL && g_entities[self->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR )
		{//fell to the ground and we're trying to pull it back
		usingForce = qtrue;
		}
		}
		*/
	}

	//RoboPhred: dont regen force power while in a special saber move.
	if(self->client->ps.weapon == WP_SABER && BG_SaberInSpecial(self->client->ps.saberMove))
		usingForce = qtrue;

	//RoboPhred: actually use usingForce, previously it just checked forcePowersActive.
	//Also, check debounce time here.
	if ( (!usingForce || self->client->ps.fd.forcePowersActive == (1 << FP_DRAIN)) &&
		self->client->ps.fd.forcePowerRegenDebounceTime < level.time ) {
			//when not using the force, regenerate at 1 point per half second

			//RoboPhred: dont re-check everything, we have usingForce for that.
			/*
			if ( !self->client->ps.saberInFlight && self->client->ps.fd.forcePowerRegenDebounceTime < level.time &&
			(self->client->ps.weapon != WP_SABER || !BG_SaberInSpecial(self->client->ps.saberMove)) )
			{
			*/

			//RoboPhred: take into account the time since last check
			int regenTime;
			float regenPoints = 1;

			if (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) {
				if (self->client->holdingObjectiveItem &&
					g_entities[self->client->holdingObjectiveItem].inuse &&
					g_entities[self->client->holdingObjectiveItem].genericValue15)
				{ //1 point per 7 seconds.. super slow
					regenTime = 7000;
				}
				else if (self->client->siegeClass != -1 &&
					(bgSiegeClasses[self->client->siegeClass].classflags & (1<<CFL_FASTFORCEREGEN)))
				{ //if this is siege and our player class has the fast force regen ability, then recharge with 1/5th the usual delay
					regenTime = (int)(g_forceRegenTime.integer * 0.2);
				}
				else
					regenTime = g_forceRegenTime.integer;
			}
			else if (gameMode(GM_ALLWEAPONS)) {
				if (self->client->ps.powerups[PW_REDFLAG] || self->client->ps.powerups[PW_BLUEFLAG]
				|| self->client->ps.powerups[PW_NEUTRALFLAG])
				{
					regenTime = g_forceRegenTime.integer * 1.5;
				}
				else {
					regenTime = g_forceRegenTime.integer;
				}
			} else if (g_gametype.integer == GT_REBORN) {
				int time = 100 - (25 * (level.teamScores[TEAM_BLUE] - level.teamScores[TEAM_RED]));
				if (time < 0)
					time = 0;
				else if (time > 200)
					time = 200;

				regenTime = time;
			}
			else if ( g_gametype.integer == GT_POWERDUEL && self->client->sess.duelTeam == DUELTEAM_LONE ) {
				if ( g_duel_fraglimit.integer )	{
					regenTime =	(int)(g_forceRegenTime.integer *
						(0.6 + (.3 * (float)self->client->sess.wins / (float)g_duel_fraglimit.integer)));
				}
				else{
					regenTime = (int)(g_forceRegenTime.integer * 0.7);
				}
			}
			else if(prof == PROF_JEDI)
				regenTime = Jedi_GetForceRegenDebounce(self);
			else if (g_gametype.integer == GT_FFA && prof == PROF_ADMIN && !duelInProgress(&self->client->ps))
				regenTime = 1;
			else 
				regenTime = g_forceRegenTime.integer;

			if(regenTime <= 0)
				regenTime = 1;

			//Scale regen points based on time since last regen.
			regenPoints = (float)regenPoints * ((level.time - self->client->ps.fd.forcePowerRegenDebounceTime) / 
				(float)regenTime);

			if (g_gametype.integer != GT_HOLOCRON || g_MaxHolocronCarry.value)
			{
				//if (!g_jediVmerc.integer || self->client->ps.weapon == WP_SABER)
				//let non-jedi force regen since we're doing a more strict jedi/non-jedi thing... this gives dark jedi something to drain
				{
					if (self->client->ps.powerups[PW_FORCE_BOON])
						regenPoints *= 6;
					else if (self->client->ps.isJediMaster && g_gametype.integer == GT_JEDIMASTER)
						regenPoints *= 4; //jedi master regenerates 4 times as fast
					else if (g_gametype.integer == GT_REBORN && level.teamScores[TEAM_RED] == 1)
						regenPoints *= 2;
				}
			}
			else
			{ //regenerate based on the number of holocrons carried
				holoregen = 0;
				holo = 0;
				while (holo < NUM_FORCE_POWERS) {
					if (self->client->ps.holocronsCarried[holo])
						holoregen++;
					holo++;
				}

				regenPoints *= holoregen;
			}

			if( (int) regenPoints > 0) {
				//Have a point to regen, do it.
				WP_ForcePowerRegenerate( self, (int) regenPoints );

				//If we have no points to regen, don't increase the debounce time.  This will ensure we keep to points per time.
				self->client->ps.fd.forcePowerRegenDebounceTime = level.time + regenTime;
			}
	}
	else {
		//Cannot regen force power now.  Keep the debounce time current for the sake of handeling smaller values than the frames run.
		self->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	}

powersetcheck:

	if (prepower && self->client->ps.fd.forcePower < prepower)
	{
		int dif = ((prepower - self->client->ps.fd.forcePower)/2);
		if (dif < 1)
		{
			dif = 1;
		}

		self->client->ps.fd.forcePower = (prepower-dif);
	}
}

qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )
{
	int	dodgeAnim = -1;

	if ( !self || !self->client || self->health <= 0 )
	{
		return qfalse;
	}

	if (!g_forceDodge.integer)
	{
		return qfalse;
	}

	if (g_forceDodge.integer != 2)
	{
		if (!(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
		{
			return qfalse;
		}
	}

	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//can't dodge in mid-air
		return qfalse;
	}

	if ( self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (g_forceDodge.integer == 2)
	{
		if (self->client->ps.fd.forcePowersActive)
		{ //for now just don't let us dodge if we're using a force power at all
			return qfalse;
		}
	}

	if (g_forceDodge.integer == 2)
	{
		if ( !WP_ForcePowerUsable( self, FP_SPEED ) )
		{//make sure we have it and have enough force power
			return qfalse;
		}
	}

	if (g_forceDodge.integer == 2)
	{
		if ( Q_irand( 1, 7 ) > self->client->ps.fd.forcePowerLevel[FP_SPEED] )
		{//more likely to fail on lower force speed level
			return qfalse;
		}
	}
	else
	{
		//We now dodge all the time, but only on level 3
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{//more likely to fail on lower force sight level
			return qfalse;
		}
	}

	switch( hitLoc )
	{
	case HL_NONE:
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
		return qfalse;

	case HL_BACK_RT:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
	case HL_CHEST:
	case HL_WAIST:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodgeAnim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodgeAnim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	default:
		return qfalse;
	}

	if ( dodgeAnim != -1 )
	{
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodgeAnim;
		self->client->ps.forceHandExtendTime = level.time + 300;

		self->client->ps.powerups[PW_SPEEDBURST] = level.time + 100;

		if (g_forceDodge.integer == 2)
		{
			ForceSpeed( self, 500 );
		}
		else
		{
			G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
		}
		return qtrue;
	}
	return qfalse;
}
