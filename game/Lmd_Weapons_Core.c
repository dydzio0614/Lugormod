

//FIXME: some projectiles can be reflected, others cannot.  Currently, I have all projectiles being reflectable,
//and thats No Good.

#ifdef LMD_NEW_WEAPONS

#include "g_local.h"

#include "Lmd_Weapons_Core.h"
#include "Lmd_Accounts_Stats.h"
#include "Lmd_Commands_Public.h"


void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void WP_TraceSetStart( gentity_t *ent, vec3_t start, vec3_t mins, vec3_t maxs );
int G_GetHitLocation(gentity_t *target, vec3_t ppoint);
qboolean meditateProtect (gentity_t *ent);
void DismembermentTest(gentity_t *self);

void Weapon_Homing_Think( gentity_t *ent ) {
	vec3_t newdir, targetdir, up = {0,0,1}, right; 
	vec3_t org;
	float dot, dot2, dis;
	int i;

	int vel = ent->speed;

	if ( ent->genericValue1 && ent->genericValue1 < level.time ) {
		//time's up, we're done, remove us
		if ( ent->genericValue2 ){
			//explode when die
			ent->die( ent, &g_entities[ent->r.ownerNum], &g_entities[ent->r.ownerNum], 0, MOD_UNKNOWN );
		}
		else {
			//just remove when die
			G_FreeEntity( ent );
		}
		return;
	}
	if ( !ent->enemy || !ent->enemy->client || ent->enemy->health <= 0 || ent->enemy->client->ps.powerups[PW_CLOAKED] )
	{//no enemy or enemy not a client or enemy dead or enemy cloaked
		if ( !ent->genericValue1  ){
			//doesn't have its own self-kill time
			ent->nextthink = level.time + 10000;
			ent->think = G_FreeEntity;
		}
		return;
	}

	if ( (ent->spawnflags & 1) ){
		//vehicle rocket
		if ( ent->enemy->client && ent->enemy->client->NPC_class == CLASS_VEHICLE )
		{//tracking another vehicle
			if ( ent->enemy->client->ps.speed + 4000 > vel )
				vel = ent->enemy->client->ps.speed + 4000;
		}
	}

	if ( ent->enemy && ent->enemy->inuse ) {	
		float newDirMult = ent->angle ? ent->angle * 2.0f : 1.0f;
		float oldDirMult = ent->angle ? (1.0f - ent->angle) * 2.0f : 1.0f;

		VectorCopy( ent->enemy->r.currentOrigin, org );
		org[2] += (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5f;

		VectorSubtract( org, ent->r.currentOrigin, targetdir );
		VectorNormalize( targetdir );

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct( targetdir, ent->movedir );
		if ( (ent->spawnflags & 1) ){
			//vehicle rocket
			if ( ent->radius > -1.0f ){
				//can lose the lock if DotProduct drops below this number
				if ( dot < ent->radius ){
					//lost the lock!!!
					return;
				}
			}
		}


		// a dot of 1.0 means right-on-target.
		if ( dot < 0.0f ){	
			// Go in the direction opposite, start a 180.
			CrossProduct( ent->movedir, up, right );
			dot2 = DotProduct( targetdir, right );

			if ( dot2 > 0 ){	
				// Turn 45 degrees right.
				VectorMA( ent->movedir, 0.4f*newDirMult, right, newdir );
			}
			else{	
				// Turn 45 degrees left.
				VectorMA( ent->movedir, -0.4f*newDirMult, right, newdir );
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = ( ( targetdir[2] * newDirMult) + (ent->movedir[2] * oldDirMult) ) * 0.5;

			// let's also slow down a lot
			vel *= 0.5f;
		}
		else if ( dot < 0.70f ){	
			// Still a bit off, so we turn a bit softer
			VectorMA( ent->movedir, 0.5f*newDirMult, targetdir, newdir );
		}
		else
		{	
			// getting close, so turn a bit harder
			VectorMA( ent->movedir, 0.9f*newDirMult, targetdir, newdir );
		}

		// add crazy drunkenness
		for (i = 0; i < 3; i++ )
			newdir[i] += crandom() * ent->random * 0.25f;

		// decay the randomness
		ent->random *= 0.9f;

		if ( ent->enemy->client	&& ent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE ){
			//tracking a client who's on the ground, aim at the floor...?
			// Try to crash into the ground if we get close enough to do splash damage
			dis = Distance( ent->r.currentOrigin, org );

			if ( dis < 128 ){
				// the closer we get, the more we push the rocket down, heh heh.
				newdir[2] -= (1.0f - (dis / 128.0f)) * 0.6f;
			}
		}

		VectorNormalize( newdir );

		VectorScale( newdir, vel * 0.5f, ent->s.pos.trDelta );
		VectorCopy( newdir, ent->movedir );
		SnapVector( ent->s.pos.trDelta ); // save net bandwidth
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
	}

	ent->nextthink = level.time + FRAMETIME;
}

/*
=============================================================================
Stun baton
=============================================================================
*/
#if 0
extern vmCvar_t g_grapplingHook;
void WP_FireGrapplingHook( gentity_t *ent, qboolean alt_fire );
/*
#define STUN_BATON_DAMAGE			20
#define STUN_BATON_ALT_DAMAGE		20
#define STUN_BATON_RANGE			8
*/

void Weapon_Stunbaton_Fire(gentity_t *ent, weaponFire_t *data) {
	if (g_grapplingHook.integer == 2 || (g_grapplingHook.integer && (ent->client && ent->client->hook))){
		WP_FireGrapplingHook (ent, qfalse);
		return;
	}

	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	vec3_t		muzzleStun;

	if (!ent->client){
		VectorCopy(ent->r.currentOrigin, muzzleStun);
		muzzleStun[2] += 8;
	}
	else{
		VectorCopy(ent->client->ps.origin, muzzleStun);
		muzzleStun[2] += ent->client->ps.viewheight - 6;
	}

	VectorMA(muzzleStun, 20.0f, forward, muzzleStun);
	VectorMA(muzzleStun, 4.0f, vright, muzzleStun);

	VectorMA( muzzleStun, data->life, forward, end );

	VectorSet( maxs, data->size, data->size, data->size );
	VectorScale( maxs, -1, mins );

	trap_Trace ( &tr, muzzleStun, mins, maxs, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
		return;

	tr_ent = &g_entities[tr.entityNum];

	if (tr_ent && tr_ent->takedamage && tr_ent->client){
		//see if either party is involved in a duel
		//if (tr_ent->client->ps.duelInProgress &&
		if (duelInProgress(&tr_ent->client->ps) && tr_ent->client->ps.duelIndex != ent->s.number)
			return;

		if (ent->client && duelInProgress(&ent->client->ps) && ent->client->ps.duelIndex != tr_ent->s.number)
			return;
	}

	if ( tr_ent && tr_ent->takedamage )	{
		G_PlayEffect( EFFECT_STUNHIT, tr.endpos, tr.plane.normal );

		G_Sound( tr_ent, CHAN_WEAPON, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, forward, tr.endpos, data->damage, data->damage, data->damage.mod );

		if (tr_ent->client){ 
			//if it's a player then use the shock effect
			if ( tr_ent->client->NPC_class == CLASS_VEHICLE ){
				//not on vehicles
				if ( !tr_ent->m_pVehicle
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL 
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_FLIER )
				{//can zap animals
					tr_ent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
				}
			}
			else
				tr_ent->client->ps.electrifyTime = level.time + data->velocity;
		}
	}
}

void Weapon_Stunbaton_AltFire(gentity_t *ent, weaponFire_t *data) {
	WP_FireGrapplingHook(ent, qtrue);
}

weaponEntry_t Weapon_Stunbaton = {
	"stunbaton",
	{
		qfalse,
		Weapon_Stunbaton_Fire,
		{20, 0, 0, DAMAGE_NO_KNOCKBACK | DAMAGE_HALF_ABSORB, MOD_STUN_BATON, 0},
		6, 0, 700, 0, qfalse, 8, 0, 0, 0
	},
	{
		qfalse,
		Weapon_Stunbaton_AltFire,
		{20, 0, 0, 0, MOD_REPEATER, 0},
		6, 0, 700, 0, qfalse, 8, 0, 0, 0
	}
};
#endif

/*
=============================================================================
Bryar
=============================================================================
*/
#define BRYAR_CHARGE_UNIT 200.0f // bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove
void Weapon_Bryar_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {

	int count;
	float boxSize = 0;

	count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;
	ent->client->ps.weaponChargeTime = 0; //Lugormod glow bug fix

	if ( count < 1 )
		count = 1;
	else if ( count > 5 )
		count = 5;

	if (count > 1)
		missile->damage *= (count*1.7);
	else
		missile->damage *= (count*1.5);

	missile->s.generic1 = count; // The missile will then render according to the charge level.

	boxSize = data->projectile.size * (count * 0.5);

	VectorSet(missile->r.maxs, boxSize, boxSize, boxSize);
	VectorScale(missile->r.maxs, -1, missile->r.mins);
}

weaponEntry_t Weapon_Bryar = {
	"bryar",
	{
		1,
		NULL,
		{0, 0, 0, 0},
		{10, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BRYAR_PISTOL, 0},
		{0, 1600, qfalse, 10000, 8},
		0, NULL
	},
	{
		1,
		Weapon_Bryar_AltFire,
		{0, 0, 0, 0},
		{10, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BRYAR_PISTOL_ALT, 0},
		{1, 1600, qfalse, 10000, 8},
		0, NULL
	}
};

/*
=============================================================================
Blaster
=============================================================================
*/

weaponEntry_t Weapon_Blaster = {
	"blaster",
	{
		1,
		NULL,
		{0, 2, 0, 0},
		{20, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BLASTER, 0},
		{0, 2300, qfalse, 10000, 8},
		0, NULL
	},
	{
		1,
		NULL,
		{0, 3, 1.6f, 0},
		{20, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BLASTER, 0},
		{1, 2300, qfalse, 10000, 8},
		0, NULL
	}
};

/*
=============================================================================
Disruptor
=============================================================================
*/

void Weapon_Disruptor_Fire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	int			damage = data->damage.damage;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192;
	int			ignore, traces;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);

	if ( g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND)
		damage = 50;
	if (gameMode(GM_INSTDIS) || g_gametype.integer == GT_GHOST)
		ent->client->ps.ammo[AMMO_POWERCELL] = 999;

	memset(&tr, 0, sizeof(tr)); //to shut the compiler up

	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight;//By eyes

	VectorMA( start, shotRange, forward, end );

	ignore = ent->s.number;
	traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		if (d_projectileGhoul2Collision.integer)
		{
			trap_G2Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
		}
		else
		{
			trap_Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT );
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		//if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
		if (traceEnt && traceEnt->client && duelInProgress(&traceEnt->client->ps) &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}

		if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, G_GetHitLocation(traceEnt, tr.endpos) ) 
			|| (g_gametype.integer == GT_GHOST && traceEnt->client &&
			traceEnt->client->ps.stats[STAT_WEAPONS] & 
			(1 << WP_MELEE)))
		{//act like we didn't even hit him
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		else if (traceEnt && traceEnt->client && traceEnt->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (WP_SaberCanBlock(traceEnt, tr.endpos, 0, MOD_DISRUPTOR, qtrue, 0))
			{ //broadcast and stop the shot because it was blocked
				gentity_t *te = NULL;

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
				VectorCopy( muzzle, tent->s.origin2 );
				tent->s.eventParm = ent->s.number;

				te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}
				te->s.eventParm = 0;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				return;
			}
		}
		else if ( (traceEnt->flags&FL_SHIELDED) )
		{//stopped cold
			return;
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	VectorCopy( muzzle, tent->s.origin2 );
	tent->s.eventParm = ent->s.number;

	traceEnt = &g_entities[tr.entityNum];

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
			{
				ent->client->accuracy_hits++;
				//RoboPhred
				PlayerAcc_Stats_SetHits(ent, PlayerAcc_Stats_GetHits(ent) + 1);
			} 
			if (g_gametype.integer == GT_GHOST &&
				traceEnt->client) {

					traceEnt->client->ps.stats[STAT_WEAPONS] =
						(1 << WP_MELEE);
					traceEnt->client->ps.ammo[traceEnt->client->ps.weapon] = 0;
					traceEnt->client->ps.weapon = WP_MELEE;
					//updateGhost(traceEnt);
			}


			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, data->damage.dflags, data->damage.mod );

			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			if (traceEnt->client)
			{
				tent->s.weapon = 1;
			}
		}
		else 
		{
			// Hmmm, maybe don't make any marks on things that could break
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = 1;
		}
	}
}

