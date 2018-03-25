
#ifdef LMD_NEW_FORCEPOWERS


#include "g_local.h"
#include "Lmd_Force_Core.h"
#include "Lmd_Professions.h"
#include "w_saber.h"
#include "Lmd_Accounts_Friends.h"

qboolean isBuddy(gentity_t *ent, gentity_t *other);
void WP_AddToClientBitflags(gentity_t *ent, int entNum);
int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
void BotForceNotification (gclient_t *bot, gentity_t *attacker);

qboolean Force_Grip_Available(gentity_t *self, const void* vData) {
	if (self->client->ps.fd.forceGripUseTime > level.time)
		return qfalse;
	//TODO: move into generic usability code?
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;

	//TODO: move into generic usability code?
	if (self->client->ps.weaponTime > 0)
		return qfalse;

	return qtrue;
}

int Force_Grip_Distance(gentity_t *self, const forceGrip_t *data) {
	if (PlayerAcc_Prof_GetProfession(self) == PROF_ADMIN)
		return 4096;
	return data->range;
}

qboolean Force_Grip_Start(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceGrip_t);

	trace_t tr;
	vec3_t tfrom, tto, fwd;

	int gripdist = Force_Grip_Distance(self, data);

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0] * gripdist;
	tto[1] = tfrom[1] + fwd[1] * gripdist;
	tto[2] = tfrom[2] + fwd[2] * gripdist;

	trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 && tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time &&
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum])) ) //don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS && g_entities[tr.entityNum].client->ps.m_iVehicleNum){
			//a player on a vehicle
			gentity_t *vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle){
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{ //push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)&g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;

		//Ufo: so their force regen is restored if they were caught jumping
		if (g_entities[tr.entityNum].client->ps.fd.forcePowersActive & (1 << FP_LEVITATION) )
			Force_StopPower(&g_entities[tr.entityNum], FP_LEVITATION);

		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;

		BG_ForcePowerDrain( &self->client->ps, FP_GRIP, 30 );
	}
	else {
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
		return qfalse;
	}

	self->client->ps.powerups[PW_DISINT_4] = level.time + 60000;

	return qtrue;
}

float Force_Grip_AbsorbPower(gentity_t *self, gentity_t *targ, const forceGrip_t *data) {
	int powerLevel = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	int modPowerLevel = WP_AbsorbConversion(targ, targ->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_GRIP,
		powerLevel, data->forcepower);
	if (modPowerLevel == -1)
		modPowerLevel = powerLevel;
	return ((float)modPowerLevel) / ((float)powerLevel);
}

qboolean Force_Grip_Run(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceGrip_t);

	gentity_t *gripEnt;
	int gripLevel = 0;
	trace_t tr;
	vec3_t a;
	vec3_t fwd, fwd_o, start_o, nvel;
	int gripDist;

	if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD || self->client->ps.fd.forcePower < 1){
		return qfalse;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_RAGE] < level.time){ //Ufo: changed to rage
		//This is sort of not ideal. Using the debounce value reserved for pull for this because pull doesn't need it.
		Force_DrainForceEnergy( self, FP_GRIP, data->forcedrain );
		self->client->ps.fd.forcePowerDebounce[FP_RAGE] = level.time + 100;
	}

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gripEnt = &g_entities[self->client->ps.fd.forceGripEntityNum];

	if (!gripEnt || !gripEnt->client || !gripEnt->inuse || gripEnt->health < 1 ||
		!ForcePowerUsableOn(self, gripEnt, FP_GRIP))
	{
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		if (gripEnt && gripEnt->client && gripEnt->inuse)
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		return qfalse;
	}

	gripDist = Force_Grip_Distance(self, data);
	gripDist *= Force_Grip_AbsorbPower(self, gripEnt, data);

	if (!gripDist)	{
		return qfalse;
	}

	VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);
	if (VectorLength(a) > gripDist){
		return qfalse;
	}

	if ( !InFront( gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) && 
		!data->omni)
	{
		return qfalse;
	}

	trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0f && tr.entityNum != gripEnt->s.number)
		return qfalse;

	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time){
		//2 damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		G_Damage(gripEnt, self, self, NULL, NULL, data->damage, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
	}

#ifndef LMD_NEW_JETPACK
	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.
