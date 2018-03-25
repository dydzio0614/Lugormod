
#include "g_local.h"
#include "Lmd_Entities_Public.h"

#include "Lmd_Actor.h"

extern stringID_table_t animTable[MAX_ANIMATIONS+1];


#define ACTOR_ATTRIBUTE_LIST \
	{"ModelScale", "The scale to apply to the model."}, \
	{"LegsAnim", "The text name of a legs animation to apply.  Use the admin command \'/anims' to find an animation."}, \
	{"TorsoAnim", "The text name of a torso animation to apply.  Use the admin command \'/anims' to find an animation."}, \
	{"Color", "The custom RGB color of the model, if available."}

void ParseActorAttributeEntityKeys(gentity_t *self) {
	char *arg;

	//G_SpawnString("skin", "", &self->GenericStrings[0]); //Ufo: not needed here

	G_SpawnString("torsoAnim", "", &arg);
	self->genericValue1 = GetIDForString(animTable, arg);
	G_Free(arg);

	G_SpawnString("legsAnim", "", &arg);
	self->genericValue2 = GetIDForString(animTable, arg);
	G_Free(arg);

	//Ufo:
	G_SpawnVector("color", "0 0 0", self->genericVec1);
}

void ApplyActorAttributeEntityKeys(gentity_t *self, gentity_t *actor) {

	if(self->modelScale[0] > 0)
		Actor_SetScale(actor, self->modelScale[0]);
	
	if(self->genericValue1 > -1) 
		Actor_SetAnimation_Legs(actor, self->genericValue1, -1);

	if(self->genericValue2 > -1) 
		Actor_SetAnimation_Torso(actor, self->genericValue2, -1);

	//Ufo:
	actor->client->ps.customRGBA[0] = self->genericVec1[0];
	actor->client->ps.customRGBA[1] = self->genericVec1[1];
	actor->client->ps.customRGBA[2] = self->genericVec1[2];

	//Actor_SetSpeed(actor, self->speed); //Ufo: not needed here
}

const entityInfoData_t lmd_actor_spawnflags[] = {
	{"1", "Drop to floor."},
	{NULL, NULL}
};
const entityInfoData_t lmd_actor_keys[] = {
	{"NPC_TargetName", "The name to use for lmd_actor_modify or lmd_actor_action."},
	{"Target", "The target to use after spawning an actor."},
	{"Model", "The model to use for this actor."},
	{"Skin", "The model skin to use for this actor."},
	ACTOR_ATTRIBUTE_LIST,
	NULL
};

entityInfo_t lmd_actor_info = {
	"An interactive non-player character controllable by the lmd_actor_* entities.\n"
	"THIS IS A PREVIEW.  The actor system is not yet complete.  This will be greatly expanded in the future.\n"
	"As of now, the actor is only a standing immoble model.  Future additions will allow it to be controlled."
	"Fore more information, visit\n"
	"lugormod.robophreddev.net/tasks/task/interactive-non-player-characters-actors",
	lmd_actor_spawnflags,
	lmd_actor_keys,
};

void lmd_actor_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	gentity_t *actor = Actor_Create(self->model, self->GenericStrings[0], self->s.origin, self->s.angles);
	if(!actor)
		return;

	actor->fullName = self->fullName;
	actor->NPC_targetname = self->NPC_targetname;
	actor->parent = self;

	ApplyActorAttributeEntityKeys(self, actor);

	//Test for drop to floor
	if ( self->spawnflags & 1 ) {
		trace_t		tr;
		vec3_t		bottom;

		VectorCopy( actor->r.currentOrigin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		trap_Trace( &tr, actor->r.currentOrigin, actor->r.mins, actor->r.maxs, bottom, actor->s.number, MASK_NPCSOLID );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 ) {
			Actor_SetOrigin(actor, tr.endpos);
		}
	}
	G_UseTargets(self, activator);
}

void lmd_actor(gentity_t *self) {

	if ( !self->fullName || !self->fullName[0])
		self->fullName = "Actor";

	if(!self->model || !self->model[0]) {
		EntitySpawnError("lmd_actor requires a model key.");
		G_FreeEntity(self);
		return;
	}
	ParseActorAttributeEntityKeys(self);

	G_SpawnString("skin", "", &self->GenericStrings[0]);

	if(!self->GenericStrings[0][0])
		self->GenericStrings[0] = "default";

	if(self->targetname)
		self->use = lmd_actor_use;
	else
		lmd_actor_use(self, NULL, self);
}

const entityInfoData_t lmd_actor_modify_keys[] = {
	{"NPC_Target", "The name of the actors to affect."},
	{"Target", "This target will be fired once this entity finishes applying all modifications."},
	{"Origin", "If non-zero, the actor will be teleported to this origin."},
	{"Angles", "If non-zero, the actor will be turned to face these angles."},
	ACTOR_ATTRIBUTE_LIST,
	{NULL, NULL}
};
entityInfo_t lmd_actor_modify_info = {
	"Change actor attributes on the fly. All actors matching the NPC_Target with their NPC_TargetName will be affected.",
	NULL,
	lmd_actor_modify_keys,
};

//Ufo: redesigned a bit
void lmd_actor_modify_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	gentity_t *actor = NULL;
	while(actor = G_Find(actor, FOFS(classname), "Actor")) {
		if(Q_stricmp(actor->NPC_targetname, self->NPC_target) != 0)
			continue;

		if (self->s.origin[0] || self->s.origin[1] || self->s.origin[2])
			Actor_SetOrigin(actor, self->s.origin);

		if (self->s.angles[0] || self->s.angles[1] || self->s.angles[2])
			Actor_SetAngles(actor, self->s.angles);

		ApplyActorAttributeEntityKeys(self, actor);
	}
	G_UseTargets(self, activator);
}

void lmd_actor_modify(gentity_t *self) {
	if(!self->NPC_target || !self->NPC_target[0]) {
		EntitySpawnError("lmd_actor_modify requires the NPC_Target key.");
		G_FreeEntity(self);
		return;
	}
	ParseActorAttributeEntityKeys(self);
	self->use = lmd_actor_modify_use; //Ufo: was missing
}