
#include "g_local.h"

qboolean SpawnEntModel(gentity_t *ent, qboolean isSolid, qboolean isAnimated);

/*
lmd_turret

Multi purpose turret.
This can be controled by the player.  When spawnflag 2 is set, the turret
will aim where the player aims and shoot when the player fires.  The player will
still be able to move around and fire when this is active, so use a restrict or 
camera if you do not want this.

Spawnflags:
1: start on (if 2 is not present).
2: user controlable.

firerate: fire rate
firespread: random spread for shots
firespeed: speed of each shot
angles: default angles
pitch: min and max height the user can aim in, default is 0 (no limit)
yaw: min and max width the user can aim in, default is 0 (no limit)
aimspeed: aim speed

model
mins
maxs
modelangles: angle shift for model.


usability keys.
*/

void lmd_turret_player_think(gentity_t *ent) {

}

void lmd_turret_ai_think(gentity_t *ent) {

}

void lmd_turret(gentity_t *ent) {

	SpawnEntModel(ent, qtrue, qfalse);

	if(ent->spawnflags & 2)
		ent->think = lmd_turret_player_think;
	else
		ent->think = lmd_turret_ai_think;
}