
#ifdef LMD_NEW_FORCEPOWERS

#include "g_local.h"
#include "Lmd_Force_Core.h"
#include "w_saber.h"
#include "Lmd_Accounts_Friends.h"

qboolean isBuddy(gentity_t *ent,gentity_t *other);
void WP_AddToClientBitflags(gentity_t *ent, int entNum);

qboolean Force_Heal_Available(gentity_t *self, const void *vData) {
	if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH])
		return qfalse;
	return qtrue;
}

qboolean Force_Heal_Start(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceHeal_t);



	self->health += data->health;
	if (self->health > self->client->ps.stats[STAT_MAX_HEALTH]){
		self->health = self->client->ps.stats[STAT_MAX_HEALTH];
	}
	Force_DrainForceEnergy(self, FP_HEAL, data->forcepower);
	/*
	else
	{
	WP_ForcePowerStart( self, FP_HEAL, 0 );
	}
	*/
	//NOTE: Decided to make all levels instant.

	G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav") );
	return qfalse;
}

#if 0
qboolean Force_Heal_Run(gentity_t *self) {
	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1){
		if (self->client->ps.velocity[0] || self->client->ps.velocity[1] || self->client->ps.velocity[2]){
			return qfalse;
		}
	}

	if (self->client->ps.fd.forceHealTime > level.time)
		return qtrue;

	if ( self->health > self->client->ps.stats[STAT_MAX_HEALTH]){ 
		//rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
		return qfalse;
	}

	self->client->ps.fd.forceHealTime = level.time + 1000;
	self->health++;
	self->client->ps.fd.forceHealAmount++;

	if ( self->health > self->client->ps.stats[STAT_MAX_HEALTH]){
		self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		return qfalse;
	}

	if ( (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount >= 25) ||
		(self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount >= 33)){
			return qfalse;
	}
	return qtrue;
}

void Force_Heal_Stop(gentity_t *self) {
	self->client->ps.fd.forceHealAmount = 0;
	self->client->ps.fd.forceHealTime = 0;
}
#endif

forceHeal_t Force_Heal_Levels[5] = {
	//Ufo: nerfed
	{10, 50},
	{15, 50},
	{25, 50},
	{30, 50},
	{35, 50},
};

forcePower_t Force_Heal = {
	Force_Heal_Available,
	Force_Heal_Start,
	NULL, //Force_Heal_Run,
	NULL, //Force_Heal_Stop,
	qfalse,
	FORCELEVELDATA(Force_Heal_Levels, forceHeal_t),
	256,
};



void WP_AddAsMindtricked(forcedata_t *fd, int entNum){
	if (!fd)
		return;

	if (entNum > 47)
		fd->forceMindtrickTargetIndex4 |= (1 << (entNum-48));
	else if (entNum > 31)
		fd->forceMindtrickTargetIndex3 |= (1 << (entNum-32));
	else if (entNum > 15)
		fd->forceMindtrickTargetIndex2 |= (1 << (entNum-16));
	else
		fd->forceMindtrickTargetIndex |= (1 << entNum);
}

