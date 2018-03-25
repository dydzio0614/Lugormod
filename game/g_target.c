// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "Lmd_Entities_Public.h"
#include "Lmd_Accounts_Core.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if( !activator->client ) {
		return;
	}

	if( activator->client->ps.powerups[PW_REDFLAG] ) {
		Team_ReturnFlag( TEAM_RED );
	} else if( activator->client->ps.powerups[PW_BLUEFLAG] ) {
		Team_ReturnFlag( TEAM_BLUE );
	} else if( activator->client->ps.powerups[PW_NEUTRALFLAG] ) {
		Team_ReturnFlag( TEAM_FREE );
	}

	//RoboPhred
	activator->client->pushEffectTime = 0;

	memset( activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t *ent ) {
	ent->use = Use_target_remove_powerups;
}

//==========================================================

void Use_target_powerup( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if( !activator->client ) {
		return;
	}
	//RoboPhred
	if(ent->genericValue10 == PW_PULL)
	{
		activator->client->pushEffectTime = level.time + ent->wait * 1000;
	}
	else
		activator->client->ps.powerups[ent->genericValue10] = level.time + ent->wait * 1000;
}

void SP_target_powerup( gentity_t *ent ) {
	int pw;

	G_SpawnInt("powerup","1",&pw);
	if (pw < 0 || pw > PW_NUM_POWERUPS) {
		EntitySpawnError("Invalid powerup.");
		G_Free(ent);
		return;
	}
	ent->genericValue10 = pw;
	ent->use = Use_target_powerup;
	if (!ent->wait) {
		ent->wait = 30;
	}
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8) NO_RETRIGGER

NO_RETRIGGER - Keeps the delay from resetting the time if it is
activated again while it is counting down to an event.

