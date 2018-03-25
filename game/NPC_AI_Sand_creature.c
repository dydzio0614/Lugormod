#include "b_local.h"
#include "g_nav.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		10
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

float SCenemyDist = 0;

void Sand_creature_SetBolts( gentity_t *self )
{
	if ( self && self->client )
	{
		renderInfo_t *ri = &self->client->renderInfo;
		ri->headBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*mouth");
                
                //ri->headBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*head_eyes");
		//ri->cervicalBolt = trap_G2API_AddBolt(self->ghoul2, 0, "neck_bone" );
		//ri->chestBolt = trap_G2API_AddBolt(self->ghoul2, 0, "upper_spine");
		//ri->gutBolt = trap_G2API_AddBolt(self->ghoul2, 0, "mid_spine");
		//ri->torsoBolt = trap_G2API_AddBolt(self->ghoul2, 0, "lower_spine");
		//ri->crotchBolt = trap_G2API_AddBolt(self->ghoul2, 0, "rear_bone");
		//ri->elbowLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_arm_elbow");
		//ri->elbowRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_arm_elbow");
		//ri->handLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_hand");
		//ri->handRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_hand");
		//ri->kneeLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*hips_l_knee");
		//ri->kneeRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*hips_r_knee");
		//ri->footLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_leg_foot");
		//ri->footRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_leg_foot");
	}
}

/*
-------------------------
NPC_Sand_creature_Precache
-------------------------
*/
void NPC_Sand_creature_Precache( void )
{
	/*
	int i;
	for ( i = 1; i < 4; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/growl%d.wav", i) );
	}
	for ( i = 1; i < 3; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/snort%d.wav", i) );
	}
	*/
	G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
	//G_SoundIndex( "sound/chars/wampa/chomp.wav" );
}


/*
-------------------------
Sand_creature_Idle
-------------------------
*/
//gentity_t *G_PlayEffect(int fxID, vec3_t org, vec3_t ang);

void Sand_creature_Idle( void )
{
        //NPC->r.contents = 0;
        
        NPCInfo->localState = LSTATE_CLEAR;
        //If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
	        ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
}

