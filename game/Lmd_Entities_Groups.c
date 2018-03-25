
#include "g_local.h"

#include "Lmd_Data.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Confirm.h"

//For target/targetname ajust: level.mTargetAdjust

char *GroupPath(void){
	static char path[MAX_QPATH] = "";
	if(!path[0])
		Lmd_Data_GetDataPath("groupentities", path, sizeof(path));
	return path;
}

//TODO: check if you can do "../", possible security vunrability
gentity_t *tryImport(KeyPairSet_t *set);
int Groups_ImportFile(char *filename, char *newName, vec3_t groupOrigin, gentity_t *creatorEnt) {
	fileHandle_t f;
	char *buf, *str;
	int count = 0;

	int len = trap_FS_FOpenFile(va("%s/%s.lmd", GroupPath(), filename), &f, FS_READ);
	if(!f || len <= 0)
		return -1;
	buf = (char *)G_Alloc(len);
	trap_FS_Read(buf, len, f);
	trap_FS_FCloseFile(f);
	str = buf;

	qboolean saveable;
	if(creatorEnt->client && Auths_PlayerHasAuthFlag(creatorEnt, AUTH_SAVEPLACED))
		saveable = qtrue;
	else
		saveable = qfalse;


	KeyPairSet_t set = {0, NULL};
	vec3_t origin;
	gentity_t *spawned;
	char *line;
	while(str && str[0]) {
		line = COM_ParseLine((const char **)&str);
		if(!line[0])
			continue;
		Lmd_Pairs_ParseDatastring(&set, line);
		Lmd_Pairs_SetKey(&set, "group", newName);
		line = Lmd_Pairs_GetKey(&set, "origin");
		if(line) {
			sscanf(line, "%f %f %f", &origin[0], &origin[1], &origin[2]);
			VectorAdd(origin, groupOrigin, origin);
		}
		else
			VectorCopy(groupOrigin, origin);
		Lmd_Pairs_SetKey(&set, "origin", vtos2(origin));
		spawned = tryImport(&set);
		Lmd_Pairs_Clear(&set);
		if(!spawned)
			continue;
		count++;
		if(saveable)
			Lmd_Entities_SetSaveable(spawned->Lmd.spawnData, qtrue);
		else if(!creatorEnt->client){
			//we are an instance ent, save us to its spawned tree
		}
	}
	G_Free(buf);
	return count;
}

void Cmd_ImportGroup_f(gentity_t *ent, int iArg){
	//importgroup <file> <name>
	vec3_t origin;
	int argc = trap_Argc();
	if(argc < 3) {
		Disp(ent, "^3Usage: ImportGroup ^2<file> <name> ^5[origin]");
		return;
	}
	if(argc < 4) {
		trace_t tr;
		vec3_t dir;
		VectorCopy(ent->client->renderInfo.eyePoint, origin);
		AngleVectors(ent->client->ps.viewangles, dir, NULL,NULL);
		VectorNormalize(dir);
		VectorMA(origin, 8192, dir, origin);
		trap_Trace(&tr, ent->client->renderInfo.eyePoint, NULL, NULL, origin, ent->s.number, ent->clipmask);
		if(tr.fraction == 1) {
			Disp(ent, "^3Target location too far away.");
		}
		VectorCopy(tr.endpos, origin);
	}
	else {
		sscanf(ConcatArgs(3), "%f %f %f", &origin[0], &origin[1], &origin[2]);
	}
	char file[MAX_STRING_CHARS], name[MAX_STRING_CHARS];
	trap_Argv(1, file, sizeof(file));
	trap_Argv(2, name, sizeof(name));
	int i = Groups_ImportFile(file, name, origin, ent);
	if(i < 0)
		Disp(ent, "^3Could not find file.");
	else
		Disp(ent, va("^2%i^3 entities imported.", i));
}

