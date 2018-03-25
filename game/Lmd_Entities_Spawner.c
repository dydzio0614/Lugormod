
#include "g_local.h"

#include "Lmd_Arrays.h"
#include "Lmd_Data.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_EntityCore.h"


#define entbasepath MapEntPath()
char *MapEntPath(void){
	static char path[MAX_QPATH] = "";
	if(!path[0])
		Lmd_Data_GetDataPath("mapentities", path, sizeof(path));
	return path;
}

typedef struct SpawnData_s{
	qboolean canSave;
	gentity_t *spawned;
	KeyPairSet_t keys;
}SpawnData_t;

struct EntitySpawns_s{
	unsigned int count;
	SpawnData_t **entities;
}EntitySpawns;

struct MapSpawns_s{
	unsigned int count;
	SpawnData_t **entities;
}MapSpawns;

void AddSpawnField(char *field, char *value);
gentity_t* G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );
qboolean ParseField(char **sourcep, char *dest, char start, char stop);
qboolean NextSection (char **sourcep);

qboolean disablesenabled = qtrue;
//extern qboolean disablesenabled;

SpawnData_t* newSpawnstring(){
	unsigned int index;
	for(index = 0; index < EntitySpawns.count; index++){
		if(EntitySpawns.entities[index] == NULL)
			break;
	}
	if(index == EntitySpawns.count)
		index = Lmd_Arrays_AddArrayElement((void **)&EntitySpawns.entities, sizeof(SpawnData_t *), &EntitySpawns.count);
	EntitySpawns.entities[index] = (SpawnData_t *)G_Alloc(sizeof(SpawnData_t));
	return EntitySpawns.entities[index];
}

int getSpawnDataIndex(SpawnData_t *spawnData) {
	int i;
	for(i = 0; i < EntitySpawns.count; i++) {
		if(EntitySpawns.entities[i] == spawnData)
			return i;
	}
	return -1;
}

SpawnData_t* newMapSpawnstring(){
	//map spawnstrings are constant, so we dont need to scan for empty unused slots.
	unsigned int index = Lmd_Arrays_AddArrayElement((void **)&MapSpawns.entities, sizeof(SpawnData_t *), &MapSpawns.count);
	MapSpawns.entities[index] = (SpawnData_t *)G_Alloc(sizeof(SpawnData_t));
	return MapSpawns.entities[index];
}

qboolean noWorldspawnError = qfalse;
void removeSpawnstring(SpawnData_t *spawnData){
	int i;
	int index = getSpawnDataIndex(spawnData);
	if(index < 0)
		return;
	if(noWorldspawnError == qfalse) {
		for(i = 0; i < EntitySpawns.entities[index]->keys.count; i++) {
			if(Q_stricmp(EntitySpawns.entities[index]->keys.pairs[i].key, "classname") == 0) {
				if(Q_stricmp(EntitySpawns.entities[index]->keys.pairs[i].value, "worldspawn") == 0) {
					trap_SendServerCommand(-1, "print \"^1Attempted to remove worldspawn spawnstring!\n^3Tell phred about this, and what might have caused it.\n^1Ignoring removal\n\"");
					return;
				}
			}
		}
	}

	Lmd_Pairs_Clear(&spawnData->keys);

	G_Free(spawnData);
	EntitySpawns.entities[index] = NULL;
}


SpawnData_t* ParseEntitySpawnstring(char *string){
	SpawnData_t *spawn = newSpawnstring();
	Lmd_Pairs_ParseDatastring(&spawn->keys, string);
	return spawn;
}

gentity_t* G_SpawnGEntityPtrFromSpawnVars(gentity_t *ent, SpawnData_t *spawnData, qboolean inSubBSP);

qboolean restrictSpawnErrors = qfalse;
gentity_t *spawnEntityFromData(SpawnData_t *spawnData){
	//do this before we spawn, an entity deleted on spawn will remove the spawnstring.
	char buf[MAX_STRING_CHARS];
	Lmd_Entities_getSpawnstring(spawnData, buf, sizeof(buf));
	gentity_t *ent = spawnEntity(NULL, spawnData);
	if(ent)
		return ent;
	else if(!restrictSpawnErrors){
		Com_Printf("^3Unable to spawn custom entity with spawnstring:\n%s\n\n", buf);
	}
	return NULL;
}