void G_ClearEnemy (gentity_t *self);
void NPC_PlayConfusionSound( gentity_t *self );
void NPC_Jedi_PlayConfusionSound( gentity_t *self );
void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone );
qboolean ForceTelepathyCheckDirectNPCTarget( gentity_t *self, trace_t *tr, qboolean *forceActive, const forceTelepathy_t *data){
	gentity_t	*traceEnt;
	qboolean	targetLive = qfalse, mindTrickDone = qfalse;
	vec3_t		tfrom, tto, fwd;
	float		radius = data->range;

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*radius/2;
	tto[1] = tfrom[1] + fwd[1]*radius/2;
	tto[2] = tfrom[2] + fwd[2]*radius/2;

	trap_Trace( tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID );

	if ( tr->entityNum == ENTITYNUM_NONE || tr->fraction == 1.0f || tr->allsolid || tr->startsolid )
		return qfalse;

	traceEnt = &g_entities[tr->entityNum];

	if( traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
		return qfalse;

	if ( traceEnt->client  ){
		switch ( traceEnt->client->NPC_class ){
		case CLASS_GALAKMECH:
		case CLASS_ATST:
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
		return qfalse;

	if ( targetLive && traceEnt->NPC ){ //hit an organic non-player
		vec3_t	eyeDir;
		if ( G_ActivateBehavior( traceEnt, BSET_MINDTRICK ) ){ //activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
		}
		else if ( (self->NPC && traceEnt->client->playerTeam != self->client->playerTeam)
			|| (!self->NPC && traceEnt->client->playerTeam != self->client->sess.sessionTeam) )
		{//an enemy
			int override = 0;
			if ( (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK) ){

			}
			else if ( traceEnt->s.weapon != WP_SABER && traceEnt->client->NPC_class != CLASS_REBORN )
			{//haha!  Jedi aren't easily confused!
				if ( data->affectnpcjedi ) {//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					if ( traceEnt->s.weapon != WP_NONE ) {//don't charm people who aren't capable of fighting... like ugnaughts and droids
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
						if ( self->NPC ){//NPC
							newPlayerTeam = self->client->playerTeam;
							newEnemyTeam = self->client->enemyTeam;
						}
						else
						{//client/bot
							if ( self->client->sess.sessionTeam == TEAM_BLUE ) {//rebel
								newPlayerTeam = NPCTEAM_PLAYER;
								newEnemyTeam = NPCTEAM_ENEMY;
							}
							else if ( self->client->sess.sessionTeam == TEAM_RED ) {//imperial
								newPlayerTeam = NPCTEAM_ENEMY;
								newEnemyTeam = NPCTEAM_PLAYER;
							}
							else {//neutral - wan't attack anyone
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
						traceEnt->NPC->charmedTime = level.time + data->npcduration;
					}
				}
				else {//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + data->npcduration;
					NPC_PlayConfusionSound( traceEnt );
					if ( traceEnt->enemy )
						G_ClearEnemy( traceEnt );
				}
			}
			else
				NPC_Jedi_PlayConfusionSound( traceEnt );
		}
		else if ( traceEnt->client->playerTeam == self->client->playerTeam ) {//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if ( traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC!=NULL && 
				!(traceEnt->NPC->scriptFlags & SCF_NO_RESPONSE) )
			{
				NPC_UseResponse( traceEnt, self, qfalse );
			}
		}
		AngleVectors( traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL );
		VectorNormalize( eyeDir );
		G_PlayEffectID( G_EffectIndex( "force/force_touch" ), traceEnt->client->renderInfo.eyePoint, eyeDir );

		mindTrickDone = qtrue;
	}
	else {
		if ( data->divertnpcs && tr->fraction * 2048 > 64 )
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffectID( G_EffectIndex( "force/force_touch" ), tr->endpos, tr->plane.normal );
			//FIXME: these events don't seem to always be picked up...?
			AddSoundEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue );//, qtrue );
			AddSightEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, 50 );
			*forceActive = qtrue;
		}
	}
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if(mindTrickDone)
		*forceActive = qtrue;
	return mindTrickDone;
}

qboolean Force_Telepathy_Available(gentity_t *self, const void *data) {
	//Ufo:
	if (g_gametype.integer != GT_FFA && (self->client->ps.powerups[PW_REDFLAG] || self->client->ps.powerups[PW_BLUEFLAG]))
	{ //can't mindtrick while carrying the flag
		return qfalse;
	}
	//FIXME: move into general usable code
	if (self->client->ps.weaponTime > 0)
		return qfalse;
	//FIXME: move into general usable code?
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;
	return qtrue;
}

