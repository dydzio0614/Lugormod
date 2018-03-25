

#include "g_local.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Stats.h"
#include "Lmd_Professions.h"

qboolean PlayerUseableCheck(gentity_t *self, gentity_t *activator);
void PlayerUsableGetKeys(gentity_t *ent);

extern vmCvar_t lmd_stashdepotime;
extern vmCvar_t lmd_stashrate;
extern vmCvar_t lmd_stashcr;

qboolean SpawnEntModel(gentity_t *ent, qboolean isSolid, qboolean isAnimated);
void PlayerUsableGetKeys(gentity_t *ent);

//TODO: credit msg will not show if the event msg is not set, fix this.

//FEATURE: Add the key stash_targetname to lmd_stashspawnpoint, sets the targetname for new spawned stashes.
//		Stashes used by this targetname will reset
//FEATURE: spawnflag to make stash solid and pick up by use key?  This might cause issues with the above TODO.
//		Need to make it tell the difference between a player crouching and using it directly, and a player using it
//		indirectly from another ent.  Or possibly have use when not picked up will make the user pick it up, use when
//		picked up makes it reset (or drop?).
//POSSIBLY: new entities: lmd_playerdropstash, lmd_playerdropcr, lmd_playerdothings 
//			entities to make a player do actions.
//			lmd_playerevent: targets for player enter trig, player exit trig, player fire weapon, use forcepower,
//			and others.

void lmd_stashzone(gentity_t *ent){
	/*
	A zone to define an area for stashes.  Also controls how many stashes can be spawned per zone.
	A stash may be set to delete itself when removed from its zone.
	spawnflags:
		1: start inactive (stashes with no active zone can be taken anywhere).
		2: master zone.  Any stash spawns that are outside a zone by their zone key will be affected by the count key of a master zone.  Having multiple zones under a single key with this spawnflag is stupid.
	keys:
		targetname: name of this zone.
		count: number of stashes that can be spawned at once within this zone.  This overrides any settings of individual stash spawns.  This is per zone entity, so if a stash is brought into another zone of the same name but out of this one, it will no longer count for this zone.
		
	*/

	G_SpawnVector("mins", "0 0 0", ent->r.mins);
	G_SpawnVector("maxs", "0 0 0", ent->r.maxs);
	if(ent->spawnflags & 1)
		ent->flags |= FL_INACTIVE;
	ent->r.svFlags = SVF_NOCLIENT;
	ent->count = 0; //probably not needed
	trap_LinkEntity(ent);
}

gentity_t *entityInStashZone(gentity_t *ent, char *zone){
	const static vec3_t range = { 40, 40, 52 }; //from G_TouchTriggers, really should be placed as a const in g_local
	vec3_t mins, maxs;
	int touch[MAX_GENTITIES];
	int i, num;
	gentity_t	*hit;

	VectorSubtract( ent->r.currentOrigin, range, mins );
	VectorAdd( ent->r.currentOrigin, range, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	//can't use ent->r.absmin apparently, because that has a one unit pad, or so They say.
	VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];
		if(Q_stricmp(hit->classname, "lmd_stashzone") != 0)
			continue;
		if(Q_stricmp(hit->targetname, zone) != 0)
			continue;
		if(hit->flags & FL_INACTIVE)
			continue;
		if(trap_EntityContact(mins, maxs, hit))
			return hit;
	}
	return NULL;
}

void lmd_stash_stopcarry(gentity_t *self){
	self->activator->client->Lmd.moneyStash = NULL;
	self->activator = NULL;	
}