unsigned int Group_Count(char *name, qboolean saveable) {
	unsigned int count = 0;
	gentity_t *ent = NULL;
	while((ent = G_Find(ent, FOFS(Lmd.group), name))) {
		if(saveable && !Lmd_Entities_IsSaveable(ent))
			continue;
		count++;
	}
	return count;
}

unsigned int Group_Center(char *name, qboolean saveable, vec3_t center, qboolean floorOrigin) {
	int lowest = INT_MAX;

	vec3_t mins = {INT_MAX, INT_MAX, INT_MAX}, maxs = {-INT_MAX, -INT_MAX, -INT_MAX};
	int i;

	unsigned int count = 0;
	gentity_t *ent = NULL;
	while((ent = G_Find(ent, FOFS(Lmd.group), name))) {
		if(saveable && !Lmd_Entities_IsSaveable(ent))
			continue;
#if 1
		for(i = 0; i < 3; i++) {
			if(ent->r.absmin[i] < mins[i])
				mins[i] = ent->r.absmin[i];
			if(ent->r.absmax[i] > maxs[i])
				maxs[i] = ent->r.absmax[i];
		}
#else
		VectorAdd(ent->r.absmin, ent->r.absmax, entCenter);
		VectorScale(entCenter, 0.5f, entCenter);
		if(count == 0)
			VectorCopy(entCenter, center);
		else {
			VectorAdd(entCenter, center, center);
			VectorScale(center, 0.5f, center);
		}
#endif
		if(ent->r.absmin[2] < lowest)
			lowest = ent->r.absmin[2];
		count++;
	}
#if 1
	VectorAdd(mins, maxs, center);
	VectorScale(center, 0.5f, center);
#endif
	if(floorOrigin)
		center[2] = lowest; //origin z is lowest spot.
	return count;
}

void getSpawnstringPairs(SpawnData_t* spawnData, KeyPairSet_t *set);
int Groups_SaveFile(char *groupname, char *filename) {
	unsigned int count;
	gentity_t *ent = NULL;
	char *str;
	vec3_t origin = {0, 0, 0};
	vec3_t center;
	int i;
	fileHandle_t f;
	KeyPairSet_t set = {0, NULL};

	count = Group_Center(groupname, qtrue, center, qtrue);
	if(count == 0) {
		return -2;
	}

	trap_FS_FOpenFile(va("%s/%s.lmd", GroupPath(), filename), &f, FS_WRITE);

	if(!f)
		return -1;


	ent = NULL;
	while((ent = G_Find(ent, FOFS(Lmd.group), groupname))) {
		if(!Lmd_Entities_IsSaveable(ent))
			continue;
		Lmd_Pairs_Clear(&set);
		getSpawnstringPairs(ent->Lmd.spawnData, &set);
		str = Lmd_Pairs_GetKey(&set, "origin");
		if(str) {
			sscanf(str, "%f %f %f", &origin[0], &origin[1], &origin[2]);
			VectorSubtract(origin, center, origin);
		}
		else
			VectorCopy(center, origin); //write it anyway.
		Lmd_Pairs_SetKey(&set, "origin", vtos2(origin));
		i = Lmd_Pairs_FindKey(&set, "group");
		if(i > -1) //we should always have this.
			Lmd_Pairs_Remove(&set, i);
		str = Lmd_Pairs_ToDatastring(&set);
		trap_FS_Write(str, strlen(str), f);
		G_Free(str);
		trap_FS_Write("\n", 1, f);
	}
	trap_FS_FCloseFile(f);
	return count;
}

typedef struct Cmd_SaveGroup_Confirm_Data_s{
	char group[MAX_STRING_CHARS];
	char file[MAX_STRING_CHARS];
}Cmd_SaveGroup_Confirm_Data_t;
void Cmd_SaveGroup_Confirm(gentity_t *ent, void *data) {
	Cmd_SaveGroup_Confirm_Data_t *sData = (Cmd_SaveGroup_Confirm_Data_t *)data;
	int i = Groups_SaveFile(sData->group, sData->file);
	if(i == -1) {
		Disp(ent, "^1Error saving file.");
	}
	else if(i == -2) {
		Disp(ent, "^3No entities in that group.");
	}
	else {
		Disp(ent, va("^2%i^3 entities saved.", i));
	}
}