qboolean G_CanDisruptify(gentity_t *ent)
{
	if (!ent || !ent->inuse || !ent->client || ent->s.eType != ET_NPC ||
		ent->s.NPC_class != CLASS_VEHICLE || !ent->m_pVehicle)
	{ //not vehicle
		return qtrue;
	}

	if (ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
	{ //animal is only type that can be disintigeiteigerated
		return qtrue;
	}

	//don't do it to any other veh
	return qfalse;
}

#define DISRUPTOR_CHARGE_UNIT 50.0f // distruptor charging gives us one more unit every 50ms--if you change this, you'll have to do the same in bg_pmove

void Weapon_Disruptor_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	int			damage = data->damage.damage, skip;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192.0f;
	int			i;
	int			count, maxCount = 60;
	int			traces = data->projectile.bounce;
	qboolean	fullCharge = qfalse;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);

	if (g_gametype.integer == GT_GHOST ) {
		Weapon_Disruptor_Fire(ent, missile, data);
		return;
	}


	VectorCopy( muzzle, muzzle2 ); // making a backup copy
	if (ent->client) {
		VectorCopy( ent->client->ps.origin, start );
		start[2] += ent->client->ps.viewheight;//By eyes

		count = ( level.time - ent->client->ps.weaponChargeTime ) / DISRUPTOR_CHARGE_UNIT;
		ent->client->ps.weaponChargeTime = 0; //Lugormod glow bug fix

		if ( g_gametype.integer == GT_SIEGE ||
			g_gametype.integer == GT_BATTLE_GROUND)
		{//maybe a full alt-charge should be a *bit* more dangerous in Siege mode?
			//maxCount = ceil((200.0f-(float)damage)/2.0f);//cap at 200 damage total
			maxCount = 200;//the previous line ALWAYS evaluated to 135 - was that on purpose?
		}
		else if (gameMode(GM_INSTDIS)) {
			ent->client->ps.ammo[AMMO_POWERCELL] = 999;
			count = maxCount = 9999;
		}
	}
	else {
		VectorCopy( ent->r.currentOrigin, start );
		start[2] += 24;

		count = ( 100 ) / DISRUPTOR_CHARGE_UNIT;
	}

	count *= 2;

	if ( count < 1 )
		count = 1;
	else if ( count >= maxCount ) {
		count = maxCount;
		fullCharge = qtrue;
	}

	// more powerful charges go through more things
	if ( count < 10 )
		traces = 1;
	else if ( count < 20 )
		traces = 2;

	damage += count;

	skip = ent->s.number;

	for (i = 0; i < traces; i++ ) {
		VectorMA( start, shotRange, forward, end );

		if (d_projectileGhoul2Collision.integer)
			trap_G2Trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
		else
			trap_Trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT );

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		//if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
		if (traceEnt && traceEnt->client && duelInProgress(&traceEnt->client->ps) &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if (Jedi_DodgeEvasion(traceEnt, ent, &tr, G_GetHitLocation(traceEnt, tr.endpos)))
		{
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}
		else if (traceEnt && traceEnt->client && traceEnt->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (WP_SaberCanBlock(traceEnt, tr.endpos, 0, MOD_DISRUPTOR_SNIPER, qtrue, 0))
			{ //broadcast and stop the shot because it was blocked
				gentity_t *te = NULL;

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
				VectorCopy( muzzle, tent->s.origin2 );
				tent->s.shouldtarget = fullCharge;
				tent->s.eventParm = ent->s.number;

				te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}
				te->s.eventParm = 0;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				return;
			}
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		VectorCopy( muzzle, tent->s.origin2 );
		tent->s.shouldtarget = fullCharge;
		tent->s.eventParm = ent->s.number;

		// If the beam hits a skybox, etc. it would look foolish to add impact effects
		if ( render_impact ) 
		{
			if ( traceEnt->takedamage && traceEnt->client )
			{
				tent->s.otherEntityNum = traceEnt->s.number;

				// Create a simple impact type mark
				tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				tent->s.eFlags |= EF_ALT_FIRING;

				if ( LogAccuracyHit( traceEnt, ent )) 
				{
					if (ent->client)
					{
						ent->client->accuracy_hits++;
						//RoboPhred
						PlayerAcc_Stats_SetHits(ent, PlayerAcc_Stats_GetHits(ent) + 1);
					}
				}
			} 
			else 
			{
				if ( traceEnt->r.svFlags & SVF_GLASS_BRUSH 
					|| traceEnt->takedamage 
					|| traceEnt->s.eType == ET_MOVER )
				{
					if ( traceEnt->takedamage )
					{
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, data->damage.dflags, data->damage.mod );

						tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
						tent->s.eventParm = DirToByte( tr.plane.normal );
					}
				}
				else
				{
					// Hmmm, maybe don't make any marks on things that could break
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
					tent->s.eventParm = DirToByte( tr.plane.normal );
				}
				break; // and don't try any more traces
			}

			if ( (traceEnt->flags&FL_SHIELDED) )
			{//stops us cold
				break;
			}

			if ( traceEnt->takedamage )
			{
				vec3_t preAng;
				int preHealth = traceEnt->health;
				int preLegs = 0;
				int preTorso = 0;

				if (traceEnt->client) {
					preLegs = traceEnt->client->ps.legsAnim;
					preTorso = traceEnt->client->ps.torsoAnim;
					VectorCopy(traceEnt->client->ps.viewangles, preAng);
				}

				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, data->damage.dflags, data->damage.mod);

				if (traceEnt->client && preHealth > 0 && traceEnt->health <= 0 && fullCharge &&
					G_CanDisruptify(traceEnt))
				{ //was killed by a fully charged sniper shot, so disintegrate
					VectorCopy(preAng, traceEnt->client->ps.viewangles);

					traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					VectorCopy(tr.endpos, traceEnt->client->ps.lastHitLoc);

					traceEnt->client->ps.legsAnim = preLegs;
					traceEnt->client->ps.torsoAnim = preTorso;

					traceEnt->r.contents = 0;

					VectorClear(traceEnt->client->ps.velocity);
				}

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
				tent->s.eventParm = DirToByte( tr.plane.normal );
				if (traceEnt->client)
				{
					tent->s.weapon = 1;
				}
			}
		}
		else // not rendering impact, must be a skybox or other similar thing?
		{
			break; // don't try anymore traces
		}

		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
	}
}

