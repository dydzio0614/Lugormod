
#ifdef LMD_EXPERIMENTAL

#include "g_local.h"

/*
For spawn rate:
should always keep a few modifier crystals out
*/
#define HOLOCRON_CLASSNAME "prof_jedi_holocron"

#define MIN_HOLOCRONS 1
#define MAX_HOLOCRONS 2
#define HOLOCRONS_COUNT(numJedi) ((numJedi < 4) ? MIN_HOLOCRONS : MAX_HOLOCRONS)

#define MAX_HOLOCRON_TIME 10 * 60 * 1000 //600000
#define MIN_HOLOCRON_TIME 60 * 1000 //60000
#define HOLOCRON_JEDI_MAXCOUNT 8
#define HOLOCRON_JEDI_OFFSET (MAX_HOLOCRON_TIME - MIN_HOLOCRON_TIME) / HOLOCRON_JEDI_MAXCOUNT//67500 max at 8 jedi


#define MIN_CRYSTALS 2
/*
4: 2
6: 3
7: 4
8: 5
*/
#define MAX_CRYSTALS 7


extern char *holocronTypeModels[];

unsigned int Jedi_Count();
gentity_t* pick_random_spot (void);

int PickHolocronType() {
	/*
	Pick the forcepower that the least people have?
	unsigned int counts[NUM_FORCE_POWERS];
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		
	}
	*/
	int fp = Q_irand(FP_FIRST, FP_SABER_OFFENSE);
	if(fp == FP_SABER_OFFENSE)
		return FP_SABERTHROW;

	int start = fp;
	gentity_t *found = NULL;
	qboolean loop = qtrue;
	while(loop) {
		loop = qfalse;
		while((found = G_Find(found, FOFS(classname), HOLOCRON_CLASSNAME)) != 0) {
			if(found->count == fp) {
				fp++;
				if(fp == start)
					return -1; //bail out, all are in use
				if(fp == FP_SABER_OFFENSE)
					fp = FP_SABERTHROW;
				else if(fp == FP_SABERTHROW)
					fp = FP_FIRST;
				loop = qtrue;
				break;
			}
		}
	}
	//currently attack and def are set to 3 for free.

	return fp;
}

qboolean SpawnHolocron() {
	int type = PickHolocronType();
	if(type < 0)
		return qfalse;

	gentity_t *spot = pick_random_spot();
	if(!spot)
		return qfalse;

	gentity_t *ent = G_Spawn();
	if(!ent)
		return qfalse;

	spot->enemy = ent;

	G_SetOrigin(ent, spot->s.origin);

	ent->classname = HOLOCRON_CLASSNAME;

	//these two lines ajust glowey effect.  Should show it for "special" holocrons.
	//first one is 1 one dark, 2 light, 3 neutral
	ent->s.trickedentindex3 = 3;
	//holocron type
	ent->s.trickedentindex4 = ent->count = type;

	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID | CONTENTS_BODY;

	ent->r.svFlags |= SVF_BROADCAST;
	ent->s.eFlags2 |= EF2_CANSEE;

	ent->s.modelindex = G_ModelIndex(holocronTypeModels[ent->count]);
	ent->s.eType = ET_HOLOCRON;

	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	VectorSet( ent->r.maxs, 8, 8, 8 );
	VectorSet( ent->r.mins, -8, -8, -8 );

	ent->s.apos.trType = TR_LINEAR;
	ent->s.apos.trDelta[1] = 180;
	ent->s.apos.trDuration = 3200;
	ent->s.apos.trTime = level.time;

	ent->s.pos.trType = TR_SINE;
	ent->s.pos.trDelta[2] = 5;
	ent->s.pos.trDuration = 3900;
	ent->s.pos.trTime = level.time;

	trap_LinkEntity(ent);
	return qtrue;
}

unsigned int CountHolocrons() {
	unsigned int i, count = 0;
	for(i = MAX_CLIENTS; i < MAX_GENTITIES; i++) {
		if(!g_entities[i].inuse)
			continue;
		if(Q_stricmp(g_entities[i].classname, HOLOCRON_CLASSNAME) == 0)
			count++;
	}
	return count;
}

unsigned int lastHolocronSpawn = 0;
void CheckHolocronSpawn(int numJedi) {
	if(CountHolocrons() > HOLOCRONS_COUNT(numJedi))
		return;

	int time = MIN_HOLOCRON_TIME;
	time += numJedi * HOLOCRON_JEDI_OFFSET;
	if(time > MAX_HOLOCRON_TIME)
		time = MAX_HOLOCRON_TIME;
	if(lastHolocronSpawn != 0 && level.time - lastHolocronSpawn < time)
		return;
	if(SpawnHolocron())
		lastHolocronSpawn = level.time;
}

unsigned int lastCrystalSpawn = 0;


void CheckJediItemSpawn() {
	int count = Jedi_Count();
	CheckHolocronSpawn(count);
}



void JediHoloTest(gentity_t *player) {


	gentity_t *ent = G_Spawn();
	if(!ent)
		return;

	VectorCopy(player->r.currentOrigin, ent->s.origin);
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	ent->count = atoi(arg);

	vec3_t dest;
	trace_t tr;

	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	//The hell, is this used?
	ent->s.isJediMaster = qtrue;

	VectorSet( ent->r.maxs, 8, 8, 8 );
	VectorSet( ent->r.mins, -8, -8, -8 );

	//ent->s.origin[2] += 0.1;
	ent->s.origin[2] -= ent->r.mins[2] - 8.1;
	ent->r.maxs[2] -= 0.1;

	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
	if ( tr.startsolid )
	{
		G_Printf ("SP_misc_holocron: misc_holocron startsolid at %s\n", vtos(ent->s.origin));
		G_FreeEntity( ent );
		return;
	}

	//add the 0.1 back after the trace
	ent->r.maxs[2] += 0.1;

	G_SetOrigin( ent, tr.endpos );

	if (ent->count < 0)
	{
		ent->count = 0;
	}

	if (ent->count >= NUM_FORCE_POWERS)
	{
		ent->count = NUM_FORCE_POWERS-1;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex(holocronTypeModels[ent->count]);//(ent->count - 128);
	ent->s.eType = ET_HOLOCRON;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;

	ent->s.trickedentindex4 = ent->count;

	if (forcePowerDarkLight[ent->count] == FORCE_DARKSIDE)
	{
		ent->s.trickedentindex3 = 1;
	}
	else if (forcePowerDarkLight[ent->count] == FORCE_LIGHTSIDE)
	{
		ent->s.trickedentindex3 = 2;
	}
	else
	{
		ent->s.trickedentindex3 = 3;
	}

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	trap_LinkEntity(ent);

}

#endif