//const int mindTrickTime[NUM_FORCE_POWER_LEVELS + 2]
qboolean Force_Telepathy_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceTelepathy_t);
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	qboolean	forceActive = qfalse;


	if ( ForceTelepathyCheckDirectNPCTarget( self, &tr, &forceActive, data ) ) {//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		self->client->ps.fd.forcePowerDuration[FP_TELEPATHY] = level.time + data->duration;
		return qtrue;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = center[i] - data->range;
		maxs[i] = center[i] + data->range;;
	}

	if (data->arc == 0){
		if (tr.fraction != 1.0 &&  tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].inuse &&
			g_entities[tr.entityNum].client && g_entities[tr.entityNum].client->pers.connected &&
			g_entities[tr.entityNum].client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			WP_AddAsMindtricked(&self->client->ps.fd, tr.entityNum);

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;

			forceActive = qtrue;
		}
	}
	else {
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		//Ufo: had 2 instances of incrementation on e
		for(; e < numListedEntities; e++) {
			ent = &g_entities[entityList[e]];
			
			if(!ent->client || ent == self)
				continue;
			
			VectorCopy(ent->client->ps.origin, thispush_org);

			VectorCopy(self->client->ps.origin, tto);
			tto[2] += self->client->ps.viewheight;
			VectorSubtract(thispush_org, tto, a);
			vectoangles(a, a);
			
			if (!InFieldOfVision(self->client->ps.viewangles, data->arc, a))
				continue;
			else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
				continue;
			else if (OnSameTeam(self, ent))
				continue;
			gotatleastone = qtrue;
			WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);
			//e++;
		}

		if (gotatleastone){
			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
			self->client->ps.fd.forcePowerDuration[FP_TELEPATHY] = level.time + data->duration;
			forceActive = qtrue;
		}
	}
	if(forceActive) {
		Force_DrainForceEnergy(self, FP_TELEPATHY, data->forcepower);
	}
	return forceActive;
}

void Force_Telepathy_Stop(gentity_t *self, const void *data) {
	//Ufo:
	//G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav") );
	self->client->ps.fd.forcePowerDuration[FP_TELEPATHY] = level.time + 0;
	self->client->ps.fd.forceMindtrickTargetIndex = 0;
	self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
	self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
	self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
}

extern int mindTrickTime[NUM_FORCE_POWER_LEVELS + 2];
forceTelepathy_t Force_Telepathy_Levels[5] = {
	{20000, mindTrickTime[1], MAX_TRICK_DISTANCE,		0,		qfalse,	qfalse, 20},
	{25000, mindTrickTime[2], MAX_TRICK_DISTANCE,		180,	qtrue,	qfalse,	20},
	{30000, mindTrickTime[3], MAX_TRICK_DISTANCE * 2,	360,	qtrue,	qtrue,	20},
	{40000, mindTrickTime[4], MAX_TRICK_DISTANCE * 3,	360,	qtrue,	qtrue,	20},
	{50000, mindTrickTime[5], MAX_TRICK_DISTANCE * 4,	360,	qtrue,	qtrue,	20}
};

forcePower_t Force_Telepathy = {
	Force_Telepathy_Available,
	Force_Telepathy_Start,
	NULL,
	Force_Telepathy_Stop,
	qfalse,
	FORCELEVELDATA(Force_Telepathy_Levels, forceTelepathy_t),
	256,
};


extern int protectLoopSound;
qboolean Force_Protect_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceProtect_t);
	forceAbsorb_t *absorbData = (forceAbsorb_t *)Force_GetPlayerForceData(self, FP_ABSORB);
	// Make sure to turn off Force Rage and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
		Force_StopPower( self, FP_RAGE );
	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)
		//Ufo: flipped purpose of g_fixforce
		&& (!absorbData->allowprotect || g_fixForce.integer & (1 << FP_PROTECT)))
	{
		Force_StopPower( self, FP_ABSORB );
	}

	Force_DrainForceEnergy(self, FP_PROTECT, data->forcepower);

	G_PreDefSound(self->client->ps.origin, PDSOUND_PROTECT);
	G_Sound( self, TRACK_CHANNEL_3, protectLoopSound );
	self->client->ps.fd.forcePowerDuration[FP_PROTECT] = level.time + data->duration;

	return qtrue;
}

