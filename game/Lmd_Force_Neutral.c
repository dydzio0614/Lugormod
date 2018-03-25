
#ifdef LMD_NEW_FORCEPOWERS


//Check TODOs on this file

#include "g_local.h"
#include "Lmd_Force_Core.h"

#include "Lmd_Professions.h"
#include "Lmd_Prof_Merc.h"


//for jump
#include "w_saber.h"

int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
void G_LetGoOfWall( gentity_t *ent );
void BotForceNotification (gclient_t *bot, gentity_t *attacker);


int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd ){
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy( self->client->ps.viewangles, view );
	view[0] = 0;
	AngleVectors( view, forward, right, NULL );
	if ( ucmd->forwardmove && ucmd->rightmove ){
		if ( ucmd->forwardmove > 0 )
			pushFwd = 50;
		else
			pushFwd = -50;
		if ( ucmd->rightmove > 0 )
			pushRt = 50;
		else
			pushRt = -50;
	}
	else if ( ucmd->forwardmove || ucmd->rightmove ){
		if ( ucmd->forwardmove > 0 )
			pushFwd = 100;
		else if ( ucmd->forwardmove < 0 )
			pushFwd = -100;
		else if ( ucmd->rightmove > 0 )
			pushRt = 100;
		else if ( ucmd->rightmove < 0 )
			pushRt = -100;
	}

	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);

	G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);

	if (self->client->ps.fd.forceJumpCharge < JUMP_VELOCITY+40)
		self->client->ps.fd.forceJumpCharge = JUMP_VELOCITY+400;

	if (self->client->ps.velocity[2] < -30)
		self->client->ps.velocity[2] = -30;

	VectorMA( self->client->ps.velocity, pushFwd, forward, jumpVel );
	VectorMA( self->client->ps.velocity, pushRt, right, jumpVel );
	jumpVel[2] += self->client->ps.fd.forceJumpCharge;

	if ( pushFwd > 0 && self->client->ps.fd.forceJumpCharge > 200 )
		return FJ_FORWARD;
	else if ( pushFwd < 0 && self->client->ps.fd.forceJumpCharge > 200 )
		return FJ_BACKWARD;
	else if ( pushRt > 0 && self->client->ps.fd.forceJumpCharge > 200 )
		return FJ_RIGHT;
	else if ( pushRt < 0 && self->client->ps.fd.forceJumpCharge > 200 )
		return FJ_LEFT;
	else
		return FJ_UP;
}

qboolean Force_Levitation_Available(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceLevitate_t);
	if ( self->s.groundEntityNum == ENTITYNUM_NONE && 
		(!(data->airjump) || self->client->ps.fd.forcePower < 1)) {
			return qfalse;
	}
	return qtrue;
}

qboolean Force_Levitation_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceLevitate_t);
	float forceJumpChargeInterval;
	vec3_t	jumpVel;

	self->client->fjDidJump = qtrue;

	forceJumpChargeInterval = data->strength/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	WP_GetVelocityForForceJump( self, jumpVel, &self->client->pers.cmd );

	//FIXME: sound effect
	self->client->ps.fd.forceJumpZStart = self->client->ps.origin[2];//remember this for when we land
	VectorCopy( jumpVel, self->client->ps.velocity );
	//wasn't allowing them to attack when jumping, but that was annoying
	//self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

	Force_DrainForceEnergy( self, FP_LEVITATION, 
		//Is this right?  forceJumpChargeInterval was already divided by charge and frame, yet its done again here.
		(int)(self->client->ps.fd.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*data->forcepower));
	//self->client->ps.fd.forcePowerDuration[FP_LEVITATION] = level.time + self->client->ps.weaponTime;
	self->client->ps.fd.forceJumpCharge = 0;
	self->client->ps.forceJumpFlip = qtrue;
	return qtrue;
}

qboolean Force_Levitation_Run(gentity_t *self, const void *vData) {
	if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.fd.forceJumpZStart )
		return qfalse;
	return qtrue;
}

extern float forceJumpStrength[NUM_FORCE_POWER_LEVELS + 2];

