
#if 0

#include "g_local.h"

#define TURRET_MODEL_CANNON "models/map_objects/imp_mine/turret_canon.glm"
#define TURRET_MODEL_DAMAGE "models/map_objects/imp_mine/turret_damage.md3"
#define TURRET_MODEL_LASER "models/map_objects/wedge/laser_cannon_model.glm"

#define TURRET_FLAG_TURBO 1

#define TURRET_FLAG_INACTIVE 128

void lmd_turret_SetModel_Turbo( gentity_t *self, qboolean dying ) {
	if( dying ) {
		trap_G2API_RemoveGhoul2Model( &self->ghoul2, 0 );
		G_KillG2Queue( self->s.number );
		self->s.modelGhoul2 = 0;
	}
	else {
		self->s.modelindex = G_ModelIndex( TURRET_MODEL_LASER );
		//set the new onw
		trap_G2API_InitGhoul2Model( &self->ghoul2, TURRET_MODEL_LASER, 0, 0, 0, 0, 0);

		self->s.modelGhoul2 = 1;
		self->s.g2radius = 128 * self->s.iModelScale;

		G2Tur_SetBoneAngles(self, "pitch", vec3_origin);
		self->genericValue11 = trap_G2API_AddBolt( self->ghoul2, 0, "*muzzle1" );
		self->genericValue12 = trap_G2API_AddBolt( self->ghoul2, 0, "*muzzle2" );
	}
}

void lmd_turret_SetModel_Ceiling( gentity_t *self, qboolean dying ) {
	if( dying ) {
		self->s.modelindex = G_ModelIndex( TURRET_MODEL_DAMAGE );
		self->s.modelindex2 = G_ModelIndex( TURRET_MODEL_CANNON );

		trap_G2API_RemoveGhoul2Model( &self->ghoul2, 0 );
		G_KillG2Queue( self->s.number );
		self->s.modelGhoul2 = 0;
	}
	else {
		self->s.modelindex = G_ModelIndex( TURRET_MODEL_CANNON );
		self->s.modelindex2 = G_ModelIndex( TURRET_MODEL_DAMAGE );
		//set the new one
		trap_G2API_InitGhoul2Model( &self->ghoul2, TURRET_MODEL_CANNON, 0, 0, 0, 0, 0);

		self->s.modelGhoul2 = 1;
		self->s.g2radius = 80 * self->s.iModelScale;

		G2Tur_SetBoneAngles(self, "Bone_body", vec3_origin);
		self->genericValue11 = trap_G2API_AddBolt( self->ghoul2, 0, "*flash03" );
	}
}

void lmd_turret_SetModel( gentity_t *self, qboolean dying )
{
	if(self->spawnflags & TURRET_FLAG_TURBO)
		lmd_turret_SetModel_Turbo(self, dying);
	else
		lmd_turret_SetModel_Ceiling(self, dying);
}

void lmd_turret_exit(gentity_t *ent) {
	//Not in use.
	if(!ent->activator)
		return;
}

void lmd_turret_think(gentity_t *ent) {
	if(!ent->activator->client || !ent->activator->inuse || ent->activator->client->pers.connected != CON_CONNECTED) {
		lmd_turret_exit(ent);
		return;	
	}
	if(ent->activator->health <= 0) {
		lmd_turret_exit(ent);
		return;
	}
	ent->nextthink = level.time + FRAMETIME;
}

void lmd_turret_enter(gentity_t *ent, gentity_t *player) {
	//already in use
	if(ent->activator != NULL)
		return;

	//No bots allowed.
	if(!ent->client || ent->r.svFlags & SVF_BOT)
		return;

	ent->activator = player;
	ent->think = asdf;
	ent->nextthink = level.time + FRAMETIME;
}

void lmd_turret_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	if(ent->flags & FL_INACTIVE)
		return;


}