weaponEntry_t Weapon_Disruptor = {
	"disruptor",
	{
		0,
		Weapon_Disruptor_Fire,
		{600, 5, 0, 0},
		{30, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, 0},
		{0, 0, qfalse, 0, 0},
		0, NULL
	},
	{
		0,
		Weapon_Disruptor_AltFire,
		{1300, 6, 0, 0},
		//Ufo: knockback is bad here
		{70, 0, 0, DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER, 0},
		{0, 0, qfalse, 0, 3},
		0, NULL
	}
};

/*
=============================================================================
Bowcaster
=============================================================================
*/

#define BOWCASTER_VEL_RANGE			0.3f
#define BOWCASTER_CHARGE_UNIT		200.0f

void Weapon_Bowcaster_Fire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	int			count;
	float		vel;
	vec3_t		angs, dir;
	int i;
	int damage = data->damage.damage;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);

	if(ent->client) {
		count = ( level.time - ent->client->ps.weaponChargeTime ) / BOWCASTER_CHARGE_UNIT;
		ent->client->ps.weaponChargeTime = 0; //Lugormod glow bug fix
	}
	else
		count = 1;

	if ( count < 1 )
		count = 1;
	else if ( count > 5 )
		count = 5;

	if ( !(count & 1 ))
	{
		// if we aren't odd, knock us down a level
		count--;
	}

	//scale the damage down based on how many are about to be fired
	if (count <= 1)
		damage = 50;
	else if (count == 2)
		damage = 45;
	else if (count == 3)
		damage = 40;
	else if (count == 4)
		damage = 35;
	else
		damage = 30;

	for (i = 0; i < count; i++ ) {
		// create a range of different velocities
		vel = data->projectile.velocity * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forward, angs );

		//Ufo: X-shaped spread sounds more useful
		if (i)
		{
			angs[PITCH] += 0.3f * ((i < 3) ? data->launcher.spread : -data->launcher.spread);
			angs[YAW] += 0.3f * ((i % 2) ? data->launcher.spread : -data->launcher.spread);
		}
