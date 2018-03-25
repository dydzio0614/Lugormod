#include "g_local.h"

gentity_t *Lmd_logic_entity(int index);

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Stats.h"
#include "Lmd_Professions.h"
#include "Lmd_Prof_Merc.h"

//RoboPhred
extern vmCvar_t lmd_stashdepotime;
extern vmCvar_t lmd_stashrate;
//Ufo:
extern vmCvar_t lmd_stashcr;

//RoboPhred
#define MONEY_DISP_TIME lmd_stashrate.integer
//#define MONEY_DISP_TIME 60000 //60 s
//#define MONEY_DISP_TIME 30000 //30 s
//RoboPhred:
#define MONEY_HACKING_TIME lmd_stashdepotime.integer
//#define MONEY_HACKING_TIME 15000 //15 s
//#define STASH_CASH 10 // credits/player
//Ufo:
#define STASH_CASH lmd_stashcr.integer // credits/player

int G_CountHumanPlayers (int team);
gentity_t *current_stash = NULL;
void make_money_stash (void);

int moneyDisps(void){
	int i = MAX_CLIENTS;
	gentity_t *ent;
	int count = 0;

	while (i++ < level.num_entities) {//MAX_GENTITIES) {
		ent = &g_entities[i];
		if (!ent || !ent->inuse) {
			continue;
		}
		if (Q_stricmp(ent->classname, "money_dispenser") == 0){
			count++;
		}
	}
	return count;
}

/*
void money_stash_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) 
{
G_FreeEntity(self);
}
*/

void money_stash_think (gentity_t *ent){
	qboolean respawn = qfalse;
	if(!ent->parent){
		//RoboPhred: timeout
		if(ent->timestamp > 0 && ent->timestamp < level.time)
			respawn = qtrue;

		//Ufo: players abused these methods to respawn a stash with better CR count
		/*
		trace_t tr;

		//RoboPhred
		int ents[MAX_GENTITIES];
		int entCount;
		int i;

		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, 
			CONTENTS_LAVA |CONTENTS_SLIME | MASK_SOLID | MASK_PLAYERSOLID);
		if(tr.startsolid && tr.entityNum >= MAX_CLIENTS)
			respawn = qtrue;

		//RoboPhred
		if(trap_PointContents(ent->r.currentOrigin, ent->s.number) & CONTENTS_NODROP)
			respawn = qtrue;
		entCount = trap_EntitiesInBox(ent->r.absmin, ent->r.absmax, ents, MAX_GENTITIES);
		for(i = 0;i<entCount;i++){
			gentity_t *check = &g_entities[ents[i]];
			if(check->inuse == qfalse)
				continue;
			if(Q_stricmp(check->classname, "trigger_hurt") == 0){
				respawn = qtrue;
				break;
			}
		}
		*/
	}

	if(ent->activator) {
		if(ent->activator->client->ps.fallingToDeath)
			respawn = qtrue;
		if(ent->activator->health <= 0) {
			//wait some time to see if we are dropped properly.
			if(ent->attackDebounceTime <= 0)
				ent->attackDebounceTime = level.time + 300;
			if(ent->attackDebounceTime < level.time) {
				G_Printf("^3Money stash on a dead player was not dropped, respawning...\n");
				respawn = qtrue;
			}
		}
	}
	ent->attackDebounceTime = 0;

	if(respawn){
		if (ent->activator && ent->activator->client) {
			ent->activator->client->Lmd.moneyStash = NULL;
		}
		G_FreeEntity(ent);
		make_money_stash();
		return;
	}

	//RoboPhred: make sure they still have it.
	if(ent->activator && ent->activator->client)
		ent->activator->client->ps.powerups[PW_NEUTRALFLAG] = INT_MAX;

	ent->nextthink = level.time + 100;
}

