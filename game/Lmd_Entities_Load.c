
#include "g_local.h"

#include "Lmd_Arrays.h"
#include "Lmd_Database.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_EntityCore.h"


#define entbasepath MapEntPath()
char *MapEntPath(void){
	static char path[MAX_QPATH] = "";
	if(!path[0])
		Lmd_Database_GetDataPath("mapentities", path, sizeof(path));
	return path;
}

typedef struct EntitySpawn_s{
	qboolean canSave;
	gentity_t *spawned;
	KeyPairSet_t keys;
}EntitySpawn_t;

struct EntitySpawns_s{
	unsigned int count;
	EntitySpawn_t **entities;
}EntitySpawns;

struct MapSpawns_s{
	unsigned int count;
	EntitySpawn_t **entities;
}MapSpawns;

void AddSpawnField(char *field, char *value);
gentity_t* G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );
qboolean ParseField(char **sourcep, char *dest, char start, char stop);
qboolean NextSection (char **sourcep);

qboolean disablesenabled = qtrue;
//extern qboolean disablesenabled;

unsigned int newSpawnstring(){
	unsigned int index;
	for(index = 0;index<EntitySpawns.count;index++){
		if(EntitySpawns.entities[index] == NULL)
			break;
	}
	if(index == EntitySpawns.count)
		index = Lmd_Arrays_AddArrayElement((void **)&EntitySpawns.entities, sizeof(EntitySpawn_t *), &EntitySpawns.count);
	EntitySpawns.entities[index] = (EntitySpawn_t *)G_Alloc(sizeof(EntitySpawn_t));
	memset(EntitySpawns.entities[index], 0, sizeof(EntitySpawn_t));
	return index;
}

unsigned int newMapSpawnstring(){
	//map spawnstrings are constant, so we dont need to scan for empty unused slots.
	unsigned int index = Lmd_Arrays_AddArrayElement((void **)&MapSpawns.entities, sizeof(EntitySpawn_t *), &MapSpawns.count);
	MapSpawns.entities[index] = (EntitySpawn_t *)G_Alloc(sizeof(EntitySpawn_t));
	memset(MapSpawns.entities[index], 0, sizeof(EntitySpawn_t));
	return index;
}

void removeSpawnstring(int index){
	Lmd_Pairs_Clear(&EntitySpawns.entities[index]->keys);
	G_Free(EntitySpawns.entities[index]);
	EntitySpawns.entities[index] = NULL;
}


int ParseEntitySpawnstring(char *string){
	int stringIndex = newSpawnstring();
	EntitySpawn_t *spawn = EntitySpawns.entities[stringIndex];
	Lmd_Pairs_ParseDatastring(&spawn->keys, string);
	return stringIndex;
}

gentity_t* G_SpawnGEntityPtrFromSpawnVars(gentity_t *ent, int customIndex, qboolean inSubBSP);

qboolean restrictSpawnErrors = qfalse;
gentity_t *spawnEntityFromIndex(int index){
	//do this before we spawn, an entity deleted on spawn will remove the spawnstring.
	char buf[MAX_STRING_CHARS];
	Lmd_Entities_getSpawnstring(index, buf, sizeof(buf));
	gentity_t *ent = spawnEntity(NULL, index);
	if(ent)
		return ent;
	else if(!restrictSpawnErrors){
		Com_Printf("^1Unable to spawn custom entity with spawnstring:\n%s\n\n", buf);
	}
	return NULL;
}

int Lmd_Entities_getSpawnstringKeyIndex(EntitySpawn_t *ent, char *key){
	int i;
	for(i = 0;i<ent->keys.count;i++){
		if(Q_stricmp(key, ent->keys.pairs[i].key) == 0){
			return i;
		}
	}
	return -1;
}

void SP_worldspawn(void);

qboolean entityIsWorldspawn(unsigned int index){
	int i = Lmd_Entities_getSpawnstringKeyIndex(EntitySpawns.entities[index], "classname");
	if(i < 0)
		return qfalse;
	if(Q_stricmp(EntitySpawns.entities[index]->keys.pairs[i].value, "worldspawn") != 0)
		return qfalse;
	return qtrue;
}

