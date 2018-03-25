#include "g_local.h"

gentity_t *Lmd_logic_entity(int index);
int Lmd_logic_entityindex(gentity_t *ent);
int Lmd_logic_count();

gentity_t *GetEnt(int index) {
	if(index < 0)
		return NULL;
	if(index < MAX_GENTITIES)
		return &g_entities[index];
	else
		return Lmd_logic_entity(index - MAX_GENTITIES);
}

inline gentity_t *IterateEnts(gentity_t *from) {

	if(!from) {
		from = &g_entities[0];
		if(from->inuse)
			return from;
	}

	if(from >= g_entities && from < &g_entities[level.num_entities]) {
		from++;
		while(from >= g_entities && from < &g_entities[level.num_entities]) {
			if(!from->inuse) {
				from++;
				continue;
			}
			return from;
		}
		//Only do this if we transfer from normal to logical.
		from = GetEnt(MAX_GENTITIES);
		if(from->inuse)
			return from;
	}
	
	if(from->s.number >= MAX_GENTITIES) {
		while(from = Lmd_logic_entity(from->s.number - MAX_GENTITIES + 1)) {
			if(!from->inuse) {
				continue;
			}
			return from;
		}
	}

	return NULL;
}

void G_InitGentity( gentity_t *e ) {
	//RoboPhred
	int number;
	//sometimes called on active entities
	if(e->inuse == qfalse) {
		//Is needed to solve odd issues with in-place respawns.
		//specifically on misc_turretG2 enemy and model angles with spawnstring edit.
		memset(e, 0, sizeof(gentity_t));
		number = Lmd_logic_entityindex(e);
		if(number > -1) {
			e->s.number = MAX_GENTITIES + number;
			e->Lmd.logical = qtrue;
		}
		else
			e->s.number = e - g_entities;
	}

	e->inuse = qtrue;
	e->classname = "noclass";

	e->r.ownerNum = ENTITYNUM_NONE;
	e->s.modelGhoul2 = 0; //assume not
	trap_ICARUS_FreeEnt( e );	//ICARUS information must be added after this point
}

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/

gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match)
{
	char	*s;

	if(!match)
		return NULL;

	while(from = IterateEnts(from)) {
		s = *(char **) ((byte *)from + fieldofs);
		if(s && Q_stricmp(s, match) == 0)
			return from;
	}

	return NULL;
}

int EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount, qboolean logical) {
	int count = trap_EntitiesInBox(mins, maxs, list, maxcount);
	if(logical) {
		int logicals = Lmd_logic_count();
		int i;
		gentity_t *check;
		for(i = 0; i < logicals; i++) {
			if(count == maxcount)
				break;
			check = GetEnt(MAX_GENTITIES + i);
			if(trap_EntityContact(mins, maxs, check) == qtrue) {
				list[count] = check->s.number;
				count++;
			}
		}
	}
	return count;
}
/*
============
G_RadiusList - given an origin and a radius, return all entities that are in use that are within the list
============
*/
int G_RadiusList ( vec3_t origin, float radius,	gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES], qboolean logical)					  
{
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;

	if ( radius < 1 ) 
	{
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES, logical );

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		ent = GetEnt(entityList[e]);

		if ((ent == ignore) || !(ent->inuse) || ent->takedamage != takeDamage)
			continue;

		//RoboPhred: Find point entities
		if(VectorCompare(ent->r.absmin, vec3_origin) && VectorCompare(ent->r.absmax, vec3_origin)) {
			if(ent->r.svFlags & SVF_USE_CURRENT_ORIGIN)
				VectorCopy(ent->r.currentOrigin, v);
			else
				VectorCopy(ent->s.origin, v);
			VectorSubtract(v, origin, v);
		}
		else {
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( origin[i] < ent->r.absmin[i] ) 
				{
					v[i] = ent->r.absmin[i] - origin[i];
				} else if ( origin[i] > ent->r.absmax[i] ) 
				{
					v[i] = origin[i] - ent->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) 
		{
			continue;
		}

		// ok, we are within the radius, add us to the incoming list
		ent_list[ent_count] = ent;
		ent_count++;

	}
	// we are done, return how many we found
	return(ent_count);
}
