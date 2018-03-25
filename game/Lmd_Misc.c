
#include "g_local.h"


void MouseDroid_Spawn(gentity_t *ent) {
	/*
0x01b1d5b4
    [0]: -12.031250
    [1]: -6.0625000
    [2]: -24.031250
maxs
0x01b1d5c8
    [0]: 4.5156250
    [1]: 5.5312500
    [2]: -11.828125
	*/
	ent->s.modelindex = G_ModelIndex("models/players/mouse/lower.md3");
	VectorSet(ent->r.mins, -12, -6, -24);
	VectorSet(ent->r.maxs, 4, 5, -11);

	ent->r.contents = CONTENTS_SOLID;
	ent->physicsObject = qtrue;
	ent->bounceCount = 1;
	ent->s.apos.trType = TR_INTERPOLATE;
	ent->s.apos.trTime = level.time;
	ent->s.pos.trType = TR_INTERPOLATE;
	ent->s.pos.trTime = level.time;
}