gentity_t* spawnEntity(gentity_t *ent, int index){
	unsigned int i;
	EntitySpawn_t *spawn;
	gentity_t *oldSpawn = NULL;
	gentity_t *newSpawn;
	spawn = EntitySpawns.entities[index];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	for(i = 0;i<spawn->keys.count;i++){
		AddSpawnField(spawn->keys.pairs[i].key, spawn->keys.pairs[i].value);
	}

	level.spawning = qtrue;
	if(entityIsWorldspawn(index)) {
		//we can only spawn a worldspawn it specifically given the world ent, no turning other ents into world.
		if(ent->s.number == ENTITYNUM_WORLD) {
			SP_worldspawn();
			level.spawning = qfalse;
			return ent;
		}
		level.spawning = qfalse;
		removeSpawnstring(index); //since normal ents have the index freed on fail to spawn.
		return NULL;
	}
	
	if(spawn->spawned && spawn->spawned != ent)
		oldSpawn = spawn->spawned;

	if(ent) {
		ent->Lmd.customIndex = -1; //dont let G_FreeEntity delete our entry.  This will be reset on spawn.
		G_FreeEntity(ent);
		G_InitGentity(ent);
	}

#ifdef LMD_ENTITYCORE_MEMCHECK
	assert(_CrtCheckMemory());
#endif
	disablesenabled = qfalse;
	newSpawn = G_SpawnGEntityPtrFromSpawnVars(ent, index, qfalse);
	disablesenabled = qtrue;
#ifdef LMD_ENTITYCORE_MEMCHECK
	assert(_CrtCheckMemory());
#endif
	//now that are index was used and set, we can do this validly.
	if(oldSpawn){
		G_FreeEntity(oldSpawn);
	}

	//If we get NULL returned, then the entity never spawned in the first place, 
	//so we need to remove the spawnstring ourselfs.
	if(newSpawn == NULL) {
		removeSpawnstring(index);
		return NULL;
	}
	//If we were deleted when spawning, our custom ent entry would have been deleted.
	else if(!newSpawn->inuse)
		return NULL;

	level.spawning = qfalse;

	spawn->spawned = newSpawn;

	return spawn->spawned;
}

gentity_t *trySpawn(char *string){
	int index = ParseEntitySpawnstring(string);
	return spawnEntityFromIndex(index);
}

gentity_t *tryImport(KeyPairSet_t *set) {
	int index = newSpawnstring();
	Lmd_Pairs_Merge(&EntitySpawns.entities[index]->keys, set);
	return spawnEntityFromIndex(index);
}

void getSpawnstringPairs(int index, KeyPairSet_t *set) {
	Lmd_Pairs_Merge(set, &EntitySpawns.entities[index]->keys);
}

int cloneSpawnstring(int index){
	int i, newSpawn = newSpawnstring();
	for(i = 0;i<EntitySpawns.entities[index]->keys.count;i++){
		Lmd_Pairs_New(&EntitySpawns.entities[newSpawn]->keys, EntitySpawns.entities[index]->keys.pairs[i].key,
			EntitySpawns.entities[index]->keys.pairs[i].value);
	}
	return newSpawn;
}

qboolean Lmd_Entities_IsSaveable(gentity_t *ent){
	if(ent->Lmd.customIndex < 0)
		return qfalse;
	return EntitySpawns.entities[ent->Lmd.customIndex]->canSave;
}

void Lmd_Entities_SetSaveable(int index, qboolean saveable){
	EntitySpawns.entities[index]->canSave = saveable;
}

unsigned int Lmd_Entites_GetSpawnstringLen(int index) {
	int i, len = 0;
	for(i = 0;i<EntitySpawns.entities[index]->keys.count;i++){
		len += strlen(EntitySpawns.entities[index]->keys.pairs[i].key) + 1;
		len += strlen(EntitySpawns.entities[index]->keys.pairs[i].value) + 1;
	}
	return len + 1; //for null char
}