void Cmd_SaveGroup_f(gentity_t *ent, int iArg){
	//groupsave <group name> <file name>
	int argc = trap_Argc();
	if(argc < 3) {
		Disp(ent, "^3Usage: groupsave ^2<group name> <file name>\n"
			"Save the group into a file.");
		return;
	}
	char group[MAX_STRING_CHARS];
	char file[MAX_STRING_CHARS];
	trap_Argv(1, group, sizeof(group));
	trap_Argv(2, file, sizeof(file));

	fileHandle_t f;
	if(trap_FS_FOpenFile(va("%s/%s.lmd", GroupPath(), file), &f, FS_READ) > -1) {
		trap_FS_FCloseFile(f);
		Disp(ent, "^3That file already exists.  Confirm to override.\n");
		Cmd_SaveGroup_Confirm_Data_t *data = (Cmd_SaveGroup_Confirm_Data_t *)G_Alloc(sizeof(Cmd_SaveGroup_Confirm_Data_t));
		Q_strncpyz(data->group, group, sizeof(data->group));
		Q_strncpyz(data->file, file, sizeof(data->file));
		Confirm_Set(ent, Cmd_SaveGroup_Confirm, data);
		return;
	}

	int i = Groups_SaveFile(group, file);
	if(i == -1) {
		Disp(ent, "^1Error saving file.");
	}
	else if(i == -2) {
		Disp(ent, "^3No entities in that group.");
	}
	else {
		Disp(ent, va("^2%i^3 entities saved.", i));
	}
}

//Ufo: was not passing player as an argument, and wrongly displayed messages to server console
void Groups_Modify(char *name, void (*modify)(SpawnData_t *spawnData, void *data), void *data, gentity_t* creatorEnt) {
	gentity_t **ents;
	SpawnData_t **backupSpawns;
	int count = 0;
	qboolean failed = qfalse;
	gentity_t *ent = NULL;
	gentity_t *check;
	int i;
	while((ent = G_Find(ent, FOFS(Lmd.group), name))) {
		count++;
	}
	ents = (gentity_t **)G_Alloc(sizeof(gentity_t *) * count);
	backupSpawns = (SpawnData_t **)G_Alloc(sizeof(SpawnData_t *) * count);
	count = 0;
	ent = NULL;
	while((ent = G_Find(ent, FOFS(Lmd.group), name))) {
		ents[count] = ent;
		backupSpawns[count] = cloneSpawnstring(ent->Lmd.spawnData);

		count++;

		modify(ent->Lmd.spawnData, data);
		//FIXME: if I ever do the logical entity sanity check to stop server ents from going logical, then a new
		//ent will be spawned here, and could possibly be refound by G_Find.
		//Need to get that presistant entity index set up.
		check = spawnEntity(ent, ent->Lmd.spawnData);
		if(!check || !check->inuse) {
			failed = qtrue;
			break;
		}
	}

	if(failed) {
		Disp(creatorEnt, "^1An entity failed to respawn, reverting group...");
		int fCount = 0;
		for(i = 0; i < count; i++) {
			removeSpawnstring(ent->Lmd.spawnData);
			check = spawnEntity(ents[i], backupSpawns[i]);
			if(!check || !check->inuse)
				fCount++;
		}
		if(fCount) {
			Disp(creatorEnt, va("^3%i^1 group entit%s failed to recover.", fCount, (fCount != 1)?"ies":"y"));
		}
		else
			Disp(creatorEnt, "^3Group recovered.");
	}
	else {
		for(i = 0; i < count; i++) {
			removeSpawnstring(backupSpawns[i]);
		}
		Disp(creatorEnt, va("^2%i^3 entit%s modified.", count, (count != 1)?"ies":"y"));
	}
	G_Free(ents);
	G_Free(backupSpawns);
}

