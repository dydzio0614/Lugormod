
#include "g_local.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Professions.h"

qboolean PlayerItem_CanSpawnItem(gentity_t *player);
qboolean PlayerItem_Spawn(gentity_t *player, gentity_t *item);

void GiveCredits(gentity_t *ent, int cr, char *reason) {
	int cur = PlayerAcc_GetCredits(ent);
	if(cr < 0 && cur - cr < 0)
		cr = 0;

	else if(cr > 0) {
		char *msg = va("^3You received ^2CR %i^3 %s.", cr, (reason != NULL) ? reason : "");
		Disp(ent, msg);
		trap_SendServerCommand(ent->s.number, va("cp \"%s\"", msg));
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area.wav"));
	}
	else if(cr < 0){
		char *msg = va("^3You lost ^1CR %i^3 %s.", cr, (reason != NULL) ? reason : "");
		Disp(ent, msg);
		trap_SendServerCommand(ent->s.number, va("cp \"%s\"", msg));
	}

	PlayerAcc_SetCredits(ent, cur + cr);

}

void hurl (gentity_t *ent, gentity_t *dropped);
void Touch_Credits (gentity_t *ent, gentity_t *other, trace_t *trace) {

	if(ent->pain_debounce_time > level.time)
		return;
	if(!other || !other->client || other->NPC || other->r.svFlags & SVF_BOT)
		return;
	if (other->health < 1)
		return;
	if ( other->client->pers.connected != CON_CONNECTED)
		return;

	if (duelInProgress(&other->client->ps))
		return;


	if ( other->client->ps.pm_type == PM_SPECTATOR )
	{//spectators don't pick stuff up
		return;
	}

	if(ent->genericValue11 == other->s.number && ent->genericValue10 > level.time)
		return;

	ent->pain_debounce_time = level.time + 500;
	if(!other->client->pers.Lmd.account){
		trap_SendServerCommand(other->s.number, "cp \"^3You cannot pick up these credits.  You must register first.\"");
		return;
	}
	
	trap_SendServerCommand(other->s.number, va("cp \"^3You picked up CR ^2%i^3.\"", ent->count));
	G_Sound(other, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area.wav"));
	PlayerAcc_SetCredits(other, PlayerAcc_GetCredits(other) + ent->count);
	//RoboPhred
	G_UseTargets(ent, other);
	G_FreeEntity(ent);
}

qboolean DropCredits(gentity_t *ent, int amount){
	int credits = PlayerAcc_GetCredits(ent);
	gentity_t		*dropped;

	if (amount < 1) {
		return qfalse;
	}

	if (amount > credits) {
		amount = credits;
	}
	if (amount < 1) {
		Disp(ent, "^3You don't have any credits.");
		return qfalse;
	}

	if(!PlayerItem_CanSpawnItem(ent)){
		Disp(ent, "^3You have too many items out already.");
		return qfalse;
	}

	PlayerAcc_SetCredits(ent, credits - amount);

	if (!(dropped = G_Spawn())) {
		return qfalse;
	}

	VectorSet(dropped->r.mins, -4, -4, -1);
	VectorSet(dropped->r.maxs,  4,  4, 2);
	dropped->flags = FL_BOUNCE_HALF;
	dropped->s.eFlags &= ~(EF_NODRAW);
	dropped->s.modelindex = G_ModelIndex("models/items/datapad.glm");
	dropped->s.modelGhoul2 = 1;
	dropped->s.eType = ET_GENERAL;
	dropped->r.contents = CONTENTS_TRIGGER;
	dropped->clipmask = MASK_PLAYERSOLID;
	dropped->bounceCount = -5;
	dropped->physicsBounce = 0.50;		// items are bouncy
	dropped->parent = ent;
	dropped->classname = "credits";
	dropped->touch = Touch_Credits;
	dropped->flags |= FL_DROPPED_ITEM;
	dropped->count = amount;
	hurl(ent, dropped);


	PlayerItem_Spawn(ent, dropped);

	return qtrue;
}

int recallDroppedCredits(gentity_t *ent){
	int count = 0;
	//Account_t *acc = getAccount(ent);
	gentity_t *t = NULL;
	while ((t = G_Find(t, FOFS(classname), "credits")) != NULL) {
		if (t->parent != ent) {
			continue;
		}
		count++;
		PlayerAcc_SetCredits(ent, PlayerAcc_GetCredits(ent) + t->count);
		G_FreeEntity(t);
	}
	return count;
}

#define CREDITS_ADD_TIME 1800 //seconds
#define ADD_CREDITS 5

void creditsByTime(gentity_t *ent, int playtime){
	int credits = 0;
	int myTime = PlayerAcc_GetTime(ent);
	if(!myTime)
		return;

	if ((myTime % CREDITS_ADD_TIME) > ((myTime + playtime) % CREDITS_ADD_TIME)){
		credits++;
	}
	credits += (int)(floor((double)playtime/CREDITS_ADD_TIME));

	PlayerAcc_SetCredits(ent, PlayerAcc_GetCredits(ent) + ADD_CREDITS * credits * PlayerAcc_Prof_GetLevel(ent));
}

