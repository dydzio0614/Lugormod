
#include "Lmd_API_Entities.h"

#include "g_local.h"

void Lmd_AddSpawnableEntry(spawn_t spawnData);

void LmdApi_Entities_ClearSpawnKeys() {
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;
}

void AddSpawnField(char *field, char *value);
void LmdApi_Entities_AddSpawnKey(char *key, char *value) {
	AddSpawnField(key, value);
}

gentity_t* G_SpawnGEntityFromSpawnVars(qboolean inSubBSP);
gentity_t *LmdApi_Entities_SpawnFromKeys() {
	if (level.numSpawnVars == 0) {
		return NULL;
	}
	level.spawning = qtrue;
	gentity_t *ent = G_SpawnGEntityFromSpawnVars(qfalse);
	level.spawning = qfalse;
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;
	return ent;
}

void LmdApi_Entities_RegisterSpawnableEntity(char *name, void (*spawn)(gentity_t *ent), const entLogicalData_t logical, const entityInfo_t *info) {
	spawn_t data = {name, spawn, logical, info};

	// This will make a new copy of data.
	Lmd_AddSpawnableEntry(data);
}

LmdApi_Entities_v1_t api_entities_v1 = {
	// Creates a new entity
	G_Spawn,

	// Frees an entity, and attempt to free any data on it.
	G_FreeEntity,

	// Gets the specified entity (logical or non-logical) from the entity number.
	GetEnt,

	// Returns the entity following 'last', or NULL if no more entities.
	// Pass NULL into 'last' to get the first entity.
	// This will return entities in order of entity number, and return logical entities after game entities.
	IterateEnts,

	// Use all entities with the given targetname on behalf of the entity and activator.
	G_UseTargets2,


	// Entity spawning

	/*
	Registers an entity with the spawn system.  An entity registered in this way
	will be spawnable with "/place", and will save and load through map files.
	*/
	LmdApi_Entities_RegisterSpawnableEntity,

	// Send the message to the user who tried to spawn the entity, or to the console if
	// this entity was spawned by the initial map load.
	EntitySpawnError,

	// Reads a spawn key and writes it to the output.
	G_SpawnString,

	// Reads a spawn key as an int and writes it to the output.
	G_SpawnInt,

	// Reads a spawn key as a float and writes it to the output.
	G_SpawnFloat,

	// Reads a spawn key as a vec3_t and writes it to the output.
	G_SpawnVector,

	// Clears out any pending spawn key/value pairs.
	LmdApi_Entities_ClearSpawnKeys,

	// Adds a key/value pair to the list of pairs to spawn an entity.
	AddSpawnField,

	// Spawns an entity from key/value pairs.
	LmdApi_Entities_SpawnFromKeys,
};

const void *LmdApi_Get_Entities(unsigned int version) {
	if (version == LMDAPI_ENTITIES_VERSION_1) {
		return &api_entities_v1;
	}

	return NULL;
}