"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
//Ufo: multithread option
void Think_Target_Delay( gentity_t *ent ) {
	if (ent->spawnflags & 2)
	{
		int* ptr = &ent->health;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (ptr[i] == 0)
				continue;
			if (ptr[i] <= level.time)
			{
				ptr[i] = 0;
				G_UseTargets2(ent, GetEnt(i), ent->target);
				continue;
			}
			if (ent->nextthink == 0 || ptr[i] < ent->nextthink)
				ent->nextthink = ptr[i];
		}
	}
	else
		G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if (ent->spawnflags & 2 && activator->s.number < MAX_CLIENTS)
	{
		int* ptr = &ent->health + activator->s.number;
		if (*ptr && ent->spawnflags & 1)
			return;
		*ptr = level.time + (ent->wait + ent->random * crandom()) * 1000;
		ent->think = Think_Target_Delay;
		if (ent->nextthink == 0 || *ptr < ent->nextthink)
			ent->nextthink = *ptr;
		return;
	}
	else if (ent->nextthink > level.time && (ent->spawnflags & 1))
	{ //Leave me alone, I am thinking.
		return;
	}
	G_ActivateBehavior(ent,BSET_USE);
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	//RoboPhred
	if(ent->spawnflags & 1)
		AddScore(activator, ent->r.currentOrigin, ent->count - activator->client->ps.persistant[PERS_SCORE]);
	else
		AddScore( activator, ent->r.currentOrigin, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
"wait"		don't fire off again if triggered within this many milliseconds ago
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/

//RoboPhred
//Ufo: Added data tags support

char* GetPasswordByIndex(const char* index)
{
	if (!index || !index[0])
		return NULL;
	gentity_t *ent = NULL;
	while(ent = IterateEnts(ent)) {
		if(Q_stricmp(ent->fullName, index) != 0)
			continue;
		if(Q_stricmp(ent->classname, "lmd_pwterminal") != 0)
			continue;
		return ent->target3;
	}
	return "";
}

char* Accounts_Custom_GetValue(Account_t *acc, char *key);
void Send_Target_Print(gentity_t *ent, int targ) {

	char buf[MAX_STRING_CHARS];
	strncpy(buf, ent->message, MAX_STRING_CHARS);
	char* ptr;
	if (ent->activator->m_pVehicle && ent->activator->m_pVehicle->m_pPilot)
		ent->activator = (gentity_t*)ent->activator->m_pVehicle->m_pPilot;
	if (ent->activator->s.number < MAX_CLIENTS)
	{
		if (ptr = strstr(buf, "\\id"))
		{
			*ptr = '\0';
			strncpy(buf, va("%s%s%s", buf, ent->activator->client->pers.netname, ptr + 3), MAX_STRING_CHARS);
		}
		if (ptr = strstr(buf, "\\h"))
		{
			*ptr = '\0';
			strncpy(buf, va("%s%d%s", buf, ent->activator->client->ps.stats[STAT_HEALTH], ptr + 2), MAX_STRING_CHARS);
		}
		if (ptr = strstr(buf, "\\a"))
		{
			*ptr = '\0';
			strncpy(buf, va("%s%d%s", buf, ent->activator->client->ps.stats[STAT_ARMOR], ptr + 2), MAX_STRING_CHARS);
		}
		if (ptr = strstr(buf, "\\cs"))
		{
			*ptr = '\0';
			strncpy(buf, va("%s%s%s", buf, Accounts_Custom_GetValue(ent->activator->client->pers.Lmd.account, ent->target2), ptr + 3), MAX_STRING_CHARS);
		}
		if (ptr = strstr(buf, "\\pw"))
		{
			*ptr = '\0';
			strncpy(buf, va("%s%s%s", buf, GetPasswordByIndex(ent->target2), ptr + 3), MAX_STRING_CHARS);
		}
	}

	if(buf[0] == '@' && buf[1] != '@') { //Ufo: fixed, was buf[1] == '@' and it didn't happen at all
		trap_SendServerCommand(targ, va("cps \"%s\"", buf));
	}
	else if(ent->spawnflags & 8) {
		trap_SendServerCommand(targ, va("print \"%s\n\"", buf));
	}
	else if(ent->spawnflags & 16) {
		trap_SendServerCommand(targ, va("chat \"%s\"", buf));
	}
/*
	else if(ent->spawnflags & 16) {
		char msg[MAX_STRING_CHARS];
		Q_strncpyz(msg, ent->message, sizeof(msg));
		char *s = msg, *c = s;
		while(c[0]) {
			if(c[0] == '\n'){
				c[0] = 0;
				trap_SendServerCommand(targ, va("chat \"%s\"", s));
				s = ++c;
			}
			c++;
		}
		if(s[0])
			trap_SendServerCommand(targ, va("chat \"%s\"", s));
	}
*/
	else
		trap_SendServerCommand(targ, va("cp \"%s\n\"", buf));
}

void Use_Target_Print_Go (gentity_t *ent){

	//RoboPhred
	int i;

	if (!ent || !ent->inuse)
	{
		Com_Printf("ERROR: Bad ent in Use_Target_Print");
		return;
	}

	if (ent->wait)
	{
		if (ent->genericValue14 >= level.time)
		{
			return;
		}
		ent->genericValue14 = level.time + ent->wait;
	}

	/*
	//RoboPhred
	if(ent->message[0] == '@' && ent->message[1] != '@')
		cmd = "cps";
	else if(ent->spawnflags & 8)
		cmd = "print";
	else if(ent->spawnflags & 16)
		cmd = "chat";
	else
		cmd = "cp";
	*/
	G_ActivateBehavior(ent, BSET_USE);

	if(ent->spawnflags & 4)
		Send_Target_Print(ent, ent->activator->s.number);
	else if(ent->spawnflags & 3){
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].pers.connected != CON_CONNECTED)
				continue;
			if(ent->spawnflags & 1 && level.clients[i].sess.sessionTeam != TEAM_RED)
				continue;
			if(ent->spawnflags & 2 && level.clients[i].sess.sessionTeam != TEAM_BLUE)
				continue;
			Send_Target_Print(ent, i);
		}
	}
	else
		Send_Target_Print(ent, -1);
	/*
	if ( ( ent->spawnflags & 4 ) ) 
	{//private, to one client only
		if (!activator || !activator->inuse)
		{
			Com_Printf("ERROR: Bad activator in Use_Target_Print");
		}
		if ( activator && activator->client )
		{//make sure there's a valid client ent to send it to
			trap_SendServerCommand( activator->s.number, va("%s \"%s\"", cmd, ent->message ));
		}
		//NOTE: change in functionality - if there *is* no valid client ent, it won't send it to anyone at all
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			G_TeamCommand( TEAM_RED, va("%s \"%s\"", cmd, ent->message) );
		}
		if ( ent->spawnflags & 2 ) {
			G_TeamCommand( TEAM_BLUE, va("%s \"%s\"", cmd, ent->message) );
			if(isChat)
				target_print_sendechos(activator, ent->message);
		}
		return;
	}
	trap_SendServerCommand( -1, va("%s \"%s\"", cmd, ent->message ));
	*/
	G_UseTargets(ent, ent->activator);
}