int Lmd_Entities_getSpawnstringKeyIndex(SpawnData_t *ent, char *key){
	int i;
	if(!ent)
		return -1;
	for(i = 0; i < ent->keys.count; i++){
		if(Q_stricmp(key, ent->keys.pairs[i].key) == 0){
			return i;
		}
	}
	return -1;
}

char* Lmd_Entities_getSpawnstringKeyValue(SpawnData_t *ent, int index) {
	if(index < 0 || index >= ent->keys.count)
		return NULL;
	return ent->keys.pairs[index].value;
}

void SP_worldspawn(void);

qboolean entityIsWorldspawn(SpawnData_t *spawnData){
	int i = Lmd_Entities_getSpawnstringKeyIndex(spawnData, "classname");
	if(i < 0)
		return qfalse;
	if(Q_stricmp(spawnData->keys.pairs[i].value, "worldspawn") != 0)
		return qfalse;
	return qtrue;
}

gentity_t* spawnEntity(gentity_t *ent, SpawnData_t *spawnData){
	unsigned int i;
	gentity_t *oldSpawn = NULL;
	gentity_t *newSpawn;

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	for(i = 0; i < spawnData->keys.count; i++){
		AddSpawnField(spawnData->keys.pairs[i].key, spawnData->keys.pairs[i].value);
	}

	level.spawning = qtrue;
	if(entityIsWorldspawn(spawnData)) {
		//we can only spawn a worldspawn it specifically given the world ent, no turning other ents into world.
		if(ent->s.number == ENTITYNUM_WORLD) {
			SP_worldspawn();
			level.spawning = qfalse;
			return ent;
		}
		level.spawning = qfalse;
		removeSpawnstring(spawnData); //since normal ents have the index freed on fail to spawn.
		return NULL;
	}
	
	if(spawnData->spawned && spawnData->spawned != ent)
		oldSpawn = spawnData->spawned;

	if(ent) {
		ent->Lmd.spawnData = NULL; //dont let G_FreeEntity delete our entry.  This will be reset on spawn.
		G_FreeEntity(ent);
		G_InitGentity(ent);
	}

#ifdef LMD_ENTITYCORE_MEMCHECK
	assert(_CrtCheckMemory());
#endif
	disablesenabled = qfalse;
	newSpawn = G_SpawnGEntityPtrFromSpawnVars(ent, spawnData, qfalse);
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
		removeSpawnstring(spawnData);
		return NULL;
	}
	//If we were deleted when spawning, our custom ent entry would have been deleted.
	else if(!newSpawn->inuse)
		return NULL;

	level.spawning = qfalse;

	spawnData->spawned = newSpawn;

	return spawnData->spawned;
}

gentity_t *trySpawn(char *string){
	return spawnEntityFromData(ParseEntitySpawnstring(string));
}

gentity_t *tryImport(KeyPairSet_t *set) {
	SpawnData_t *spawn = newSpawnstring();
	Lmd_Pairs_Merge(&spawn->keys, set);
	return spawnEntityFromData(spawn);
}

void getSpawnstringPairs(SpawnData_t *spawnData, KeyPairSet_t *set) {
	Lmd_Pairs_Merge(set, &spawnData->keys);
}

SpawnData_t* cloneSpawnstring(SpawnData_t *spawnData){
	int i;
	SpawnData_t *newSpawn = newSpawnstring();
	for(i = 0; i < spawnData->keys.count; i++){
		Lmd_Pairs_New(&newSpawn->keys, spawnData->keys.pairs[i].key, spawnData->keys.pairs[i].value);
	}
	return newSpawn;
}

qboolean Lmd_Entities_IsSaveable(gentity_t *ent){
	if(!ent->Lmd.spawnData || !ent->Lmd.spawnData->canSave)
		return qfalse;
	return qtrue;
}

void Lmd_Entities_SetSaveable(SpawnData_t *spawnData, qboolean saveable){
	spawnData->canSave = saveable;
}

unsigned int Lmd_Entites_GetSpawnstringLen(SpawnData_t *spawnData) {
	int i, len = 0;
	for(i = 0; i < spawnData->keys.count; i++){
		len += strlen(spawnData->keys.pairs[i].key) + 1;
		len += strlen(spawnData->keys.pairs[i].value) + 1;
	}
	return len + 1; //for null char
}