#endif

	gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

	if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > data->duration){
		return qfalse;
	}

	if (data->moveType > 0 && gripEnt->client->ps.forceGripMoveInterval < level.time)	{
		gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;
		
		//we can move, so we can be thrown off an edge.
		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;

		if(data->moveType == 1) {
			gripEnt->client->ps.velocity[2] = 30;
		}
		else if(data->moveType == 2) {
			float nvLen	= 0;

			VectorCopy(gripEnt->client->ps.origin, start_o);
			AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
			fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
			fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
			fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
			fwd_o[2] += 16;
			VectorSubtract(fwd_o, start_o, nvel);

			nvLen = VectorLength(nvel);

			if (nvLen < 16){
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*8;
				gripEnt->client->ps.velocity[1] = nvel[1]*8;
				gripEnt->client->ps.velocity[2] = nvel[2]*8;
			}
			else if (nvLen < 64){
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*128;
				gripEnt->client->ps.velocity[1] = nvel[1]*128;
				gripEnt->client->ps.velocity[2] = nvel[2]*128;
			}
			else if (nvLen < 128){
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*256;
				gripEnt->client->ps.velocity[1] = nvel[1]*256;
				gripEnt->client->ps.velocity[2] = nvel[2]*256;
			}
			else if (nvLen < 200){
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*512;
				gripEnt->client->ps.velocity[1] = nvel[1]*512;
				gripEnt->client->ps.velocity[2] = nvel[2]*512;
			}
			else{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0]*700;
				gripEnt->client->ps.velocity[1] = nvel[1]*700;
				gripEnt->client->ps.velocity[2] = nvel[2]*700;
			}
		}

		if (data->choke && 
			(level.time - gripEnt->client->ps.fd.forceGripStarted) > 3000 &&
			!self->client->ps.fd.forceGripDamageDebounceTime)
		{ //if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, data->choke, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

			//Ufo: if we still have any saber blade left on, turn it off now
			gripEnt->client->ps.saberHolstered = 2;

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP)){ 
				//choking, so don't let him keep gripping himself
				Force_StopPower(gripEnt, FP_GRIP);
			}
		}
		else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 4000)
			return qfalse;
	}
	return qtrue;
}

void Force_Grip_Stop(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceGrip_t);
	//Ufo:
	self->client->ps.fd.forceGripUseTime = level.time + 4000;
	if (data->throatcrush && g_entities[self->client->ps.fd.forceGripEntityNum].client &&
		g_entities[self->client->ps.fd.forceGripEntityNum].health > 0 &&
		g_entities[self->client->ps.fd.forceGripEntityNum].inuse &&
		(level.time - g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.fd.forceGripStarted) > 500)
	{ //if we had our throat crushed in for more than half a second, gasp for air when we're let go
		G_EntitySound( &g_entities[self->client->ps.fd.forceGripEntityNum], CHAN_VOICE, G_SoundIndex("*gasp.wav") );
	}

	if (g_entities[self->client->ps.fd.forceGripEntityNum].client &&
		g_entities[self->client->ps.fd.forceGripEntityNum].inuse)
	{

		g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.forceGripChangeMovetype = PM_NORMAL;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		self->client->ps.forceHandExtendTime = 0;

	self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

	self->client->ps.powerups[PW_DISINT_4] = 0;
}

forceGrip_t Force_Grip_Levels[5] = {
	//Ufo: fixed omni of level 3
	{MAX_GRIP_DISTANCE,	2, 0,	5000, 0, GRIP_DRAIN_AMOUNT, 1, qfalse,	qfalse},
	{MAX_GRIP_DISTANCE,	2, 20,	4000, 1, GRIP_DRAIN_AMOUNT, 1, qtrue,	qfalse},
	{MAX_GRIP_DISTANCE,	2, 40,	4000, 2, GRIP_DRAIN_AMOUNT, 1, qtrue,	qtrue},
	{MAX_GRIP_DISTANCE * 2,	2, 40,	4000, 2, GRIP_DRAIN_AMOUNT, 1, qtrue,	qtrue},
	{MAX_GRIP_DISTANCE * 4,	2, 40,	4000, 2, GRIP_DRAIN_AMOUNT, 1, qtrue,	qtrue},
};

forcePower_t Force_Grip = {
	Force_Grip_Available,
	Force_Grip_Start,
	Force_Grip_Run,
	Force_Grip_Stop,
	qtrue,
	FORCELEVELDATA(Force_Grip_Levels, forceGrip_t),
	256,
};


