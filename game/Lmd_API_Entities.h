
#include "Lmd_Entities_Public.h"

#include "gentity_t.h"

#define LMDAPI_ENTITIES_VERSION_1 (0)

#define LMDAPI_ENTITIES_VERSION_CURRENT LMDAPI_ENTITIES_VERSION_1

/*
Functions for manipulating entities.

The entities api will be considered incompatible to older versions whenever the gentity_t structure changes.
*/
typedef struct LmdApi_Entities_v1_s {
	// Creates a new entity
	gentity_t* (*createEntity)();

	// Frees an entity, and attempt to free any data on it.
	void (*freeEntity)(gentity_t *ent);

	// Gets the specified entity (logical or non-logical) from the entity number.
	// This function is safe, and will return NULL on an invalid entity number.
	gentity_t* (*getEntity)(int entityNumber);

	// Returns the entity following 'last', or NULL if no more entities.
	// Pass NULL into 'last' to get the first entity.
	// This will return entities in order of entity number, and return logical entities after game entities.
	gentity_t* (*iterateEntities)(gentity_t *last);


	// Use all entities with the given targetname on behalf of the entity and activator.
	void (*useTargets)(gentity_t *ent, gentity_t *activator, const char *targetname);


	// Entity spawning

	/*
	Registers an entity with the spawn system.  An entity registered in this way
	will be spawnable with "/place", and will save and load through map files.

	If the entity already exists, its spawn data will be overridden.

	To ensure the entity loads with the map, this should always be called before any calls to GAME_INIT

	Note that the 'info' argument must remain available while the server is running.  It is recommended to point this
	at a global variable.  If the value goes out of scope, crashes are likely.
	*/
	void (*registerSpawnableEntity)(char *name, void (*spawn)(gentity_t *ent), const entLogicalData_t logical, const entityInfo_t *info);

	// Send the message to the user who tried to spawn the entity, or to the console if
	// this entity was spawned by the initial map load.
	void (*reportSpawnFailure)(const char *msg);

	// Reads a spawn key and writes it to the output.
	qboolean (*readSpawnValueString)(const char *key, const char *defaultString, char **out);

	// Reads a spawn key as an int and writes it to the output.
	qboolean (*readSpawnValueInt)(const char *key, const char *defaultString, int *out);

	// Reads a spawn key as a float and writes it to the output.
	qboolean (*readSpawnValueFloat)(const char *key, const char *defaultString, float *out);

	// Reads a spawn key as a vec3_t and writes it to the output.
	qboolean (*readSpawnValueVec3)( const char *key, const char *defaultString, vec3_t out );

	// Clears out any pending spawn key/value pairs.
	void (*clearSpawnKeys)();

	// Adds a key/value pair to the list of pairs to spawn an entity.
	void (*addSpawnKey)(char *key, char *value);

	// Spawns an entity from key/value pairs.
	gentity_t* (*spawnFromKeys)();
} LmdApi_Entities_v1_t;


// Represents the current memory function list at the time of compile.
#define LmdApi_Entities_t LmdApi_Entities_v1_t

#ifdef LUGORMOD
const void *LmdApi_Get_Entities(unsigned int version);
#else
// Fetch the current function list at the time of compile.
// May return NULL if the version is no longer supported.
#define LmdApi_GetCurrent_Entities() (LmdApi_Entities_t*) LmdApi_Get_Entities(LMDAPI_MEMORY_VERSION_CURRENT)
#endif