void Lmd_Entities_getSpawnstring(int index, char *buf, int bufSze){
	int i, len = 0, tlen;
	char *tmp;
	buf[0] = 0;
	for(i = 0;i<EntitySpawns.entities[index]->keys.count;i++){
		tmp = va("%s,%s,", EntitySpawns.entities[index]->keys.pairs[i].key, EntitySpawns.entities[index]->keys.pairs[i].value);
		tlen = strlen(tmp);
		if(len + tlen >= bufSze)
			return;
		Q_strcat(buf, bufSze, tmp);
		len += tlen;
	}
}

qboolean Lmd_Entities_getSpawnstringKey(int index, char *key, char *value, int valSze){
	int i = Lmd_Entities_getSpawnstringKeyIndex(EntitySpawns.entities[index], key);
	if(i < 0)
		return qfalse;
	Q_strncpyz(value, EntitySpawns.entities[index]->keys.pairs[i].value, valSze);
	return qtrue;
}

void Lmd_Entities_setSpawnstringKey(int index, char *key, char *value){
	int i = Lmd_Entities_getSpawnstringKeyIndex(EntitySpawns.entities[index], key);
	if(i < 0)
		Lmd_Pairs_New(&EntitySpawns.entities[index]->keys, key, value);
	else{
		G_Free(EntitySpawns.entities[index]->keys.pairs[i].value);
		EntitySpawns.entities[index]->keys.pairs[i].value = G_NewString2(value);
	}
}

qboolean Lmd_Entities_deleteSpawnstringKey(int index, char *key){
	int i = Lmd_Entities_getSpawnstringKeyIndex(EntitySpawns.entities[index], key);
	if(i < 0)
		return qfalse;

	Lmd_Pairs_Remove(&EntitySpawns.entities[index]->keys, i);
	return qtrue;
}

extern qboolean disablesenabled;

int ParseEntityLoadstring(char *string){
	unsigned int stringIndex = newSpawnstring();
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	EntitySpawn_t *spawn = EntitySpawns.entities[stringIndex];
	char *sp = string;
	while(ParseField(&sp, key, '\"', '\"')){
		ParseField(&sp, value, '\"', '\"');
		Lmd_Pairs_New(&spawn->keys, key, value);
	}
	return stringIndex;
}

qboolean G_ParseSpawnVars(qboolean inSubBSP);
void LoadMapDefaults(void){
	unsigned int i, index;
	EntitySpawn_t *spawn;
	while(G_ParseSpawnVars(qfalse)){
		//linux saves the location of MapSpawns.entities before running the function,
		//so the function can realloc it somewhere else and cause a seg fault.
		index = newMapSpawnstring();
		spawn = MapSpawns.entities[index];
		//spawn = MapSpawns.entities[newMapSpawnstring()];
		for(i = 0;i<level.numSpawnVars;i++){
			Lmd_Pairs_New(&spawn->keys, level.spawnVars[i][0], level.spawnVars[i][1]);
		}
	}
}

void ImportMapDefaults(void){
	unsigned int i, i2, index;
	EntitySpawn_t *spawn;
	for(i = 0;i<MapSpawns.count;i++){
		//linux saves the location of MapSpawns.entities before running the function,
		//so the function can realloc it somewhere else and cause a seg fault.
		index = newSpawnstring();
		spawn = EntitySpawns.entities[index];
		spawn->canSave = qtrue;
		//spawn = EntitySpawns.entities[newSpawnstring()];
		for(i2 = 0;i2<MapSpawns.entities[i]->keys.count;i2++){
			Lmd_Pairs_New(&spawn->keys, MapSpawns.entities[i]->keys.pairs[i2].key,
				MapSpawns.entities[i]->keys.pairs[i2].value);
		}
	}
}

void scriptrunner_run (gentity_t *self);
void ClearEntities(void){
	int i = 0;
	while(EntitySpawns.count > i){
		if(EntitySpawns.entities[i] == NULL){
			i++;
			continue;
		}
		if(EntitySpawns.entities[i]->spawned){
			EntitySpawns.entities[i]->spawned->neverFree = qfalse;
			G_FreeEntity(EntitySpawns.entities[i]->spawned);
		}
		else
			removeSpawnstring(i);
	}
	//We should have 0 entries now, free all slots.
	Lmd_Arrays_RemoveAllElements((void **)&EntitySpawns.entities);
	EntitySpawns.count = 0;
}