/*
		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * data->launcher.spread * 0.2f;
		angs[YAW]	+= ((i+0.5f) * data->launcher.spread - count * 0.5f * data->launcher.spread );
*/

		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( muzzle, dir, vel, 10000, ent, qtrue );

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;

		VectorSet( missile->r.maxs, data->projectile.size, data->projectile.size, data->projectile.size );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = data->damage.dflags;
		missile->methodOfDeath = data->damage.mod;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce
		missile->bounceCount = 0;
	}
}

weaponEntry_t Weapon_Bowcaster = {
	"bowcaster",
	{
		0,
		Weapon_Bowcaster_Fire,
		{1000, 5, 5.0f, 0},
		{50, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BOWCASTER, 0},
		{2, 1300, qfalse, 10000, 0},
		0, NULL
	},
	{
		1,
		NULL,
		{750, 5, 0, 0},
		{50, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_BOWCASTER, 0},
		{2, 1300, qfalse, 10000, 3},
		FL_BOUNCE, NULL
	}
};

/*
=============================================================================
Repeater
=============================================================================
*/

void Weapon_Repeater_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	missile->s.pos.trDelta[2] += 40.0f; //give a slight boost in the upward direction
	if ( g_gametype.integer == GT_SIEGE || g_gametype.integer == GT_BATTLE_GROUND)
		missile->splashRadius = 80;
	else
		missile->splashRadius = data->damage.splashradius;
}

weaponEntry_t Weapon_Repeater = {
	"repeater",
	{
		1,
		NULL,
		{0, 1, 1.4f, 0},
		{14, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_REPEATER, 0},
		{0, 1600, qfalse, 10000, 8},
		0, NULL
	},
	{
		1,
		Weapon_Repeater_AltFire,
		{0, 15, 0, 0},
		{60, 60, 128, DAMAGE_DEATH_KNOCKBACK, MOD_REPEATER_ALT, MOD_REPEATER_ALT_SPLASH},
		{3, 1100, qtrue, 10000, 8},
		0, NULL
	}
};

/*
=============================================================================
DEMP2
=============================================================================
*/

void dropMoneyStash(gentity_t *ent);
void Jedi_Decloak( gentity_t *self );
void Weapon_DEMP2_AltRadiusDamage( gentity_t *ent ){
	float		frac;
	float		dist, radius, fact;
	gentity_t	*gent;
	int			iEntityList[MAX_GENTITIES];
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*myOwner = NULL;
	int			numListedEntities, i, e;
	vec3_t		mins, maxs;
	vec3_t		v, dir;

	if (ent->r.ownerNum >= 0 &&	ent->r.ownerNum < MAX_GENTITIES)
		myOwner = &g_entities[ent->r.ownerNum];

	if (!myOwner || !myOwner->inuse || !myOwner->client) {
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	frac = ( level.time - ent->genericValue5 ) / 800.0f; // / 1600.0f; // synchronize with demp2 effect

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

	radius = frac * 200.0f; // 200 is max radius...the model is aprox. 100 units tall...the fx draw code mults. this by 2.

	//RoboPhred
	if(ent->genericValue1)
		fact = ent->count / 100.0f;
	else
		fact = ent->count * 0.6;

	if (fact < 1)
		fact = 1;

	radius *= fact;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = ent->r.currentOrigin[i] - radius;
		maxs[i] = ent->r.currentOrigin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

	i = 0;
	while (i < numListedEntities) {
		entityList[i] = &g_entities[iEntityList[i]];
		i++;
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		gent = entityList[ e ];

		if ( !gent || !gent->takedamage || !gent->r.contents ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( ent->r.currentOrigin[i] < gent->r.absmin[i] ) 
				v[i] = gent->r.absmin[i] - ent->r.currentOrigin[i];
			else if ( ent->r.currentOrigin[i] > gent->r.absmax[i] ) 
				v[i] = ent->r.currentOrigin[i] - gent->r.absmax[i];
			else 
				v[i] = 0;
		}

		// shape is an ellipsoid, so cut vertical distance in half`
		v[2] *= 0.5f;

		dist = VectorLength( v );

		if ( dist >= radius ) {
			// shockwave hasn't hit them yet
			continue;
		}

		if (dist + ( 16 * ent->count ) < ent->genericValue6) {
			// shockwave has already hit this thing...
			continue;
		}

		VectorCopy( gent->r.currentOrigin, v );
		VectorSubtract( v, ent->r.currentOrigin, dir);

		// push the center of mass higher than the origin so players get knocked into the air more
		dir[2] += 12;

		if (gent != myOwner){
			G_Damage( gent, myOwner, myOwner, dir, ent->r.currentOrigin, ent->damage, DAMAGE_DEATH_KNOCKBACK, ent->splashMethodOfDeath );
			if ( gent->takedamage && gent->client ){
				/*
				//RoboPhred
				if(gent->client->Lmd.moneyStash && Q_irand(1, 10) <= stashDrop){
					dropMoneyStash(gent);
				}
				//RoboPhred
				if(ent->genericValue2 > 0){
					gent->client->Lmd.customSpeed.value = (int)floor(g_speed.value / 3.0f);
					gent->client->Lmd.customSpeed.time = level.time + ent->genericValue2 + Q_irand(0, 2000);
					gent->client->ps.electrifyTime = gent->client->Lmd.customSpeed.time;
				}
				*/

				if ( gent->client->ps.electrifyTime < level.time )
				{//electrocution effect
					if (gent->s.eType == ET_NPC && gent->s.NPC_class == CLASS_VEHICLE &&
						gent->m_pVehicle && (gent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER || gent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER))
					{ //do some extra stuff to speeders/walkers
						gent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
					}
					else if ( gent->s.NPC_class != CLASS_VEHICLE 
						|| (gent->m_pVehicle && gent->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER) )
					{//don't do this to fighters
						gent->client->ps.electrifyTime = level.time + Q_irand( 300, 800 );
					}
				}
				if ( gent->client->ps.powerups[PW_CLOAKED] )
				{//disable cloak temporarily
					Jedi_Decloak( gent );
					gent->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				}
			}
		}
	}

	// store the last fraction so that next time around we can test against those things that fall between that last point and where the current shockwave edge is
	ent->genericValue6 = radius;

	if ( frac < 1.0f ) {
		// shock is still happening so continue letting it expand
		ent->nextthink = level.time + 50;
	}
	else {
		//don't just leave the entity around
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
	}
}

void Weapon_DEMP2_AltDetonate( gentity_t *ent ) {
	gentity_t *efEnt;

	G_SetOrigin( ent, ent->r.currentOrigin );
	if (!ent->pos1[0] && !ent->pos1[1] && !ent->pos1[2]) {
		//don't play effect with a 0'd out directional vector
		ent->pos1[1] = 1;
	}

#ifdef LMD_EXPERIMENTAL
	int scale = trap_Cvar_VariableIntegerValue("lmd_experimental_demp2altfirescale");
	if(scale > 0){
		ent->count = scale;
	}
#endif

	//Let's just save ourself some bandwidth and play both the effect and sphere spawn in 1 event
	efEnt = G_PlayEffect( EFFECT_EXPLOSION_DEMP2ALT, ent->r.currentOrigin, ent->pos1 );

	if(efEnt){
		//RoboPhred: I need more control, enough with the random multipliers.
		if(ent->genericValue1)
		//if(scale <= 0 && ent->genericValue1)
			//if(g_entities[ent->r.ownerNum].client && Accounts_Prof_GetProfession((&g_entities[ent->r.ownerNum])) == PROF_TECH)
			efEnt->s.weapon = ent->genericValue1;
		else
			efEnt->s.weapon = ent->count*2;
	}

	ent->genericValue5 = level.time;
	ent->genericValue6 = 0;
	ent->nextthink = level.time + 50;
	ent->think = Weapon_DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

#define DEMP2_CHARGE_UNIT			700.0f	// demp2 charging gives us one more unit every 700ms--if you change this, you'll have to do the same in bg_weapons
#define DEMP2_ALT_RANGE				4096

void Weapon_DEMP2_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	int damage;
	int count;
	float fact;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);

	missile = G_Spawn();

#if 0
	if(ent->client && PlayerAcc_Prof_GetProfession(ent) == PROF_TECH){
		damage = 4;

		//client/server hard coded max charge time of 2100.  At 1 unit per 50, that is 42 max units.
		count = ( level.time - ent->client->ps.weaponChargeTime ) / 50;

		if(count < 42.0f * (1.0f / 5.0f))
			count = 42.0f * (1.0f / 5.0f);
		else if(count > 42)
			count = 42;

		//count *= (float)(PlayerAcc_Prof_GetSkill(ent, PROF_TECH, SK_TECH_DEMP2) + 1) / 6.0f;
		fact = count / 42.0f;

		//energy = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, SPEC_TECH_CRAFTPOINTS);

		//maxFuel = TechProf_MaxCraftpoints(ent) / 4;
		//fuelFact = (float)energy / (float)maxFuel;
		//if(fuelFact > 1.0f)
		//	fuelFact = 1.0f;
		//if(fact > fuelFact)
		//	fact = fuelFact;


		//max size of 6, min of 2
		//Setting this makes count a precent of the range to use
		missile->genericValue1 = 2 + floor((fact * 4.0f));
		count = 100.0f * fact;
		//energy -= maxFuel * fact;
		//PlayerAcc_Prof_SetSpec(ent, PROF_TECH, SPEC_TECH_CRAFTPOINTS, energy);
		//ent->client->ps.fd.forcePower = energy;

		G_SetOrigin(missile, ent->r.currentOrigin);
		VectorSet(ent->pos1, 0, 1, 0);
	}
	else{
#endif

	vec3_t	start, end;
	trace_t	tr;
	damage = data->damage.damage;

	count = ( level.time - ent->client->ps.weaponChargeTime ) / DEMP2_CHARGE_UNIT;

	VectorCopy( muzzle, start );
	VectorMA( start, DEMP2_ALT_RANGE, forward, end );

	if(count > 3)
		count = 3;
	else if(count < 1)
		count = 1;

	fact = count*0.8;
	if (fact < 1)
		fact = 1;
	damage *= fact;

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT);

	G_SetOrigin(missile, tr.endpos);
	//In SP the impact actually travels as a missile based on the trace fraction, but we're
	//just going to be instant. -rww
	VectorCopy( tr.plane.normal, missile->pos1 );

#if 0
	}