qboolean Force_Lightning_Available(gentity_t *self, const void *vData) {
	//TODO: move these to generic usability
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;

	if (self->client->ps.weaponTime > 0)
		return qfalse;


	if ( self->client->ps.fd.forcePower < 25)
		return qfalse;
	return qtrue;
}

qboolean Force_Lightning_Start(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceLightning_t);
	
	//FIXME: make sure all offensive forcepowers have this.
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;


	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/lightning") );

	self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
	
	Force_DrainForceEnergy(self, FP_LIGHTNING, data->forcepower);

	return qtrue;
}

float Force_Lightning_AbsorbPower(gentity_t *self, gentity_t *targ, const forceLightning_t *data) {
	int powerLevel;
	powerLevel = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
	int modPowerLevel = WP_AbsorbConversion(targ, targ->client->ps.fd.forcePowerLevel[FP_ABSORB], self, 
		FP_LIGHTNING, powerLevel, data->forcepower);
	if (modPowerLevel == -1)
		modPowerLevel = powerLevel;
	return ((float)modPowerLevel) / ((float)powerLevel);
}

extern void Jedi_Decloak( gentity_t *self );
void Force_Lightning_Damage(gentity_t *self, gentity_t *target, vec3_t dir, const forceLightning_t *data) {

	if (!target->client && target->s.eType == ET_NPC){
		//g2animent
		if (target->s.genericenemyindex < level.time)
			target->s.genericenemyindex = level.time + 2000;
	}
	if ( target->client ){
		//an enemy or object
		if (target->client->noLightningTime >= level.time){ 
			//give them power and don't hurt them.
			target->client->ps.fd.forcePower++;
			if (target->client->ps.fd.forcePower > 100)
				target->client->ps.fd.forcePower = 100;
			return;
		}
		if (ForcePowerUsableOn(self, target, FP_LIGHTNING)){
			//Ufo: default damage is too high
			int	dmg = 1; //Q_irand(1, 2);

			float absorb = Force_Lightning_AbsorbPower(self, target, data);
			if(absorb == 0)
				return;
			if(absorb != 1) {
				absorb = 1.0f - absorb;
				target->client->noLightningTime = level.time + 100 + (absorb * 300.0f);
			}

			if ( self->client->ps.weapon == WP_MELEE && data->twohanded )
			{//2-handed lightning
				//jackin' 'em up, Palpatine-style
				dmg *= 2;
			}
			if (g_gametype.integer == GT_REBORN ) {
				dmg *=3;
			}


			if (dmg){
				//rww - Shields can now absorb lightning too.
				G_Damage( target, self, self, dir, self->client->ps.origin, dmg, 0, MOD_FORCE_DARK );
			}
			if ( target->client ){
				if ( !Q_irand( 0, 2 ) ){
					G_Sound( target, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
				}

				if (target->client->ps.electrifyTime < (level.time + 400)){
					//only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
					target->client->ps.electrifyTime = level.time + 800;
				}
				if ( target->client->ps.powerups[PW_CLOAKED] ){
					//disable cloak temporarily
					Jedi_Decloak( target );
					target->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				}
			}
		}
	}
}

qboolean Force_Lightning_Run(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceLightning_t);
	if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD){ 
		//Animation for hand extend doesn't end with hand out, so we have to limit lightning intervals by animation intervals (once hand starts to go in in animation, lightning should stop)
		//Keep it running though
		return qtrue;
	}

	if (data->duration > 0 ){
		if ( (self->client->pers.cmd.buttons & BUTTON_FORCE_LIGHTNING) || 
			((self->client->pers.cmd.buttons & BUTTON_FORCEPOWER) &&
			self->client->ps.fd.forcePowerSelected == FP_LIGHTNING) )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
		}
	}
	// OVERRIDEFIXME
	if (self->client->ps.fd.forcePower < 25){
		//FIXME: should this be handled in available check when running?
		return qfalse;
	}
	
	
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( data->arc ){
		//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );

		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = center[i] - data->range;
			maxs[i] = center[i] + data->range;
		}

		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		for(e = 0; e < numListedEntities; e++) {
			traceEnt = &g_entities[iEntityList[e]];

			if ( traceEnt == self )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( center[i] < traceEnt->r.absmin[i] ) 
					v[i] = traceEnt->r.absmin[i] - center[i];
				else if ( center[i] > traceEnt->r.absmax[i] ) 
					v[i] = center[i] - traceEnt->r.absmax[i];
				else 
					v[i] = 0;
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			//FIXME: need to figure out what this value actually is and ajust it for angle affect.
			if ( DotProduct( dir, forward ) < 0.5 )
				continue;

			//must be close enough
			if ( VectorLength( v ) >= data->range ) {
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
			Force_Lightning_Damage( self, traceEnt, dir, data);
		}
	}
	else{
		VectorMA( self->client->ps.origin, data->range, forward, end );

		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			Force_DrainForceEnergy(self, FP_LIGHTNING, data->forcepower); //Ufo: was missing
			return qtrue;
		}

		traceEnt = &g_entities[tr.entityNum];
		Force_Lightning_Damage( self, traceEnt, forward, data);
	}
	Force_DrainForceEnergy(self, FP_LIGHTNING, data->forcepower);
	return qtrue;
}