void money_stash_touch (gentity_t *self, gentity_t *other, trace_t *trace){
	if (!other || !other->client || other->NPC || other->r.svFlags & SVF_BOT){
		return;
	}

	if (PlayerAcc_Prof_GetProfession(other) == PROF_ADMIN){
		return;
	}

	if(duelInProgress(&other->client->ps))
		return;

	if(other->health < 1) //dead people can't pick us up.
		return;

	if(other->client->Lmd.moneyStash ) //this guy's already carrying a stash
		return;

	if ( other->client->ps.pm_type == PM_SPECTATOR)//spectators don't pick stuff up
		return;

	if(other->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)
		return;
	
	//Ufo:
	if(other->client->invulnerableTimer > level.time)
		return;

	if(other->client->pers.Lmd.persistantFlags & SPF_IONLYSABER)
		return;

	if(other->client->pers.Lmd.persistantFlags & SPF_CORTOSIS)
		return;

	//RoboPhred: still recovering from a previous action, cant pick it up.
	//Ufo:
	if(other->client->ps.weaponTime > 0 && !(other->client->Lmd.restrict & 16))
		return;
	//RoboPhred: stunned, cant pick it up.
	if(other->client->ps.electrifyTime >= level.time)
		return;

	//no unreged players.
	if(!other->client->pers.Lmd.account)
		return;

	//RoboPhred: no stash for protected meditators.
	if(g_meditateProtect.integer && other->client->ps.legsAnim == BOTH_MEDITATE && other->client->ps.torsoAnim == BOTH_MEDITATE)
		return;

	if (self->genericValue10 > level.time && self->genericValue11 == other->s.number) {
		//don't pick it up if you just dropped it
		return;
	}

	if (self->noise_index) {
		G_Sound(other, CHAN_AUTO, self->noise_index);
	}

	if (self->count < 10) {
		int ammount = 0;
		int maxamt = 3;
		ammount = G_CountHumanPlayers (-1) * STASH_CASH;
		while (!Q_irand(0,3) && maxamt--) {
			ammount *= 2;
		}
		self->count = ammount;
	}

	other->client->ps.eFlags2 |= EF2_CANSEE;
	other->client->Lmd.moneyStash = self;
	self->s.eFlags |= EF_NODRAW;
	self->r.contents = 0;
	self->activator = other;
	trap_LinkEntity(self);

	//RoboPhred: number of times picked up
	self->genericValue1++;

	other->client->ps.powerups[PW_NEUTRALFLAG] = INT_MAX;
	//other->client->ps.isJediMaster = qtrue;
	//G_GlobalSound( other, CHAN_ANNOUNCER, G_SoundIndex( "sound/effects/mpalarm" ) );

	//Ufo:
	trap_SendServerCommand(-1, va("print \"%s ^3picked up a money stash containing ^2CR %i\n\"", other->client->pers.netname, self->count));
	trap_SendServerCommand(other-g_entities, va("cp \"^3You picked up a money stash\n^3containing ^2CR %i\"", self->count));
	//self->s.boltToPlayer = other->s.number + 1;

	if(self->parent && self->parent->inuse && Q_stricmp(self->parent->classname, "random_spot") == 0 && self->parent->enemy == self) {
		self->parent->enemy = NULL;
	}
	self->parent = NULL;

}

//void SP_misc_model_breakable (gentity_t *ent);

//RoboPhred

int trap_RealTime( qtime_t *qtime );

void SP_money_stash (gentity_t *ent){
	//assert(ent && ent->inuse);
	if(!ent || !ent->inuse){
		return;
	}
	//ent->die = money_stash_die;
	ent->r.svFlags |= SVF_BROADCAST;
	ent->s.eFlags2 |= EF2_CANSEE;
	//ent->flags &= ~FL_DROPPED_ITEM;
	ent->flags |= FL_DROPPED_ITEM;
	ent->noise_index = G_SoundIndex("sound/interface/secret_area.wav");
	//ent->s.iModelScale = 25;
	G_Free(ent->classname);
	ent->classname = "money_stash";
	//ent->s.modelGhoul2 = 0;
	//ent->model = "models/map_objects/imperial/cargo_sm.md3";
	//ent->model = "models/map_objects/mp/crystal_blue.md3";
	//ent->s.apos.trBase[ROLL] += 90;
	//ent->model = "models/items/psgun.glm";
	ent->model = "models/items/datapad.glm";

	ent->s.modelindex = G_ModelIndex(ent->model);
	//trap_G2API_InitGhoul2Model(&ent->ghoul2, ent->model, 0, 0, 0, 0, 0);
	ent->s.modelGhoul2 = 1;
	//ent->s.g2radius = 64;

	VectorSet(ent->r.mins, -4, -4, -1);
	VectorSet(ent->r.maxs,  4,  4, 2);
	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.pos.trType = TR_STATIONARY;
	ent->touch = money_stash_touch;
	ent->r.contents = CONTENTS_TRIGGER | CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID | CONTENTS_BODY;
	ent->s.eType = ET_GENERAL;
	trace_t tr;
	vec3_t origin;
	ent->s.origin[2] += 16;
	VectorCopy(ent->s.origin, origin);
	origin[2] -= 512;
	trap_Trace (&tr, ent->s.origin, ent->r.mins, ent->r.maxs, origin, ent->s.number, MASK_PLAYERSOLID);

	G_SetOrigin(ent, tr.endpos);
	ent->count = 0;

	ent->think = money_stash_think;
	ent->nextthink = level.time + 100;

	current_stash = ent;
	trap_LinkEntity(ent);
}