//Ufo: redesigned a bit to make "delay" working
void Think_Target_Print(gentity_t* ent) {
	Use_Target_Print_Go(ent);
}

void Use_Target_Print(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	ent->activator = activator;
	if (ent->delay <= 0) {
		Use_Target_Print_Go(ent);
	}
	else {
		ent->think = Think_Target_Print;
		//Ufo:
		ent->nextthink = level.time + ent->delay;
	}
}
void SP_target_print( gentity_t *ent ) {
	//RoboPhred
	if(!ent->message || !ent->message[0])
	{
		G_FreeEntity(ent);
		return;
	}
	ent->use = Use_Target_Print;
	//RoboPhred: force a delay
	if(ent->wait <= 0)
		ent->wait = 700;
	//Ufo:
	if (!ent->target2 || !ent->target2[0])
		G_SpawnString("arg", "", &ent->target2);
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	G_ActivateBehavior(ent,BSET_USE);

	if (ent->spawnflags & 3) {	// looping sound toggles
		if (ent->s.loopSound)
		{
			ent->s.loopSound = 0;	// turn it off
			ent->s.loopIsSoundset = qfalse;
			ent->s.trickedentindex = 1;
		}
		else
		{
			ent->s.loopSound = ent->noise_index;	// start it
			ent->s.loopIsSoundset = qfalse;
			ent->s.trickedentindex = 0;
		}
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

//RoboPhred
qboolean target_speaker_allowlogical() {
	int spawnflags;
	G_SpawnInt("spawnflags", "0", &spawnflags);
	if(spawnflags & 8 && !(spawnflags & 1))
		return qtrue;
	return qfalse;
}

void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s = NULL;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( G_SpawnString ( "soundSet", "", &s ) )
	{	// this is a sound set
		ent->s.soundSetIndex = G_SoundSetIndex(s);
		ent->s.eFlags = EF_PERMANENT;
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		trap_LinkEntity (ent);
		return;
	}

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) ) {
		//RoboPhred: just delete outselvs
		Com_Printf( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
		G_FreeEntity(ent);
		return;
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if ( s[0] == '*' ) {
		ent->spawnflags |= 8;
	}

	Q_strncpyz( buffer, s, sizeof(buffer) );

	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
		ent->s.loopIsSoundset = qfalse;
	}

	ent->use = Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think (gentity_t *self) {
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA (point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir, 
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy (tr.endpos, self->s.origin2);

	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			G_Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;

	if (!activator || !activator->client || (activator->s.eType != ET_PLAYER && activator->s.eType != ET_NPC)) {
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	//RoboPhred
	if(!(self->spawnflags & 1) && duelInProgress(&activator->client->ps))
		return;

	//RoboPhred
	if(!self->target || !self->target[0])
		dest = self;
	else{
		dest = G_PickTarget( self->target );
		if (!dest) {
			G_Printf ("Couldn't find teleporter destination\n");
			return;
		}
	}

	//RoboPhred: spawnflag 2 to restrict effect
	TeleportPlayer( activator, dest->s.origin, dest->s.angles, self->spawnflags & 2);
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	//RoboPhred
	/*Their target is themselvs if neccessary.
	if (!self->targetname)
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));
	*/
	self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM x x x x INACTIVE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them

INACTIVE  Can't be used until activated

wait - set to -1 to use it only once
*/
void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	qboolean ranscript = qfalse;
	if ( ( self->spawnflags & 1 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
			return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
			return;
	}

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	ranscript = G_ActivateBehavior( self, BSET_USE );
	if ( self->wait == -1 )
	{//never use again
		if ( ranscript )
		{//crap, can't remove!
			self->use = NULL;
		}
		else
		{//remove
			//RoboPhred: always stay I guess
			//if (!self->spawnString) {//Lugormod
			//	self->think = G_FreeEntity;
			//	self->nextthink = level.time + FRAMETIME;
			//} else {
			self->use = NULL;
			//}
		}
	}
	if ( self->spawnflags & 4 ) {
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			GlobalUse( ent, self, activator );
		}
		return;
	}
	G_UseTargets (self, activator);


	//RoboPhred
	if(self->target2)
		G_UseTargets2(self, activator, self->target2);
	if(self->target3)
		G_UseTargets2(self, activator, self->target3);
	if(self->target4)
		G_UseTargets2(self, activator, self->target4);
	if(self->target5)
		G_UseTargets2(self, activator, self->target5);
	if(self->target6)
		G_UseTargets2(self, activator, self->target6);
}

void SP_target_relay (gentity_t *self) {
	self->use = target_relay_use;
	if ( self->spawnflags&128 )
	{
		self->flags |= FL_INACTIVE;
	}
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	G_ActivateBehavior(self,BSET_USE);
	G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
	/*
	G_SetAngles( self, self->s.angles );
	self->s.eType = ET_INVISIBLE;
	*/
}

static void target_location_linkup(gentity_t *ent)
{
	int i;
	int n;

	if (level.locationLinked) 
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for (i = 0, ent = g_entities, n = 1;
		i < level.num_entities;
		i++, ent++) {
			if (ent->classname && !Q_stricmp(ent->classname, "target_location")) {
				// lets overload some variables!
				ent->health = n; // use for location marking
				trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
				n++;
				ent->nextTrain = level.locationHead;
				level.locationHead = ent;
			}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ){
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}

/*QUAKED target_counter (1.0 0 0) (-4 -4 -4) (4 4 4) x x x x x x x INACTIVE
Acts as an intermediary for an action that takes multiple inputs.

INACTIVE cannot be used until used by a target_activate

target2 - what the counter should fire each time it's incremented and does NOT reach it's count

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.

bounceCount - number of times the counter should reset to it's full count when it's done
*/
extern void G_DebugPrint( int level, const char *format, ... );
void target_counter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count == 0 )
	{
		return;
	}

	//gi.Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
	self->count--;

	if ( activator )
	{
		G_DebugPrint( WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname, (self->genericValue1-self->count), self->genericValue1 );
	}

	if ( self->count )
	{
		if ( self->target2 )
		{
			//gi.Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
			G_UseTargets2( self, activator, self->target2 );
		}
		return;
	}

	G_ActivateBehavior( self,BSET_USE );

	if ( self->spawnflags & 128 )
	{
		self->flags |= FL_INACTIVE;
	}

	self->activator = activator;
	G_UseTargets( self, activator );

	if ( self->count == 0 )
	{
		if ( self->bounceCount == 0 )
		{
			return;
		}
		self->count = self->genericValue1;
		if ( self->bounceCount > 0 )
		{//-1 means bounce back forever
			self->bounceCount--; 
		}
	}
}

void SP_target_counter (gentity_t *self)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}
	//if ( self->bounceCount > 0 )//let's always set this anyway
	{//we will reset when we use up our count, remember our initial count
		self->genericValue1 = self->count;
	}

	//RoboPhred: damn you ravensoft.  You add an entity value specificly for this then forget to hook it to the key...
	G_SpawnInt("bounceCount", "0", &self->bounceCount);

	self->use = target_counter_use;
}