#endif

#ifdef LMD_EXPERIMENTAL
	fact = trap_Cvar_VariableIntegerValue("lmd_experimental_demp2_size") / 100.0f;
	if(fact) {
		missile->genericValue1 = 2 + floor((fact * 4.0f));
		count = 100.0f * fact;
	}
#endif

	missile->count = count;

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->think = Weapon_DEMP2_AltDetonate;
	missile->nextthink = level.time;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2;
	missile->splashRadius = data->damage.splashradius; //this is limited in the Demp2_AltRadiusDamage function (I think)

	missile->r.ownerNum = ent->s.number;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

	ent->client->ps.weaponChargeTime = 0; //Lugormod glow bug fix
}

weaponEntry_t Weapon_DEMP2 = {
	"demp2",
	{
		1,
		NULL,
		{0, 8, 0, 0},
		{25, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_DEMP2, 0},
		{2, 1800, qfalse, 10000, 0},
		0, NULL
	},
	{
		0,
		Weapon_DEMP2_AltFire,
		{0, 25, 0, 0},
		{8, 8, 256, DAMAGE_DEATH_KNOCKBACK, MOD_DEMP2_ALT, 0},
		{2, 1800, qfalse, 10000, 0},
		0, NULL
	}
};

/*
=============================================================================
Flechette
=============================================================================
*/

#define FLECHETTE_SPREAD			4.0f
#define FLECHETTE_VEL				3500
#define FLECHETTE_MINE_RADIUS_CHECK	256

void Weapon_Flechette_Alt_Explode( gentity_t *self ) {
	vec3_t v;
	self->s.pos.trDelta[0] = 1;
	self->s.pos.trDelta[1] = 0;
	self->s.pos.trDelta[2] = 0;

	self->takedamage = qfalse;

	if (self->activator)
		//Ufo: fixed mod
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, self->splashMethodOfDeath );


	VectorCopy(self->s.pos.trDelta, v);
	//Explode outward from the surface

	//Does flechette ever use this?
	if (self->s.time == -2)	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}

	G_PlayEffect(EFFECT_EXPLOSION_FLECHETTE, self->r.currentOrigin, v);

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

/*Not used
void Weapon_Flechette_Alt_Die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	Weapon_Flechette_Alt_Explode(self);
}
*/

void Weapon_Flechette_Alt_Think( gentity_t *ent ) {
	int			count, i;
	qboolean	blow = qfalse;
	gentity_t *ent_list[MAX_GENTITIES];

	// if it isn't time to auto-explode, do a small proximity check
	if ( ent->delay > level.time ) {
		count = G_RadiusList( ent->r.currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list, qfalse);

		for ( i = 0; i < count; i++ ) {
			if ( ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->activator->s.number ) {
				blow = qtrue;
				break;
			}
		}
	}
	else {
		// well, we must die now
		blow = qtrue;
	}

	if ( blow ) {
		ent->think = Weapon_Flechette_Alt_Explode;
		ent->nextthink = level.time + 200;
	}
	else {
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//TODO: is this required?
void touch_NULL( gentity_t *ent, gentity_t *other, trace_t *trace );

void Weapon_Flechette_AltFireDo( vec3_t start, vec3_t fwd, gentity_t *self, weaponFire_t *data) {
	gentity_t	*missile = CreateMissile( start, fwd, data->projectile.velocity + random() * 700,
		data->projectile.life + random() * 2000, self, qtrue );

	missile->think = Weapon_Flechette_Alt_Explode;

	missile->activator = self;

	missile->s.weapon = WP_FLECHETTE;
	missile->classname = "flech_alt";
	missile->mass = 4;

	VectorSet(missile->r.maxs, data->projectile.size, data->projectile.size, data->projectile.size);
	VectorScale(missile->r.maxs, -1, missile->r.mins);
	missile->clipmask = MASK_SHOT;

	missile->touch = touch_NULL;

	missile->s.pos.trType = TR_GRAVITY;

	missile->flags = data->flags;
	missile->s.eFlags |= EF_ALT_FIRING;

	missile->bounceCount = data->projectile.bounce;

	missile->damage = data->damage.damage;
	missile->dflags = 0;
	missile->splashDamage = data->damage.splashdamage;
	missile->splashRadius = data->damage.splashradius;

	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = data->damage.mod;
	missile->splashMethodOfDeath = data->damage.splashmod;

	VectorCopy( start, missile->pos2 );
}

void Weapon_Flechette_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	vec3_t 	dir, fwd, start, angs;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);
	int i;

	vectoangles( forward, angs );
	VectorCopy( muzzle, start );

	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	for ( i = 0; i < 2; i++ ) {
		VectorCopy( angs, dir );

		dir[PITCH] -= random() * 4 + 8; // make it fly upwards
		dir[YAW] += crandom() * 2;
		AngleVectors( dir, fwd, NULL, NULL );

		Weapon_Flechette_AltFireDo( start, fwd, ent, data);
	}
}