int thereIsAMoneyStash(void){
	int i = 0;
	if (!current_stash || !current_stash->inuse || Q_stricmp("money_stash", current_stash->classname)) {
		current_stash = NULL;
		return -1;
	}

	if(current_stash->activator)
		return current_stash->activator->s.number;

	return current_stash->s.number;
}

gentity_t* pick_random_spot (void);
void make_money_stash (void){
	if((g_cmdDisable.integer * 2) & (1 << 8)){
		return;
	}
	gentity_t *ent, *stash;
	if (!(ent = pick_random_spot())) {
		return;
	}
	if(!(stash = G_Spawn())){
		return;
	}
	ent->enemy = stash;
	stash->parent = ent;
	VectorCopy(ent->s.origin, stash->s.origin);   
	SP_money_stash(stash);
	return;    
}

void resetStash() {
	if(current_stash != NULL){
		if(current_stash->activator) {
			current_stash->activator->client->Lmd.moneyStash = NULL;
			current_stash->activator->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		if(current_stash->parent)
			current_stash->parent->enemy = NULL;
		G_FreeEntity(current_stash);
	}
	make_money_stash();
}

//void AddPoints(gentity_t *ent, int points);

//RoboPhred
void depositMoneyStash(gentity_t *ent){
	if (current_stash == NULL || ent == NULL) {
		return;
	}

	int amount = current_stash->count;
	ent->client->Lmd.moneyStash = NULL;

	G_FreeEntity(current_stash);
	current_stash = NULL;

	ent->client->ps.powerups[PW_NEUTRALFLAG] = 0;
	ent->client->ps.eFlags2 &= ~EF2_CANSEE;

	PlayerAcc_Stats_SetStashes(ent, PlayerAcc_Stats_GetStashes(ent) + 1);

	PlayerAcc_SetCredits(ent, PlayerAcc_GetCredits(ent) + amount);

	trap_SendServerCommand(ent->s.number, va("cp \"^3You received ^2%i^3 credits.\"", amount));
	trap_SendServerCommand(-1, va("print \"%s ^3deposited the money stash.\n\"", ent->client->pers.netname));
	G_LogPrintf("%s deposited the money stash. CR %i.\n", ent->client->pers.netname, amount);
}

void money_dispenser_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	vec3_t dir;
	int diff;
	char *isthere;
	if (level.time < self->genericValue1) {
		return;
	}
	if (duelInProgress(&activator->client->ps)) {
		return;
	}
	self->genericValue1 = level.time + 100;

	VectorSubtract(activator->client->ps.origin, self->r.currentOrigin, dir);
	vectoangles(dir,dir);
	if (dir[YAW] > 180) {
		dir[YAW] -= 360; }
	diff = (int)Q_fabs(dir[YAW] - self->s.apos.trBase[YAW]);
	if (diff > 180) {
		diff = 360 - diff; }

	if ( diff > 40) {
		//face the front
		return;
	}

	if(!activator->client->pers.Lmd.account){
		trap_SendServerCommand(activator->s.number, "cp \"^3You must be logged in to use the bank terminal.\"");
		Disp(activator, "^3You must be logged in to use the bank terminal.");
		return;
	}

	if(thereIsAMoneyStash() == -1){
		isthere = "no";
	}
	else{
		isthere = "a";
	}

	if (!activator->client || !activator->client->Lmd.moneyStash || !activator->client->Lmd.moneyStash->inuse ||
		Q_stricmp ("money_stash", activator->client->Lmd.moneyStash->classname) || !current_stash ||
		activator->client->Lmd.moneyStash != current_stash){
			trap_SendServerCommand(activator-g_entities, va("cp \"^3You do not have any\n^3cash to deposit.\n^3Find a money stash.\n^5Currently there is %s money stash.\"", isthere));
			self->genericValue1 = level.time + 2000;
			return;
	}

	if (other == activator && MONEY_HACKING_TIME > 0){
		if (activator->client->isHacking != self->genericValue10){
			activator->client->isHacking = self->genericValue10;
			activator->client->ps.hackingTime = level.time + MONEY_HACKING_TIME;
			activator->client->ps.hackingBaseTime = MONEY_HACKING_TIME;
			VectorCopy(activator->client->ps.viewangles, activator->client->hackingAngles);
			if(self->activator != activator || self->aimDebounceTime < level.time) {
				trap_SendServerCommand(-1, va("print \"%s ^3started depositing money stash.\n\"",	activator->client->pers.netname));
				self->aimDebounceTime = level.time + 3000;
			}
			self->activator = activator;
			return;
		}
		else if (activator->client->ps.hackingTime > level.time && current_stash == activator->client->Lmd.moneyStash ){
			//is hacking
			return;
		}
		activator->client->ps.hackingTime = 0;
		activator->client->ps.hackingBaseTime = 0;
		activator->client->isHacking = 0;
	}

	self->nextthink = level.time + 1000;
	self->genericValue1 = level.time + 2000;

	depositMoneyStash(activator);
}