/*QUAKED target_random (.5 .5 .5) (-4 -4 -4) (4 4 4) USEONCE
Randomly fires off only one of it's targets each time used

USEONCE	set to never fire again
*/

void target_random_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			t_count = 0, pick;
	gentity_t	*t = NULL;

	//gi.Printf("target_random %s used by %s (entnum %d)\n", self->targetname, activator->targetname, activator->s.number );
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{
		self->use = 0;
	}

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return;
	}

	if(t_count == 1)
	{
		G_UseTargets (self, activator);
		return;
	}

	//FIXME: need a seed
	pick = Q_irand(1, t_count);
	t_count = 0;
	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
		else
		{
			continue;
		}

		if (t == self)
		{
			//				gi.Printf ("WARNING: Entity used itself.\n");
		}
		else if(t_count == pick)
		{
			if (t->use != NULL)	// check can be omitted
			{
				GlobalUse(t, self, activator);
				return;
			}
		}

		if (!self->inuse)
		{
			Com_Printf("entity was removed while using targets\n");
			return;
		}
	}
}

void SP_target_random (gentity_t *self)
{
	self->use = target_random_use;
}

int	numNewICARUSEnts = 0;
void scriptrunner_run (gentity_t *self)
{
	/*
	if (self->behaviorSet[BSET_USE])
	{	
	char	newname[MAX_FILENAME_LENGTH];

	sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] );

	ICARUS_RunScript( self, newname );
	}
	*/

	if ( self->count != -1 )
	{
		if ( self->count <= 0 )
		{
			self->use = 0;
			self->behaviorSet[BSET_USE] = NULL;
			return;
		}
		else
		{
			--self->count;
		}
	}

	if (self->behaviorSet[BSET_USE])
	{
		if ( self->spawnflags & 1 )
		{
			if ( !self->activator )
			{
				if (g_developer.integer)
				{
					Com_Printf("target_scriptrunner tried to run on invalid entity!\n");
				}
				return;
			}

			//if ( !self->activator->sequencer || !self->activator->taskManager )
			if (!trap_ICARUS_IsInitialized(self->s.number))
			{//Need to be initialized through ICARUS
				if ( !self->activator->script_targetname || !self->activator->script_targetname[0] )
				{
					//We don't have a script_targetname, so create a new one
					self->activator->script_targetname = va( "newICARUSEnt%d", numNewICARUSEnts++ );
				}

				if ( trap_ICARUS_ValidEnt( self->activator ) )
				{
					trap_ICARUS_InitEnt( self->activator );
				}
				else
				{
					if (g_developer.integer)
					{
						Com_Printf("target_scriptrunner tried to run on invalid ICARUS activator!\n");
					}
					return;
				}
			}

			if (g_developer.integer)
			{
				Com_Printf( "target_scriptrunner running %s on activator %s\n", self->behaviorSet[BSET_USE], self->activator->targetname );
			}
			trap_ICARUS_RunScript( self->activator, va( "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] ) );
		}
		else
		{
			if ( g_developer.integer && self->activator )
			{
				Com_Printf( "target_scriptrunner %s used by %s\n", self->targetname, self->activator->targetname );
			}
			G_ActivateBehavior( self, BSET_USE );
		}
	}

	if ( self->wait )
	{
		self->nextthink = level.time + self->wait;
	}
}

void target_scriptrunner_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->nextthink > level.time )
	{
		return;
	}

	//Disp(activator, va("Debug: running script %s", 
	//                   self->targetname));
	//Com_Printf("running script %s\n", self->targetname);

	self->activator = activator;
	self->enemy = other;
	if ( self->delay )
	{//delay before firing scriptrunner
		self->think = scriptrunner_run;
		self->nextthink = level.time + self->delay;
	}
	else
	{
		scriptrunner_run (self);
	}
}