qboolean Sand_creature_CheckRoar( gentity_t *self )
{
	if ( self->wait < level.time )
	{
                
		self->wait = level.time + Q_irand( 5000, 20000 );
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_ATTACK2, (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD) );
                //G_PlayEffect(G_EffectIndex("env/sand_spray"), NPC->r.currentOrigin, NPC->s.apos.trBase);
                
		TIMER_Set( self, "rageTime", self->client->ps.legsTimer );
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Sand_creature_Patrol
-------------------------
*/
void Sand_creature_Patrol( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
	else
	{
		if ( TIMER_Done( NPC, "patrolTime" ))
		{
			TIMER_Set( NPC, "patrolTime", crandom() * 5000 + 5000 );
		}
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Sand_creature_Idle();
		return;
	}
	Sand_creature_CheckRoar( NPC );
	TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
}
 
/*
-------------------------
Sand_creature_Move
-------------------------
*/

void Sand_creature_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
                NPCInfo->goalEntity = NPC->enemy;

		if ( NPC->enemy )
		{//pick correct movement speed and anim
			//run by default
                        
			ucmd.buttons &= ~BUTTON_WALKING;
			if ( !TIMER_Done( NPC, "runfar" ) 
				|| !TIMER_Done( NPC, "runclose" ) )
			{//keep running with this anim & speed for a bit
                                if ( TIMER_Done( NPC, "puff" ) )
                                {
                                        //vec3_t upDir;
                                        //VectorSet(upDir, 0,0,1);
                                        
                                        //G_PlayEffect(G_EffectIndex("env/sand_move"), NPC->r.currentOrigin, upDir);
                                        //VectorSet (NPC->s.apos.trBase, 0, 0, 1);
                                        
                                        G_AddEvent(NPC, EV_PLAY_EFFECT_ID, 
                                                   G_EffectIndex("env/sand_move"));
        
                                        TIMER_Set( NPC, "puff", Q_irand( 200, 400 ) );
                                }
                                
                        }
			else if ( !TIMER_Done( NPC, "walk" ) )
			{//keep walking for a bit
                                if ( TIMER_Done( NPC, "puff" ) )
                                {
                                        //vec3_t upDir;
                                        //VectorSet(upDir, 0,0,1);
                                        
                                        //G_PlayEffect(G_EffectIndex("env/sand_move"), NPC->r.currentOrigin, upDir);
                                        //VectorSet (NPC->s.apos.trBase, 0, 0, 1);
                                        
                                        G_AddEvent(NPC, EV_PLAY_EFFECT_ID, 
                                                   G_EffectIndex("env/sand_move"));
        
                                        TIMER_Set( NPC, "puff", Q_irand( 300, 500 ) );
                                }
                                     
				ucmd.buttons |= BUTTON_WALKING;
			}
			else if ( visible && SCenemyDist > 384 && NPCInfo->stats.runSpeed == 180 )
			{//fast run
				NPCInfo->stats.runSpeed = 300;
				TIMER_Set( NPC, "runfar", Q_irand( 2000, 4000 ) );
                                //G_PlayEffect(G_EffectIndex("env/sand_move_breach"), NPC->r.currentOrigin, NPC->s.apos.trBase); 
                                //G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/sand_creature/slither.wav" ) );
                                //G_AddEvent(NPC, EV_PLAY_EFFECT_ID, 
                                //           G_EffectIndex("env/sand_move_breach"));
                                //NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_WALK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
                                
			}
			else if ( SCenemyDist > 256 && NPCInfo->stats.runSpeed == 300 )
			{//slow run, upright
				NPCInfo->stats.runSpeed = 180;
				TIMER_Set( NPC, "runclose", Q_irand( 3000, 5000 ) );
                                TIMER_Set( NPC, "puff", Q_irand( 200, 400 ) );
			}
			else if ( SCenemyDist < 128 )
			{//walk
				NPCInfo->stats.runSpeed = 180;
				ucmd.buttons |= BUTTON_WALKING;
				TIMER_Set( NPC, "walk", Q_irand( 4000, 6000 ) );
                                TIMER_Set( NPC, "puff", Q_irand( 400, 800 ) );
                                //G_PlayEffect(G_EffectIndex("env/sand_move_breach"), NPC->r.currentOrigin, NPC->s.apos.trBase); 
                                G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/sand_creature/slither.wav" ) );
                                //G_AddEvent(NPC, EV_PLAY_EFFECT_ID, 
                                //           G_EffectIndex("env/sand_move_breach"));
			}
		}
                
		//if ( NPCInfo->stats.runSpeed == 300 )
		//{//need to use the alternate run - hunched over on all fours
		//	NPC->client->ps.eFlags2 |= EF2_USE_ALT_ANIM;
		//}
		NPC_MoveToGoal( qtrue );
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
//extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//extern void G_Knockdown( gentity_t *victim );
extern void G_Dismember( gentity_t *ent, gentity_t *enemy, vec3_t point, int limbType, float limbRollBase, float limbPitchBase, int deathAnim, qboolean postDeath );
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );


void Sand_creature_Bite( void )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 100;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPC->client->renderInfo.headBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the sand_creature ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}
		if (radiusEnt->s.NPC_class == CLASS_SAND_CREATURE) {
                        continue;
                }
                

		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
                        /*
                        NPC->enemy = radiusEnt;//make him my new best friend
                        radiusEnt->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
                        //FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
                        radiusEnt->client->ps.hasLookTarget = qtrue;
                        radiusEnt->client->ps.lookTarget = NPC->s.number;
                        NPC->activator = radiusEnt;//remember him
                        NPC->count = 1;//in my mouth
                        //wait to attack
                        TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + Q_irand(500, 2500) );
                        if ( radiusEnt->health > 0 && radiusEnt->pain )
                        {//do pain on enemy
                                radiusEnt->pain( radiusEnt, NPC, 100 );
                                //GEntity_PainFunc( radiusEnt, NPC, NPC, radiusEnt->r.currentOrigin, 0, MOD_CRUSH );
                        }
                        else if ( radiusEnt->client )
                        {
                                radiusEnt->client->ps.forceHandExtend = HANDEXTEND_NONE;
                                radiusEnt->client->ps.forceHandExtendTime = 0;
                                NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
                        }
                        */
                        G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 150, 300 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
                        /*
                        if ( radiusEnt->health <= 0 && radiusEnt->client )
                        {//killed them, chance of dismembering
                                if ( !Q_irand( 0, 1 ) )
                                        {//bite something off
                                                int hitLoc = Q_irand( G2_MODELPART_HEAD, G2_MODELPART_RLEG );
                                                if ( hitLoc == G2_MODELPART_HEAD )
                                                {
                                                        NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
                                                }
                                                else if ( hitLoc == G2_MODELPART_WAIST )
                                                {
                                                        NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATHBACKWARD2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
                                                }
                                                //radiusEnt->client->dismembered = qfalse;
                                                //FIXME: the limb should just disappear, cuz I ate it
                                                G_Dismember( radiusEnt, NPC, radiusEnt->r.currentOrigin, hitLoc, 90, 0, radiusEnt->client->ps.torsoAnim, qtrue);
                                                //G_DoDismemberment( radiusEnt, radiusEnt->r.currentOrigin, MOD_SABER, 1000, hitLoc, qtrue );
                                        }
                        }
                        */
                        G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/sand_creature/bite.mp3" ) );
                }
        }
}
/*
void Sand_creature_Surface( void )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPC->client->renderInfo.headBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the sand_creature ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}
		
		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			if ( //tryGrab 
                             //&& NPC->count != 1 //don't have one in mouth already - FIXME: any number in mouth!
                             //&& 
                                radiusEnt->client->NPC_class != CLASS_RANCOR
                                && radiusEnt->client->NPC_class != CLASS_GALAKMECH
                                && radiusEnt->client->NPC_class != CLASS_ATST
                                && radiusEnt->client->NPC_class != CLASS_GONK
                                && radiusEnt->client->NPC_class != CLASS_R2D2
                                && radiusEnt->client->NPC_class != CLASS_R5D2
                                && radiusEnt->client->NPC_class != CLASS_MARK1
                                && radiusEnt->client->NPC_class != CLASS_MARK2
                                && radiusEnt->client->NPC_class != CLASS_MOUSE
                                && radiusEnt->client->NPC_class != CLASS_PROBE
                                && radiusEnt->client->NPC_class != CLASS_SEEKER
                                && radiusEnt->client->NPC_class != CLASS_REMOTE
                                && radiusEnt->client->NPC_class != CLASS_SENTRY
                                && radiusEnt->client->NPC_class != CLASS_INTERROGATOR
                                && radiusEnt->client->NPC_class != CLASS_VEHICLE 
                                && radiusEnt->client->NPC_class != CLASS_SAND_CREATURE)
			{//grab
                                
                        //if ( NPC->count == 2 )
                        //{//have one in my mouth, remove him
                        //	TIMER_Remove( NPC, "clearGrabbed" );
                        //	Sand_creature_DropVictim( NPC );
                        //}
                                
				NPC->enemy = radiusEnt;//make him my new best friend
				radiusEnt->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
				//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
				radiusEnt->client->ps.hasLookTarget = qtrue;
				radiusEnt->client->ps.lookTarget = NPC->s.number;
				NPC->activator = radiusEnt;//remember him
				NPC->count = 1;//in my mouth
				//wait to attack
				TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + Q_irand(500, 2500) );
				if ( radiusEnt->health > 0 && radiusEnt->pain )
				{//do pain on enemy
					radiusEnt->pain( radiusEnt, NPC, 100 );
					//GEntity_PainFunc( radiusEnt, NPC, NPC, radiusEnt->r.currentOrigin, 0, MOD_CRUSH );
				}
				else if ( radiusEnt->client )
				{
					radiusEnt->client->ps.forceHandExtend = HANDEXTEND_NONE;
					radiusEnt->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
			}
			
		}
	}
}
*/
//------------------------------
void Sand_creature_Attack( float distance/*, qboolean doCharge*/ )
{
	if ( !TIMER_Exists( NPC, "attacking" ) )
	{
                        
                if ( Q_irand(0, 2) /*&& !doCharge*/ )
		{//attack1
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 750 );
		}/*
		else if ( doCharge || (distance > 270 && distance < 430 && !Q_irand(0, 1)) )
		{//leap
			vec3_t	fwd, yawAng;
			VectorSet( yawAng, 0, NPC->client->ps.viewangles[YAW], 0 );
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_WALK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 500 );
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, distance*1.5f, NPC->client->ps.velocity );
			NPC->client->ps.velocity[2] = 150;
			NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
                        }*/
		else
		{//attack2
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 250 );
		}
                
		TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + random() * 200 );
		//allow us to re-evaluate our running speed/anim
		TIMER_Set( NPC, "runfar", -1 );
		TIMER_Set( NPC, "runclose", -1 );
		TIMER_Set( NPC, "walk", -1 );
                TIMER_Set( NPC, "puff", -1 );
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{/*
		case BOTH_WALK2:
			//Sand_creature_Slash( NPC->client->renderInfo.handRBolt, qfalse );
			
                        //do second hit
			TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
                 */
		case BOTH_ATTACK1:
			//Sand_creature_Slash( NPC->client->renderInfo.handRBolt, qfalse );
                        Sand_creature_Bite();
                        
			//TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
		case BOTH_ATTACK2:
			//Sand_creature_Slash( NPC->client->renderInfo.handLBolt, qtrue );
			//Sand_creature_Surface();
                        Sand_creature_Bite();

                        break;
		}
	}
	/*else if ( TIMER_Done2( NPC, "attack_dmg2", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_ATTACK1:
			Sand_creature_Bite();
                        //Sand_creature_Slash( NPC->client->renderInfo.handLBolt, qfalse );
			break;
		case BOTH_ATTACK2:
			Sand_creature_Bite();
                        //Sand_creature_Slash( NPC->client->renderInfo.handLBolt, qfalse );
			break;
		}
	}
        */
	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );
        
	if ( NPC->client->ps.legsAnim == BOTH_ATTACK1 )//&& distance > (NPC->r.maxs[0]+MIN_DISTANCE) )
	{//okay to keep moving NO
		ucmd.buttons &= ~BUTTON_WALKING;
		Sand_creature_Move( qfalse );
	}
        
}