void Lmd_Entities_getSpawnstring(SpawnData_t *spawnData, char *buf, int bufSze){
	int i, len = 0, tlen;
	char *tmp;
	buf[0] = 0;
	for(i = 0; i < spawnData->keys.count; i++){
		tmp = va("%s,%s,",spawnData->keys.pairs[i].key, spawnData->keys.pairs[i].value);
		tlen = strlen(tmp);
		if(len + tlen >= bufSze)
			return;
		Q_strcat(buf, bufSze, tmp);
		len += tlen;
	}
}

qboolean Lmd_Entities_getSpawnstringKey(SpawnData_t *spawnData, char *key, char *value, int valSze){
	int i = Lmd_Entities_getSpawnstringKeyIndex(spawnData, key);
	if(i < 0)
		return qfalse;
	Q_strncpyz(value, spawnData->keys.pairs[i].value, valSze);
	return qtrue;
}

void Lmd_Entities_setSpawnstringKey(SpawnData_t *spawnData, char *key, char *value){
	int i = Lmd_Entities_getSpawnstringKeyIndex(spawnData, key);
	if(i < 0)
		Lmd_Pairs_New(&spawnData->keys, key, value);
	else{
		G_Free(spawnData->keys.pairs[i].value);
		spawnData->keys.pairs[i].value = G_NewString2(value);
	}
}

qboolean Lmd_Entities_deleteSpawnstringKey(SpawnData_t *spawnData, char *key){
	int i = Lmd_Entities_getSpawnstringKeyIndex(spawnData, key);
	if(i < 0)
		return qfalse;

	Lmd_Pairs_Remove(&spawnData->keys, i);
	return qtrue;
}

extern qboolean disablesenabled;

SpawnData_t* ParseEntityLoadstring(char *string){
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	char *sp = string;
	SpawnData_t *spawn = newSpawnstring();
	while(ParseField(&sp, key, '\"', '\"')){
		ParseField(&sp, value, '\"', '\"');
		Lmd_Pairs_New(&spawn->keys, key, value);
	}
	return spawn;
}

qboolean G_ParseSpawnVars(qboolean inSubBSP);
void LoadMapDefaults(){
	unsigned int i;
	SpawnData_t *spawn;
	while(G_ParseSpawnVars(qfalse)){
		spawn = newMapSpawnstring();
		for(i = 0; i < level.numSpawnVars; i++){
			Lmd_Pairs_New(&spawn->keys, level.spawnVars[i][0], level.spawnVars[i][1]);
		}
	}
}

SpawnData_t *ImportMapDefaults(){
	unsigned int i, i2;
	SpawnData_t *spawn;
	SpawnData_t *worldspawn;
	for(i = 0; i < MapSpawns.count; i++){
		spawn = newSpawnstring();
		spawn->canSave = qtrue;
		for(i2 = 0; i2 < MapSpawns.entities[i]->keys.count; i2++){
			if(Q_stricmp(MapSpawns.entities[i]->keys.pairs[i2].key, "classname") == 0 &&
				Q_stricmp(MapSpawns.entities[i]->keys.pairs[i2].value, "worldspawn") == 0) {
				worldspawn = spawn;
			}
			Lmd_Pairs_New(&spawn->keys, G_NewString(MapSpawns.entities[i]->keys.pairs[i2].key),
				G_NewString(MapSpawns.entities[i]->keys.pairs[i2].value));
		}
	}
	return worldspawn;
}

void scriptrunner_run (gentity_t *self);
void ClearEntities(void){
	int i;

	noWorldspawnError = qtrue;


	for(i = 0; i < EntitySpawns.count; i++) {
		if(EntitySpawns.entities[i] == NULL){
			continue;
		}
		if(EntitySpawns.entities[i]->spawned) {
			if(EntitySpawns.entities[i]->spawned->inuse == qfalse) {
				trap_SendServerCommand(-1, "print \"^1LMD ERROR: ^3Tried to free an entity from spawnstring that was already freed.\n^3Entity still has a spawnstring entry.\n^3Clearing spawnstring and continuing...\n\"");
				removeSpawnstring(EntitySpawns.entities[i]);
				continue;
			}
			EntitySpawns.entities[i]->spawned->neverFree = qfalse;
			G_FreeEntity(EntitySpawns.entities[i]->spawned);
		}
		else
			removeSpawnstring(EntitySpawns.entities[i]);
	}
	
	//We should have 0 entries now, free all slots.
	Lmd_Arrays_RemoveAllElements((void **)&EntitySpawns.entities);
	EntitySpawns.count = 0;


	noWorldspawnError = qfalse;
}

