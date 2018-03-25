

#include "g_local.h"

int count_random_spots (void){
	int c = 0;
	int runs;
	gentity_t *ent;

	for(runs = 0;runs<2;runs++){
		ent = NULL;
		while((ent = G_Find(ent, FOFS(classname), "random_spot")) != NULL) {
			if(runs == 0 && ent->genericValue10 > level.time)
				continue;
			if(ent->enemy)
				continue;
			c++;
		}
		if(c > 0)
			break;
	}
	return c;
}

gentity_t* pick_random_spot (void){
	gentity_t *ent;
	int runs;
	int select;
	int i;
	int random_spots = count_random_spots();
	if (!random_spots) {
		return NULL;
	}

	for(runs = 0; runs < 2; runs++){
		i = MAX_CLIENTS;
		select = Q_irand(1, random_spots);
		ent = NULL;
		while((ent = G_Find(ent, FOFS(classname), "random_spot")) != NULL) {
			if((runs == 0 && ent->genericValue10 > level.time) || --select)
				continue;
			if(ent->enemy)
				continue;
			//can't be picked again in 10 minutes.
			ent->genericValue10 = level.time + 600000;
			return ent;
		}
	}
	return NULL;
}

void random_spot_think(gentity_t *ent) {
	if(ent->enemy && !ent->enemy->inuse)
		ent->enemy = NULL;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_random_spot (gentity_t *ent) {
	assert(ent && ent->inuse);
	if (g_gametype.integer != GT_FFA
		&& g_gametype.integer != GT_TEAM 
		&& g_gametype.integer != GT_HOLOCRON) {
			G_FreeEntity(ent);
			return;
	}
	if (ent->s.origin[0] == 0 &&
		ent->s.origin[1] == 0 &&
		ent->s.origin[2] == 0) {
			G_FreeEntity(ent);
			return;
	}

	G_Free(ent->classname);
	ent->classname = "random_spot";
	ent->s.eType = ET_INVISIBLE;
	ent->s.eFlags = EF_NODRAW;
	ent->clipmask = 0;
	ent->r.contents = 0;
	ent->think = random_spot_think;
	ent->nextthink = 0;
	ent->takedamage = qfalse;
	ent->health = 0;
	ent->targetname = NULL;
	ent->target = NULL;
	ent->script_targetname = NULL;
	ent->genericValue10 = 0;
	trap_LinkEntity(ent);
}