forceLevitate_t Force_Levitation_Levels[5] = {
	{forceJumpStrength[1], 10, qfalse},
	{forceJumpStrength[2], 10, qfalse},
	{forceJumpStrength[3], 10, qfalse},
	{forceJumpStrength[4], 10, qfalse},
	{forceJumpStrength[5], 10, qtrue},
};
forcePower_t Force_Levitation = {
	Force_Levitation_Available,
	Force_Levitation_Start,
	Force_Levitation_Run,
	NULL,
	qfalse,
	FORCELEVELDATA(Force_Levitation_Levels, forceLevitate_t),
	256,
};


extern int speedLoopSound;
qboolean Force_Speed_Start( gentity_t *self, const void *vData ){
	GETFORCEDATA(forceSpeed_t);
	self->client->ps.fd.forcePowerDuration[FP_SPEED] = level.time + data->duration;
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
	G_Sound( self, TRACK_CHANNEL_2, speedLoopSound );
	Force_DrainForceEnergy(self, FP_SPEED, data->forcepower);
	return qtrue;
}

void Force_Speed_Stop(gentity_t *self, const void *data) {
	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_2-50], CHAN_VOICE);
}

//WP_ForcePowerStart has force used as duration * 0.025, but this is only for instantanious speed for dodging.
#define SPEEDLEVEL(duration) {duration, 50}
forceSpeed_t Force_Speed_Levels[5] = {
	SPEEDLEVEL(10000),
	SPEEDLEVEL(15000),
	SPEEDLEVEL(20000),
	SPEEDLEVEL(25000),
	SPEEDLEVEL(35000),
};

forcePower_t Force_Speed = {
	NULL,
	Force_Speed_Start,
	NULL,
	Force_Speed_Stop,
	qfalse,
	FORCELEVELDATA(Force_Speed_Levels, forceSpeed_t),
	256,
};

qboolean G_InGetUpAnim(playerState_t *ps);
qboolean Force_Throw_Available(gentity_t *self, const void *vData) {
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && (self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN || !G_InGetUpAnim(&self->client->ps)))
		return qfalse;

	if (!g_useWhileThrowing.integer && self->client->ps.saberInFlight)
		return qfalse;

	if (self->client->ps.weaponTime > 0)
		return qfalse;

	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
		return qfalse;

	return qtrue;
}

qboolean Force_Throw_CanCounter(gentity_t *self, gentity_t *thrower, qboolean pull){
	int powerUse = 0;

	if (self->client->ps.weaponTime > 0)
		return qfalse;

	//Ufo: more painful
	if (thrower->client->ps.fd.forceRageRecoveryTime >= level.time)
		return qtrue;

	if (G_InGetUpAnim(&thrower->client->ps))
		return qtrue;

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
		return qfalse;

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		return qfalse;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
		return qfalse;

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

	if ( self->health <= 0 )
		return qfalse;

	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
		return qfalse;

	if (self->client->ps.weaponstate == WEAPON_CHARGING || self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return qfalse;
	}

	if (pull)
		powerUse = FP_PULL;
	else
		powerUse = FP_PUSH;

	if ( !Force_CanUsePower( self, powerUse ) )
		return qfalse;

	return qtrue;
}