void Force_Lightning_Stop(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceLightning_t);
	if ( data->debounce )
		self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + data->debounce; //3000;

	if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		self->client->ps.forceHandExtendTime = 0; //reset hand position

	self->client->ps.activeForcePass = 0;
}

forceLightning_t Force_Lightning_Levels[5] = {
	//Ufo: fixed radius of level 2
	{2048,				0, 0,		3000, 1, qfalse},
	{2048,				0, Q3_INFINITE,	1500, 1, qfalse},
	{FORCE_LIGHTNING_RADIUS,	1, Q3_INFINITE,	1500, 1, qtrue},
	{FORCE_LIGHTNING_RADIUS * 1.5,	1, Q3_INFINITE,	1500, 1, qtrue},
	{FORCE_LIGHTNING_RADIUS * 2,	1, Q3_INFINITE,	1500, 1, qtrue}
};

forcePower_t Force_Lightning = {
	Force_Lightning_Available,
	Force_Lightning_Start,
	Force_Lightning_Run,
	Force_Lightning_Stop,
	qtrue,
	FORCELEVELDATA(Force_Lightning_Levels, forceLightning_t),
	256,
};

qboolean Force_Rage_Available(gentity_t *self, const void *vData) {
	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
		return qfalse;
	return qtrue;
}

extern int rageLoopSound;
qboolean Force_Rage_Start(gentity_t *self, const void* vData) {
	GETFORCEDATA(forceRage_t);
	if (self->health < 10)
		return qfalse;

	if (self->health < 20 
		&& g_fixForce.integer & (1 << FP_RAGE) 
		&& g_gametype.integer != GT_SIEGE 
		&& g_gametype.integer != GT_BATTLE_GROUND
		&& g_gametype.integer != GT_JEDIMASTER) {
			//Lugormod
			return qfalse;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
		Force_StopPower( self, FP_PROTECT );
	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
		Force_StopPower( self, FP_ABSORB );

	G_Sound( self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/rage.wav") );
	G_Sound( self, TRACK_CHANNEL_3, rageLoopSound );

	self->client->ps.fd.forcePowerDuration[FP_RAGE] = level.time + data->duration;
	Force_DrainForceEnergy(self, FP_RAGE, data->forcepower);
	return qtrue;
}

qboolean Force_Rage_Run(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceRage_t);
	if (self->client->ps.forceRageDrainTime < level.time){
		if(g_fixForce.integer & (1 << FP_RAGE) && g_gametype.integer != GT_SIEGE
				&& g_gametype.integer != GT_JEDIMASTER
				&& g_gametype.integer != GT_BATTLE_GROUND)
		{
			self->health -= 1;
		}
		else
			self->health -= data->selfdamage;
		self->client->ps.forceRageDrainTime = level.time + data->draintime;
	}

	if (self->health < 1){
		self->health = 1;
		self->client->ps.stats[STAT_HEALTH] = self->health;
		return qfalse;
	}

	self->client->ps.stats[STAT_HEALTH] = self->health;
	return qtrue;
}

void Force_Rage_Stop(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceRage_t);
	self->client->ps.fd.forceRageRecoveryTime = level.time + data->recoverytime;
	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
}

forceRage_t Force_Rage_Levels[5] = {
	{2, 8000,	150,	10000, 50},
	{2, 14000,	300, 	10000, 50},
	{2, 20000,	450,	10000, 50},
	{1, 20000,	600,	10000, 50},
	{1, 20000,	750,	10000, 50},
};

forcePower_t Force_Rage = {
	Force_Rage_Available,
	Force_Rage_Start,
	Force_Rage_Run,
	Force_Rage_Stop,
	qfalse,
	FORCELEVELDATA(Force_Rage_Levels, forceRage_t),
	256,
};