//----------------------------------
void Sand_creature_Combat( void )
{
	// If we cannot see our target or we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
                
		Sand_creature_Move( qtrue );
		return;
	}
	else
	{
		float		distance = SCenemyDist = Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
		qboolean	advance = (qboolean)( distance > (MIN_DISTANCE) ? qtrue : qfalse  );
		//qboolean	doCharge = qfalse;
                
		// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
		//FIXME: always seems to face off to the left or right?!!!!
		NPC_FaceEnemy( qtrue );
                
                
		if ( advance )
		{//have to get closer
                        vec3_t	yawOnlyAngles;
                        VectorSet( yawOnlyAngles, 0, NPC->r.currentAngles[YAW], 0 );
			//if ( NPC->enemy->health > 0//enemy still alive
                        //     && fabs(distance-350) <= 80 //enemy anywhere from 270 to 430 away
                        //&& InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, yawOnlyAngles, 20, 20 ) 
                        //      )//enemy generally in front
			//{//10% chance of doing charge anim
                        //if ( !Q_irand( 0, 9 ) )
			//	{//go for the charge
                        //doCharge = qtrue;
                        //	advance = qfalse;
			//	}
			//}
		}
                
		if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPC, "takingPain", qtrue ))
			{
				NPCInfo->localState = LSTATE_CLEAR;
			}
			else
			{
				Sand_creature_Move( qtrue );
			}
		}
		else
		{
			//if ( !Q_irand( 0, 20 ) )
			//{//FIXME: only do this if we just damaged them or vice-versa?
			//	if ( Sand_creature_CheckRoar( NPC ) )
			//	{
			//		return;
			//	}
			//}
			//if ( !Q_irand( 0, 1 ) )
			//{//FIXME: base on skill
                        Sand_creature_Attack( distance/*, doCharge*/ );
                        //}
		}
	}
}