/*QUAKED target_scriptrunner (1 0 0) (-4 -4 -4) (4 4 4) runonactivator x x x x x x INACTIVE
--- SPAWNFLAGS ---
runonactivator - Will run the script on the entity that used this or tripped the trigger that used this
INACTIVE - start off

----- KEYS ------
Usescript - Script to run when used
count - how many times to run, -1 = infinite.  Default is once
wait - can't be used again in this amount of seconds (Default is 1 second if it's multiple-use)
delay - how long to wait after use to run script

*/
void SP_target_scriptrunner( gentity_t *self )
{
	/*
	if (g_gametype.integer == GT_FFA
	|| g_gametype.integer == GT_TEAM) {
	G_FreeEntity(self);
	return;
	}
	*/
	float v;
	if ( self->spawnflags & 128 )
	{
		self->flags |= FL_INACTIVE;
	}

	if (g_gametype.integer != GT_SIEGE && g_dontLoadNPC.integer){
		if (self->count == 1) {
			self->wait = 60; //not sure this is used
		}
		self->count = -1;
	}

	if ( !self->count )
	{
		self->count = 1;//default 1 use only
	}
	/*
	else if ( !self->wait )
	{
	self->wait = 1;//default wait of 1 sec
	}
	*/
	// FIXME: this is a hack... because delay is read in as an int, so I'm bypassing that because it's too late in the project to change it and I want to be able to set less than a second delays
	// no one should be setting a radius on a scriptrunner, if they are this would be bad, take this out for the next project
	v = 0.0f;
	G_SpawnFloat( "delay", "0", &v );
	self->delay = v * 1000;//sec to ms
	self->wait *= 1000;//sec to ms

	G_SetOrigin( self, self->s.origin );
	self->use = target_scriptrunner_use;
}
/*
void G_ToggleActiveState(char *targetstring) //Lugormod
{
gentity_t	*target = NULL;
while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
{
target->flags ^= FL_INACTIVE;
}
}
*/
void G_SetActiveState(char *targetstring, qboolean actState)
{
	gentity_t	*target = NULL;
	while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
	{
		target->flags = actState ? (target->flags&~FL_INACTIVE) : (target->flags|FL_INACTIVE);
		/*
		if (actState) {
		Com_Printf("info: %s (%i) activated.", 
		target->targetname, target->s.number);
		} else {
		Com_Printf("info: %s (%i) deactivated.", 
		target->targetname, target->s.number);
		}
		*/
	}
}