qboolean Force_TeamReplenish_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceTeamReplenish_t);
	int i;
	gentity_t *ent;
	vec3_t a;
	int numpl = 0;
	gentity_t *pl[MAX_CLIENTS];
	int poweradd = 0;
	gentity_t *te = NULL;

	qboolean targetFriends = self->client->pers.Lmd.buddyindex[0] || self->client->pers.Lmd.buddyindex[1] || PlayerAcc_Friends_Count(self) > 0;

	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];

		if(self == ent)
			continue;
		if(!ent->client)
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
		if(g_gametype.integer == GT_FFA) {
			if(((self->client->pers.Lmd.buddyindex[0] > 0 || self->client->pers.Lmd.buddyindex[1] > 0) && !isBuddy(self, ent)) ||
				(PlayerAcc_Friends_Count(self) > 0 && !PlayerAcc_Friends_IsFriend(self, ent->client->sess.Lmd.id)))
			{
				continue;
			}
		}
*/
		else if(OnSameTeam(self, ent) == qfalse)
			continue;

		if(ent->client->ps.fd.forcePower >= ent->client->ps.fd.forcePowerMax)
			continue;

		if(!ForcePowerUsableOn(self, ent, FP_TEAM_FORCE))
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
		poweradd = data->power[0];
	else if (numpl == 2)
		poweradd = data->power[1];
	else
		poweradd = data->power[2];

	Force_DrainForceEnergy(self, FP_TEAM_FORCE, data->forcepower);

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + data->debounce;
	i = 0;

	for(i = 0; i < numpl; i++) {
		ent = pl[i];
		ent->client->ps.fd.forcePower += poweradd;
		if (ent->client->ps.fd.forcePower > ent->client->ps.fd.forcePowerMax)
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax;

		if (!te){
			te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 2; //eventParm 1 is heal, eventParm 2 is force regen
		}

		WP_AddToClientBitflags(te, ent->s.number);
	}
	return qfalse;
}

forceTeamReplenish_t Force_TeamReplenish_Levels[5] = {
	//Ufo: fixed forcepower of level 5
	{256,		50,	33, 25, 2000, 50},
	{256 * 1.5,	50,	33, 25, 2000, 33},
	{256 * 2,	50,	33, 25, 2000, 25},
	{256 * 3,	50,	33, 25, 2000, 25},
	{256 * 3,	100,	66, 50, 2000, 50},
};

forcePower_t Force_TeamReplenish = {
	NULL,
	Force_TeamReplenish_Start,
	NULL,
	NULL,
	qfalse,
	FORCELEVELDATA(Force_TeamReplenish_Levels, forceTeamHeal_t),
	256,
};


qboolean Force_Drain_Available(gentity_t *self, const void *vData) {
	if(self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;
	if(self->client->ps.weaponTime > 0)
		return qfalse;
	if(self->client->ps.fd.forcePower < 25)
		return qfalse;
	return qtrue;
}

qboolean Force_Drain_Start(gentity_t *self, const void *vData) {

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/drain.wav") );
	self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
	return qtrue;
}

float Force_Drain_AbsorbPower(gentity_t *self, gentity_t *targ, const forceDrain_t *data) {
	int powerLevel;
	powerLevel = self->client->ps.fd.forcePowerLevel[FP_DRAIN];
	int modPowerLevel = WP_AbsorbConversion(targ, targ->client->ps.fd.forcePowerLevel[FP_ABSORB], self, 
		FP_DRAIN, powerLevel, 1); //Ufo: was FP_LIGHTNING
	if (modPowerLevel == -1)
		modPowerLevel = powerLevel;
	return ((float)modPowerLevel) / ((float)powerLevel);
}

void Force_Drain_Damage( gentity_t *self, gentity_t *target, vec3_t dir, vec3_t impactPoint, const forceDrain_t *data){

	if ( target->client && (!OnSameTeam(self, target) || g_friendlyFire.integer) &&
		self->client->ps.fd.forceDrainTime < level.time && target->client->ps.fd.forcePower )
	{
			//an enemy or object
		if (!target->client && target->s.eType == ET_NPC){
			//g2animent
			if (target->s.genericenemyindex < level.time)
				target->s.genericenemyindex = level.time + 2000;
		}
		if (ForcePowerUsableOn(self, target, FP_DRAIN)){
			int dmg = data->damage;

			if (target->client){
				dmg *= Force_Drain_AbsorbPower(self, target, data);
				//Lugormod Needed?
				BotForceNotification (target->client, self);
			}

			if (dmg)
				//Ufo: drain minimum if ionlysaber
				target->client->ps.fd.forcePower -= (self->client->pers.Lmd.persistantFlags & SPF_IONLYSABER) ? 1 : dmg;
			if (target->client->ps.fd.forcePower < 0)
				target->client->ps.fd.forcePower = 0;

			if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH]){
				self->health += dmg;
				if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					self->health = self->client->ps.stats[STAT_MAX_HEALTH];
				self->client->ps.stats[STAT_HEALTH] = self->health;
			}

			target->client->ps.fd.forcePowerRegenDebounceTime = level.time + data->regenstun; //don't let the client being drained get force power back right away

			if (target->client->forcePowerSoundDebounce < level.time){
				gentity_t *tent = G_TempEntity( impactPoint, EV_FORCE_DRAINED);
				tent->s.eventParm = DirToByte(dir);
				tent->s.owner = target->s.number;

				target->client->forcePowerSoundDebounce = level.time + 400;
			}
		}
	}
}