void money_dispenser_think (gentity_t *ent){
	if(thereIsAMoneyStash() == -1 &&  G_CountHumanPlayers(-1) > 1 && !Q_irand(0,3 * moneyDisps()))
		make_money_stash();
	ent->nextthink = level.time + MONEY_DISP_TIME + Q_irand(0, 10) * 1000;
}

void zone_think (gentity_t *ent){
	if (!ent->parent || !ent->parent->inuse || Q_strncmp(ent->classname, ent->parent->classname, strlen(ent->parent->classname)) ||
		ent->parent->genericValue10 != ent->s.number){
			G_FreeEntity(ent);
			return;
	}
	ent->nextthink = level.time + 5000;
}


void SP_money_dispenser (gentity_t *ent){
	if (g_gametype.integer != GT_FFA) {
		ent->s.eFlags = EF_NODRAW;
		ent->r.contents = 0;
		ent->clipmask = 0;
		return;

	}

	//ent->think = money_dispenser_gosolid;
	//ent->nextthink = level.time + 100;
	//ent->model = "models/map_objects/bounty/cache.md3";
	//ent->model = "map_objects/imperial/control_panel";
	ent->s.modelindex = G_ModelIndex("models/map_objects/imperial/control_panel.md3");
	G_SetOrigin( ent, ent->s.origin );
	//VectorCopy(ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	//ent->model = "map_objects/byss/control_panel";
	//SP_misc_model_breakable(ent);
	ent->use = money_dispenser_use;
	ent->r.svFlags = SVF_PLAYER_USABLE;
	ent->s.eType = ET_GENERAL;
	//ent->s.eFlags |= EF_NODRAW;
	//G_Free(ent->classname);
	//ent->classname = "money_dispenser";
	VectorSet(ent->r.mins, -30, -30, 0);
	VectorSet(ent->r.maxs, 30, 30, 50);

	ent->genericValue1 = 0;
	ent->think = money_dispenser_think;
	ent->nextthink = level.time + 1000;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_PLAYERSOLID;
	trap_LinkEntity(ent);
	//I need better rand_init
	qtime_t now;
	trap_RealTime(&now);
	Rand_Init( (60 * (60 * (24 *  now.tm_yday) 
		+ now.tm_hour) + now.tm_min) + now.tm_sec );
	gentity_t *zone = G_Spawn();
	if (!zone) {
		G_FreeEntity(ent);
		return;
	}
	zone->classname = "money_dispenser_zone";
	zone->parent = ent;
	zone->think = zone_think;
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
	ent->genericValue10 = zone->s.number;

	trap_LinkEntity(zone);
}