weaponEntry_t Weapon_Flechette = {
	"flechette",
	{
		5,
		NULL,
		{700, 10, 4.0f, 0},
		{12, 0, 0, DAMAGE_DEATH_KNOCKBACK, MOD_FLECHETTE, 0},
		{1, 3500, qfalse, 10000, 5},
		FL_BOUNCE_SHRAPNEL, NULL

	},
	{
		0,
		Weapon_Flechette_AltFire,
		{800, 15, 0, 0},
		{60, 60, 128, DAMAGE_DEATH_KNOCKBACK, MOD_FLECHETTE_ALT_SPLASH, MOD_FLECHETTE_ALT_SPLASH},
		{3, 700, qfalse, 1500, 50}, 
		FL_BOUNCE_HALF, NULL
	}
};

/*
=============================================================================
Rocket
=============================================================================
*/

extern void G_ExplodeMissile( gentity_t *ent );
void Weapon_RocketLauncher_RocketDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	self->die = 0;
	self->r.contents = 0;

	G_ExplodeMissile( self );

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

void Weapon_RocketLauncher_Fire(gentity_t *ent, gentity_t *missile, weaponFire_t *data, qboolean alt) {
	int dif = 0;
	float rTime;

	if (ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE){
		float lockTimeInterval = ((g_gametype.integer==GT_SIEGE) ? 2400.0f : 1200.0f ) / 16.0f;
		rTime = ent->client->ps.rocketLockTime;

		if (rTime == -1)
			rTime = ent->client->ps.rocketLastValidTime;

		dif = ( level.time - rTime ) / lockTimeInterval;

		if (dif < 0)
			dif = 0;

		//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
		if ( dif >= 10 && rTime != -1 )	{
			missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

			if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy)){ 
				//if enemy became invalid, died, or is on the same team, then don't seek it
				missile->angle = 0.5f;
				missile->think = Weapon_Homing_Think;
				missile->nextthink = level.time + FRAMETIME;
			}
		}

		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}


	//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = Weapon_RocketLauncher_RocketDie;
	//===testing being able to shoot rockets out of the air==================================

	missile->clipmask = MASK_SHOT;

	missile->speed = data->projectile.velocity;

	//Lugormod
	if (gameMode(GM_ROCKET_ARENA) && ent->client->ps.ammo[AMMO_ROCKETS] == 0) {
		ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_ROCKET_LAUNCHER);
	}
}

void Weapon_RocketLauncher_Primary(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	Weapon_RocketLauncher_Fire(ent, missile, data, qfalse);
}

void Weapon_RocketLauncher_Alt(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	ent->client->ps.weaponChargeTime = 0;
	Weapon_RocketLauncher_Fire(ent, missile, data, qtrue);
}

weaponEntry_t Weapon_RocketLauncher = {
	"rocket",
	{
		1,
		Weapon_RocketLauncher_Primary,
		{0, 1, 0, 0},
		{100, 100, 160, DAMAGE_DEATH_KNOCKBACK, MOD_ROCKET, MOD_ROCKET_SPLASH},
		{3, 900, qfalse, 10000, 0},
		0, NULL
	},
	{
		1,
		Weapon_RocketLauncher_Alt,
		{0, 2, 0, 0},
		{100, 100, 160, DAMAGE_DEATH_KNOCKBACK, MOD_ROCKET_HOMING, MOD_ROCKET_HOMING_SPLASH},
		{3, 900 / 2, qfalse, 10000, 0},
		0, NULL
	},
};


/*
=============================================================================
Concussion
=============================================================================
*/