qboolean Force_Drain_Run(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceDrain_t);
	if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		return qfalse;

	if ( data->duration )
	{//higher than level 1
		if ( (self->client->pers.cmd.buttons & BUTTON_FORCE_DRAIN) ||
			((self->client->pers.cmd.buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_DRAIN) )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
		}
	}
	if (self->client->ps.fd.forcePower < 25)
		return qfalse;

	trace_t	tr;
	vec3_t	end, forward;
	gentity_t *traceEnt;
	qboolean gotOneOrMore = qfalse;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( data->arc ){
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = center[i] - data->range;
			maxs[i] = center[i] + data->range;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		for ( e = 0 ; e < numListedEntities ; e++ ) {
			traceEnt = &g_entities[iEntityList[e]];

			if ( traceEnt == self )
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
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( center[i] < traceEnt->r.absmin[i] ) 
					v[i] = traceEnt->r.absmin[i] - center[i];
				else if ( center[i] > traceEnt->r.absmax[i] ) 
					v[i] = center[i] - traceEnt->r.absmax[i];
				else 
					v[i] = 0;
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if (DotProduct( dir, forward ) < 0.5 )
				continue;

			//must be close enough
			if ( VectorLength( v ) >= data->range ) 
				continue;

			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
				continue;

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
				continue;

			// ok, we are within the radius, add us to the incoming list
			Force_Drain_Damage( self, traceEnt, dir, ent_org, data);
			gotOneOrMore = qtrue;
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );

		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid ||
			!g_entities[tr.entityNum].client || !g_entities[tr.entityNum].inuse )
		{
			//Lugormod Shouldn't be able to just go on and on ..
			BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, data->forcepower );
			return 0;
		}

		traceEnt = &g_entities[tr.entityNum];
		Force_Drain_Damage( self, traceEnt, forward, tr.endpos, data);
		gotOneOrMore = qtrue;
	}

	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN] + FORCE_LEVEL_3;

	BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, data->forcepower );

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + data->regenstun;
	return qtrue;
}

void Force_Drain_Stop(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceDrain_t);
	self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + data->debounce;

	if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		self->client->ps.forceHandExtendTime = 0; //reset hand position

	self->client->ps.activeForcePass = 0;
}

forceDrain_t Force_Drain_Levels[5] = {
	//Ufo: drainlocking no longer possible
	{2, 800, MAX_DRAIN_DISTANCE, 0, 0,		3000, 5},
	{3, 800, MAX_DRAIN_DISTANCE, 0, Q3_INFINITE,	3000, 5},
	{4, 800, MAX_DRAIN_DISTANCE, 1, Q3_INFINITE,	1500, 5},
	{6, 800, MAX_DRAIN_DISTANCE, 1, Q3_INFINITE,	1500, 7},
	{9, 800, MAX_DRAIN_DISTANCE, 1, Q3_INFINITE,	1500, 10},
};

forcePower_t Force_Drain = {
	Force_Drain_Available,
	Force_Drain_Start,
	Force_Drain_Run,
	Force_Drain_Stop,
	qtrue,
	FORCELEVELDATA(Force_Drain_Levels, forceDrain_t),
	256,
};
#endif