/*
-------------------------
NPC_Sand_creature_Pain
-------------------------
*/
//void NPC_Sand_creature_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
/*
void NPC_Sand_creature_Pain( gentity_t *self, gentity_t *attacker, int damage ) 
{
	qboolean hitBySand_creature = qfalse;
	if ( attacker&&attacker->client&&attacker->client->NPC_class==CLASS_SAND_CREATURE )
	{
		hitBySand_creature = qtrue;
	}
	if ( attacker 
		&& attacker->inuse 
		&& attacker != self->enemy
		&& !(attacker->flags&FL_NOTARGET) )
	{
		if ( (!attacker->s.number&&!Q_irand(0,3))
			|| !self->enemy
			|| self->enemy->health == 0
			|| (self->enemy->client&&self->enemy->client->NPC_class == CLASS_SAND_CREATURE)
			|| (!Q_irand(0, 4 ) && DistanceSquared( attacker->r.currentOrigin, self->r.currentOrigin ) < DistanceSquared( self->enemy->r.currentOrigin, self->r.currentOrigin )) ) 
		{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
			//FIXME: if can't nav to my enemy, take this guy if I can nav to him
			G_SetEnemy( self, attacker );
			TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			if ( hitBySand_creature )
			{//stay mad at this Sand_creature for 2-5 secs before looking for attacker enemies
				TIMER_Set( self, "sand_creatureInfight", Q_irand( 2000, 5000 ) );
			}
		}
	}
	if ( (hitBySand_creature|| Q_irand( 0, 100 ) < damage )//hit by sand_creature, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_GESTURE1
		&& self->client->ps.legsAnim != BOTH_GESTURE2
		&& TIMER_Done( self, "takingPain" ) )
	{
		if ( !Sand_creature_CheckRoar( self ) )
		{
			if ( self->client->ps.legsAnim != BOTH_ATTACK1
				&& self->client->ps.legsAnim != BOTH_ATTACK2
				&& self->client->ps.legsAnim != BOTH_ATTACK3 )
			{//cant interrupt one of the big attack anims
				if ( self->health > 100 || hitBySand_creature )
				{
					TIMER_Remove( self, "attacking" );

					VectorCopy( self->NPC->lastPathAngles, self->s.angles );

					if ( !Q_irand( 0, 1 ) )
					{
						NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					TIMER_Set( self, "takingPain", self->client->ps.legsTimer+Q_irand(0, 500) );
					//allow us to re-evaluate our running speed/anim
					TIMER_Set( self, "runfar", -1 );
					TIMER_Set( self, "runclose", -1 );
					TIMER_Set( self, "walk", -1 );

					if ( self->NPC )
					{
						self->NPC->localState = LSTATE_WAITING;
					}
				}
			}
		}
	}
}
*/