int LoadEntitiesData(const char *filename, qboolean noDefaults){
	char *buf = NULL, *str;
	unsigned int i;
	SpawnData_t *worldspawn = NULL;
	SpawnData_t *spawnData;
	gentity_t *spawned;

	restrictSpawnErrors = qtrue;

	ClearEntities();

	if(filename){
		str = buf = Lmd_Data_AllocFileContents(va("%s/%s_%s.lmd", entbasepath, level.rawmapname, filename));
		if(!buf){
			G_Printf("^3Entities data not found for %s.  Loading default map entities.\n", filename);
		}

		if(buf){
			while(NextSection(&str)){
				spawnData = ParseEntityLoadstring(str);
				spawnData->canSave = qtrue;
				if(entityIsWorldspawn(spawnData)){
					if(worldspawn)
						Com_Printf("^3Multiple worldspawns in lugormod entity file, ignoring...\n");
					else {
						worldspawn = spawnData;
					}
				}
			}
		}
	}

	if(!worldspawn){
		if(noDefaults && MapSpawns.count > 0) {
			worldspawn = newSpawnstring();
			//Import the first entity, which will be the worldspawn.
			for(i = 0; i < MapSpawns.entities[0]->keys.count; i++){
				Lmd_Pairs_New(&worldspawn->keys, MapSpawns.entities[0]->keys.pairs[i].key,
					MapSpawns.entities[0]->keys.pairs[i].value);
			}
		}
		else {
			worldspawn = ImportMapDefaults();
		}
	}

	if(!worldspawn) {
		trap_SendServerCommand(-1, "print \"^1Worldspawn load failed.  ^3This is usually caused by the quake 3 entity parser refusing to run more than twice.  Try removing some misc_bsp entities.\n\"");
		worldspawn = newSpawnstring();
	}

	for(i = 0;i < worldspawn->keys.count; i++) {
		AddSpawnField(worldspawn->keys.pairs[i].key, worldspawn->keys.pairs[i].value);
	}
	g_entities[ENTITYNUM_WORLD].Lmd.spawnData = worldspawn;
	SP_worldspawn();

	for(i = 0; i < EntitySpawns.count; i++) {
		if(EntitySpawns.entities[i] != NULL && EntitySpawns.entities[i] != worldspawn){
			spawnEntityFromData(EntitySpawns.entities[i]);
		}
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
	LoadEntitiesData(NULL, qfalse);
}

extern vmCvar_t lmd_saveallplaced;
void SaveEntitiesData(const char *filename){
	int i, c;
	SpawnData_t *ent;
	char *str;
	fileHandle_t file;

	if(EntitySpawns.count > 0) {
		int i;
		int e;
		qboolean worldspawnFound = qfalse;
		for(e = 0; e < EntitySpawns.count; e++) {
			if(!EntitySpawns.entities[e])
				continue;
			for(i = 0; i < EntitySpawns.entities[e]->keys.count; i++) {
				if(Q_stricmp(EntitySpawns.entities[e]->keys.pairs[i].key, "classname") == 0) {
					if(Q_stricmp(EntitySpawns.entities[e]->keys.pairs[i].value, "worldspawn") == 0) {
						worldspawnFound = qtrue;
						break;
					}
				}
			}
		}
		if(!worldspawnFound)
			trap_SendServerCommand(-1, "print \"^1No worldspawn detected on save!\n^1Load this save with ^3\'/mapents load <name> nodefaults\'^1 and resave it!\n\"");
	}

	trap_FS_FOpenFile(va("%s/%s_%s.lmd", entbasepath, level.rawmapname, filename), &file, FS_WRITE);
	for(c = 0; c < EntitySpawns.count; c++) {
		ent = EntitySpawns.entities[c];
		if(!ent) //should never happen, somehow the spawnstring got out of sync.
			continue;
		if(lmd_saveallplaced.integer <= 0 && ent->canSave == qfalse)
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

void ShutdownEntitySystem() {
	int i;

	ClearEntities();

	for(i = 0; i < MapSpawns.count; i++) {
		Lmd_Pairs_Clear(&MapSpawns.entities[i]->keys);
		G_Free(MapSpawns.entities[i]);
	}
	Lmd_Arrays_RemoveAllElements((void **)&MapSpawns.entities);
	MapSpawns.count = 0;

}