typedef struct vecModifierData_s {
	vec3_t offset;
	vec3_t origin;
}vecModifierData_t;

void Groups_NudgeModifier(SpawnData_t *spawnData, void *data) {
	vecModifierData_t *nData = (vecModifierData_t *)data;
	char str[MAX_STRING_CHARS];
	vec3_t origin;
	if(Lmd_Entities_getSpawnstringKey(spawnData, "origin", str, sizeof(str)))
		sscanf(str, "%f %f %f", &origin[0], &origin[1], &origin[2]);
	else
		VectorCopy(vec3_origin, origin);
	VectorAdd(origin, nData->offset, origin);

	Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(origin));
}

//AHA: see HandleEntityAdjustment
//level.mOriginAdjust
//level.mRotationAdjust
void Groups_RotateModifier(SpawnData_t *spawnData, void *data) {
	vecModifierData_t *vData = (vecModifierData_t *)data;
	char str[MAX_STRING_CHARS];
	vec3_t orig;
	vec3_t offset;
	vec3_t dir;
	//vec3_t origAng;
	if(Lmd_Entities_getSpawnstringKey(spawnData, "origin", str, sizeof(str)))
		sscanf(str, "%f %f %f", &orig[0], &orig[1], &orig[2]);
	else
		VectorCopy(vec3_origin, orig);

#if 0
	VectorSubtract(orig, vData->origin, origAng);
	vectoangles(origAng, origAng);
#endif

	VectorSubtract(orig, vData->origin, orig);

	VectorSet(dir, 0, 1, 0);
	RotatePointAroundVector( offset, dir, orig, vData->offset[0]);
	VectorSet(dir, 0, 0, 1);
	RotatePointAroundVector( orig, dir, offset, vData->offset[1]);
	VectorSet(dir, 1, 0, 0);
	RotatePointAroundVector( offset, dir, orig, vData->offset[2]);

	VectorAdd(offset, vData->origin, orig);

	Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(orig));

#if 0
	VectorSubtract(orig, vData->origin, offset);
	vectoangles(offset, offset);
#endif

	if(Lmd_Entities_getSpawnstringKey(spawnData, "angles", str, sizeof(str)))
		sscanf(str, "%f %f %f", &orig[0], &orig[1], &orig[2]);
	else
		VectorCopy(vec3_origin, orig);

	/*
	Huh... for a proper angle rotation in one test, a catw with angles 90 90 0 needs to become 45 0 -90

	Ang			Old			New
	90 0 0:		90 90 0		90 0 90 (90 0 -90?)
	0 90 0:		90 90 0		-90 0 0 (270 0 0)
	0 90 0:		0  0  0		0 90 0

	0 45 0:		0 180 90	0 225 90

	0 90 0		90 90 0		-90 0 0

	n[0] = o[1] + (m[0] * cos(o[0])

	Need to control exact x, y, z axies for whatever rotation it is in
	*/
#if 0
	VectorSubtract(offset, origAng, origAng);
	VectorAdd(orig, origAng, orig);