int LoadEntitiesData(const char *filename){
	char *buf = NULL, *str;
	unsigned int i, spawnIndex = 0;
	int worldspawn = -1;
	gentity_t *spawned;

	restrictSpawnErrors = qtrue;

	ClearEntities();

	if(filename){
		str = buf = Lmd_Database_AllocFileContents(va("%s/%s_%s.lmd", entbasepath, level.rawmapname, filename));
		if(!buf){
			G_Printf("^3Entities data not found for %s.  Loading default map entities.\n", filename);
		}

		if(buf){
			while(NextSection(&str)){
				spawnIndex = ParseEntityLoadstring(str);
				EntitySpawns.entities[spawnIndex]->canSave = qtrue;
				if(entityIsWorldspawn(spawnIndex)){
					if(worldspawn > -1)
						Com_Printf("^3Multiple worldspawns in lugormod entity file, ignoring...\n");
					else
						worldspawn = spawnIndex;
				}
			}
		}
	}

	if(worldspawn <= -1){
		worldspawn = EntitySpawns.count; //will become the worldspawn, since the entities are imported in order.
		ImportMapDefaults();
	}

	if(!EntitySpawns.entities[worldspawn]) {
		Com_Printf("Worldspawn load failed.  This is caused by the quake 3 entity parser not running more than twice.  Try removing some misc_bsp entities.\n");
		G_Error("No worldspawn loaded.  See console.");
	}

	for(i = 0;i<EntitySpawns.entities[worldspawn]->keys.count;i++) {
		AddSpawnField(EntitySpawns.entities[worldspawn]->keys.pairs[i].key,
			EntitySpawns.entities[worldspawn]->keys.pairs[i].value);
	}
	g_entities[ENTITYNUM_WORLD].Lmd.customIndex = worldspawn;
	SP_worldspawn();

	spawnIndex = 0;
	while(spawnIndex < EntitySpawns.count){
		if(EntitySpawns.entities[spawnIndex] != NULL && spawnIndex != worldspawn){
			spawnEntityFromIndex(spawnIndex);
		}
		spawnIndex++;
	}

	spawned = G_Spawn();
	if(spawned){
		G_UseTargets2(spawned, spawned, "initial_entity");
		if(g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN] && g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN][0] )
		{//World has a spawn script, but we don't want the world in ICARUS and running scripts,
			//so make a scriptrunner and start it going.
			spawned->behaviorSet[BSET_USE] = g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN];
			spawned->count = 1;
			spawned->think = scriptrunner_run;
			spawned->nextthink = level.time + 100;
			spawned->classname = "worldspawn_scriptrunner";

			if ( spawned->inuse )
				trap_ICARUS_InitEnt( spawned );
		}
		else
			G_FreeEntity(spawned);
	}

	G_Free(buf);

	restrictSpawnErrors = qfalse;

	return 0;
}

void ClearMap(void){
	LoadEntitiesData(NULL);
}

extern vmCvar_t lmd_saveallplaced;
void SaveEntitiesData(const char *filename){
	int i, c;
	EntitySpawn_t *ent;
	char *str;
	fileHandle_t file;
	trap_FS_FOpenFile(va("%s/%s_%s.lmd", entbasepath, level.rawmapname, filename), &file, FS_WRITE);
	for(c = 0; c < EntitySpawns.count; c++) {
		ent = EntitySpawns.entities[c];
		if(!ent) //should never happen, somehow the spawnstring got out of sync.
			continue;
		if(lmd_saveallplaced.integer <= 0 && EntitySpawns.entities[c]->canSave == qfalse)
			continue;
		trap_FS_Write("{\n", 2, file);
		for(i = 0;i<ent->keys.count;i++){
			str = va("\"%s\" \"%s\"\n", ent->keys.pairs[i].key, ent->keys.pairs[i].value);
			trap_FS_Write(str, strlen(str), file);
		}
		trap_FS_Write("}\n", 2, file);
	}

	trap_FS_FCloseFile(file);
}