void Weapon_Concussion_AltFire(gentity_t *ent, gentity_t *missile, weaponFire_t *data) {
	int			skip, traces = data->projectile.bounce;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = data->projectile.velocity;
	qboolean	hitDodged = qfalse;
	vec3_t shot_mins, shot_maxs;
	int			i;
	int damage = data->damage.damage;
	vec3_t forward, vright, up;
	vec3_t muzzle;
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	CalcMuzzlePoint(ent, forward, vright, up, muzzle);

	if (g_gametype.integer == GT_GHOST) {
		ent->client->ps.ammo[AMMO_METAL_BOLTS] = 999;
	} else if (gameMode(GM_INSTGIB)) {
		damage = 9999;
		ent->client->ps.ammo[AMMO_METAL_BOLTS] = 999;
	} else {
		//Shove us backwards for half a second
		VectorMA( ent->client->ps.velocity, -200, forward, ent->client->ps.velocity );

		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
		if ( (ent->client->ps.pm_flags&PMF_DUCKED) )
		{//hunkered down
			ent->client->ps.pm_time = 100;
		}
		else
		{
			ent->client->ps.pm_time = 250;
		}
	}
	//	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION;
	//FIXME: only if on ground?  So no "rocket jump"?  Or: (see next FIXME)
	//FIXME: instead, set a forced ucmd backmove instead of this sliding

	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	skip = ent->s.number;

	//Make it a little easier to hit guys at long range
	VectorSet( shot_maxs, data->projectile.size, data->projectile.size, data->projectile.size );
	VectorSubtract(vec3_origin, shot_maxs, shot_mins);

	for ( i = 0; i < traces; i++ ){
		VectorMA( start, shotRange, forward, end );

		if (d_projectileGhoul2Collision.integer)
			trap_G2Trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
		else
			trap_Trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT );

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
			}

			if (traceEnt->ghoul2)
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
		}
		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
			render_impact = qfalse;

		if ( tr.entityNum == ent->s.number )
		{
			// should never happen, but basically we don't want to consider a hit to ourselves?
			// Get ready for an attempt to trace through another person
			VectorCopy( tr.endpos, muzzle2 );
			VectorCopy( tr.endpos, start );
			skip = tr.entityNum;
#ifdef _DEBUG
			Com_Printf( "BAD! Concussion gun shot somehow traced back and hit the owner!\n" );			
#endif
			continue;
		}


		if ( tr.fraction >= 1.0f )
			break;

		if ( traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC 
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
		if ( !hitDodged ) {
			if ( render_impact ) {
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) 
					|| !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
					|| traceEnt->s.eType == ET_MOVER )
				{
					qboolean noKnockBack;


					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) {
						//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->accuracy_hits++;
						//RoboPhred
						PlayerAcc_Stats_SetHits(ent, PlayerAcc_Stats_GetHits(ent) + 1);
					} 

					noKnockBack = (traceEnt->flags&FL_NO_KNOCKBACK);//will be set if they die, I want to know if it was on *before* they died
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH ){
						//hehe
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, 10, data->damage.dflags, MOD_CONC_ALT );
						break;
					}
					G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, data->damage.dflags, MOD_CONC_ALT );

					//do knockback and knockdown manually
					if ( traceEnt->client && g_gametype.integer != GT_GHOST && !meditateProtect(traceEnt) 
						&& !(traceEnt->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)
						&& (!duelInProgress(&traceEnt->client->ps) || traceEnt->client->ps.duelIndex == ent->s.number)) 
					{//only if we hit a client

						vec3_t pushDir;
						VectorCopy( forward, pushDir );
						if ( pushDir[2] < 0.2f )
							pushDir[2] = 0.2f;

						if ( traceEnt->health > 0 )
						{//alive
							//if ( G_HasKnockdownAnims( traceEnt ) )
							if (!noKnockBack && !traceEnt->localAnimIndex && traceEnt->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN &&
								BG_KnockDownable(&traceEnt->client->ps)) //just check for humanoids..
							{//knock-downable
								//G_Knockdown( traceEnt, ent, pushDir, 400, qtrue );
								vec3_t plPDif;
								float pStr;

								//cap it and stuff, base the strength and whether or not we can knockdown on the distance
								//from the shooter to the target
								VectorSubtract(traceEnt->client->ps.origin, ent->client->ps.origin, plPDif);
								pStr = 500.0f-VectorLength(plPDif);
								if (pStr < 150.0f)
								{
									pStr = 150.0f;
								}
								if (pStr > 200.0f)
								{
									traceEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									traceEnt->client->ps.forceHandExtendTime = level.time + 1100;
									traceEnt->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
								}
								traceEnt->client->ps.otherKiller = ent->s.number;
								traceEnt->client->ps.otherKillerTime = level.time + 5000;
								traceEnt->client->ps.otherKillerDebounceTime = level.time + 100;

								traceEnt->client->ps.velocity[0] += pushDir[0]*pStr;
								traceEnt->client->ps.velocity[1] += pushDir[1]*pStr;
								traceEnt->client->ps.velocity[2] = pStr;
							}
						} else if (gameMode(GM_INSTGIB)) {
							DismembermentTest(traceEnt);
						}

					}

					if ( traceEnt->s.eType == ET_MOVER )
					{//stop the traces on any mover
						break;
					}
				}
				else 
				{
					// we only make this mark on things that can't break or move
					//	tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
					//	tent->s.eventParm = DirToByte(tr.plane.normal);
					//	tent->s.eFlags |= EF_ALT_FIRING;

					//tent->svFlags |= SVF_BROADCAST;
					//eh? why broadcast?
					//	VectorCopy( tr.plane.normal, tent->pos1 );

					//mmm..no..don't do this more than once for no reason whatsoever.
					break; // hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle2 );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
		hitDodged = qfalse;
	}
	//just draw one beam all the way to the end
	//	tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
	//	tent->svFlags |= SVF_BROADCAST;
	//again, why broadcast?

	//	tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
	//	tent->s.eventParm = DirToByte(tr.plane.normal);
	//	tent->s.eFlags |= EF_ALT_FIRING;
	//	VectorCopy( muzzle, tent->s.origin2 );

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, muzzle, dir );

	//	shotDist = VectorNormalize( dir );

	//let's pack all this junk into a single tempent, and send it off.
	tent = G_TempEntity(tr.endpos, EV_CONC_ALT_IMPACT);
	//RoboPhred
	if(!tent)
		return;
	tent->s.eventParm = DirToByte(tr.plane.normal);
	tent->s.owner = ent->s.number;
	VectorCopy(dir, tent->s.angles);
	VectorCopy(muzzle, tent->s.origin2);
	VectorCopy(forward, tent->s.angles2);

}

weaponEntry_t Weapon_Concussion = {
	"conc",
	{
		1,
		NULL,
		{0, 40, 0, 0},
		{75, 40, 200, DAMAGE_DEATH_KNOCKBACK, MOD_CONC, 0},
		{0, 3000, qfalse, 10000, 0},
		0, NULL
	},
	{
		1,
		Weapon_Concussion_AltFire,
		{0, 50, 0, 0},
		{25, 0, 0, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC, MOD_CONC_ALT, 0},
		{1, 8192, qfalse, 0, 3},
		0, NULL
	}
};


weaponEntry_t *weaponEntries[WP_NUM_WEAPONS] = {
	NULL,//WP_NONE,
	NULL,//WP_STUN_BATON,
	NULL,//WP_MELEE,
	NULL,//WP_SABER,
	&Weapon_Bryar, //WP_BRYAR_PISTOL,
	&Weapon_Blaster,//WP_BLASTER,
	&Weapon_Disruptor,//WP_DISRUPTOR,
	&Weapon_Bowcaster,//WP_BOWCASTER,
	&Weapon_Repeater, //WP_REPEATER,
	&Weapon_DEMP2,//WP_DEMP2,
	&Weapon_Flechette,//WP_FLECHETTE,
	&Weapon_RocketLauncher,//WP_ROCKET_LAUNCHER,
	NULL,//WP_THERMAL,
	NULL,//WP_TRIP_MINE,
	NULL,//WP_DET_PACK,
	&Weapon_Concussion,//WP_CONCUSSION,
	//WP_BRYAR_OLD,
	//WP_EMPLACED_GUN,
	//WP_TURRET,
};

int Weapon_FireRate(gentity_t *ent, weapon_t weaponNum, qboolean altFire) {
	weaponEntry_t *weapon = weaponEntries[weaponNum];
	if(altFire) {
		if(!weapon || weapon->secondary.launcher.rate <= 0)
			return weaponData[weaponNum].altFireTime;
		return weapon->secondary.launcher.rate;
	}
	else {
		if(!weapon || weapon->primary.launcher.rate <= 0)
			return weaponData[weaponNum].fireTime;
		return weapon->primary.launcher.rate;
	}
}

int Weapon_FireEnergy(gentity_t *ent, weapon_t weaponNum, qboolean altFire) {
	weaponEntry_t *weapon = weaponEntries[weaponNum];
	if(altFire) {
		if(!weapon)
			return weaponData[weaponNum].altEnergyPerShot;
		return weapon->secondary.launcher.energy;
	}
	else {
		if(!weapon)
			return weaponData[weaponNum].energyPerShot;
		return weapon->primary.launcher.energy;
	}
}