#elif 0

	//This is close, but not the proper way.  Need offset as angle, not as scaler
	AngleVectors(orig, NULL, NULL, offset);
	orig[0] += vData->offset[0] * offset[0];
	orig[1] += vData->offset[0] * offset[1];
	orig[2] += vData->offset[0] * offset[2];

	AngleVectors(vData->offset, offset, NULL, NULL);
	/*
	AngleVectors(orig, offset, NULL, NULL);
	orig[0] = (orig[0] + vData->offset[1]) * offset[0];
	orig[1] = (orig[1] + vData->offset[1]) * -offset[1];
	orig[2] = (orig[2] + vData->offset[1]) * offset[2];

	orig[0] += (orig[0] + vData->offset[1]) * -offset[0]; //works
	orig[1] += (orig[0] + vData->offset[1]) * -offset[1]; //fails

	orig[0] = (orig[0] - vData->offset[1]) * -offset[0]; //fails
	orig[1] = (orig[1] - vData->offset[1]) * -offset[1]; //works

	*/
	//90 + 90 = 180
	//180 * -1 = -180
	//90 + -180 = -90
	orig[0] = (orig[0] - vData->offset[1]) * -offset[0];
	orig[1] = (orig[1] - vData->offset[1]) * -offset[1];
	orig[2] = (orig[2] - vData->offset[1]) * -offset[2];

	AngleVectors(orig, offset, NULL, NULL);
	orig[0] += vData->offset[2] * offset[0];
	orig[1] += vData->offset[2] * offset[1];
	orig[2] += vData->offset[2] * offset[2];

#elif 0
	AngleVectors(orig, offset, NULL, NULL);
	orig[0] += vData->offset[0] * offset[0];
	orig[1] += vData->offset[0] * offset[1];
	orig[2] += vData->offset[0] * offset[2];

	AngleVectors(orig, NULL, NULL, offset);
	orig[0] += vData->offset[1] * offset[0];
	orig[1] += vData->offset[1] * offset[1];
	orig[2] += vData->offset[1] * offset[2];

	AngleVectors(orig, NULL, offset, NULL);
	orig[0] += vData->offset[2] * offset[0];
	orig[1] += vData->offset[2] * offset[1];
	orig[2] += vData->offset[2] * offset[2];

#elif 0
	AngleVectors(orig, offset, NULL, NULL);
	orig[0] += vData->offset[0] * offset[0];
	orig[1] += vData->offset[1] * offset[0];
	orig[2] += vData->offset[2] * offset[0];

	orig[0] += vData->offset[0] * offset[1];
	orig[1] += vData->offset[1] * offset[1];
	orig[2] += vData->offset[2] * offset[1];

	orig[0] += vData->offset[0] * offset[2];
	orig[1] += vData->offset[1] * offset[2];
	orig[2] += vData->offset[2] * offset[2];

#elif 0
	AngleVectors(orig, offset, NULL, NULL);
	orig[0] += vData->offset[0] * offset[0];
	orig[0] += vData->offset[0] * offset[1];
	orig[0] += vData->offset[0] * offset[2];

	AngleVectors(orig, NULL, NULL, offset);
	orig[1] += vData->offset[1] * offset[0];
	orig[1] += vData->offset[1] * offset[1];
	orig[1] += vData->offset[1] * offset[2];
#elif 0
	vec3_t axis[3];
	AnglesToAxis(vData->offset, axis);
	//VectorAdd(orig, vData->offset, orig);
	VectorRotate(orig, axis, orig);

#elif 0
	/* Uses above
	VectorSubtract(orig, vData->origin, origAng);
	vectoangles(origAng, origAng);
	*/
	VectorAdd(origAng, vData->offset, origAng);
	VectorAdd(orig, vData->origin, orig);
#elif 0
	VectorCopy(vData->offset, dir);
	VectorNormalize(dir);
	float degrees = VectorLength(vData->offset);
	RotatePointAroundVector( offset, dir, orig, degrees);

#elif 0
	//seems like its on the right track, but still not it.
	orig[0] += (vData->offset[0] * cos(DEG2RAD(orig[0]))) + (vData->offset[0] * sin(DEG2RAD(orig[1])));
	orig[1] += (vData->offset[1] * cos(DEG2RAD(orig[0])));// + (vData->offset[1] * sin(orig[1]));
	orig[2] += (vData->offset[2] * cos(orig[1])) + (vData->offset[2] * sin(orig[2]));


#elif 0
	VectorCopy(vData->offset, offset);
	VectorNormalize(offset);
	orig[0] += vData->offset[0] * offset[0];
	orig[1] += vData->offset[1] * offset[1];
	orig[2] += vData->offset[2] * offset[2];

