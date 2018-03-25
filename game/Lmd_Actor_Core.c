
#include "g_local.h"

extern vec3_t playerMins;
extern vec3_t playerMaxs;

void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName);

typedef struct actorInstance_s {
	struct {
		gentity_t *entDest;
		vec3_t vecDest;
		int radius;
	}destination;
}actorInstance_t;

//TODO: handle attributes, see NPC_ParseParms

void Actor_SetOrigin(gentity_t *actor, vec3_t origin) {
	VectorCopy(origin, actor->s.origin);
	G_SetOrigin(actor, origin);
	VectorCopy(origin, actor->client->ps.origin);
	actor->s.pos.trType = TR_INTERPOLATE;
	actor->s.pos.trTime = level.time;
}

void Actor_SetAngles(gentity_t *actor, vec3_t angles) {
	/*
	actor->s.apos.trType = TR_INTERPOLATE;
	actor->s.apos.trTime = level.time;
	VectorCopy( actor->s.angles, actor->s.apos.trBase );
	VectorClear( actor->s.apos.trDelta );
	actor->s.apos.trDuration = 0;
	*/
	SetClientViewAngle(actor, angles);
}

void Actor_SetModel(gentity_t *actor, char *model, char *skin) {
	//Make sure we don't free the model if we aren't changing the pointer which might happen if we change skin but not model).
	if(actor->model != model) {
		G_Free(actor->model);
		actor->model = G_NewString2(model);
	}
	SetupGameGhoul2Model(actor, model, skin);
}

void scaleEntity(gentity_t *scaleEnt, int scale);
void Actor_SetScale(gentity_t *actor, float scale) {
	scaleEntity(actor, scale * 100);
}

void Actor_SetAnimation_Torso(gentity_t *actor, int animID, int length) {
	G_SetAnim(actor, SETANIM_TORSO, animID, SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS, 0);
	if(length < 0)
		actor->client->ps.torsoTimer = Q3_INFINITE;
	else if(length > 0)
		actor->client->ps.torsoTimer = length;
}

void Actor_SetAnimation_Legs(gentity_t *actor, int animID, int length) {
	G_SetAnim(actor, SETANIM_LEGS, animID, SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS, 0);
	if(length < 0)
		actor->client->ps.legsTimer = Q3_INFINITE;
	else if(length > 0)
		actor->client->ps.legsTimer = length;
}

void Actor_SetAnimation_Both(gentity_t *actor, int animID, int length) {
	G_SetAnim(actor, SETANIM_BOTH, animID, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS, 0);
	if(length < 0)
		actor->client->ps.torsoTimer = actor->client->ps.legsTimer = Q3_INFINITE;
	else if(length > 0)
		actor->client->ps.torsoTimer = actor->client->ps.legsTimer = length;
}

void Actor_SetSpeed(gentity_t *actor, int speed) {
	actor->speed = speed;
}
void Actor_SetDestination(gentity_t *actor, vec3_t dest, int radius) {
	VectorCopy(dest, actor->s.origin2);
	if(radius <= 0)
		radius = 15;
	actor->radius = radius;
}

void Actor_Think(gentity_t *actor) {
	actor->client->pers.cmd.serverTime = level.time - 50;

	if(actor->speed > 0) {
		actor->client->Lmd.customSpeed.time = Q3_INFINITE;
		actor->client->Lmd.customSpeed.value = actor->speed;
	}
	else
		actor->client->Lmd.customSpeed.time = 0;

	vec3_t desiredLocation;
	VectorCopy(actor->s.origin2, desiredLocation);

	if(VectorCompare(desiredLocation, vec3_origin) == qfalse) {
		vec3_t direction;
		VectorSubtract(desiredLocation, actor->r.currentOrigin, direction);
		if(VectorLength(direction) >= actor->radius) {
			VectorNormalize(direction);
			Actor_SetAngles(actor, direction);
			VectorScale(direction, 25, direction);
			actor->client->pers.cmd.forwardmove = direction[0];
			actor->client->pers.cmd.rightmove = direction[1];
		}
	}

	ClientThink( actor->s.number, &actor->client->pers.cmd );
	actor->nextthink = level.time + FRAMETIME / 2;
}