/*
-------------------------
NPC_BSSand_creature_Default
-------------------------
*/
void NPC_BSSand_creature_Default( void )
{
	NPC->client->ps.eFlags2 &= ~EF2_USE_ALT_ANIM;
	//NORMAL ANIMS
	//	stand1 = normal stand
	//	walk1 = normal, non-angry walk
	//	walk2 = injured
	//	run1 = far away run
	//	run2 = close run
	//VICTIM ANIMS
	//	grabswipe = melee1 - sweep out and grab
	//	stand2 attack = attack4 - while holding victim, swipe at him
	//	walk3_drag = walk5 - walk with drag
	//	stand2 = hold victim
	//	stand2to1 = drop victim
        if (!TIMER_Done( NPC, "rageTime" )
            || !TIMER_Done( NPC, "attacking" )) {
                if (!(NPC->r.contents & CONTENTS_BODY)) {
                        NPC->r.contents = CONTENTS_BODY | CONTENTS_MONSTERCLIP;
                        NPC->clipmask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
                    
                        trap_LinkEntity(NPC);
                }
        } else if (NPC->r.contents & CONTENTS_BODY) {
                NPC->r.contents = CONTENTS_MONSTERCLIP;
                NPC->clipmask = (MASK_PLAYERSOLID 
                                 & ~CONTENTS_BODY) | CONTENTS_MONSTERCLIP;
                
                trap_LinkEntity(NPC);
        }               

        if ( !TIMER_Done( NPC, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		
                NPC_FaceEnemy( qtrue );
                return;
	}
	if ( NPC->enemy )
	{
                if ( !TIMER_Done(NPC,"attacking") )
		{//in middle of attack
                        //face enemy
			NPC_FaceEnemy( qtrue );
			//continue attack logic
			SCenemyDist = Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );
			Sand_creature_Attack( SCenemyDist/*, qfalse */);
			return;
		}
		else
		{
                	if ( TIMER_Done(NPC,"angrynoise") )
			{
			
				G_Sound( NPC, CHAN_VOICE, G_SoundIndex( va("sound/chars/sand_creature/voice%d.mp3", Q_irand(1, 3)) ) );
                                NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_WALK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
                                TIMER_Set( NPC, "angrynoise", Q_irand( 10000, 20000 ) );
                                //TIMER_Set( NPC, "attacking", 1000 );
                                //G_AddEvent(NPC, EV_PLAY_EFFECT_ID, 
                                //           G_EffectIndex("env/sand_move_breach"));
             
			}
			//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
			if( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_SAND_CREATURE )
			{//got mad at another Sand_creature, look for a valid enemy
				if ( TIMER_Done( NPC, "sand_creatureInfight" ) )
				{
					NPC_CheckEnemyExt( qtrue );
				}
			}
			else
			{
				if ( ValidEnemy( NPC->enemy ) == qfalse )
				{
					TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
					if ( !NPC->enemy->inuse || level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
					{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
						NPC->enemy = NULL;
						Sand_creature_Patrol();
						NPC_UpdateAngles( qtrue, qtrue );
						//just lost my enemy
						if ( (NPC->spawnflags&2) )
						{//search around me if I don't have an enemy
							NPC_BSSearchStart( NPC->waypoint, BS_SEARCH );
							NPCInfo->tempBehavior = BS_DEFAULT;
						}
						else if ( (NPC->spawnflags&1) )
						{//wander if I don't have an enemy
							NPC_BSSearchStart( NPC->waypoint, BS_WANDER );
							NPCInfo->tempBehavior = BS_DEFAULT;
						}
						return;
					}
				}
				if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
				{
					gentity_t *newEnemy, *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
					NPC->enemy = NULL;
					newEnemy = NPC_CheckEnemy( (qboolean)(NPCInfo->confusionTime < level.time), qfalse, qfalse );
					NPC->enemy = sav_enemy;
					if ( newEnemy && newEnemy != sav_enemy )
					{//picked up a new enemy!
						NPC->lastEnemy = NPC->enemy;
						G_SetEnemy( NPC, newEnemy );
						//hold this one for at least 5-15 seconds
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
					}
					else
					{//look again in 2-5 secs
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
					}
				}
			}
			Sand_creature_Combat();
			return;
		}
	}
	else 
	{
		if ( TIMER_Done(NPC,"idlenoise") )
		{
			TIMER_Set( NPC, "idlenoise", Q_irand( 2000, 4000 ) );
		}
		if ( (NPC->spawnflags&2) )
		{//search around me if I don't have an enemy
			if ( NPCInfo->homeWp == WAYPOINT_NONE )
			{//no homewap, initialize the search behavior
				NPC_BSSearchStart( WAYPOINT_NONE, BS_SEARCH );
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			ucmd.buttons |= BUTTON_WALKING;
			NPC_BSSearch();//this automatically looks for enemies
		}
		else if ( (NPC->spawnflags&1) )
		{//wander if I don't have an enemy
			if ( NPCInfo->homeWp == WAYPOINT_NONE )
			{//no homewap, initialize the wander behavior
				NPC_BSSearchStart( WAYPOINT_NONE, BS_WANDER );
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			ucmd.buttons |= BUTTON_WALKING;
			NPC_BSWander();
			if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
			{
				if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
				{
					Sand_creature_Idle();
				}
				else
				{
					Sand_creature_CheckRoar( NPC );
					TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				}
			}
		}
		else
		{
			if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
			{
				Sand_creature_Patrol();
			}
			else
			{
				Sand_creature_Idle();
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
