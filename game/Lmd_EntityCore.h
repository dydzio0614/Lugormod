
SpawnData_t* ParseEntitySpawnstring(char *string);
void removeSpawnstring(SpawnData_t *spawnData);
gentity_t *spawnEntityFromData(SpawnData_t *spawnData);
gentity_t* spawnEntity(gentity_t *ent, SpawnData_t *spawnData);
gentity_t *trySpawn(char *string);

qboolean Lmd_Entities_getSpawnstringKey(SpawnData_t *spawnData, char *key, char *value, int valSze);
void Lmd_Entities_setSpawnstringKey(SpawnData_t *spawnData, char *key, char *value);
qboolean Lmd_Entities_deleteSpawnstringKey(SpawnData_t *spawnData, char *key);

qboolean Lmd_Entities_IsSaveable(gentity_t *ent);
void Lmd_Entities_SetSaveable(SpawnData_t *spawnData, qboolean saveable);

unsigned int Lmd_Entites_GetSpawnstringLen(SpawnData_t *spawnData);
void Lmd_Entities_getSpawnstring(SpawnData_t *spawnData, char *buf, int bufSze);

SpawnData_t* cloneSpawnstring(SpawnData_t *spawnData);

int LoadEntitiesData(const char *filename, qboolean noDefaults);
void SaveEntitiesData(const char *filename);