void lmd_turret(gentity_t *ent) {

	ent->modelScale[1] = ent->modelScale[2] = ent->modelScale[0]; //modelScale[0] gets initialized with modelScale key.
	ent->s.iModelScale = ent->modelScale[0] * 100;
	if (ent->s.iModelScale) {
		if (ent->s.iModelScale > 1023)
			ent->s.iModelScale = 1023;
	}
	
	if(ent->spawnflags & TURRET_FLAG_INACTIVE)
		ent->flags & FL_INACTIVE;
		

	lmd_turret_SetModel(ent, qfalse);

	ent->use = lmd_turret_use;




	vec3_t		fwd;
	int			t;

	G_SetAngles( ent, base->s.angles );
	AngleVectors( ent->r.currentAngles, fwd, NULL, NULL );

	G_SetOrigin(ent, ent->s.origin);

	ent->s.eType = ET_GENERAL;

	// Set up our explosion effect for the ExplodeDeath code....
	G_EffectIndex( "turret/explode" );
	G_EffectIndex( "sparks/spark_exp_nosnd" );


	// this is really the pitch angle.....
	base->speed = 0;

	// respawn time defaults to 20 seconds
	if ( (base->spawnflags&SPF_TURRETG2_CANRESPAWN) && !base->count )
	{
		base->count = 20000;
	}

	G_SpawnFloat( "shotspeed", "0", &base->mass );
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 20000;
		}

		if ( !base->health )
		{
			base->health = 2000;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 32768;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 1000;// + random() * 500;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 200;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 500;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 500;
		}

		if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
		{
			if (base->spawnflags&2) {//Lugormod upside down
				VectorSet( base->r.mins,-64.0f,-64.0f,-50.0f );
				VectorSet( base->r.maxs, 64.0f, 64.0f, 25.0f );
			} else {
				//VectorSet(base->r.maxs, 64.0f, 64.0f, 30.0f);
				//VectorSet(base->r.mins,-64.0f,-64.0f,-30.0f);
				VectorSet( base->r.maxs, 64.0f, 64.0f, 50.0f );
				VectorSet( base->r.mins,-64.0f,-64.0f,-20.0f );
			}

		}
		//start in "off" anim
		TurboLaser_SetBoneAnim( base, 4, 5 );
		if ( g_gametype.integer == GT_SIEGE )
		{//FIXME: designer-specified?
			//FIXME: put on other entities, too, particularly siege objectives and bbrushes...
			base->s.eFlags2 |= EF2_BRACKET_ENTITY;
		}
	}
	else
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 1100;
		}

		if ( !base->health )
		{
			base->health = 100;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 512;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 150 + random() * 55;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 10;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 25;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 5;
		}

		if ( base->spawnflags & 2 )
		{//upside-down, invert r.mins and maxe

			//VectorSet( base->r.maxs, 10.0f, 10.0f, 28.0f );
			//VectorSet( base->r.mins, -10.0f, -10.0f, 0.0f );
			VectorSet( base->r.maxs, 10.0f, 10.0f, 40.0f );
			VectorSet( base->r.mins, -10.0f, -10.0f, 3.0f );
			//Lugormod the height is weird when upside down
		}
		else
		{
			VectorSet( base->r.maxs, 10.0f, 10.0f, 0.0f );
			//VectorSet( base->r.mins, -10.0f, -10.0f, -28.0f );
			VectorSet( base->r.mins, -10.0f, -10.0f, -40.0f );
		}
	}

	//stash health off for respawn.  NOTE: cannot use maxhealth because that might not be set if not showing the health bar
	base->genericValue6 = base->health;

	G_SpawnInt( "showhealth", "0", &t );
	if (t)
	{ //a non-0 maxhealth value will mean we want to show the health on the hud
		base->maxHealth = base->health;
		G_ScaleNetHealth(base);
		base->s.shouldtarget = qtrue;
		//base->s.owner = MAX_CLIENTS; //not owned by any client
	}

	if (base->s.iModelScale)
	{ //let's scale the bbox too...
		float fScale = base->s.iModelScale/100.0f;
		VectorScale(base->r.mins, fScale, base->r.mins);
		VectorScale(base->r.maxs, fScale, base->r.maxs);
	}

	// Precache special FX and moving sounds
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		base->genericValue13 = G_EffectIndex( "turret/turb_muzzle_flash" );
		base->genericValue14 = G_EffectIndex( "turret/turb_shot" );
		base->genericValue15 = G_EffectIndex( "turret/turb_impact" );
		//FIXME: Turbo Laser Cannon sounds!
		G_SoundIndex( "sound/vehicles/weapons/turbolaser/turn.wav" );
	}
	else
	{
		G_SoundIndex( "sound/chars/turret/startup.wav" );
		G_SoundIndex( "sound/chars/turret/shutdown.wav" );
		G_SoundIndex( "sound/chars/turret/ping.wav" );
		G_SoundIndex( "sound/chars/turret/move.wav" );
	}

	base->r.contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;
	base->r.contents |= CONTENTS_SOLID;//Lugormod

	//base->max_health = base->health;
	base->takedamage = qtrue;
	base->die  = turretG2_die;

	base->material = MAT_METAL;
	//base->r.svFlags |= SVF_NO_TELEPORT|SVF_NONNPC_ENEMY|SVF_SELF_ANIMATING;

	// Register this so that we can use it for the missile effect
	RegisterItem( BG_FindItemForWeapon( WP_BLASTER ));

	// But set us as a turret so that we can be identified as a turret
	base->s.weapon = WP_TURRET;

	trap_LinkEntity( base );
}

#endif