void ClientThink( int clientNum, usercmd_t *ucmd );
gentity_t* Actor_Create(char *model, char *skin, vec3_t origin, vec3_t angles) {
	gentity_t *actor = G_Spawn();
	if(!actor)
		return NULL;

	actor->classname = "Actor";
	actor->s.eType = ET_NPC;


#if 0
	actor->NPC = (gNPC_t *)G_Alloc(sizeof(gNPC_t));
	if ( actor->NPC == NULL ) {		
		Com_Printf ( "ERROR: ACTOR G_Alloc NPC failed\n" );
		G_FreeEntity(actor);
		return NULL;
	}

	actor->NPC->tempGoal = G_Spawn();
	if ( actor->NPC->tempGoal == NULL ) {
		G_Free(actor->NPC);
		actor->NPC = NULL;
		G_FreeEntity(actor);
		return NULL;
	}

	actor->NPC->defaultBehavior = actor->NPC->behaviorState = BS_CINEMATIC;

	actor->NPC->tempGoal->classname = "NPC_goal";
	actor->NPC->tempGoal->parent = actor;
	actor->NPC->tempGoal->r.svFlags |= SVF_NOCLIENT;

	actor->NPC->combatPoint = -1;
	VectorClear( actor->NPC->lastClearOrigin );

	actor->NPC_type = "jawa";
#endif


	

	actor->client = (gclient_t *)G_Alloc(sizeof(gclient_t));
	if ( actor->client == NULL ) {
		Com_Printf ( "ERROR: ACTOR G_Alloc client failed\n" );
		G_FreeEntity(actor);
		return NULL;
	}

	//actor->client->NPC_class = CLASS_PRISONER; //CLASS_NONE;

	actor->playerState = &actor->client->ps;
	actor->client->ps.clientNum = actor->s.number;
	actor->client->ps.weapon = WP_NONE;
	actor->client->ps.pm_type = PM_FREEZE;
	/*
	actor->client->ps.pm_flags |= PMF_RESPAWNED;
	actor->client->respawnTime = level.time;
	*/

	actor->client->ps.persistant[PERS_TEAM] = actor->client->sess.sessionTeam = TEAM_FREE;

	Actor_SetOrigin(actor, origin);

	Actor_SetAngles(actor, angles);

	Actor_SetModel(actor, model, skin);

	VectorCopy(playerMins, actor->r.mins);
	VectorCopy(playerMaxs, actor->r.maxs);
	actor->client->ps.crouchheight = CROUCH_MAXS_2;
	actor->client->ps.standheight = DEFAULT_MAXS_2;
	
	/*
	actor->client->ps.pm_flags |= PMF_RESPAWNED;
	actor->client->ps.persistant[PERS_SPAWN_COUNT]++;
	
	actor->client->airOutTime = level.time + 12000;

	actor->client->pers.maxHealth = actor->client->ps.stats[STAT_MAX_HEALTH] = actor->health = 100;

	actor->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	actor->client->ps.rocketLockTime = 0;

	actor->client->ps.gravity = (int)g_gravity.value;

	actor->client->ps.weaponstate = WEAPON_IDLE;
	*/

	actor->client->renderInfo.headYawRangeLeft = 80;
	actor->client->renderInfo.headYawRangeRight = 80;
	actor->client->renderInfo.headPitchRangeUp = 45;
	actor->client->renderInfo.headPitchRangeDown = 45;
	actor->client->renderInfo.torsoYawRangeLeft = 60;
	actor->client->renderInfo.torsoYawRangeRight = 60;
	actor->client->renderInfo.torsoPitchRangeUp = 30;
	actor->client->renderInfo.torsoPitchRangeDown = 50;
	actor->client->renderInfo.lookTarget = ENTITYNUM_NONE;
	VectorCopy( actor->r.currentOrigin, actor->client->renderInfo.eyePoint );

	/*
	actor->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	actor->client->ps.pm_time = 100;

	actor->client->respawnTime = level.time;
	actor->client->inactivityTime = (int)(level.time + g_inactivity.value * 1000);
	actor->client->latched_buttons = 0;

	actor->client->ps.ping = 3 * 50; //reaction time, apparently.
	*/
	

	actor->r.contents = CONTENTS_BODY;
	actor->clipmask = MASK_NPCSOLID;

	actor->s.groundEntityNum = ENTITYNUM_NONE;

	actor->mass = 10;

	/*
	actor->waterlevel = 0;
	actor->watertype = 0;
	*/

	//With this, the cursor targets it as yellow.  Without, red.
	actor->s.owner = ENTITYNUM_NONE;
	//Actor_SetAnimation_Both(actor, BOTH_FORCEHEAL_STOP); //BOTH_STAND1);

	//Work damn you!
	//G_SetAnim( actor, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL, 0);
	//actor->client->ps.legsTimer = actor->client->ps.torsoTimer = 10000;



	//Remove this when done testing!
	/*
	NPC_SetWeapons(actor);
	actor->NPC->currentAmmo = actor->client->ps.ammo[weaponData[actor->client->ps.weapon].ammoIndex];
	ChangeWeapon( actor, actor->client->ps.weapon );
	*/

	SetClientViewAngle(actor, angles);

	//Need to do this, else we cant move
	actor->client->ps.stats[STAT_HEALTH] = actor->health = 100;

	G_SetAnim( actor, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL, 0);

	//actor->waypoint = actor->NPC->homeWp = -1;

	trap_LinkEntity(actor);

	
	//NPC_Begin(actor);
	//actor->takedamage = qfalse;
	actor->think = Actor_Think;
	Actor_Think(actor);

	return actor;
}

//WP_SaberParseParms( saberName, &NPC->client->saber[0] );