void Weapon_FireProjectile(gentity_t *ent, weapon_t weaponNum, qboolean altFire, weaponFire_t *data) {
	
	weaponEntry_t *weapon = weaponEntries[weaponNum];
	vec3_t angle;
	gentity_t *missile;

	ent->client->accuracy_shots++;
	PlayerAcc_Stats_SetShots(ent, PlayerAcc_Stats_GetShots(ent) + 1);

	VectorCopy(ent->client->ps.viewangles, angle);
	angle[PITCH] += crandom() * ((data->launcher.spread + (VectorLength(ent->client->ps.velocity) * data->launcher.spreadrate)));
	angle[YAW] += crandom() * ((data->launcher.spread + (VectorLength(ent->client->ps.velocity) * data->launcher.spreadrate)));

	vec3_t muzzle, forward, right, up;
	AngleVectors(angle, forward, right, up);
	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	missile = CreateMissile( muzzle, forward, data->projectile.velocity, data->projectile.life, ent, altFire );
	missile->s.weapon = weaponNum;

	if(altFire) {
		missile->classname = va("%s_alt_proj", weapon->name);
		missile->s.eFlags |= EF_ALT_FIRING;
	}
	else
		missile->classname = va("%s_proj", weapon->name);

	missile->damage = data->damage.damage;
	missile->methodOfDeath = data->damage.mod;
	missile->splashDamage = data->damage.splashdamage;
	missile->splashRadius = data->damage.splashradius;
	missile->splashMethodOfDeath = data->damage.splashmod;
	missile->dflags = data->damage.dflags;

	missile->flags = data->flags;
	missile->bounceCount = data->projectile.bounce;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	VectorSet( missile->r.maxs, data->projectile.size, data->projectile.size, data->projectile.size );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	if(data->projectile.gravity)
		missile->s.pos.trType = TR_GRAVITY;

	if(data->fire)
		data->fire(ent, missile, data);
}

qboolean Weapon_Fire(gentity_t *ent, weapon_t weaponNum, qboolean altFire) {
	weaponEntry_t *weapon = weaponEntries[weaponNum];
	if(weapon == NULL)
		return qfalse;

	if (gameMode(GM_INSTGIB) && weaponNum == WP_CONCUSSION) {
		altFire = qtrue;
	}

	weaponFire_t *data = (altFire) ? &weapon->secondary : &weapon->primary;

	if(data->projectiles == 0)
		data->fire(ent, NULL, data);
	else {
		int i;
		for(i = 0; i < data->projectiles; i++)
			Weapon_FireProjectile(ent, weaponNum, altFire, data);
	}

	return qtrue;
}

#ifdef LMD_EXPERIMENTAL

#include "Lmd_Commands_Data.h"

void Cmd_WpSet(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if(!arg[0]) {
		Disp(ent, va("wpnm\nCur: %i", ent->client->ps.weapon));
		return;
	}
	int weapon = atoi(arg);
	if(weapon >= WP_NUM_WEAPONS) {
		Disp(ent, "inval wpnm");
		return;
	}
	trap_Argv(2, arg, sizeof(arg));
	if(!arg[0]) {
		Disp(ent, "0 pri 1 sec");
		return;
	}
	if(!weaponEntries[weapon]) {
		Disp(ent, "ndata");
		return;
	}

	weaponFire_t *data;
	if(atoi(arg) == 1)
		data = &weaponEntries[weapon]->secondary;
	else 
		data = &weaponEntries[weapon]->primary;

	char val[MAX_STRING_CHARS];

	trap_Argv(3, arg, sizeof(arg));
	trap_Argv(4, val, sizeof(arg));

	if(Q_stricmp(arg, "projectiles") == 0) {
		if(!val[0])
			Disp(ent, va("%i", data->projectiles));
		else if(data->projectiles > 0 && val > 0)
			data->projectiles = atoi(val);
		else
			Disp(ent, "Does not use projectiles, or tried to specify projectile on one that doesn't use it.");
	}
	else if(Q_stricmp(arg, "launcher") == 0) {
		Q_strncpyz(arg, val, sizeof(val));
		trap_Argv(5, val, sizeof(val));
		if(Q_stricmp(arg, "rate") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->launcher.rate));
			else
				data->launcher.rate = atoi(val);
		}
		else if(Q_stricmp(arg, "energy") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->launcher.energy));
			else
				data->launcher.energy = atoi(val);
		}
		else if(Q_stricmp(arg, "spread") == 0) {
			if(!val[0])
				Disp(ent, va("%f", data->launcher.spread));
			else
				data->launcher.spread = atof(val);
		}
		else if(Q_stricmp(arg, "spreadrate") == 0) {
			if(!val[0])
				Disp(ent, va("%f", data->launcher.spreadrate));
			else
				data->launcher.spreadrate = atof(val);
		}
		else
			Disp(ent, "rate | energy | spread | spreadrate");
	}
	else if(Q_stricmp(arg, "damage") == 0) {
		Q_strncpyz(arg, val, sizeof(val));
		trap_Argv(5, val, sizeof(arg));
		if(Q_stricmp(arg, "damage") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.damage));
			else
				data->damage.damage = atoi(val);
		}
		else if(Q_stricmp(arg, "splashdamage") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.splashdamage));
			else
				data->damage.splashdamage = atoi(val);
		}
		else if(Q_stricmp(arg, "splashradius") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.splashradius));
			else
				data->damage.splashradius = atoi(val);
		}
		else if(Q_stricmp(arg, "dflags") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.dflags));
			else
				data->damage.dflags = atoi(val);
		}
		else if(Q_stricmp(arg, "mod") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.mod));
			else
				data->damage.mod = atoi(val);
		}
		else if(Q_stricmp(arg, "splashmod") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->damage.splashmod));
			else
				data->damage.splashmod = atoi(val);
		}
		else
			Disp(ent, "damage | splashdamage | splashradius | dflags | mod | splashmod");
	}
	else if(Q_stricmp(arg, "projectile") == 0) {
		Q_strncpyz(arg, val, sizeof(val));
		trap_Argv(5, val, sizeof(arg));
		if(Q_stricmp(arg, "size") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->projectile.size));
			else
				data->projectile.size = atoi(val);
		}
		else if(Q_stricmp(arg, "velocity") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->projectile.velocity));
			else
				data->projectile.velocity = atoi(val);
		}
		else if(Q_stricmp(arg, "gravity") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->projectile.gravity));
			else
				data->projectile.gravity = (atoi(val) == 0) ? qfalse : qtrue;
		}
		else if(Q_stricmp(arg, "life") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->projectile.life));
			else
				data->projectile.life = atoi(val);
		}
		else if(Q_stricmp(arg, "bounce") == 0) {
			if(!val[0])
				Disp(ent, va("%i", data->projectile.bounce));
			else
				data->projectile.bounce = atoi(val);
		}
		else
			Disp(ent, "size | velocity | gravity | life | bounce");
	}
	else if(Q_stricmp(arg, "flags") == 0) {
		if(!val[0]) {
			Disp(ent, va("%i", data->flags));
			Disp(ent, "bounce: 1048576");
		}
		else
			data->flags = atoi(val);
	}
	else
		Disp(ent, "projectiles | launcher | damage | projectile | flags");
}

cmdEntry_t weaponDebugCommands[] = {
	{"dbg_wpset", "", Cmd_WpSet, 0, qtrue, 1, 0, 0, 0, qfalse},
	{NULL},
};

#endif

#endif