#define ACT_ACTIVE		qtrue
#define ACT_INACTIVE	qfalse

void target_activate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_ACTIVE);

	//RoboPhred
	if(self->target2)
		G_SetActiveState(self->target2, ACT_ACTIVE);
	if(self->target3)
		G_SetActiveState(self->target3, ACT_ACTIVE);
	if(self->target4)
		G_SetActiveState(self->target4, ACT_ACTIVE);
	if(self->target5)
		G_SetActiveState(self->target5, ACT_ACTIVE);
	if(self->target6)
		G_SetActiveState(self->target6, ACT_ACTIVE);
}

void target_deactivate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_INACTIVE);
	/*
	if (g_dontLoadNPC.integer && //Lugormod indeed try to reset
	(g_gametype.integer == GT_FFA ||
	g_gametype.integer == GT_TEAM)){
	self->think = target_activate_think;
	self->nextthink = level.time + 60000;
	}
	*/
	//RoboPhred
	if(self->target2)
		G_SetActiveState(self->target2, ACT_INACTIVE);
	if(self->target3)
		G_SetActiveState(self->target3, ACT_INACTIVE);
	if(self->target4)
		G_SetActiveState(self->target4, ACT_INACTIVE);
	if(self->target5)
		G_SetActiveState(self->target5, ACT_INACTIVE);
	if(self->target6)
		G_SetActiveState(self->target6, ACT_INACTIVE);

}

//FIXME: make these apply to doors, etc too?
/*QUAKED target_activate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be usable/triggerable
*/
void SP_target_activate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->use = target_activate_use;
}

/*QUAKED target_deactivate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be non-usable/triggerable
*/
void SP_target_deactivate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->use = target_deactivate_use;
}