void hurl (gentity_t *ent, gentity_t *dropped);

//RoboPhred
void dropMoneyStash_Old(gentity_t *ent){
	//Lugormod hurl money stash if we got one
	if ((g_gametype.integer != GT_FFA && g_gametype.integer != GT_TEAM) || !ent->client ||
		ent->client->Lmd.moneyStash == NULL) {
			return;
	}

	gentity_t *dropped = ent->client->Lmd.moneyStash;
	if (!dropped->inuse || Q_stricmp("money_stash", dropped->classname)) {
		ent->client->Lmd.moneyStash = NULL;
		return;
	}


	//RoboPhred
	//dropped->think = stash_remove_and_spawn;
	dropped->timestamp = level.time + 300000; //five minutes
	dropped->r.ownerNum = ent->s.number;
	dropped->activator = NULL;
	dropped->clipmask = MASK_PLAYERSOLID;
	dropped->physicsBounce = 0.50;
	dropped->flags |= FL_DROPPED_ITEM;
	dropped->s.eFlags &= ~EF_NODRAW;
	hurl(ent, dropped);
	dropped->touch = money_stash_touch;
	dropped->r.contents = CONTENTS_TRIGGER|CONTENTS_SOLID;
	ent->client->ps.powerups[PW_NEUTRALFLAG] = 0;
	ent->client->Lmd.moneyStash = NULL;
	trap_SendServerCommand(-1, va("print \"%s ^3dropped the money stash.\n\"", ent->client->pers.netname));
	ent->client->ps.eFlags2 &= ~EF2_CANSEE;
}

int StashHolder(void){
	if(thereIsAMoneyStash() <= 0)
		return -2;

	if(current_stash->activator == NULL)
		return -1;

	if(current_stash->activator->s.number >= MAX_CLIENTS || !current_stash->activator->client)
		return -3;

	return current_stash->activator->s.number;
}

extern int merc_stash_range[6];
void TryStashGuide(gentity_t *ent) {
	int prof = PlayerAcc_Prof_GetProfession(ent);
	gclient_t *client = ent->client;
	if((prof == PROF_MERC || prof == PROF_ADMIN) && client->ps.zoomMode == 2 && client->Lmd.training.dispTime < level.time){
		int s = thereIsAMoneyStash();
		client->Lmd.training.dispTime = level.time + 2000;
		if(s > -1){
			vec3_t origin;                        
			int diff;
			VectorSubtract(g_entities[s].r.currentOrigin, client->ps.origin, origin);
			if (s < MAX_CLIENTS || VectorLength(origin) <= merc_stash_range[PlayerProf_Merc_GetStashRangeSkill(ent)]){
				G_ClientSound( ent, CHAN_ANNOUNCER, G_SoundIndex( "sound/interface/update" ) );
				vectoangles(origin,origin);
				AnglesSubtract(client->ps.viewangles,origin, origin);
				origin[ROLL] = 0;
				diff = (int)Q_fabs(VectorLength(origin));
				client->Lmd.training.dispTime = level.time + 250;
				client->Lmd.training.dispTime += diff * 10;
			}
		}
	}

	if ((prof == PROF_JEDI || prof == PROF_ADMIN) && (client->ps.fd.forcePowersActive & (1 << FP_SEE)) && 
		client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_3 && client->Lmd.training.dispTime < level.time)
	{
		int s = thereIsAMoneyStash();
		client->Lmd.training.dispTime = level.time + 950;
		if (s >= MAX_CLIENTS){
			vec3_t origin, upVec;
			VectorSet(upVec, -90,0,0);
			VectorSubtract(client->ps.origin, g_entities[s].r.currentOrigin, origin);
			if(s < MAX_CLIENTS || VectorLength(origin) <= 4 * 1024){
				//FIXME: display this entity to all using te->r.broadcastClients, rather than create a new entity.

				gentity_t *te = G_PlayEffectID(G_EffectIndex("env/tractor_beam"), g_entities[s].r.currentOrigin, upVec);
				te->r.svFlags |= SVF_SINGLECLIENT;
				te->r.singleClient = ent->s.number;
			}
		}
	}
}