qboolean G_EntIsDoor( int entityNum );
qboolean Force_Throw_IsThrowable (gentity_t *ent, qboolean pull){
	if ( Q_stricmp( "lightsaber", ent->classname ) == 0 )
		return qtrue;

	if (ent->spawnflags & 2 && G_EntIsDoor(ent->s.number)){
		if ( ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2 )
			return qfalse;
		return qtrue;
	}

	if ( Q_stricmp( "func_static", ent->classname ) == 0 && 
		((ent->spawnflags & 1 /*F_PUSH*/) || (ent->spawnflags & 2 /*F_PULL*/)) ) 
	{
		return qtrue;
	}

	if ( Q_stricmp( "func_button", ent->classname ) == 0 )
	{//we might push it
		if ( pull || !(ent->spawnflags & SPF_BUTTON_FPUSHABLE) )
		{//not force-pushable, never pullable
			return qfalse;
		}
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
	if ( Q_stricmp( "misc_model_breakable", ent->classname) == 0 && ent->flags & FL_DROPPED_ITEM) {
		return qtrue;
	}
	if ( Q_stricmp( "money_stash", ent->classname) == 0 ) {
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
	if(ent->s.eType == ET_MISSILE) {
		if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
		{//can't force-push/pull stuck missiles (detpacks, tripmines)
			return qfalse;
		}
		if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
		{//only thermal detonators can be pushed once stopped
			return qfalse;
		}
		return qtrue;
	}
	if ( ent->NPC ) {
		if(ent->client->NPC_class == CLASS_GALAKMECH || ent->client->NPC_class == CLASS_ATST
			|| ent->client->NPC_class == CLASS_RANCOR || ent->client->NPC_class == CLASS_SAND_CREATURE) {
				return qfalse;
		}
		return qtrue;
	}
	if(ent->client)
		return qtrue;
	return qfalse;
}

#define FORCE_PUSH_BASEPOWER 256
int Force_Throw_PushPower(gentity_t *ent, qboolean pull) {
	//FIXME: return power from force modifications.
	//Ufo: yes
	return FORCE_PUSH_BASEPOWER * ((PlayerAcc_Prof_GetProfession(ent) == PROF_MERC) ? PlayerProf_Merc_GetForceResistanceSkill(ent) : ent->client->ps.fd.forcePowerLevel[(pull)?FP_PULL:FP_PUSH]);
}

qboolean Force_Throw_BreakGrip(gentity_t *self, qboolean pull, gentity_t *gripper) {
	//FIXME: do this based on power levels.
	//Ufo: level 3 push/pull should be able to break level 5 grip
	return (self->client->ps.fd.forcePowerLevel[(pull)?FP_PULL:FP_PUSH] + 2 >= gripper->client->ps.fd.forcePowerLevel[FP_GRIP]);
}

float Force_Throw_AbsorbPower(gentity_t *self, gentity_t *targ, const forceThrow_t *data, qboolean pull) {
	int powerLevel;
	int power;
	if(pull)
		power = FP_PULL;
	else
		power = FP_PUSH;
	powerLevel = self->client->ps.fd.forcePowerLevel[power];
	int modPowerLevel = WP_AbsorbConversion(targ, targ->client->ps.fd.forcePowerLevel[FP_ABSORB], self, power, powerLevel, data->forcepower);
	if (modPowerLevel == -1)
		return 1.0f;
	//Ufo: speed combined with absorb doesn't need to be that resistant
	if (targ->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
		modPowerLevel++;
	if (modPowerLevel > powerLevel)
		return 1.0f;
	return ((float)modPowerLevel) / ((float)powerLevel);
}

extern void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace );

void Force_Throw_Client(gentity_t *self, gentity_t *target, int pushPower, qboolean pull, const forceThrow_t *data) {
	//#error Need to port over newer ForceResist merc skill to this.

	int otherPushPower = Force_Throw_PushPower(target, pull);
	qboolean canPullWeapon = qtrue;
	float dirLen = 0;
	vec3_t pushDir;

	if ( g_debugMelee.integer && target->client->ps.pm_flags & PMF_STUCK_TO_WALL) {
		//push/pull them off the wall
		otherPushPower = 0;
		G_LetGoOfWall( target );
	}

	//FIXME: make this dependant on target's power.
	if (target->client->pers.cmd.forwardmove || target->client->pers.cmd.rightmove)
		otherPushPower *= 0.75;
	//Ufo: ducking will help against pull spam
	else if (pull && target->s.eType != ET_NPC && target->client->ps.pm_flags & PMF_DUCKED && target->client->ps.groundEntityNum != ENTITYNUM_NONE)
		pushPower *= 0.25;

	//Ufo: heavy gun users are more prone
	if (target->client->ps.weapon > WP_BOWCASTER || target->client->jetPackOn)
		otherPushPower *= 0.25;
	else if (target->client->ps.weapon != WP_SABER)
		otherPushPower *= 0.75;
	//Ufo: special cases, only when target has saber as a weapon
	else if (self->client->ps.fd.forceRageRecoveryTime >= level.time || G_InGetUpAnim(&self->client->ps))
		pushPower *= 0.25;
	else if (target->client->ps.fd.forcePowersActive & (1 << FP_RAGE))
		pushPower *= 0.5;

	if (otherPushPower && Force_Throw_CanCounter(target, self, pull)) {
		if ( pull ) {
			G_Sound( target, CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
			target->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			target->client->ps.forceHandExtendTime = level.time + 400;
		}
		else {
			G_Sound( target, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
			target->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			target->client->ps.forceHandExtendTime = level.time + 1000;
		}
		target->client->ps.powerups[PW_DISINT_4] = target->client->ps.forceHandExtendTime + 200;

		if (pull)
			target->client->ps.powerups[PW_PULL] = target->client->ps.powerups[PW_DISINT_4];
		else
			target->client->ps.powerups[PW_PULL] = 0;

		//Make a counter-throw effect

		if (otherPushPower >= pushPower) {
			pushPower = 0;
			canPullWeapon = qfalse;
		}
		else {
			int powerDif = (pushPower - otherPushPower);

			//FIXME: better values
			//try to just directly subtract others power from ours?
			//Makes more sense, but would make push/pull less useful.
			if (powerDif >= FORCE_PUSH_BASEPOWER * 3)
				pushPower -= (int)(pushPower*0.2);
			else if (powerDif >= FORCE_PUSH_BASEPOWER * 2)
				pushPower -= (int)(pushPower*0.4);
			else if (powerDif >= FORCE_PUSH_BASEPOWER)
				pushPower -= (int)(pushPower*0.8);

			if (pushPower < 0)
				pushPower = 0;
		}
	}

	//shove them
	if ( pull ) {
		VectorSubtract(self->client->ps.origin, target->client->ps.origin, pushDir);
		if (VectorLength(pushDir) <= 256) {
			int randfact = 0;
			int skill = 0;
	
			//FIXME: find a better way, this method still uses level based values.
			if(pushPower >= FORCE_PUSH_BASEPOWER * 5)
				randfact = 15;
			else if(pushPower >= FORCE_PUSH_BASEPOWER * 4)
				randfact = 13;
			else if(pushPower >= FORCE_PUSH_BASEPOWER * 3)
				randfact = 10;
			else if(pushPower >= FORCE_PUSH_BASEPOWER * 2)
				randfact = 7;
			else if(pushPower >= FORCE_PUSH_BASEPOWER * 1)
				randfact = 3;
	
			if ((skill = PlayerProf_Merc_GetForceResistanceSkill(target))) {
				randfact -= skill * 3;
				if (randfact < 0)
					randfact = 0;
			}
	
			if (canPullWeapon && Q_irand(1, 10) <= randfact && !OnSameTeam(self, target)) {
				vec3_t uorg, vecnorm;
	
				VectorCopy(self->client->ps.origin, uorg);
				uorg[2] += 64;
	
				VectorSubtract(uorg, self->client->ps.origin, vecnorm);
				VectorNormalize(vecnorm);
	
				TossClientWeapon(target, vecnorm, 500);
			}
		}
	}
	else
		VectorSubtract( target->client->ps.origin, self->client->ps.origin, pushDir );

	if ((pushPower > otherPushPower || target->client->ps.m_iVehicleNum) && target->client){
		//Lugormod
		BotForceNotification (target->client, self);

		if (data->knockdown && target->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN){
			dirLen = VectorLength(pushDir);


			if (BG_KnockDownable(&target->client->ps) &&
				//hack to get it power based.  Roughly equal to the old level based way.
				dirLen <= (int)ceil(256.0 * (1.0 - (((float)otherPushPower) / ((float)pushPower)))))
			{ //can only do a knockdown if fairly close
				target->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
				target->client->ps.forceHandExtendTime = level.time + 700;
				target->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
				target->client->ps.quickerGetup = qtrue;
			}
			else if (target->s.number < MAX_CLIENTS && target->client->ps.m_iVehicleNum &&
				dirLen <= 128.0f )
			{ //a player on a vehicle
				gentity_t *vehEnt = &g_entities[target->client->ps.m_iVehicleNum];
				if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle){
					if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
						vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
					{ //push the guy off
						vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)target, qfalse);
					}
				}
			}
		}
	}

	if (!dirLen)
		dirLen = VectorLength(pushDir);

	VectorNormalize(pushDir);

	//escape a force grip if we're in one
	if (self->client->ps.fd.forceGripBeingGripped > level.time){ 
		if (target->client->ps.fd.forceGripEntityNum == self->s.number){
			if (Force_Throw_BreakGrip(self, pull, target)){
				Force_StopPower(target, FP_GRIP);
				self->client->ps.fd.forceGripBeingGripped = 0;
				target->client->ps.fd.forceGripUseTime = level.time + 2500; //Ufo: was 1000 //since we just broke out of it..
			}
		}
	}

	target->client->ps.otherKiller = self->s.number;
	target->client->ps.otherKillerTime = level.time + 5000;
	target->client->ps.otherKillerDebounceTime = level.time + 100;

	pushPower -= (int)(dirLen*0.7);
	if (pushPower < 16)
		pushPower = 16;

	//fullbody push effect
	target->client->pushEffectTime = level.time + 600;

	target->client->ps.velocity[0] = pushDir[0] * pushPower;
	target->client->ps.velocity[1] = pushDir[1] * pushPower;

	if ((int)target->client->ps.velocity[2] == 0){ 
		//if not going anywhere vertically, boost them up a bit
		target->client->ps.velocity[2] = pushDir[2] * pushPower;

		if (target->client->ps.velocity[2] < 128)
			target->client->ps.velocity[2] = 128;
	}
	else
		target->client->ps.velocity[2] = pushDir[2] * pushPower;
}

void Force_Throw_Entity(gentity_t *self, vec3_t forward, gentity_t *target, unsigned int pushPower, qboolean pull, const forceThrow_t *data) {
	if ( target->s.eType == ET_MISSILE && target->s.pos.trType != TR_STATIONARY &&
		(target->s.pos.trType != TR_INTERPOLATE || target->s.weapon != WP_THERMAL) )
		//rolling and stationary thermal detonators are dealt with below
	{
		if(!pull)
			G_ReflectMissile( self, target, forward );
	}
	else if ( !Q_stricmp( "func_static", target->classname)) {
		if ( !pull && (target->spawnflags & 1/*F_PUSH*/) )
			GlobalUse( target, self, self );
		else if ( pull && (target->spawnflags & 2/*F_PULL*/))
			GlobalUse( target, self, self );
	}
	//RoboPhred
	else if(target->spawnflags & 2 && G_EntIsDoor(target->s.number)) {
		vec3_t	pos1, pos2;
		vec3_t	trFrom;
		vec3_t size, end, center;
		trace_t tr;

		VectorCopy(self->client->ps.origin, trFrom);
		trFrom[2] += self->client->ps.viewheight;

		AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
		VectorNormalize( forward );
		VectorMA( trFrom, data->radius, forward, end );
		trap_Trace( &tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum != target->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
			return;

		if ( VectorCompare( vec3_origin, target->s.origin ) ){
			//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
			VectorSubtract( target->r.absmax, target->r.absmin, size );
			VectorMA( target->r.absmin, 0.5, size, center );
			if ( (target->spawnflags&1) && target->moverState == MOVER_POS1 ){
				//if at pos1 and started open, make sure we get the center where it *started* because we're
				//going to add back in the relative values pos1 and pos2
				VectorSubtract( center, target->pos1, center );
			}
			else if ( !(target->spawnflags&1) && target->moverState == MOVER_POS2 ){
				//if at pos2, make sure we get the center where it *started* because we're going to add back
				//in the relative values pos1 and pos2
				VectorSubtract( center, target->pos2, center );
			}
			VectorAdd( center, target->pos1, pos1 );
			VectorAdd( center, target->pos2, pos2 );
		}
		else{
			//actually has an origin, pos1 and pos2 are absolute
			VectorCopy( target->r.currentOrigin, center );
			VectorCopy( target->pos1, pos1 );
			VectorCopy( target->pos2, pos2 );
		}

		if ( Distance( pos1, trFrom ) < Distance( pos2, trFrom ) ){
			//pos1 is closer
			if ( target->moverState == MOVER_POS1 ){
				//at the closest pos
				if ( pull ){
					//trying to pull, but already at closest point, so screw it
					return;
				}
			}
			else if ( target->moverState == MOVER_POS2 ){
				//at farthest pos
				if ( !pull ){
					//trying to push, but already at farthest point, so screw it
					return;
				}
			}
		}
		else{
			//pos2 is closer
			if ( target->moverState == MOVER_POS1 ){
				//at the farthest pos
				if ( !pull ){
					//trying to push, but already at farthest point, so screw it
					return;
				}
			}
			else if ( target->moverState == MOVER_POS2 ){
				//at closest pos
				if ( pull ){
					//trying to pull, but already at closest point, so screw it
					return;
				}
			}
		}
		GlobalUse( target, self, self );
	}
	else if ( Q_stricmp( "func_button", target->classname ) == 0 ){
		Touch_Button( target, self, NULL );
	}
	else if (Q_stricmp( "lightsaber", target->classname ) == 0){

	}
	else if (target->flags & FL_DROPPED_ITEM) { //Lugormod other stuff
		vec3_t pushDir;
		//get size
		vec3_t size;
		vec_t weight;

		VectorSubtract (target->r.maxs, target->r.mins, size);
		weight = VectorLength(size);

		pushPower *= 64 / weight;
		if (pushPower > 1536)
			pushPower = 1536;

		if (pushPower <= 1) 
			return;


		if ( pull )
			VectorSubtract( self->client->ps.origin, target->s.origin, pushDir );
		else
			VectorSubtract( target->s.origin, self->client->ps.origin, pushDir );
		
		VectorNormalize(pushDir);
		VectorScale(pushDir,
			pushPower,
			target->s.pos.trDelta);
		if (target->s.pos.trDelta[2] < pushPower / 10) 
			target->s.pos.trDelta[2] = pushPower / 10;

		target->s.pos.trType = TR_GRAVITY;
		target->s.pos.trTime = level.time;
	}
}

void Force_Throw_Start(gentity_t *self, qboolean pull, const forceThrow_t *data){
	unsigned int numEntities = 0;
	int	entityList[MAX_GENTITIES];
	gentity_t *entities[MAX_GENTITIES];
	gentity_t *ent;

	trace_t tr;

	vec3_t fwdangles, forward, right, center;
	vec3_t mins, maxs, dir, v, orig;
	vec3_t size;
	vec3_t tfrom, fwd, tto;

	int power = (pull) ? FP_PULL : FP_PUSH;

	unsigned int i, e;

	Force_DrainForceEnergy(self, power, data->forcepower);

	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame){
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;
		self->client->ps.saberLockHits += data->saberlockhits;
		return;
	}

	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( pull )	{
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE){
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			if ( (g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND) 
				&& self->client->ps.weapon == WP_SABER ) 
			{//hold less so can attack right after a pull
				self->client->ps.forceHandExtendTime = level.time + 200;
				self->client->ps.powerups[PW_DISINT_4] = level.time + 400;
			}
			else
			{
				self->client->ps.forceHandExtendTime = level.time + 400;
				self->client->ps.powerups[PW_DISINT_4] = level.time + 900;
			}
		}
		//self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else {
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE){
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps)){
			if(self->client->ps.forceDodgeAnim <= 4)
				self->client->ps.forceDodgeAnim += 8;
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = center[i] - data->radius;
		maxs[i] = center[i] + data->radius;
	}

	if (data->arc == 0) {
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0] * data->radius / 2;
		tto[1] = tfrom[1] + fwd[1] * data->radius / 2;
		tto[2] = tfrom[2] + fwd[2] * data->radius / 2;

		trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1.0 && tr.entityNum != ENTITYNUM_NONE){
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC){
				//g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
			}

			numEntities = 1;
			entities[0] = &g_entities[tr.entityNum];

			if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], power))
				return;
		}
		else
			return;
	}
	else {
		int found = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for(e = 0; e < found; e++) {
			ent = GetEnt(entityList[e]);
			
			//Ufo: was missing
			if (ent == self)
				continue;

			if (!ent->client && ent->s.eType == ET_NPC) {
				//g2animent
				if (ent->s.genericenemyindex < level.time)
					ent->s.genericenemyindex = level.time + 2000;
			}

			if (!Force_Throw_IsThrowable(ent, pull))
				continue;

			for ( i = 0 ; i < 3 ; i++ ) {
				if ( center[i] < ent->r.absmin[i] )
					v[i] = ent->r.absmin[i] - center[i];
				else if ( center[i] > ent->r.absmax[i] ) 
					v[i] = center[i] - ent->r.absmax[i];
				else
					v[i] = 0;
			}

			VectorSubtract( ent->r.absmax, ent->r.absmin, size );
			VectorMA( ent->r.absmin, 0.5, size, orig );

			VectorSubtract( orig, center, dir );
			VectorNormalize( dir );
			//FIXME: whats this hard coded value.  Related to arc?  Need to find a way to use full 360 arc.
			if (DotProduct( dir, forward ) < 0.6 )
				continue;

			if ( VectorLength( v ) >= data->radius ) 
				continue;

			//in PVS?
			if ( !ent->r.bmodel && !trap_InPVS( orig, self->client->ps.origin ) )
				continue;

			if (ent->client) {
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(ent->client->ps.origin, tto, v);
				vectoangles(v, v);

				if (!InFieldOfVision(self->client->ps.viewangles, data->arc, v))
					continue;
				if (!ForcePowerUsableOn(self, ent, power))
					continue;

				//FIXME: move this into ForcePowerUsableOn?
				if (OnSameTeam(ent, self))
					continue;

				//Ufo: speed forcepower boost
				if (DotProduct( dir, forward ) < 0.8 && ent->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
					continue;

			}

			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, orig, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number ) {
				//try from eyes too before you give up
				vec3_t eyePoint;
				VectorCopy(self->client->ps.origin, eyePoint);
				eyePoint[2] += self->client->ps.viewheight;
				trap_Trace( &tr, eyePoint, vec3_origin, vec3_origin, orig, self->s.number, MASK_SHOT );

				if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number ){
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

			entities[numEntities++] = ent;
		}
	}

	for ( i = 0; i < numEntities; i++ ){
		if (entities[i]->client) {
			int pushPower = data->power * Force_Throw_AbsorbPower(self, entities[i], data, pull);
			Force_Throw_Client(self, entities[i], pushPower, pull, data);
		}
		else{
			Force_Throw_Entity(self, forward, entities[i], data->power, pull, data);
		}
	}

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	//Ufo: using a debounce now, however we don't want it in siege and in fullforce duels
	if (g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_BATTLE_GROUND && (!self->client->ps.duelInProgress || !(self->client->Lmd.duel.duelType & DT_FULL_FORCE)))
		self->client->ps.fd.forcePowerDebounce[power] = level.time + ((pull) ? 1300 : 1500);
	if (self->client->ps.fd.forceGripBeingGripped > level.time)
		self->client->ps.fd.forceGripBeingGripped = 0;
}