void target_level_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	//RoboPhred
	trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"\n", self->message));
	//trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"", self->message));
	//trap_SendConsoleCommand(EXEC_NOW, va("map %s", self->message));
}

/*QUAKED target_level_change (1 0 0) (-4 -4 -4) (4 4 4)
"mapname" - Name of map to change to
*/
void SP_target_level_change( gentity_t *self )
{
	//RoboPhred
	//if (g_gametype.integer != GT_SINGLE_PLAYER){
	//	G_FreeEntity(self);
	//	return;
	//}

	char *s = NULL;

	G_SpawnString( "mapname", "", &s );
	self->message = G_NewString2(s);

	// RoboPhred: dont be naughty
	if ( !self->message || !self->message[0] || strchr(self->message, ';') != NULL || strchr(self->message, '\n') != NULL)
	{
		G_FreeEntity(self);
		return;
	}

	G_SetOrigin( self, self->s.origin );
	self->use = target_level_change_use;
}

void target_play_music_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	trap_SetConfigstring( CS_MUSIC, self->message );
}

/*QUAKED target_play_music (1 0 0) (-4 -4 -4) (4 4 4)
target_play_music
Plays the requested music files when this target is used.

"targetname"
"music"		music WAV or MP3 file ( music/introfile.mp3 [optional]  music/loopfile.mp3 )

If an intro file and loop file are specified, the intro plays first, then the looping
portion will start and loop indefinetly.  If no introfile is entered, only the loopfile
will play.
*/
void SP_target_play_music( gentity_t *self )
{
	char *s = NULL;

	G_SetOrigin( self, self->s.origin );
	if (!G_SpawnString( "music", "", &s )){
		//G_Error( "target_play_music without a music key at %s", vtos( self->s.origin ) );
		G_Free(self);
		return;
	}

	self->message = G_NewString(s);

	self->use = target_play_music_use;
}

/*
==============
target_credits
==============
*/

void Use_Target_Credits (gentity_t *ent, gentity_t *other, gentity_t *activator){
	//RoboPhred: silly lugor
	int activatorCreds;
	
	if (ent->flags & FL_INACTIVE) {
	//if (ent->spawnflags & 1) {
		//deactivated
		return;
	}
	if (!activator || !activator->client || activator->NPC || activator->r.svFlags & SVF_BOT)
		return;

	if(activator->client->pers.Lmd.account == NULL)
		return;

	if (activator->health < 1)
		return;

	if ( activator->client->pers.connected != CON_CONNECTED)
		return;


	if ( activator->client->ps.pm_type == PM_SPECTATOR )//spectators don't pick stuff up
		return;

	if (ent->genericValue6 && ent->genericValue6 > level.time)
		return;

	activatorCreds = PlayerAcc_GetCredits(activator);


	ent->genericValue6 = 0;

	if (ent->wait)
		ent->genericValue6 = level.time + ent->wait;

	int amount = ent->count;

	if (ent->random > 0) {
		amount += Q_irand(0,ent->random);
	}

	if (amount < 0 && (-amount) > activatorCreds){
		trap_SendServerCommand(activator->s.number, va("cp \"^3You cannot afford ^1CR %i^3.\"", -amount));
		amount = 0;
		G_UseTargets2(ent, activator, ent->target2);
	}
	else {
		if(!(ent->spawnflags & 1)) {
			if(amount > 0) {
				G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area.wav"));
				trap_SendServerCommand(activator->s.number, va("cp \"^3You received ^2CR %i^3.\"", amount));
			}
			else
				trap_SendServerCommand(activator->s.number, va("cp \"^3You lost ^1CR %i^3.\"", -amount));
		}
		PlayerAcc_SetCredits(activator, activatorCreds + amount);
		G_UseTargets( ent, activator );
	}
}

void SP_target_credits( gentity_t *ent ) {
	//if ( !ent->count ) {
	//	ent->count = 1;
	//}
	ent->use = Use_Target_Credits;
	ent->genericValue6 = 0;
}
