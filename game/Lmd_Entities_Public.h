#ifndef LMD_ENTITIES_PUBLIC_H
#define LMD_ENTITIES_PUBLIC_H

#include "q_shared.h"
#include "gentity_t.h"

typedef struct entityInfoData_s {
	const char *key;
	const char *value;
} entityInfoData_t;

typedef struct {
	const char *description;
	const entityInfoData_t *spawnflags; // Terminate array with {NULL}
	const entityInfoData_t *keys; // Terminate array with {NULL}
} entityInfo_t;

typedef struct entLogicalData_s{
	// Whether this entity can be logical.
	// If qtrue, the entity will be logical if 'check()' is NULL or returns qtrue.
	qboolean allow;

	// Called before creating the entity to determine if it should be logical.
	// All keys and values are available through G_SpawnString / G_SpawnInt and other calls.
	// If this is NULL, then the entity will always be logical, provided 'allow' is qtrue.
	qboolean (*check)(); 
}entLogicalData_t;

typedef struct {
	char *name;
	void (*spawn)(gentity_t *ent);
	const entLogicalData_t logical;
	const entityInfo_t *info;
}spawn_t;

// Macros to simplify creation of always-logical or never-logical ents.
#define Logical_True {qtrue, NULL}
#define Logical_False {qfalse, NULL}

#endif