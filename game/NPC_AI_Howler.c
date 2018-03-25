#include "b_local.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		512
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
}


/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle( void )
{
}


/*
-------------------------
Howler_Patrol
-------------------------
*/
void Howler_Patrol( void )
{
	//vec3_t dif;

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

	//rwwFIXMEFIXME: Care about all clients, not just client 0
        /*
        VectorSubtract( g_entities[0].r.currentOrigin, NPC->r.currentOrigin, dif );

	if ( VectorLengthSquared( dif ) < 256 * 256 )
	{
		G_SetEnemy( NPC, &g_entities[0] );
	}
        
	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}
        */
}
 
/*
-------------------------
Howler_Move
-------------------------
*/
void Howler_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
                if (NPC->enemy) {        
                        ucmd.buttons |= BUTTON_WALKING;
		}
                
		NPCInfo->goalEntity = NPC->enemy;
		NPC_MoveToGoal( qtrue );
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
void Howler_TryDamage( gentity_t *enemy, int damage )
{
	vec3_t	end, dir;
	trace_t	tr;
        
	if ( !enemy )
	{
		return;
	}

	AngleVectors( NPC->client->ps.viewangles, dir, NULL, NULL );
	VectorMA( NPC->r.currentOrigin, MIN_DISTANCE, dir, end );

	// Should probably trace from the mouth, but, ah well.
	trap_Trace( &tr, NPC->r.currentOrigin, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT );

	if ( tr.entityNum != ENTITYNUM_WORLD )
	{
		G_Damage( &g_entities[tr.entityNum], NPC, NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
}

//------------------------------
void Howler_Attack( void )
{
	if ( !TIMER_Exists( NPC, "attacking" ))
	{
		// Going to do ATTACK1
		TIMER_Set( NPC, "attacking", 1700 + random() * 200 );
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		TIMER_Set( NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ))
	{
		Howler_TryDamage( NPC->enemy, 5 );
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );
}

//----------------------------------
void Howler_Combat( void )
{
	float distance;
	qboolean advance;

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS4( NPC->enemy ) || UpdateGoal( ))
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		NPC_MoveToGoal( qtrue );
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	distance	= Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	advance = (qboolean)( distance > MIN_DISTANCE ? qtrue : qfalse  );

	if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
	{
		if ( TIMER_Done2( NPC, "takingPain", qtrue ))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Howler_Move(qtrue );
		}
	}
	else
	{
		Howler_Attack();
	}
}

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
void NPC_Howler_Pain( gentity_t *self, gentity_t *attacker, int damage ) 
{
	if ( damage >= 10 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Set( self, "takingPain", 2900 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		if ( self->NPC )
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
void NPC_BSHowler_Default( void )
{
        if ( NPC->enemy )
	{
                Howler_Combat();
	}
	else 
        if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		Howler_Patrol();
	}
	else
	{
		Howler_Idle();
	}
        
	if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
        {
                gentity_t *newEnemy, *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
                NPC->enemy = NULL;
                newEnemy = NPC_CheckEnemy((qboolean) (NPCInfo->confusionTime < level.time), qfalse, qfalse );
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
        NPC_UpdateAngles( qtrue, qtrue );
}
