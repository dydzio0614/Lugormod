
#include "g_local.h"

void Interact_Clear(gentity_t *ent) {
	//Ufo: what were these for?
	//ent->client->Lmd.interact.targ = NULL;
	//G_Free(ent->client->Lmd.interact.msg);
	memset(&ent->client->Lmd.interact, 0, sizeof(ent->client->Lmd.interact));
}

void Interact_Check(gentity_t *ent) {
	vec3_t len;
	//Ufo:
	if (!ent || !ent->client)
		return;
	if(ent->client->Lmd.interact.targ == NULL)
		return;
	if(ent->client->Lmd.interact.targ->flags & FL_INACTIVE || ent->client->Lmd.interact.targ->inuse == qfalse) {
		Interact_Clear(ent);
		return;
	}

	if(ent->client->Lmd.interact.radius > 0) {
		VectorSubtract(ent->s.origin, ent->client->Lmd.interact.origin, len);
		if(VectorLength(len) > ent->client->Lmd.interact.radius) {
			Interact_Clear(ent);
			return;
		}
	}

	if(ent->client->Lmd.interact.msg && ent->client->Lmd.interact.lastMsg + 900 <= level.time) {
		trap_SendServerCommand(ent->s.number, va("cp \"%s\"", ent->client->Lmd.interact.msg));
	}
}

void Interact_Set(gentity_t *ent, gentity_t *target, vec3_t origin, int radius, char *message) {
	vec3_t len;
	assert(target->interact);
	VectorSubtract(ent->s.origin, ent->client->Lmd.interact.origin, len);
	if(VectorLength(len) > ent->client->Lmd.interact.radius) {
		//Too far away, don't bother.
		return;
	}

	Interact_Clear(ent);

	ent->client->Lmd.interact.targ = target;
	ent->client->Lmd.interact.radius = radius;
	VectorCopy(origin, ent->client->Lmd.interact.origin);
	ent->client->Lmd.interact.msg = message;
}

gentity_t* AimAnyTarget (gentity_t *ent, int length);
qboolean PlayerUseableCheck(gentity_t *self, gentity_t *activator);
void Cmd_Interact_f(gentity_t *ent, int iArg){
	gentity_t *targ = ent->client->Lmd.interact.targ;
	if(!targ) {
		targ = AimAnyTarget (ent, 64);
		if(!targ)
			return;
		if(!PlayerUseableCheck(ent, targ))
			return;
	}
	if(targ->flags & FL_INACTIVE)
		return;
	if(targ->interact)
		targ->interact(targ, ent);
	else
		Disp(ent, "^3This is not an interactive terminal.");
}