#else
	VectorAdd(orig, vData->offset, orig);

#endif
	Lmd_Entities_setSpawnstringKey(spawnData, "angles", vtos2(orig));
}

gentity_t* AimAnyTarget (gentity_t *ent, int length);
void Cmd_ManipGroupVec_f(gentity_t *ent, int iArg) {
	vec3_t offset;
	char arg[MAX_STRING_CHARS];
	char group[MAX_STRING_CHARS];
	if(trap_Argc() > 5 || trap_Argc() < 4) {
		Disp(ent, va("^3Usage: group%s ^5[group name] ^2<x> <y> <z>", iArg?"rotate":"nudge"));
		return;
	}
	if (trap_Argc() == 5){
		trap_Argv(1, group, sizeof(group));

		trap_Argv(2, arg, sizeof(arg));
		offset[0] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		offset[1] = atoi(arg);
		trap_Argv(4, arg, sizeof(arg));
		offset[2] = atoi(arg);
	} 
	else{
		gentity_t *targ = AimAnyTarget(ent, 8192);
		if(!targ || !targ->Lmd.group) {
			Disp(ent, "^3Target entity is not part of a group.");
			return;
		}
		Q_strncpyz(group, targ->Lmd.group, sizeof(group));

		trap_Argv(1, arg, sizeof(arg));
		offset[0] = atoi(arg);
		trap_Argv(2, arg, sizeof(arg));
		offset[1] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		offset[2] = atoi(arg);
	}

	vecModifierData_t mod;
	VectorCopy(offset, mod.offset);
	Group_Center(group, qfalse, mod.origin, qfalse);
	if(iArg == 0)
		Groups_Modify(group, Groups_NudgeModifier, &mod, ent);
	else if(iArg == 1)
		Groups_Modify(group, Groups_RotateModifier, &mod, ent);
}

void Cmd_DeleteGroup_Confirm(gentity_t *ent, void *data) {
	char *name = (char *)data;
	gentity_t *scan = NULL;
	unsigned int c = 0;
	while((scan = G_Find(scan, FOFS(Lmd.group), name))) {
		G_FreeEntity(scan); //Ufo: was missing
		c++;
	}
	Disp(ent, va("^3Deleted ^2%i^3 entit%s", c, (c != 1)?"ies":"y"));
}

void Cmd_DeleteGroup_f(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	if(trap_Argc() < 2) {
		Disp(ent, "^3GroupDelete ^2<group name>\n"
			"^3This process ^1cannot^3 be undone!");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	unsigned int c = Group_Count(arg, qfalse);
	Disp(ent, va("^3This action will delete ^2%i^3 entit%s, and any others created in the group before confirming.\n"
		"^3This action ^1cannot^3 be undone!", c, (c != 1)?"ies":"y"));
	Confirm_Set(ent, Cmd_DeleteGroup_Confirm, G_NewString2(arg));
}

cmdEntry_t groupCommandEntries[] = {
	{"groupimport", "^1UNSUPORTED\n^3Import an entity group from a file.", Cmd_ImportGroup_f, 0, qtrue, 1, 0, 0},
	{"groupsave", "^1UNSUPORTED\n^3Save an entity group to a file.", Cmd_SaveGroup_f, 0, qtrue, 1, 0, 0},
	{"groupnudge", "^1UNSUPORTED\n^3Move a group of entities.", Cmd_ManipGroupVec_f, 0, qtrue, 1, 0, 0},
	{"grouprotate", "^1UNSUPORTED\n^3Rotate a group of entities.", Cmd_ManipGroupVec_f, 1, qtrue, 1, 0, 0},
	{"groupdelete", "^1UNSUPORTED\n^3Rotate a group of entities.", Cmd_DeleteGroup_f, 0, qtrue, 1, 0, 0},
	{NULL}
};