void lmd_stash(gentity_t *ent);
void hurl(gentity_t *ent, gentity_t *dropped);
void lmd_stash_drop(gentity_t *self){
	char msg[MAX_STRING_CHARS];
	int i;
	gentity_t *other;

	hurl(self->activator, self);

	//target3: droptarget
	G_UseTargets2(self, self->activator, self->target3);

	//GenericString 3: drop message
	//Spawnflag (4) 16: dropmessage global.
	//Spawnflag (5) 32: dropmessage screen print.
	if(self->GenericStrings[3][0]){
		if(self->GenericStrings[3][0])
			Q_strcat(msg, sizeof(msg), va("%s\n", self->GenericStrings[3]));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(self->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("^3Contains ^2CR %i", self->count));
		for(i = 0;i<MAX_CLIENTS;i++){
			other = &g_entities[i];
			if(!other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflag (4) 16: dropmessage global.
			//GenericString 0: zone
			if(self->spawnflags & 16 || entityInStashZone(other, self->GenericStrings[0])){
				Disp(other, msg);
				//Spawnflag (5) 32: dropmessage screen print.
				if(self->spawnflags & 32)
					trap_SendServerCommand(other->s.number, va("cp \"%s\"", msg));
			}
		}
	}

	lmd_stash_stopcarry(self);
	lmd_stash(self);

}

qboolean lmd_stash_validParent(gentity_t *self){
	if(self->parent && self->parent->inuse && Q_stricmp(self->parent->classname, "lmd_stashspawnpoint") == 0)
		return qtrue;
	return qfalse;
}

void lmd_stash_destroy(gentity_t *self){
	if(self->activator)
		lmd_stash_stopcarry(self);
	if(lmd_stash_validParent(self)){
		self->parent->health--;
		if(self->parent->enemy == self)
			self->parent->enemy = NULL;
	}
	G_FreeEntity(self);
}

void lmd_stash_deposit(gentity_t *self, int creditsShift, char *extraMessage){
	char msg[MAX_STRING_CHARS] = "";
	gentity_t *other;
	unsigned int i;

	if(self->health > 0){
		int cr = PlayerAcc_GetCredits(self->activator);
		PlayerAcc_Stats_SetStashes(self->activator, PlayerAcc_Stats_GetStashes(self->activator) + 1);
		PlayerAcc_SetCredits(self->activator, cr + self->health);
	}

	//GenericString 5: deposit message
	//Spawnflag (8) 256: depositmessage global.
	//Spawnflag (9) 512: depositmessage screen print.
	if(self->GenericStrings[5][0] || extraMessage[0]){
		if(self->GenericStrings[5][0])
			Q_strcat(msg, sizeof(msg), va("%s\n", self->GenericStrings[5]));
		if(extraMessage && extraMessage[0])
			Q_strcat(msg, sizeof(msg), va("%s\n", extraMessage));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(self->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("^3Contains ^2CR %i", self->count));
		for(i = 0;i<MAX_CLIENTS;i++){
			other = &g_entities[i];
			if(!other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflag (8) 256: depositmessage global.
			//GenericString 0: zone
			if(self->spawnflags & 256 || entityInStashZone(other, self->GenericStrings[0])){
				Disp(other, msg);
				//Spawnflag (9) 512: depositmessage screen print.
				if(self->spawnflags & 512)
					trap_SendServerCommand(other->s.number, va("cp \"%s\"", msg));
			}
		}
	}

	//target5: deposittarget
	G_UseTargets2(self, self->activator, self->target5);

	lmd_stash_destroy(self);
}

void lmd_stash_startDeposit(gentity_t *self, char *extraMessage){
	char msg[MAX_STRING_CHARS] = "";
	gentity_t *other;
	unsigned int i;

	//GenericString 4: start deposit message
	//Spawnflags (6) 64: startdepomessage global.
	//Spawnflags (7) 128: startdepomessage screen print.
	if(self->GenericStrings[4][0] || extraMessage){
		if(self->GenericStrings[4][0])
			Q_strcat(msg, sizeof(msg), va("%s\n", self->GenericStrings[4]));
		if(extraMessage && extraMessage[0])
			Q_strcat(msg, sizeof(msg), va("%s\n", extraMessage));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(self->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("^3Contains ^2CR %i", self->count));
		for(i = 0;i<MAX_CLIENTS;i++){
			other = &g_entities[i];
			if(!other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflags (6) 64: startdepomessage global.
			//GenericString 0: zone
			if(self->spawnflags & 64 || entityInStashZone(other, self->GenericStrings[0])){
				Disp(other, msg);
				//Spawnflags (7) 128: startdepomessage screen print.
				if(self->spawnflags & 128)
					trap_SendServerCommand(other->s.number, va("cp \"%s\"", msg));
			}
		}
	}

	//target4: startdepotarget
	G_UseTargets2(self, self->activator, self->target4);
}

unsigned int findLmdStashSpawnpoints(char *zone, gentity_t *ents[], unsigned int count, qboolean respawnFlag){
	gentity_t *check;
	unsigned int c = 0, i;
	for(i = MAX_CLIENTS;i<ENTITYNUM_MAX_NORMAL;i++){
		check = &g_entities[i];
		if(Q_stricmp(check->classname, "lmd_stashspawnpoint") != 0)
			continue;
		//GenericStrings[0]: zone
		if(Q_stricmp(check->GenericStrings[0], zone) != 0)
			continue;

		//Spawnflag (16) 65536: Stashes in this zone will respawn at this spawn.  If no spawns have this set, stashes will not reset and just delete themselvs.
		if(respawnFlag && !(check->spawnflags & 65536))
			continue;
		ents[c] = check;
		c++;
		if(c >= count)
			return c;
	}
	return c;
}

void lmd_stash_reset(gentity_t *self, gentity_t *spawn){
	unsigned int i;
	char msg[MAX_STRING_CHARS];
	gentity_t *other;

	//GenericString 6: reset message
	//Spawnflag (10) 1024: resetmessage global.
	//Spawnflag (11) 2048: resetmessage screen print.
	if(spawn->GenericStrings[6][0]){
		if(spawn->GenericStrings[6][0])
			Q_strcat(msg, sizeof(msg), va("%s\n", spawn->GenericStrings[6]));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(self->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("^3Contains ^2CR %i", self->count));
		for(i = 0;i<MAX_CLIENTS;i++){
			other = &g_entities[i];
			if(!other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflag (10) 1024: resetmessage global.
			//GenericString 0: zone
			if(spawn->spawnflags & 1024 || entityInStashZone(other, spawn->GenericStrings[0])){
				Disp(other, msg);
				//Spawnflag (11) 2048: resetmessage screen print.
				if(spawn->spawnflags & 2048)
					trap_SendServerCommand(other->s.number, va("cp \"%s\"", msg));
			}
		}
	}
}

void lmd_stash_tryReset(gentity_t *self){
	gentity_t *ents[MAX_GENTITIES];
	unsigned int count;
	gentity_t *spawn;
	count = findLmdStashSpawnpoints(self->GenericStrings[0], ents, MAX_GENTITIES, qtrue);

	if(count == 0){
		lmd_stash_destroy(self);
		return;
	}

	spawn = ents[Q_irand(0, count-1)];

	lmd_stash_reset(self, spawn);
}

void dropMoneyStash_Old(gentity_t *ent);
void dropMoneyStash(gentity_t *player){
	if(!player->client->Lmd.moneyStash || !player->client->Lmd.moneyStash->inuse)
		return;
	if(Q_stricmp(player->client->Lmd.moneyStash->classname, "money_stash") == 0){
		dropMoneyStash_Old(player);
		return;
	}
	if(Q_stricmp(player->client->Lmd.moneyStash->classname, "lmd_stash") == 0){
		lmd_stash_drop(player->client->Lmd.moneyStash);
		return;
	}
}

void lmd_stash_pickup(gentity_t *stash, gentity_t *player){
	char msg[MAX_STRING_CHARS] = "";
	gentity_t *other;
	unsigned int i;
	stash->activator = player;

	//genericValue 6: number of times picked up
	if(stash->genericValue6 == 0){
		//GenericValue 12: stashtime
		//GenericValue 13: stashtimerandom
		if(stash->genericValue12 > 0 || stash->genericValue13 > 0)
			stash->painDebounceTime = level.time + stash->genericValue12 + Q_irand(0, stash->genericValue13);

		//GenericValue 8: creditshift
		//healingrate: creditshifttime
		//healingDebounce: creditshiftdelay
		//pain_debounce_time: next cr shift
		//healingDebounce: creditshifttimedelay
		if(stash->genericValue8 > 0){
			if(stash->healingDebounce > 0)
				stash->pain_debounce_time = level.time + stash->healingDebounce;
			else
				stash->pain_debounce_time = level.time + stash->healingrate;
		}
	}
	stash->genericValue6++;

	if(lmd_stash_validParent(stash) && stash->parent->enemy == stash)
		stash->parent->enemy = NULL;

	stash->s.eFlags |= EF_NODRAW;
	stash->r.contents = 0;

	//GenericValue 15: pickuplight
	stash->s.constantLight = stash->genericValue15;

	//noise_index: noise to play when picked up
	if(stash->noise_index)
		G_Sound(player, CHAN_AUTO, stash->noise_index);


	player->client->Lmd.moneyStash = stash;

	/*
	Spawnflags
		(2) 4: pickupmessage global: send the pickup message to everyone in the server (rather than in the stash zone), if the pickupmessage key is set.
		(3) 8: pickupmessage screen print: send the pickupmessage as a screen print, as well as in the console.
	GenericString 2: pickup message
	*/
	if(stash->GenericStrings[2][0]){
		Q_strcat(msg, sizeof(msg), va("%s\n", stash->GenericStrings[2]));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(stash->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("^3Contains ^2CR %i", stash->count));
		for(i = 0;i<MAX_CLIENTS;i++){
			other = &g_entities[i];
			if(!other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflags 4: pickupmessage global: send the pickup message to everyone in the server (rather than in the stash zone), if the pickupmessage key is set.
			//GenericString 0: zone
			if(stash->spawnflags & 4 || entityInStashZone(other, stash->GenericStrings[0])){
				Disp(other, msg);
				//Spawnflags 8: pickupmessage screen print: send the pickupmessage as a screen print, as well as in the console.
				if(stash->spawnflags & 8)
					trap_SendServerCommand(other->s.number, va("cp \"%s\"", msg));
			}
		}
	}

	G_UseTargets2(stash, player, stash->target2);
}

//seperate so that we can use this to check if we should show stash in whatever method there is to find it.
qboolean lmd_stash_canPickup(gentity_t *stash, gentity_t *player){
	int playerStashes = PlayerAcc_Stats_GetStashes(player);

	if(player->client->Lmd.moneyStash){
		if(!player->client->Lmd.moneyStash->inuse || Q_stricmp(player->client->Lmd.moneyStash->classname, "lmd_stash") != 0)
			player->client->Lmd.moneyStash = NULL;
		else{
			//already have one silly...
			return qfalse;
		}
	}

	//genericValue2: minstashes, genericValue3: maxstashes
	if(stash->genericValue2 > playerStashes || playerStashes < stash->genericValue3)
		return qfalse;

	//usableProf: profession
	//usableLevel: minlevel
	//usableLevelMax: maxlevel
	//usableFlags: player flags
	if(!PlayerUseableCheck(stash, player))
		return qfalse;

	//We give credits?
	if(stash->count > 0){
		//no admins
		if(PlayerAcc_Prof_GetProfession(player) == PROF_ADMIN)
			return qfalse;

		//no unreged players.
		if(!player->client->pers.Lmd.account)
			return qfalse;
	}

	//no duelers
	if(duelInProgress(&player->client->ps))
		return qfalse;

	//dead people can't pick us up.
	if(player->health < 1)
		return qfalse;

	//spectators don't pick stuff up
	if(player->client->ps.pm_type == PM_SPECTATOR)
		return qfalse;

	if(player->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)
		return qfalse;

	//Ufo:
	if(player->client->invulnerableTimer > level.time)
		return qfalse;

	if(player->client->pers.Lmd.persistantFlags & SPF_IONLYSABER)
		return qfalse;

	if(player->client->pers.Lmd.persistantFlags & SPF_CORTOSIS)
		return qfalse;

	//still recovering from a previous action, cant pick it up.
	//Ufo:
	if(player->client->ps.weaponTime > 0 && !(player->client->Lmd.restrict & 16))
		return qfalse;

	//stunned, cant pick it up.
	if(player->client->ps.electrifyTime >= level.time)
		return qfalse;


	//no stash for protected meditators.
	if(g_meditateProtect.integer && player->client->ps.legsAnim == BOTH_MEDITATE && player->client->ps.torsoAnim == BOTH_MEDITATE)
		return qfalse;

	//don't pick it up if you just dropped it
	//GenericValue 10: reserved for hurl: time before pickuppable by dropper
	//GenericValue 11: reserved for hurl: player who dropped it.
	if(stash->genericValue10 > level.time && stash->genericValue11 == player->s.number)
		return qfalse;

	return qtrue;
}

void lmd_stash_tryPickup(gentity_t *stash, gentity_t *player){
	if(!lmd_stash_canPickup(stash, player))
		return;
	lmd_stash_pickup(stash, player);
}

void lmd_stash_touch(gentity_t *self, gentity_t *other, trace_t *trace){
	lmd_stash_tryPickup(self, other);
}

void lmd_stash_think(gentity_t *ent){
	gentity_t *zoneEnt;
	if(ent->activator){
		//count: base cr
		//genericValue8: creditsshift
		if(ent->count > 0 && ent->genericValue8 > 0 && ent->pain_debounce_time <= level.time){
			//genericValue9: creditshiftrandom
			//pain_debounce_time: next cr shift
			//healingrate: creditshifttime
			ent->health -= ent->genericValue8 + Q_irand(0, ent->genericValue9);
			if(ent->health <= 0){
				//no cr 4 u
				Disp(ent->activator, va("^3The %s has ran out.", ent->fullName));
				lmd_stash_tryReset(ent);
				return;
			}
			ent->pain_debounce_time = level.time + ent->healingrate;
		}
		if(ent->activator->health <= 0){
			lmd_stash_drop(ent);
			return;
		}
	}

	//painDebounceTime: time until deleted.
	//GenericValue 6: number of times this ent was picked up.
	if(ent->genericValue6 > 0 && ent->painDebounceTime > 0 && ent->painDebounceTime <= level.time){
		Disp(ent->activator, va("^3The %s has expired.", ent->fullName));
		lmd_stash_tryReset(ent);
		return;
	}

	//GenericString 0: zone
	if(ent->GenericStrings[0][0] && ent->genericValue1 >= 0){
		if(ent->activator)
			zoneEnt = entityInStashZone(ent->activator, ent->GenericStrings[0]);
		else
			zoneEnt = entityInStashZone(ent, ent->GenericStrings[0]);

		if(!zoneEnt){
			//GenericValue 1: zoneexittime: time between exiting a zone and being reset/deleted
			//timestamp: last time we were in a zone
			int timeLeft = (level.time - ent->timestamp) - ent->genericValue1;
			if(timeLeft <= 0){
				if(ent->activator)
					Disp(ent->activator, va("^3The %s has expired from leaving the zone.", ent->fullName));
				lmd_stash_tryReset(ent);
				return;
			}
			//GenericValue 7: last general message print time (primaraly for out-of-zone, might use it for others).
			if(ent->activator && level.time - ent->genericValue7 >= 1000){
				char msg[MAX_STRING_CHARS] = "";
				int secTime = ceil((float)timeLeft / 1000.0f);
				ent->genericValue7 = level.time;
				if(ent->GenericStrings[1][0]){
					Q_strncpyz(msg, va("%s\n", ent->GenericStrings[1]), sizeof(msg));
				}
				Q_strcat(msg, sizeof(msg), va("%s%sYou have ^2%i^3 second%s to return to the %s zone",
					(ent->GenericStrings[1][0])?ent->GenericStrings[1]:"", (ent->GenericStrings[1])?"\n":"",
					secTime, (secTime!=1)?"s":"", ent->fullName));
				Disp(ent->activator, msg);
			}
		}
		else
			ent->timestamp = level.time;
	}

	ent->nextthink = level.time + FRAMETIME;
}

void lmd_stash(gentity_t *ent){
	/*
	Other

		bounceCount: reserved for physics use.
		GenericValue 6: number of times this ent was picked up.
		health: current credit amount
		pain_debounce_time: next cr shift
		painDebounceTime: time until deleted.
		activator: current player holding this
		timestamp: last time we were in a zone
		GenericValue 7: last general message print time (primaraly for out-of-zone, might use it for others).

		<inhereted from stashspawnpoint>
		GenericString 0: zone
		GenericString 1: zone exit message
		GenericString 2: pickup message
		GenericString 3: drop message
		GenericString 4: start deposit message
		GenericString 5: deposit message

		noise_index: noise to play when picked up

		GenericValue 1: time between exiting a zone and being reset/deleted
		GenericValue 2: minstashes
		GenericValue 3: maxstashes
		GenericValue 4: basedeposittime
		GenericValue 5: basedepositrandom
		GenericValue 8: creditshift
		GenericValue 9: creditshiftrandom
		healingrate: creditshifttime
		healingDebounce: creditshifttimedelay
		GenericValue 10: reserved for hurl: time before pickuppable by dropper
		GenericValue 11: reserved for hurl: player who dropped it.
		GenericValue 12: stashtime
		GenericValue 13: stashtimerandom
		GenericValue 14: light
		GenericValue 15: pickuplight
		usableProf: profession
		usableLevel: minlevel
		usableLevelMax: maxlevel
		target1: spawntarget
		target2: pickuptarget
		target3: droptarget
		target4: startdepotarget
		target5: deposittarget
	*/


	//TODO: if credits is < 0, get cr ammount from cvar (dont think I made a cvar for this yet).  Credits of 0 is fine.

	/*
	stash pickup
		stash->count++;
	stash deposit
		(check that parent is valid)
		stash->parent->health--;
	*/

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = lmd_stash_touch;
	ent->s.eFlags &= ~EF_NODRAW;

	//GenericValue 14: light
	ent->s.constantLight = ent->genericValue14;

	ent->think = lmd_stash_think;
	ent->nextthink = level.time + FRAMETIME;

	ent->health = ent->count;

	ent->classname = "lmd_stash";

	trap_LinkEntity(ent);
}

void lmd_stash_spawn(gentity_t *ent){

	//GenericValue 4: basedeposittime
	if(ent->genericValue4 < 0)
		ent->genericValue4 = lmd_stashdepotime.integer;

	lmd_stash(ent);
	
	ent->parent->health++; //assume parent is valid, we were just spawned by it.

	//target1: spawntarget
	G_UseTargets2(ent, ent, ent->target);
}

unsigned int countZoneStashes(char *zone){
	unsigned int i, c = 0;
	gentity_t *check;
	for(i = MAX_CLIENTS;i<ENTITYNUM_MAX_NORMAL;i++){
		check = &g_entities[i];
		if(!check->inuse)
			continue;
		if(Q_stricmp(check->classname, "lmd_stash") != 0)
			continue;
		if(Q_stricmp(check->GenericStrings[0], zone) != 0)
			continue;
		c++;
	}
	return c;
}

qboolean tryStashSpawnCheck(gentity_t *spawner, qboolean ignoreOtherStashes){
	unsigned int otherStashes = 0, ownStashes = 0;
	unsigned int i;
	gentity_t *zone = NULL;
	gentity_t *check;

	zone = entityInStashZone(spawner, spawner->GenericStrings[0]);
	if(!zone){
		//outside of any zone, find a master zone
		while(zone = G_Find(zone, FOFS(targetname), spawner->GenericStrings[0])){
			if(Q_stricmp(zone->classname, "lmd_stashzone") == 0 && !(zone->flags & FL_INACTIVE) && zone->spawnflags & 2)
				break;
		}
		if(!zone)
			return qtrue; //no zones exist for us, go ahead and spawn.
	}

	for(i = MAX_CLIENTS;i<ENTITYNUM_MAX_NORMAL;i++){
		check = &g_entities[i];
		if(Q_stricmp(check->classname, "lmd_stash") != 0)
			continue;
		if(Q_stricmp(check->GenericStrings[0], zone->targetname) != 0)
			continue;
		if(check->parent == spawner)
			ownStashes++;
		else
			otherStashes++;
	}
	if(ignoreOtherStashes == qfalse && otherStashes > zone->count)
		return qfalse;
	if(spawner->count > 0 && ownStashes >= spawner->count)
		return qfalse;
	zone->count++;
	return qtrue;
}

void lmd_stashspawnpoint_spawnstash(gentity_t *ent){

	char msg[MAX_STRING_CHARS] = "";
	unsigned int i;
	gentity_t *player;
	gentity_t *stash;
	
	//Spawnflag 8192: stashes can spawn here even if a stash has been spawned by another spawn within the zone.
	if(!tryStashSpawnCheck(ent, ent->spawnflags & 8192))
		return;

	//count: number of stashes that this spawn can have spawned at once.  Defaults to 1.
	//health: number of stashes spawned
	if(ent->count > 0 && ent->health >= ent->count)
		return;

	//dont spawn if our current stash has not been picked up yet.
	if(ent->enemy && Q_stricmp(ent->enemy->classname, "lmd_stash") == 0 && ent->enemy->health < 1)
		return;

	stash = G_Spawn();
	if(!stash)
		return;
	G_SetOrigin(stash, ent->s.origin);
	stash->parent = ent;
	ent->enemy = stash;


	/*
	Spawnflags
		(0) 1: spawnmessage global: send the spawnmessage to everyone in the server (rather than in the stash zone), if the spawnmessage key is set.
		(1) 2: spawnmessage screen print: send the spawnmessage as a screen print, as well as in the console.
		(2) 4: pickupmessage global: send the pickup message to everyone in the server (rather than in the stash zone), if the pickupmessage key is set.
		(3) 8: pickupmessage screen print: send the pickupmessage as a screen print, as well as in the console.
		(4) 16: dropmessage global.
		(5) 32: dropmessage screen print.
		(6) 64: startdepomessage global.
		(7) 128: startdepomessage screen print.
		(8) 256: depositmessage global.
		(9) 512: depositmessage screen print.

		//We use the resetmessage from the spawner, so these wont be used in the stash
		//(10) 1024: resetmessage global.
		//(11) 2048: resetmessage screen print.

		(12) 4096: display stash amount: include the credits amount on all stash messages.
		(13) 8192: stashes can spawn here even if a stash has been spawned by another spawn within the zone.
		(14) 16384: start disabled.  Needs to be used by a target_activate to start spawning.
		(15) 32768: reset credits shift when dropped.
		(16) 65536: respawn the stash on reset.  Stash will respawn at this spawn.
	*/
	stash->spawnflags = ent->spawnflags;

	//GenericValue 1: time between exiting a zone and being reset/deleted
	stash->genericValue1 = ent->genericValue1;
	//GenericValue 2: minstashes
	stash->genericValue2 = ent->genericValue2;
	//GenericValue 3: maxstashes
	stash->genericValue3 = ent->genericValue3;
	//GenericValue 4: basedeposittime
	stash->genericValue4 = ent->genericValue4;
	//GenericValue 5: basedepositrandom
	stash->genericValue5 = ent->genericValue5;
	//GenericValue 8: creditshift
	stash->genericValue8 = ent->genericValue8;
	//GenericValue 9: creditshiftrandom
	stash->genericValue9 = ent->genericValue9;
	//healingrate: creditshifttime
	//GenericValue 10: creditshifttime
	stash->healingrate = ent->genericValue10;
	//healingDebounce: creditshiftdelay
	//GenericValue 11: creditshiftdelay
	stash->healingDebounce = ent->genericValue11;
	//GenericValue 12: stashtime
	stash->genericValue12 = ent->genericValue12;
	//GenericValue 13: stashtimerandom
	stash->genericValue13 = ent->genericValue13;
	//GenericValue 14: light
	stash->genericValue14 = ent->genericValue14;
	//GenericValue 15: pickuplight
	stash->genericValue15 = ent->genericValue15;

	//noise_index: noise to play when picked up
	stash->noise_index = ent->noise_index;

	//GenericString 0: zone
	stash->GenericStrings[0] = G_NewString2(ent->GenericStrings[0]);
	//GenericString 1: zone exit message
	stash->GenericStrings[1] = G_NewString2(ent->GenericStrings[1]);
	//GenericString 2: pickup message
	stash->GenericStrings[2] = G_NewString2(ent->GenericStrings[2]);
	//GenericString 3: drop message
	stash->GenericStrings[3] = G_NewString2(ent->GenericStrings[3]);
	//GenericString 4: start deposit message
	stash->GenericStrings[4] = G_NewString2(ent->GenericStrings[4]);
	//GenericString 5: deposit message
	stash->GenericStrings[5] = G_NewString2(ent->GenericStrings[5]);

	//we use the resetmsg from the spawnpoint we randomly select.
	////GenericString 6: reset message
	//stash->GenericStrings[6] = G_NewString2(ent->GenericStrings[6]);

	//Usability keys
	memcpy(&stash->Lmd.UseReq, &ent->Lmd.UseReq, sizeof(ent->Lmd.UseReq));

	//message: the name to use for this stash, defaults to "money stash"
	stash->message = G_NewString2(ent->message);

	//target1: spawntarget
	stash->target = G_NewString2(ent->target);
	//target2: pickuptarget
	stash->target2 = G_NewString2(ent->target2);
	//target3: droptarget
	stash->target3 = G_NewString2(ent->target3);
	//target4: startdepotarget
	stash->target4 = G_NewString2(ent->target4);
	//target5: stopdepotarget
	stash->target5 = G_NewString2(ent->target5);
	//target6: deposittarget
	stash->target6 = G_NewString2(ent->target6);

	//fullname: the name to use for this stash, defaults to "money stash"
	stash->fullName = G_NewString2(ent->fullName);

	stash->s.modelindex = ent->s.modelindex;
	stash->s.modelGhoul2 = ent->s.modelGhoul2;
	VectorCopy(ent->r.mins, stash->r.mins);
	VectorCopy(ent->r.maxs, stash->r.maxs);
	stash->s.iModelScale = ent->s.iModelScale;
	VectorCopy(ent->modelScale, stash->modelScale);

	//GenericValue 6: credits
	//GenericValue 7: creditsrandom
	if(ent->genericValue6 < 0)
		stash->count = lmd_stashcr.integer + Q_irand(0, ent->genericValue7);
	else
		stash->count = ent->genericValue6 + Q_irand(0, ent->genericValue7);

	lmd_stash_spawn(stash);

	//GenericString 7: spawnmessage
	if(ent->GenericStrings[7][0]){
		Q_strncpyz(msg, ent->GenericStrings[7], sizeof(msg));
		//Spawnflag 4096: display stash amount: include the credits amount on all stash messages.
		if(ent->spawnflags & 4096)
			Q_strcat(msg, sizeof(msg), va("\n^3Contains ^2CR %i", stash->count));

		for(i = 0;i<MAX_CLIENTS;i++){
			player = &g_entities[i];
			if(!player->inuse || !player->client || player->client->pers.connected != CON_CONNECTED)
				continue;
			//Spawnflag 1: spawnmessage global: send the spawnmessage to everyone in the server (rather than in the stash zone), if the spawnmessage key is set.
			//GenericString 0: zone
			if(ent->spawnflags & 1 || entityInStashZone(player, ent->GenericStrings[0])){
				Disp(player, msg);
				//Spawnflag 2: spawnmessage screen print: send the spawnmessage as a screen print, as well as in the console.
				if(ent->spawnflags & 2)
					trap_SendServerCommand(player->s.number, va("cp \"%s\"", msg));
			}
		}
	}
}

void lmd_stashspawnpoint_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	//spawnflag 16384: start disabled.  Needs to be used by a target_activate to start spawning.
	if(self->flags & FL_INACTIVE)
		return;
	lmd_stashspawnpoint_spawnstash(self);
}

void lmd_stashspawnpoint_think(gentity_t *ent){
	if(!(ent->flags & FL_INACTIVE))
		lmd_stashspawnpoint_spawnstash(ent);
	
	//wait: spawntime
	//random: spawnrandom
	ent->nextthink = level.time + (int)ent->wait + Q_irand(0, (int)ent->random);
}

void lmd_stashspawnpoint(gentity_t *ent){
	/*
	Point where stashes spawn.
	Can be triggered to manually spawn a stash.  Multiple stashes can be spawned at once.
	It is possible to make a stash with 0 credits for the purpose of making it use a target when deposited (see the deposittarget key).
	A new stash will not spawn if the last one has not been picked up.
	stashspawnpoints that have been deactivated will not spawn stashes, although they will still run their spawn timers (they will just not spawn the stash when the timer fires).
	Important: if you want to use the traditional method of one stash spawning randomly, make sure all stash spawns have a random key, else only one point will spawn stashes.  Or just set the spawn time to 0 and use a target_random targeting all the spawn points.
	spawnflags:
		(0) 1: spawnmessage global: send the spawnmessage to everyone in the server (rather than in the stash zone), if the spawnmessage key is set.
		(1) 2: spawnmessage screen print: send the spawnmessage as a screen print, as well as in the console.
		(2) 4: pickupmessage global: send the pickup message to everyone in the server (rather than in the stash zone), if the pickupmessage key is set.
		(3) 8: pickupmessage screen print: send the pickupmessage as a screen print, as well as in the console.
		(4) 16: dropmessage global.
		(5) 32: dropmessage screen print.
		(6) 64: startdepomessage global.
		(7) 128: startdepomessage screen print.
		(8) 256: depositmessage global.
		(9) 512: depositmessage screen print.
		(10) 1024: resetmessage global.
		(11) 2048: resetmessage screen print.
		(12) 4096: display stash amount: include the credits amount on all stash messages.
		(13) 8192: stashes can spawn here even if a stash has been spawned by another spawn within the zone.
		(14) 16384: start disabled.  Needs to be used by a target_activate to start spawning.
		(15) 32768: reset credits shift when dropped.
		(16) 65536: Stashes in this zone will respawn at this spawn.  If no spawns have this set, stashes will not reset and just delete themselvs.

		I am currently not going to add a spawnflag that determines if the stash can be dropped or not, although I think this might be usefull.  Request it if you need it.
		Same goes for a spawnflag to choose weather or not you can see it with sight/binoculars.
		Same for a spawnflag to reset/respawn on player death rather than drop.

	keys:
		zone: the lmd_stashzone to connect this stash to.  Zones control how many stashes can be spawned at once.  Also, this is the area to not let the stash be removed from, if specified.
		zoneexittime: If enabled by spawnflag, how many seconds to wait before deleting the stash after it leaves the zone.  Defaults to 4
		zoneexitmessage: If a player has a stash and exits the zone, this message will be displayed along with a countdown timer.  Defaults to "You have left the stash zone."

		profession: profession bitmask of who can use this.

		minlevel: min level needed to pick this up.
		maxlevel: any players above this level cannot pick this up.

		playerflags: only players with a flag(s) specified here can pick this up.

		minstashes: players cannot pick this stash up until they have picked up this many stashes (from other spawns).
		maxstashes: players cannot pick this stash up if they have picked up more than this amount of stashes.

		name: the name to use for this stash, defaults to "money stash"

		count: number of stashes that this spawn can have spawned at once.  Defaults to 1.  If this is set to 0, the stash can be picked up by unregistered players, and will not count as a stash deposit (will not count for the minstashes/maxstashes counter).

		basedeposittime: time in msecs needed to deposit.  lmd_stashdepo can modify this.
		basedepositrandom: random amount to add to the base deposit time on pickup.  Amount added is random between 0 and this number.

		credits: base cr amount
		creditsrandom: random amount to add for core credits value.

		sound: sound to make on pickup.  Defaults to interface/secret_area.wav.  If you want more sounds, such as drop/deposit sounds, use the targets or make a feature request.

		creditshift: credit amount to shift per second.  Can be positive (increase worth longer its picked up), or negitive (decrease worth longer its picked up).  If the stash amount reaches 0, the stash is reset.
		creditshiftrandom: random amount to add to offset each credit shift by.
		creditshifttime: how many seconds to run the shift for.
		creditshiftdelay: how long to wait before starting the shift.

		spawntime: time between spawns.  Set to 0 to only spawn on use.  Defaults to the contents of the cvar lmd_stashrate
		spawnrandom: random ammount between 0 and this to offset the spawn time by.

		stashtime: after this many seconds after being picked up, make the stash dissapear.
		stashtimerandom: A number between 0 and this will be added to the stash time.

		spawnmessage: message to send on spawn
		pickupmessage: message to send to players in the zone when picked up (can be sent to all players by spawnflag)
		dropmessage: message to send to players in the zone when dropped (can be sent to all players by spawnflag)
		startdepomessage: message to send to players in the zone when starting deposit (can be sent to all players by spawnflag)
		depositmessage: message to send to players in the zone when deposited (can be sent to all players by spawnflag)
		resetmessage: message to send when the stash is deleted due to leaving the zone or timing out.

		color: color glow to emit when not picked up.  Recommend making all stashes in a zone use the same color.
		light: light intensity to emit when not picked up
		pickupcolor: color glow to emit when picked up.  Recommend making all stashes in a zone use the same color.
		pickuplight: light intensity to emit when picked up.

		model: model to use.  Defaults to items/datapad.md3, Recommend making all stashes in a zone use the same model.
		mins/maxs
		

		spawntarget: target to use on spawn
		pickuptarget: target to use on pickup
		droptarget: target to use on drop
		startdepotarget: target to use on deposit start
		deposittarget: target to use on deposit.
	
	*/

	/*
		GenericValues
			1: time between exiting a zone and being reset/deleted (zone exit time)
			2: minstashes
			3: maxstashes
			4: basedeposittime
			5: basedepositrandom
			6: credits
			7: creditsrandom
			8: creditshift
			9: creditshiftrandom
			10: creditshifttime
			11: creditshiftdelay
			12: stashtime
			13: stashtimerandom
			14: light
			15: pickuplight
		GenericStrings
			0: zone
			1: zone exit message
			2: pickupmessage
			3: dropmessage
			4: startdepomessage
			5: depositmessage
			6: resetmessage
			7: spawnmessage
		Other
			usableProf: profession

			//TODO: need to make the key "sound" for this
			//noise_index: noise to play when picked up

			usableLevel: minlevel
			usableLevelMax: maxlevel

			fullname: the name to use for this stash, defaults to "money stash"

			count: number of stashes that this spawn can have spawned at once.  Defaults to 1.

			wait: spawntime
			random: spawnrandom

			target1: spawntarget
			target2: pickuptarget
			target3: droptarget
			target4: startdepotarget
			target5: deposittarget

			health: number of stashes spawned
	*/

	float tmpFloat;
	int i;
	vec3_t color;
	int c;
	char *buf = NULL;

	//zone: the lmd_stashzone to connect this stash to.  Zones control how many stashes can be spawned at once.  Also, this is the area to not let the stash be removed from, if specified.

	G_SpawnString("zone", "", &ent->GenericStrings[0]);

	//zoneexittime: If enabled by spawnflag, how many seconds to wait before deleting the stash after it leaves the zone.  Defaults to 4
	G_SpawnFloat("zoneexittime", "4.0", &tmpFloat);
	ent->genericValue1 = (int)ceil(tmpFloat * 1000.0f);

	//zoneexitmessage: If a player has a stash and exits the zone, this message will be displayed along with a countdown timer.  Defaults to "You have left the stash zone."
	G_SpawnString("zoneexitmessage", "", &ent->GenericStrings[1]);

	//profession: profession bitmask of who can use this.
	//minlevel: min level needed to pick this up.
	//maxlevel: any players above this level cannot pick this up.
	//playerflags
	PlayerUsableGetKeys(ent);

	//minstashes: players cannot pick this stash up until they have picked up this many stashes (from other spawns).
	G_SpawnInt("minstashes", "0", &ent->genericValue2);
	//maxstashes: players cannot pick this stash up if they have picked up more than this amount of stashes.
	G_SpawnInt("maxstashes", "-1", &ent->genericValue3);

	//name: the name to use for this stash, defaults to "money stash"
	G_SpawnString("name", "money stash", &ent->fullName);

	//count: number of stashes that this spawn can have spawned at once.  Defaults to 1.
	//if(ent->count == 0)
	//	ent->count = 1;
	//defaults to 1, but can be 0!
	G_SpawnInt("count", "1", &ent->count);

	//basedeposittime: time in msecs needed to deposit.  lmd_stashdepo can modify this.
	G_SpawnFloat("basedeposittime", "-1", &tmpFloat);
	ent->genericValue4 = (int)ceil(tmpFloat * 1000.0f);
	
	//basedepositrandom: random amount to add to the base deposit time on pickup.  Amount added is random between 0 and this number.
	G_SpawnFloat("basedepositrandom", "0", &tmpFloat);
	ent->genericValue5 = (int)ceil(tmpFloat * 1000.0f);

	//credits: base cr amount
	G_SpawnInt("credits", "-1", &ent->genericValue6);
	
	//creditsrandom: random amount to add for core credits value.
	G_SpawnInt("creditsrandom", "0", &ent->genericValue7);

	//sound: sound to make on pickup.  If you want more sounds, such as drop/deposit sounds, use the targets or make a feature request.
	G_SpawnString("sound", "sound/interface/secret_area.wav", &buf);
	ent->noise_index = G_SoundIndex(buf);

	//creditshift: credit amount to shift per second.  Can be positive (increase worth longer its picked up), or negitive (decrease worth longer its picked up).  If the stash amount reaches 0, the stash is reset.
	G_SpawnInt("creditshift", "0", &ent->genericValue8);
	
	//creditshiftrandom: random amount to add to offset each credit shift by.
	G_SpawnInt("creditshiftrandom", "0", &ent->genericValue9);
	
	//creditshifttime: how many seconds to run the shift for.
	G_SpawnFloat("creditshifttime", "0", &tmpFloat);
	ent->genericValue10 = (int)ceil(tmpFloat * 1000.0f);
	
	//creditshiftdelay: how long to wait before starting the shift.
	G_SpawnFloat("creditshiftdelay", "0", &tmpFloat);
	ent->genericValue11 = (int)ceil(tmpFloat * 1000.0f);

	//spawntime: time between spawns.  Set to 0 to only spawn on use.  Defaults to the contents of the cvar lmd_stashrate
	G_SpawnFloat("spawntime", va("%f", lmd_stashrate.value), &ent->wait);
	//lmd_stashrate is in msecs already.
	//ent->wait = ceil(ent->wait * 1000.0f);

	//spawnrandom: random ammount between 0 and this to offset the spawn time by.
	G_SpawnFloat("spawnrandom", "0", &ent->random);
	ent->random = ceil(ent->random * 1000.0f);

	//stashtime: after this many seconds after being picked up, make the stash dissapear.
	G_SpawnFloat("stashtime", "0", &tmpFloat);
	ent->genericValue12 = (int)ceil(tmpFloat * 1000.0f);

	//stashtimerandom: A number between 0 and this will be added to the stash time.
	G_SpawnFloat("stashtimerandom", "0", &tmpFloat);
	ent->genericValue13 = (int)ceil(tmpFloat * 1000.0f);

	//pickupmessage: message to send to players in the zone when picked up (can be sent to all players by spawnflag)
	G_SpawnString("pickupmessage", "", &ent->GenericStrings[2]);
	
	//dropmessage: message to send to players in the zone when dropped (can be sent to all players by spawnflag)
	G_SpawnString("dropmessage", "", &ent->GenericStrings[3]);
	
	//startdepomessage: message to send to players in the zone when starting deposit (can be sent to all players by spawnflag)
	G_SpawnString("startdepomessage", "", &ent->GenericStrings[4]);

	//depositmessage: message to send to players in the zone when deposited (can be sent to all players by spawnflag)
	G_SpawnString("depositmessage", "", &ent->GenericStrings[5]);

	//resetmessage: message to send when the stash is deleted due to leaving the zone or timing out.
	G_SpawnString("resetmessage", "", &ent->GenericStrings[6]);

	//spawnmessage: message to send on spawn
	G_SpawnString("spawnmessage", "", &ent->GenericStrings[7]);

	//color: color glow to emit when not picked up.  Recommend making all stashes in a zone use the same color.
	G_SpawnVector("color", "0 0 0", color);
	//light: light intensity to emit when not picked up
	G_SpawnInt("light", "0", &ent->genericValue14);
	if(ent->genericValue14 > 0){
		ent->genericValue14 /= 4;
		ent->genericValue14 = (ent->genericValue14 << 24);
		for(i = 0;i<3;i++){
			c = color[i] * 255;
			if(c > 255)
				c = 255;
			ent->genericValue14 |= (c << (8*i));
		}
	}

	//pickupcolor: color glow to emit when picked up.  Recommend making all stashes in a zone use the same color.
	G_SpawnVector("pickupcolor", "0 0 0", color);
	//pickuplight: light intensity to emit when picked up.
	G_SpawnInt("pickuplight", "0", &ent->genericValue15);
	if(ent->genericValue15 > 0){
		ent->genericValue15 /= 4;
		ent->genericValue15 = (ent->genericValue15 << 24);
		for(i = 0;i<3;i++){
			c = color[i] * 255;
			if(c > 255)
				c = 255;
			ent->genericValue15 |= (c << (8*i));
		}
	}

	//model: model to use.  Recommend making all stashes in a zone use the same model.
	//mins/maxs
	if(!ent->model || !ent->model[0]){
		if(ent->model)
			G_Free(ent->model);
		ent->model = G_NewString2("models/items/datapad.glm");
	}
	SpawnEntModel(ent, qfalse, qfalse);
	//not our model, dont show up.
	ent->r.svFlags = SVF_NOCLIENT;

	if(ent->target)
		G_Free(ent->target);
	if(ent->target2)
		G_Free(ent->target2);
	if(ent->target3)
		G_Free(ent->target3);
	if(ent->target4)
		G_Free(ent->target4);
	if(ent->target5)
		G_Free(ent->target5);
	
	//spawntarget: target to use on spawn
	G_SpawnString("spawntarget", "", &ent->target);

	//pickuptarget: target to use on pickup
	G_SpawnString("pickuptarget", "", &ent->target2);

	//droptarget: target to use on drop
	G_SpawnString("droptarget", "", &ent->target3);

	//startdepotarget: target to use on deposit start
	G_SpawnString("startdepotarget", "", &ent->target4);

	//deposittarget: target to use on deposit.
	G_SpawnString("deposittarget", "", &ent->target5);

	ent->health = 0; //number of stashes currently out.
	ent->use = lmd_stashspawnpoint_use;
	if(ent->wait > 0){
		ent->think = lmd_stashspawnpoint_think;
		ent->nextthink = level.time + (int)ent->wait + Q_irand(0, (int)ent->random);
	}
		
	//16384: start disabled.  Needs to be used by a target_activate to start spawning.
	if(ent->spawnflags & 16384)
		ent->flags |= FL_INACTIVE;

}

void lmd_stashdepo_deposit(gentity_t *ent, gentity_t *player){
	int credits = player->client->Lmd.moneyStash->health;
	//GenericValue6: bonuscredits
	//GenericValue7: bonuscreditsrandom
	credits += ent->genericValue6 + Q_irand(0, ent->genericValue7);

	//2: deposittarget: target to use on deposit.
	G_UseTargets2(ent, player, ent->target2);

	//GenericStrings[2]: depositmessage: message to add to the bottom of the depositmessage of the stash.
	lmd_stash_deposit(player->client->Lmd.moneyStash, credits, ent->GenericStrings[2]);

	ent->activator = NULL;
	ent->enemy = NULL;
	return;
}

void lmd_stashdepo_use(gentity_t *ent, gentity_t *other, gentity_t *activator){
	int playerStashes = PlayerAcc_Stats_GetStashes(activator);

	if(!activator->client->Lmd.moneyStash){
		char msg[MAX_STRING_CHARS] = "";
		//message: message to print when used without a stash.
		if(ent->message && ent->message[0]){
			Q_strcat(msg, sizeof(msg), va("%s\n", ent->message));
		}
		//Spawnflag 2: dont display stash status.  If set, the terminal will not tell the user if a stash is out when it is used.
		if(!(ent->spawnflags & 2)){
			int count = countZoneStashes(ent->GenericStrings[0]);
			//fullName: stashname: name to refer to the stashes with.  Only used for displaying the count of stashes (not used if spawnflag 2 is set).
			//healingclass: stashnameplural: same as stashname, but if there are more than 1 stashes
			Q_strcat(msg, sizeof(msg), va("^3There %s ^2%i^3 %s out.", (count!=1)?"are":"is", count, (count!=1)?ent->healingclass:ent->fullName));
		}
		trap_SendServerCommand(activator->s.number, va("cp \"%s\"", msg));
		return;
	}

	//GenericValue1: minstashes
	//GenericValue2: maxstashes
	if(ent->genericValue1 > playerStashes || playerStashes < ent->genericValue2)
		return;
	
	//Spawnflag 1: no deposit time.  Usefull if you want to make another non-player entity trigger this
	if(!(ent->spawnflags & 1)){
		//lmd_stash GenericValue 4: basedeposittime
		//lmd_stash GenericValue 5: basedepositrandom
		int depositTime = activator->client->Lmd.moneyStash->genericValue4 + Q_irand(0, 
			activator->client->Lmd.moneyStash->genericValue5);
		//lmd_stashdepo 3: depotimeoffst
		//lmd_stashdepo 4: depotimeoffsetrandom
		depositTime += ent->genericValue3 + Q_irand(0, ent->genericValue4);
		/*dont think there is a sane way to do this here, need to use a think function to clear this if the hacktime resets.
		if(ent->activator && ent->activator != activator){
			//someone is already using us.
			return;
		}
		*/
		if (depositTime > 0){
			if (other->client->isHacking != ent->genericValue10){
				other->client->isHacking = ent->genericValue10;
				other->client->ps.hackingTime = level.time + depositTime;
				other->client->ps.hackingBaseTime = depositTime;
				VectorCopy(other->client->ps.viewangles, other->client->hackingAngles);

				//until I get a check running for if we are being hacked already, this should be consitered invalid
				ent->activator = other;

				//record what stash he has.
				//until I get a check running for if we are being hacked already, this should be consitered invalid
				ent->enemy = other->client->Lmd.moneyStash;

				//GenericStrings[1]: startdepomessage: message to add to the bottom of the startdepomessage of the stash.
				lmd_stash_startDeposit(other->client->Lmd.moneyStash, ent->GenericStrings[1]);
				
				//target 0: startdepotarget: target to use on deposit start
				G_UseTargets(ent, other);

				return;
			}
			else if (other->client->ps.hackingTime > level.time){
				//is hacking
				return;
			}
			other->client->ps.hackingTime = 0;
			other->client->ps.hackingBaseTime = 0;
			other->client->isHacking = 0;
		}
	}

	//deposit the stash
	lmd_stashdepo_deposit(ent, other);
}

void lmd_stashdepozone_think (gentity_t *ent){
	if (!ent->parent || !ent->parent->inuse || Q_strncmp(ent->classname, ent->parent->classname, strlen(ent->parent->classname)) ||
		ent->parent->genericValue10 != ent->s.number){
		G_FreeEntity(ent);
		return;
	}
	ent->nextthink = level.time + 5000;
}

qboolean lmd_stashdepo_makezone(gentity_t *ent){
	gentity_t *zone = G_Spawn();
	if (!zone) {
		return qfalse;
	}
	zone->classname = "lmd_stashdepo_zone";
	zone->parent = ent;
	zone->think = lmd_stashdepozone_think;
	zone->nextthink = level.time + 1000;
	zone->s.eFlags = EF_NODRAW;
	zone->r.contents = 0;
	zone->clipmask = 0;
	zone->r.maxs[0] = ent->r.maxs[0] * 2;
	zone->r.maxs[1] = ent->r.maxs[1] * 2;
	zone->r.maxs[2] = ent->r.maxs[2];
	zone->r.mins[0] = ent->r.mins[0] * 2;
	zone->r.mins[1] = ent->r.mins[1] * 2;
	zone->r.mins[2] = 0;
	G_SetOrigin(zone, ent->r.currentOrigin);

	//10: entity number of stash deposit hackzone.
	ent->genericValue10 = zone->s.number;

	trap_LinkEntity(zone);

	return qtrue;
}

void lmd_stashdepo(gentity_t *ent){
	/*
	"Bank"
	You can set deposittime to 0 and have a trigger use this, if you dont want to require the player to use the entity directly.
	spawnflags:
		1: no deposit time.  Usefull if you want to make another non-player entity trigger this
		2: dont display stash status.  If set, the terminal will not tell the user if a stash is out when it is used.
		//4: dont show up in /stash command.
		128: start deactivated.  Must be used by a target_activate to be usable.
	keys:
		zone: only deposit stashes belonging to this zone.

		stashname: name to refer to the stashes with.  Only used for displaying the count of stashes (not used if spawnflag 2 is set).
		stashnameplural: same as stashname, but if there are more than 1 stashes

		message: message to print when used without a stash.

		profession: profession bitmask of who can use this.

		minlevel: min level needed to pick this up.
		maxlevel: any players above this level cannot pick this up.

		minstashes: players cannot pick this stash up until they have picked up this many stashes (from other spawns).
		maxstashes: players cannot pick this stash up if they have picked up more than this amount of stashes.

		depotimeoffset: offset to change the stash base deposit time by.  Can be negitive.  Stash depo time will not go below 0.
		depotimeoffsetrandom: random shift to change the deposit time by.

		bonuscredits: get extra credits for depositing here.  Can be negitive to remove credits from the stash.  Will not remove more credits than the stash has.
		bonusrandom: random amount to add to bonus credits.

		startdepomessage: message to add to the bottom of the startdepomessage of the stash.
		depositmessage: message to add to the bottom of the depositmessage of the stash.

		startdepotarget: target to use on deposit start
		deposittarget: target to use on deposit.

		mins/maxs/model

	Important: store and add a time delay for the depotimeoffsetrandom key, so a 
	player wont just keep stop and reuse to get a better time.  Clear it after a time and when someone 
	else trys to deposit.  Each time the origional depositer restarts and the time is not up, reset the timer.


	*/
	/*
		GenericValues
			1: minstashes
			2: maxstashes
			3: depotimeoffset
			4: depotimeoffsetrandom
			//5: last depotimeoffsetrandom value
			6: bonuscredits
			7: bonuscreditsrandom

			10: entity number of stash deposit hackzone.

		GenericStrings
			0: zone
			1: startdepomessage: message to add to the bottom of the startdepomessage of the stash.
			2: depositmessage: message to add to the bottom of the depositmessage of the stash.

		Targets:
			1: startdepotarget: target to use on deposit start
			2: deposittarget: target to use on deposit.
		Other:
			fullName: stashname: name to refer to the stashes with.  Only used for displaying the count of stashes (not used if spawnflag 2 is set).
			healingclass: stashnameplural: same as stashname, but if there are more than 1 stashes
	*/

	//GenericStrings[0]: zone: only deposit stashes belonging to this zone.
	G_SpawnString("zone", "", &ent->GenericStrings[0]);

	//fullName: stashname: name to refer to the stashes with.  Only used for displaying the count of stashes (not used if spawnflag 2 is set).
	G_SpawnString("stashname", "stash", &ent->fullName);
	//healingclass: stashnameplural: same as stashname, but if there are more than 1 stashes
	G_SpawnString("stashnameplural", "stashes", &ent->healingclass);

	//profession: profession bitmask of who can use this.
	//minlevel: min level needed to pick this up.
	//maxlevel: any players above this level cannot pick this up.
	PlayerUsableGetKeys(ent);

	//GenericValue1: minstashes: players cannot pick this stash up until they have picked up this many stashes (from other spawns).
	G_SpawnInt("minstashes", "0", &ent->genericValue1);
	//GenericValue2: maxstashes: players cannot pick this stash up if they have picked up more than this amount of stashes.
	G_SpawnInt("maxstashes", "0", &ent->genericValue2);

	//GenericValue3: depotimeoffset: offset to change the stash base deposit time by.  Can be negitive.  Stash depo time will not go below 0.
	G_SpawnInt("depotimeoffset", "0", &ent->genericValue3);
	//GenericValue4: depotimeoffsetrandom: random shift to change the deposit time by.
	G_SpawnInt("depotimeoffsetrandom", "0", &ent->genericValue4);

	//GenericValue6: bonuscredits: get extra credits for depositing here.  Can be negitive to remove credits from the stash.  Will not remove more credits than the stash has.
	G_SpawnInt("bonuscredits", "0", &ent->genericValue6);
	//GenericValue7: bonusrandom: random amount to add to bonus credits.
	G_SpawnInt("bonuscredits", "0", &ent->genericValue7);

	//GenericStrings[1]: startdepomessage: message to add to the bottom of the startdepomessage of the stash.
	G_SpawnString("startdepomessage", "", &ent->GenericStrings[1]);
	//GenericStrings[2]: depositmessage: message to add to the bottom of the depositmessage of the stash.
	G_SpawnString("depositmessage", "", &ent->GenericStrings[2]);


	//target: startdepotarget: target to use on deposit start
	if(ent->target)
		G_Free(ent->target);
	G_SpawnString("startdepotarget", "", &ent->target);

	//target2: deposittarget: target to use on deposit.
	if(ent->target2)
		G_Free(ent->target2);
	G_SpawnString("deposittarget", "", &ent->target2);

	if(!ent->model || !ent->model[0]){
		if(ent->model)
			G_Free(ent->model);
		ent->model = G_NewString2("models/map_objects/imperial/control_panel.md3");
	}
	SpawnEntModel(ent, qtrue, qfalse);

	if(!lmd_stashdepo_makezone(ent)){
		G_FreeEntity(ent);
		return;
	}

	ent->r.svFlags = SVF_PLAYER_USABLE;
	ent->use = lmd_stashdepo_use;

	trap_LinkEntity(ent);
}