qboolean Force_Protect_Run(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceProtect_t);
	if (self->client->ps.fd.forcePowerDebounce[FP_PROTECT] < level.time){
		BG_ForcePowerDrain( &self->client->ps, FP_PROTECT, 1 );
		if (self->client->ps.fd.forcePower < 1)
			return qfalse;
		self->client->ps.fd.forcePowerDebounce[FP_PROTECT] = level.time + data->draintime;
	}
	return qtrue;
}

int Force_Protect_ModifyDamage(gentity_t *targ, int take) {
	forceProtect_t *data = (forceProtect_t *)Force_GetPlayerForceData(targ, FP_PROTECT);
	if (targ->client->ps.fd.forcePower <= 0)
		return take;

	int maxtake = take;

	if (targ->client->forcePowerSoundDebounce < level.time){
		G_PreDefSound(targ->client->ps.origin, PDSOUND_PROTECTHIT);
		targ->client->forcePowerSoundDebounce = level.time + 400;
	}

	if(maxtake > data->maxdamage)
		maxtake = data->maxdamage;


	if (targ->client->ps.powerups[PW_FORCE_BOON] > 0)
		targ->client->ps.fd.forcePower -= (maxtake * data->forcetake) / 2;
	else
		targ->client->ps.fd.forcePower -= maxtake * data->forcetake;

	//RoboPhred: what the hell is this...  Way too many variables going into a single value...
	//maxtake is take capped, and maxtake is percentaged to damage take,
	//and added to the value of take subtracted by maxtake
	int subamt = (maxtake * data->damagetake) + (take - maxtake);
	if (targ->client->ps.fd.forcePower < 0){
		subamt += targ->client->ps.fd.forcePower;
		targ->client->ps.fd.forcePower = 0;
	}

	if (subamt){
		take -= subamt;

		if (take < 0)
			take = 0;
	}
	return take;
}

void Force_Protect_Stop(gentity_t *self, const void *vData) {
	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
}

forceProtect_t Force_Protect_Levels[5] = {
	{1,		0.40, 100, 20000, 300, 50},
	{0.5,	0.60, 200, 20000, 300, 25},
	{0.25,	0.80, 400, 20000, 300, 10},
	{0.20,	0.85, 600, 20000, 300, 10},
	{0.15,	0.90, 800, 20000, 300, 10},
};

forcePower_t Force_Protect = {
	NULL,
	Force_Protect_Start,
	Force_Protect_Run,
	Force_Protect_Stop,
	qfalse,
	FORCELEVELDATA(Force_Protect_Levels, forceProtect_t),
	256,
};



extern int absorbLoopSound;
qboolean Force_Absorb_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceAbsorb_t);
	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
		Force_StopPower( self, FP_RAGE );
	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)
		//Lugormod absorb + protect
		//Ufo: flipped purpose of g_fixforce
		&& (!data->allowprotect || g_fixForce.integer & (1 << FP_PROTECT)))
	{
		Force_StopPower( self, FP_PROTECT );
	}

	G_PreDefSound(self->client->ps.origin, PDSOUND_ABSORB);
	G_Sound( self, TRACK_CHANNEL_3, absorbLoopSound );
	self->client->ps.fd.forcePowerDuration[FP_ABSORB] = level.time + data->duration;
	BG_ForcePowerDrain(&self->client->ps, FP_ABSORB, data->forcepower);

	return qtrue;
}

qboolean Force_Absorb_Run(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceAbsorb_t);
	if (self->client->ps.fd.forcePowerDebounce[FP_ABSORB] < level.time){
		BG_ForcePowerDrain( &self->client->ps, FP_ABSORB, 1 );
		if (self->client->ps.fd.forcePower < 1)
			return qfalse;

		self->client->ps.fd.forcePowerDebounce[FP_ABSORB] = level.time + data->draintime;
	}
	return qtrue;
}

void Force_Absorb_Stop(gentity_t *self, const void *vData) {
	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
}