qboolean Force_Push_Start(gentity_t *ent, const void *data) {
	Force_Throw_Start(ent, qfalse, (forceThrow_t *)data);
	return qfalse;
}

qboolean Force_Pull_Start(gentity_t *ent, const void *data) {
	Force_Throw_Start(ent, qtrue, (forceThrow_t *)data);
	return qfalse;
}

forceThrow_t Force_Throw_Levels[5] = {
	{256,		1024, 0,	20, qfalse, 2},
	{256 * 2,	1024, 60,	20, qfalse,	4},
	{256 * 3,	1024, 180,	20, qtrue,	6},
	{256 * 4,	1024, 180,	20, qtrue,	8},
	{256 * 5,	1024, 180,	20, qtrue,	10},
};

forcePower_t Force_Push = {
	Force_Throw_Available,
	Force_Push_Start,
	NULL,
	NULL,
	qfalse,
	FORCELEVELDATA(Force_Throw_Levels, forceThrow_t),
	256,
};

forcePower_t Force_Pull = {
	Force_Throw_Available,
	Force_Pull_Start,
	NULL,
	NULL,
	qfalse,
	FORCELEVELDATA(Force_Throw_Levels, forceThrow_t),
	256,
};



extern int seeLoopSound;
qboolean Force_See_Start(gentity_t *self, const void *vData) {
	GETFORCEDATA(forceSee_t);

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/see.wav"));
	G_Sound(self, TRACK_CHANNEL_5, seeLoopSound);

	self->client->ps.fd.forcePowerDuration[FP_SEE] = level.time + data->duration;
	Force_DrainForceEnergy(self, FP_SEE, data->forcepower);

	return qtrue;
}

void Force_See_Stop(gentity_t *self, const void *vData) {
	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_5-50], CHAN_VOICE);
}

forceSee_t Force_See_Levels[5] = {
	{10000, 20},
	{20000, 20},
	{30000, 20},
	{40000, 20},
	{60000, 20},
};

forcePower_t Force_See = {
	NULL,
	Force_See_Start,
	NULL,
	Force_See_Stop,
	qfalse,
	FORCELEVELDATA(Force_See_Levels, forceSee_t),
	256,
};

#endif