forceAbsorb_t Force_Absorb_Levels[5] = {
	{20000, 600, qfalse, 50},
	{20000, 600, qfalse, 25},
	{20000, 600, qfalse, 10},
	{20000, 600, qfalse, 10},
	{20000, 600, qtrue, 10},
};

forcePower_t Force_Absorb = {
	NULL,
	Force_Absorb_Start,
	Force_Absorb_Run,
	Force_Absorb_Stop,
	qfalse,
	FORCELEVELDATA(Force_Absorb_Levels, forceAbsorb_t),
	256,
};


qboolean Force_TeamHeal_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceTeamHeal_t);
	int i;
	gentity_t *ent;
	vec3_t a;
	int numpl = 0;
	gentity_t *pl[MAX_CLIENTS];
	int healthadd = 0;
	gentity_t *te = NULL;

	qboolean targetFriends = self->client->pers.Lmd.buddyindex[0] || self->client->pers.Lmd.buddyindex[1] || PlayerAcc_Friends_Count(self) > 0;


	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];

		if (self == ent)
			continue;
		if (!ent->client)
			continue;

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			continue;

		if (g_gametype.integer == GT_FFA) {
			// Check buddy list.

			if (targetFriends) {
				if (!isBuddy(self, ent) && !PlayerAcc_Friends_IsFriend(self, ent->client->sess.Lmd.id)) {
					continue;
				}
			}
		}
		//Ufo: duplicate
/*
		if (g_gametype.integer == GT_FFA) {
			if (((self->client->pers.Lmd.buddyindex[0] > 0 || self->client->pers.Lmd.buddyindex[1] > 0) && !isBuddy(self, ent)) ||
				(PlayerAcc_Friends_Count(self) > 0 && !PlayerAcc_Friends_IsFriend(self, ent->client->sess.Lmd.id)))
			{
				continue;
			}
		}
*/
		else if (OnSameTeam(self, ent) == qfalse)
			continue;

		if (ent->client->ps.stats[STAT_HEALTH] >= ent->client->ps.stats[STAT_MAX_HEALTH] || ent->client->ps.stats[STAT_HEALTH] < 0)
			continue;

		if(!ForcePowerUsableOn(self, ent, FP_TEAM_HEAL))
			continue;

		if(!trap_InPVS(self->client->ps.origin, ent->client->ps.origin))
			continue;

		VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);
		if (VectorLength(a) > data->range)
			continue;

		pl[numpl++] = ent;
	}

	if (numpl == 0)
		return qfalse;

	if (numpl == 1)
		healthadd = data->heal[0];
	else if (numpl == 2)
		healthadd = data->heal[1];
	else
		healthadd = data->heal[2];

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] = level.time + data->debounce;
	i = 0;

	for(i = 0; i < numpl; i++) {
		ent = pl[i];
		ent->client->ps.stats[STAT_HEALTH] += healthadd;
		if (ent->client->ps.stats[STAT_HEALTH] > ent->client->ps.stats[STAT_MAX_HEALTH])
			ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH];

		ent->health = ent->client->ps.stats[STAT_HEALTH];

		//At this point we know we got one, so add him into the collective event client bitflag
		if (!te){
			te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 1; //eventParm 1 is heal, eventParm 2 is force regen

			Force_DrainForceEnergy(self, FP_TEAM_HEAL, data->forcepower);
		}

		WP_AddToClientBitflags(te, ent->s.number);
	}
	return qfalse;
}

forceTeamHeal_t Force_TeamHeal_Levels[5] = {
	//Ufo: fixed forcepower of level 5
	{256,		50,		33, 25, 2000, 50},
	{256 * 1.5,	50,		33,	25, 2000, 33},
	{256 * 2,	50,		33,	25, 2000, 25},
	{256 * 3,	50,		33,	25, 2000, 25},
	{256 * 3,	100,	66, 50, 2000, 50},
};

forcePower_t Force_TeamHeal = {
	NULL,
	Force_TeamHeal_Start,
	NULL,
	NULL,
	qfalse,
	FORCELEVELDATA(Force_TeamHeal_Levels, forceTeamHeal_t),
	256,
};

#endif
