
#include "g_local.h"

#include "Lmd_Data.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Confirm.h"
#include "Lmd_Console.h"

gentity_t* AimAnyTarget (gentity_t *ent, int length);
char *ConcatArgs (int start);
#define STANDARD_BEAM "env/hevil_bolt"
BG_field_t* getField (const char *fieldName);


char* FormattedEntString(gentity_t *ent) {
	return va("^%i%s", (ent->Lmd.spawnData) ? 5 : 3, ent->classname);
}

void DismembermentTest (gentity_t *self);
void Cmd_Kill_f (gentity_t *ent );   
void BlowUpEntity (gentity_t *ent) {
	vec3_t upVec, pos;
	VectorCopy (ent->r.currentOrigin, pos);
	pos[2] = pos[2] + ent->r.mins[2] + 64;

	VectorSet(upVec, -90,0,0);                        
	G_PlayEffectID(G_EffectIndex("env/ion_cannon_explosion"), pos, upVec);

	G_RadiusDamage( pos, NULL, 300, 512, ent, ent, MOD_TRIGGER_HURT);
	if (ent->s.number >= MAX_CLIENTS) {
		G_FreeEntity(ent);
	}
	else {
		ent->client->noCorpse = qtrue;
		Cmd_Kill_f (ent);
		if (ent->health < 1) {
			DismembermentTest(ent);
		}
	}
}

int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create );
extern vec3_t model_mins [MAX_MODELS];
extern vec3_t model_maxs [MAX_MODELS];
unsigned char G_ReadMD3MinsMaxes(const fileHandle_t fh, vec3_t mins, vec3_t maxs);
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
void Cmd_Bounds_f (gentity_t *ent, int iArg) {
	// If no args, trace model and give built in and current
	// If int args, use int as entity num and give built in and current
	// If string args, use as model path
	char *arg = ConcatArgs(1);
	int i = atoi(arg);
	gentity_t *targ = NULL;
	if (!arg[0]) {
		targ = AimAnyTarget(ent, 8192);
	}
	else if (i != 0 || (arg[0] == '0' && arg[1] == 0)) {
		targ = GetEnt(i);
	}

	if (targ)
	{
		const int lineTime = 3000;
		Disp(ent, va(
				CT_B"Mins: "CT_V"%s\n"
				CT_B"Maxs: "CT_V"%s",
				vtos2(targ->r.mins),
				vtos2(targ->r.maxs)));
		Disp(ent, va("^3Contents: ^2%x ^3Clipmask: ^2%x", ent->r.contents, ent->clipmask));

		vec3_t ofs;
		VectorSubtract(targ->r.absmax, targ->r.absmin, ofs);

		// Bottom
		
		vec3_t b1;
		vec3_t b2;
		vec3_t b3;
		vec3_t b4;

		VectorCopy(targ->r.absmin, b1);
		VectorCopy(targ->r.absmin, b2);
		VectorCopy(targ->r.absmin, b3);
		VectorCopy(targ->r.absmin, b4);

		b2[0] += ofs[0];
		b3[1] += ofs[1];
		b4[0] += ofs[0];
		b4[1] += ofs[1];

		G_TestLine(b1, b2, 0xff0000, lineTime);
		G_TestLine(b1, b3, 0xff0000, lineTime);
		G_TestLine(b2, b4, 0xff0000, lineTime);
		G_TestLine(b3, b4, 0xff0000, lineTime);

		// Top

		vec3_t t1;
		vec3_t t2;
		vec3_t t3;
		vec3_t t4;

		VectorCopy(b1, t1);
		VectorCopy(b2, t2);
		VectorCopy(b3, t3);
		VectorCopy(b4, t4);

		t1[2] += ofs[2];
		t2[2] += ofs[2];
		t3[2] += ofs[2];
		t4[2] += ofs[2];

		G_TestLine(t1, t2, 0xff0000, lineTime);
		G_TestLine(t1, t3, 0xff0000, lineTime);
		G_TestLine(t2, t4, 0xff0000, lineTime);
		G_TestLine(t3, t4, 0xff0000, lineTime);


		// Sides
		G_TestLine(b1, t1, 0xff0000, lineTime);
		G_TestLine(b2, t2, 0xff0000, lineTime);
		G_TestLine(b3, t3, 0xff0000, lineTime);
		G_TestLine(b4, t4, 0xff0000, lineTime);

	}
	else if (arg[0]) {
		int configIndex = G_FindConfigstringIndex (arg, CS_MODELS, MAX_MODELS, qfalse);
		if (configIndex > 0) {
			Disp(ent, va(
				CT_B"Mins: "CT_V"%s\n"
				CT_B"Maxs: "CT_V"%s",
				vtos(model_mins[configIndex]),
				vtos(model_maxs[configIndex])));
		}
		else {
			fileHandle_t fh = 0;
			char model[MAX_STRING_CHARS];
			char *p;
			if (!Lmd_Data_IsCleanPath(arg)) {
				Disp(ent, CT_B"Invalid path.");
				return;
			}

			Q_strncpyz(model, arg, sizeof(model));
			// Why someone would want stuff.glm.md3, I dunno... but support it anyway.
			if ((p = strstr(model, ".glm")) && p[4] == 0) {
				Disp(ent, CT_B"Ghoul2 models are not supported.");
				return;
			}
			if (!(p = strstr(model, ".md3")) || p[4] != 0) {
				Q_strcat(model, sizeof(model), ".md3");
			}

			trap_FS_FOpenFile(model, &fh, FS_READ);
			if (!fh)
			{ //try models/ then, this is assumed for registering models
				trap_FS_FOpenFile(va("models/%s", model), &fh, FS_READ);
				//trap_FS_FOpenFile(name, &fh, FS_READ);
				if (!fh)
				{
					Disp(ent, "^3Model not found, or not a valid md3 model.");
					return;
				}
			}

			if (fh) {
				vec3_t mins;
				vec3_t maxs;
				G_ReadMD3MinsMaxes(fh, mins, maxs);
				Disp(ent, va(CT_V"%s", model));
				Disp(ent, va(
					CT_B"Mins: "CT_V"%s\n"
					CT_B"Maxs: "CT_V"%s",
					vtos(mins),
					vtos(maxs)));
				trap_FS_FCloseFile(fh);
			}
		}
	}
	else {
		Disp(ent, CT_B"Usage: \'"CT_C"bounds "CT_AO"[model name | entity number]");
	}
}

void Cmd_BlowUp_f (gentity_t *ent, int iArg) {
	gentity_t *tEnt;

	if(trap_Argc() > 1) {
		tEnt = GetEnt(atoi(ConcatArgs(1)));
	}
	else {
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);
	}
	if (tEnt && tEnt->inuse) {
		BlowUpEntity(tEnt);
		Disp(ent, "^3Entity detonated.");
	}
	else 
		Disp(ent, "^3Invalid entity.");
}

qboolean Action_UndoDelete(gentity_t *ent, Action_t *action){
	char arg[MAX_STRING_CHARS];
	//actions undodelete <arg>
	trap_Argv(2, arg, sizeof(arg));
	if(Q_stricmp(arg, "respawn") == 0){
		SpawnData_t *spawnData;
		gentity_t *spawn;
		Disp(ent, va("^3Restoring entity with spawnstring: \n^2%s", action->strArgs[0]));
		spawnData = ParseEntitySpawnstring(action->strArgs[0]);
		Lmd_Entities_SetSaveable(spawnData, action->iArgs[0]);
		if(!(spawn = spawnEntityFromData(spawnData)))
			Disp(ent, "^3Failed to spawn entity.");
		else{
			Disp(ent, va("^3Entity respawned with entity number ^2%i^3.", spawn->s.number));
			return qtrue;
		}
	}
	else{
		Disp(ent, va("^3Last deleted entity had spawnstring:\n^2%s\n^3Use \'^2/actions undodelete respawn^3\' to respawn it.",
			action->strArgs[0]));
	}
	return qfalse;
}

int Lmd_Entities_getSpawnstringKeyIndex(SpawnData_t *ent, char *key);
char* Lmd_Entities_getSpawnstringKeyValue(SpawnData_t *ent, int index);

qboolean EntityMatches(gentity_t *ent, char *key, char *value, qboolean partial) {
	int index = Lmd_Entities_getSpawnstringKeyIndex(ent->Lmd.spawnData, key);
	char *keyValue;
	if(index == -1)
		return qfalse;
	keyValue = Lmd_Entities_getSpawnstringKeyValue(ent->Lmd.spawnData, index);

	if(partial)
		return (strstr(keyValue, value) != NULL);
	else
		return (Q_stricmp(keyValue, value) == 0);
}

typedef struct Cmd_DelAll_Confirm_Data_s {
	char key[MAX_STRING_CHARS];
	char value[MAX_STRING_CHARS];
}Cmd_DelAll_Confirm_Data_t;


void Cmd_DelAll_Confirm(gentity_t *ent, void *dataptr) {
	Cmd_DelAll_Confirm_Data_t *data = (Cmd_DelAll_Confirm_Data_t *)dataptr;
	gentity_t *trace = NULL;
	int c = 0;
	while(trace = IterateEnts(trace)) {
		if(!trace->Lmd.spawnData)
			continue;
		if(!EntityMatches(trace, data->key, data->value, qfalse))
			continue;
		G_FreeEntity(trace);
		c++;
	}
	Disp(ent, va("^2%i^3 entities with  %s ^2%s deleted.", c, data->key, data->value));

}

void Cmd_DelAll_f(gentity_t *ent, int iArg) {
	int argc = trap_Argc();
	char key[MAX_STRING_CHARS], *value;
	gentity_t *trace = NULL;
	int argbase = 0;
	int c = 0;

	if(argc >= 3) {
		trap_Argv(1, key, sizeof(key));
		value = ConcatArgs(2);
	}
	else if(argc == 2) {
		Q_strncpyz(key, "classname", sizeof(key));
		value = ConcatArgs(1);
	}
	else {
		Disp(ent, "^3Usage: DelAll <field or classname> [value]");
		return;
	}


	while(trace = IterateEnts(trace)) {
		if(!trace->Lmd.spawnData)
			continue;
		if(!EntityMatches(trace, key, value, qfalse))
			continue;
		c++;
	}

	Cmd_DelAll_Confirm_Data_t *data = (Cmd_DelAll_Confirm_Data_t *)G_Alloc(sizeof(Cmd_DelAll_Confirm_Data_t));
	Q_strncpyz(data->key, key, sizeof(data->key));
	Q_strncpyz(data->value, value, sizeof(data->value));
	if (Confirm_Set(ent, Cmd_DelAll_Confirm, data)) {
		Disp(ent, va("^3This command will delete ^2%i^3 entities with %s ^2%s.\n^3This command cannot be undone!", c, key, value));
	}
}

void Cmd_Delent_f(gentity_t *ent, int iArg){
	int index;
	gentity_t *targ = NULL;
	Action_t *action;
	char *spawnstring;
	int sLen;

	if(trap_Argc() < 2){
		targ = AimAnyTarget(ent, 8192);
		if(!targ){
			Disp(ent, "^3You are not aiming at a target.");
			return;
		}
	}
	else{
		index = atoi(ConcatArgs(1));
		targ = GetEnt(index);
		if(!targ || !targ->inuse){
			Disp(ent, "^3Invalid entity.");
			return;
		}
		if(targ->s.number == ENTITYNUM_WORLD) {
			Disp(ent, "^3You cannot delete the worldspawn.");
			return;
		}
	}
	if(!targ->Lmd.spawnData){
		Disp(ent, "^3You cannot delete this entity.");
		return;
	}

	sLen = Lmd_Entites_GetSpawnstringLen(targ->Lmd.spawnData);
	spawnstring = (char *)G_Alloc(sLen);
	Lmd_Entities_getSpawnstring(targ->Lmd.spawnData, spawnstring, sLen);
	Disp(ent, va("^3Deleting entity: ^2%i\n^3Spawnstring:\n^2%s", targ->s.number, spawnstring));
	if((action = PlayerActions_Add(ent, "undodelete", "View the spawnstring of, or respawn, the last deleted entity", Action_UndoDelete, qtrue))){
		action->strArgs[0] = spawnstring;
		action->iArgs[0] = Lmd_Entities_IsSaveable(targ);
	}
	G_FreeEntity(targ);
}

void Cmd_DisableEnt_f(gentity_t *ent, int iArg){
	gentity_t *targ = NULL;
	if(trap_Argc() >= 2){
		char buf[MAX_STRING_CHARS];
		int itarg;
		trap_Argv(1, buf, sizeof(buf));
		itarg = atoi(buf);
		targ = GetEnt(itarg);
	}		
	else{
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		targ = AimAnyTarget(ent, 8024);
	}

	if(!targ || !targ->inuse){
		Disp(ent, "^3Invalid entity.");
		return;
	}

	if(targ->flags & FL_TEAMSLAVE){
		targ = targ->teammaster;

		if(!targ || !targ->inuse){
			Disp(ent, "^3Entity is a part of a team bound to an invalid leader.");
			return;
		}
		Disp(ent, "^3Effect entity auto redirected to team master.");
	}
	if(targ->flags & FL_INACTIVE){
		targ->flags &= ~FL_INACTIVE;
		Disp(ent, "^3Target ^5activated");
	}
	else{
		targ->flags |= FL_INACTIVE;
		Disp(ent, "^3target ^6deactivated");
	}
}

void Cmd_Entitylist_f(gentity_t *ent, int iArg) {
	int argc = trap_Argc();
	int ofs = 0, show = 25, c = 0;
	char key[MAX_STRING_CHARS], *value;
	gentity_t *trace = NULL;
	int argbase = 0;
	qboolean partial = qfalse;
	//entitylist [offset] <key/classname> <value>

	trap_Argv(1, key, sizeof(key));
	if(atoi(key) != 0 || (key[0] == '0' && key[1] == 0)) {
		ofs = atoi(key) - 1;
		if(ofs < 0)
			ofs = 0;
		argbase = 1;
		key[0] = 0;
	}

	if(argc >= 3 + argbase) {
		trap_Argv(1 + argbase, key, sizeof(key));
		value = ConcatArgs(2 + argbase);
	}
	else if(argc == 2 + argbase) {
		Q_strncpyz(key, "classname", sizeof(key));
		value = ConcatArgs(1 + argbase);
		partial = qtrue;
	}
	DispContiguous(ent, "^3-------------------------------------");

	show = -ofs;
	while(trace = IterateEnts(trace)) {
		
		if(key[0] && (!trace->Lmd.spawnData || !EntityMatches(trace, key, value, qtrue)))
			continue;

		c++;

		show++;
		if(show <= 0)
			continue;
		if(show < 25) {
			DispContiguous(ent, va("^2%-7i %s", trace->s.number, FormattedEntString(trace)));
		}
	}
	if(ofs > c)
		DispContiguous(ent, va("^3Offset out of range.  Only ^2%i^3 entities were found.", c));
	else {
		int end = ofs + 25;
		if(end > c)
			end = c;
		DispContiguous(ent, va("^3Displaying ^2%i^3 to ^2%i^3 of ^2%i^3 entities found.", ofs + 1, end, c));
	}
	DispContiguous(ent, "^3-------------------------------------");
	DispContiguous(ent, NULL);
}

//Ufo:
void Cmd_GetEnt_f(gentity_t *ent, int iArg) {
	if (trap_Argc() < 2) {
		Disp(ent, "^3No entity specified.");
		return;
	}
	gentity_t *targ = GetEnt(atoi(ConcatArgs(1)));
	vec3_t dest, temp;

	if(!targ || !targ->inuse || (!iArg && !targ->Lmd.spawnData) || (iArg && targ->client)){
		Disp(ent, "^3Invalid entity.");
		return;
	}

	if (targ->r.bmodel){
		VectorAverage(targ->r.mins, targ->r.maxs, dest);
		if (targ->r.currentAngles[0] || targ->r.currentAngles[1] || targ->r.currentAngles[2]){
			VectorCopy(dest, temp);
			RotatePointAroundVector(dest, axisDefault[0], temp, targ->r.currentAngles[2]);
			VectorCopy(dest, temp);
			RotatePointAroundVector(dest, axisDefault[1], temp, targ->r.currentAngles[0]);
			VectorCopy(dest, temp);
			RotatePointAroundVector(dest, axisDefault[2], temp, targ->r.currentAngles[1]);
		}
		if (iArg)
			VectorAdd(dest, targ->r.currentOrigin, dest);
		else
			VectorSubtract(ent->r.currentOrigin, dest, dest);
	}
	else
		VectorCopy(iArg ? targ->r.currentOrigin : ent->r.currentOrigin, dest);
	if (iArg){
		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
		VectorCopy(dest, ent->client->ps.origin);
		Disp(ent, "^2Teleported to entity.");
	}
	else{
		Lmd_Entities_setSpawnstringKey(targ->Lmd.spawnData, "origin", va("%d %d %d", (int)dest[0], (int)dest[1], (int)dest[2]));
		spawnEntity(targ, targ->Lmd.spawnData);
		Disp(ent, "^2Entity moved to your position.");
	}
}

void Cmd_PlayerSpawnPoint_f (gentity_t *ent, int iArg) 
{
	team_t          team;
	gentity_t		*spawnent;
	//vec3_t          origin;
	char    *classname;
	SpawnData_t * spawnData;

	team = (team_t)iArg;

	if (!ent || !ent->client) {
		return;
	}


	switch (team) {
		case TEAM_RED:
			classname = "info_player_start_red";
			break;
		case TEAM_BLUE:
			classname = "info_player_start_blue";
			break;
		case TEAM_JAILED:
			classname = "info_player_jail";
			break;
		default:
			classname = "info_player_deathmatch";
			break;
	}
	spawnData = ParseEntitySpawnstring(va("classname,%s,origin,%.0f %.0f %.0f,angle,%.0f,", classname, ent->r.currentOrigin[0],
		ent->r.currentOrigin[1], ent->r.currentOrigin[2], ent->client->ps.viewangles[YAW]));
	if(Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
		Lmd_Entities_SetSaveable(spawnData, qtrue);
	spawnent = spawnEntityFromData(spawnData);
	if(!spawnent)
		Disp(ent, "^3Failed to spawn entity.");
	else {
		Disp(ent, va("^3Entity ^2%i^3 has been spawned.", spawnent->s.number));
	}
}

gentity_t *Lmd_logic_entity(int index);
int Lmd_logic_count();
void Cmd_Mapents_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(argc == 1){
		Disp(ent, "^3Usage: mapents {^2count^3} {^2save ^5<name>^3} {^2load ^5<name>^3 ^5[^3'nodefaults'^5]^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "count") == 0){
		int t = 0, c = 0, i = 0;
		int l = 0, lc = Lmd_logic_count();
		for(i = 0;i<MAX_GENTITIES;i++){
			if(g_entities[i].inuse){
				if(g_entities[i].Lmd.spawnData)
					c++;
				t++;
			}
		}
		gentity_t *scan;
		for(i = 0;i<lc;i++){
			scan = Lmd_logic_entity(i);
			if(scan->inuse){
				if(scan->Lmd.spawnData){
					c++;
				}
				l++;
				t++;
			}
		}

		Disp(ent, va(
		"^3Total entities: ^2%i\n"
		"^6Editable entities: ^2%i\n"
		"^5Game entities: ^2%i\n"
		"^2Logic entities: ^2%i", t, c, t - l, l));
		return;
	}
	else if(Q_stricmp(arg, "save") == 0){
		if(argc < 2){
			Disp(ent, "^3Expected name of entities to save.");
			return;
		}
		trap_Argv( 2, arg, sizeof( arg ) );
		SaveEntitiesData(arg);
		Disp(ent, "^2Entities saved.");
	}
	else if(Q_stricmp(arg, "load") == 0){
		qboolean nodefaults = qfalse;
		if(argc < 2){
			Disp(ent, "^3Expected name of entities to load.");
			return;
		}
		trap_Argv(3, arg, sizeof(arg));
		if(Q_stricmp(arg, "nodefaults") == 0)
			nodefaults = qtrue;
		trap_Argv(2, arg, sizeof(arg));
		LoadEntitiesData(arg, nodefaults);
		return;
	}
	else
		Disp(ent, "^3Unknown argument.  For usage information, enter the command without args.");
}

void Cmd_Nearby_f(gentity_t *ent, int iArg){
	int i = 128, count;
	char buf[MAX_STRING_CHARS];
	gentity_t *ent_list[MAX_GENTITIES];
	char search[MAX_STRING_CHARS] = "";
	trap_Argv(1, buf, sizeof(buf));
	if(buf[0])
		i = atoi(buf);
	if(trap_Argc() >= 3){
		trap_Argv(2, search, sizeof(search));
	}
	
	count = G_RadiusList(ent->r.currentOrigin, i, ent, qfalse, ent_list, qtrue);
	for (i = 0; i < count; i++) {
		if(search[0]){
			if(Q_stricmpn(ent_list[i]->classname, search, strlen(search)) != 0)
				continue;
		}
		Disp(ent, va("^2%i ^3(^2%.00f %.00f %.00f^3) ^3%s\n\"", ent_list[i]->s.number, ent_list[i]->s.origin[0], ent_list[i]->s.origin[1], 
			ent_list[i]->s.origin[2], ent_list[i]->classname));
		if(i == 75){
			Disp(ent, "^3Only the first 75 entites were displayed.");
			break;
		}
	}
}

void Cmd_NewMap_f (gentity_t *ent, int iArg)
{
	if (trap_Argc() < 2) {
		return;
	}
	char *arg;
	arg = ConcatArgs(1);
	trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"\n", arg));       
}

void Cmd_NextMap_f(gentity_t *ent, int iArg) {
	trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap");
}

void Cmd_Nudge_f(gentity_t *ent, int iArg){
	gentity_t *targ = NULL;//, *newent;
	vec3_t NudgeNudge, v2;
	SpawnData_t *curSpawn, *backupSpawn;
	char arg[MAX_STRING_CHARS];
	if(trap_Argc() > 5 || trap_Argc() < 4) {
		Disp(ent, "^3Usage: nudge ^5[entity number] ^2<x> <y> <z>");
		return;
	}
	if (trap_Argc() == 5){
		trap_Argv(1, arg, sizeof(arg));
		int EntNum = atoi(arg);
		if(EntNum >= 0) {
			if(EntNum >= MAX_CLIENTS && EntNum < MAX_GENTITIES) {
				targ = &g_entities[EntNum];
			}
			else if(EntNum >= MAX_GENTITIES) {
				targ = Lmd_logic_entity(EntNum - MAX_GENTITIES);
			}
		}

		if(!targ) {
			Disp(ent, "^3Invalid entity index.");
			return;

		}

		trap_Argv(2, arg, sizeof(arg));
		NudgeNudge[0] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		NudgeNudge[1] = atoi(arg);
		trap_Argv(4, arg, sizeof(arg));
		NudgeNudge[2] = atoi(arg);
	} 
	else{
		targ = AimAnyTarget(ent, 8192);
		trap_Argv(1, arg, sizeof(arg));
		NudgeNudge[0] = atoi(arg);
		trap_Argv(2, arg, sizeof(arg));
		NudgeNudge[1] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		NudgeNudge[2] = atoi(arg);
	}
	if(!targ || !targ->Lmd.spawnData){
		Disp(ent, "^3Invalid entity.");
		return;
	}

	if(Lmd_Entities_getSpawnstringKey(targ->Lmd.spawnData, "origin", arg, sizeof(arg)))
		sscanf(arg, "%f %f %f", &v2[0], &v2[1], &v2[2]);
	else
		VectorCopy(vec3_origin, v2);
	VectorAdd(v2, NudgeNudge, v2);

	curSpawn = targ->Lmd.spawnData;
	backupSpawn = cloneSpawnstring(curSpawn);
	Lmd_Entities_setSpawnstringKey(curSpawn, "origin", vtos2(v2));

	if(spawnEntity(targ, curSpawn) == NULL){
		Disp(ent, "^3Entity failed to respawn, reverting...");
		if(spawnEntity(targ, backupSpawn) == NULL)
			Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
	}
	else{
		Disp(ent, "^2Entity nudged.");
		removeSpawnstring(backupSpawn);
	}
}

qboolean Action_UndoPlace(gentity_t *ent, Action_t *action){
	char arg[MAX_STRING_CHARS], *spawnstring;
	int sLen;
	gentity_t *last = NULL;
	if(action->iArgs[0] < MAX_GENTITIES)
		last = &g_entities[action->iArgs[0]];
	else
		last = Lmd_logic_entity(action->iArgs[0] - MAX_GENTITIES);
	if(!last) {
		Disp(ent, "^3Invalid entity number.");
		return qtrue;
	}

	//check to make sure the entity was not deleted already.
	sLen = Lmd_Entites_GetSpawnstringLen(last->Lmd.spawnData);
	spawnstring = (char *)G_Alloc(sLen);
	Lmd_Entities_getSpawnstring(last->Lmd.spawnData, spawnstring, sLen);
	if(!last->inuse || !last->Lmd.spawnData || Q_stricmp(action->strArgs[0], spawnstring) != 0){
		Disp(ent, "^3The last placed entity was deleted or modified.");
		return qtrue;
	}
	
	//actions undoplace <arg>
	trap_Argv(2, arg, sizeof(arg));
	if(Q_stricmp(arg, "delete") == 0){
		Action_t *newAction;
		if((newAction = PlayerActions_Add(ent, "undodelete", "View the spawnstring of, or respawn, the last deleted entity", Action_UndoDelete, qtrue))){
			newAction->strArgs[0] = G_NewString(spawnstring);
			newAction->iArgs[0] = Lmd_Entities_IsSaveable(last);
		}
		G_FreeEntity(last);
		Disp(ent, "^3Last placed entity deleted.  You can undo the deletion with the undodelete action.");
		return qtrue;
	}
	else{
		Disp(ent, va("^3The last placed entity has an entity number of ^2%i^3 and has the spawnstring:\n^2%s",
			action->iArgs[0], spawnstring));
		Disp(ent, "^3Use \'^2/actions undoplace delete^3\' to delete this entity.");
	}
	G_Free(spawnstring);
	return qfalse;
}

//RoboPhred
qboolean isValidClassname(const char *classname);
extern gentity_t *errorMessageTarget;
void Cmd_Place_f (gentity_t *ent, int iArg){
	char classname[MAX_STRING_CHARS];
	char arg[MAX_STRING_CHARS];
	char *string;
	int sLen;
	gentity_t *spawn;
	Action_t *action;
	trace_t tr;
	SpawnData_t *spawnData;

	if (trap_Argc() < 3) {
		Disp(ent, "^3Usage: place <classname> <dist> <field>,<value>,...");
		return;
	}

	trap_Argv(1, classname, sizeof(classname));

	if(!isValidClassname(classname)) {
		Disp(ent, "^3There is no entity by that classname.");
		return;
	}

	string = ConcatArgs(3);

	string = va("classname,%s,%s", classname,string);

	trap_Argv(2, arg, sizeof(arg));
	spawnData = ParseEntitySpawnstring(string);
	if(arg[0] != '*') {
		vec3_t origin, dir;
		int move = atoi(arg);
		VectorCopy(ent->client->renderInfo.eyePoint, origin);
		AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
		VectorNormalize(dir);
		VectorMA(origin, 8192, dir, origin);
		trap_Trace(&tr, ent->client->renderInfo.eyePoint, NULL, NULL, origin, ent->s.number, ent->clipmask);
		VectorCopy(tr.endpos, origin);

		if(move)
			VectorMA(origin, move, tr.plane.normal, origin);
		Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(origin));
	}

	if(Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
		Lmd_Entities_SetSaveable(spawnData, qtrue);

	errorMessageTarget = ent;
	spawn = spawnEntityFromData(spawnData);
	errorMessageTarget = NULL;

	if(!spawn){
		Disp(ent, "^3Failed to spawn entity.");
		Disp(ent, va("^3Try using \'^2/entityinfo %s^3\' for help on spawning this entity.", classname));
		return;
	}

	Disp(ent, va("^3Entity spawned as number ^2%i^3.", spawn->s.number));


	if(classname[0] != '*') {
		if (spawn->r.contents) {
			qboolean change = qfalse;

			if(tr.plane.normal[2] > 0 && spawn->r.mins[2]){
				spawn->s.origin[2] -= spawn->r.mins[2];
				spawn->r.currentOrigin[2] -= spawn->r.mins[2];
				spawn->pos1[2] -= spawn->r.mins[2];
				spawn->pos2[2] -= spawn->r.mins[2];
				spawn->pos3[2] -= spawn->r.mins[2];
				spawn->s.pos.trBase[2] -= spawn->r.mins[2];
				change = qtrue;
			}
			else if(tr.plane.normal[2] < 0 && spawn->r.maxs[2]){
				spawn->s.origin[2] -= spawn->r.maxs[2];
				spawn->r.currentOrigin[2] -= spawn->r.maxs[2];
				spawn->pos1[2] -= spawn->r.maxs[2];
				spawn->pos2[2] -= spawn->r.maxs[2];
				spawn->pos3[2] -= spawn->r.maxs[2];
				spawn->s.pos.trBase[2] -= spawn->r.maxs[2];
				change = qtrue;
			}

			if(change){
				//RoboPhred: messes up doors, done above now.
				//G_SetOrigin(spawn, spawn->s.origin);
				//RoboPhred: reset origin key
				Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(spawn->s.origin));
				trap_LinkEntity(spawn);
			}
		}
	}

	sLen = Lmd_Entites_GetSpawnstringLen(spawn->Lmd.spawnData);
	string = (char *)G_Alloc(sLen);
	Lmd_Entities_getSpawnstring(spawn->Lmd.spawnData, string, sLen);
	if((action = PlayerActions_Add(ent, "undoplace", "View the entity number and spawnstring of the last placed entity, and delete it", Action_UndoPlace, qtrue))){
		action->strArgs[0] = classname; //we dont free the alloced string, so this is fine.
		action->iArgs[0] = spawn->s.number;
	}
}

void placeinfront (gentity_t * ent, const char *string) {
	gentity_t *spawn;
	vec3_t          origin,dir;
	VectorCopy(ent->r.currentOrigin, origin);
	AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(origin, ent->r.maxs[0] + 40,dir, origin);
	if(!(spawn = trySpawn(va("origin,%s,angle,%.0f,%s", vtos2(origin), ent->client->ps.viewangles[YAW], string))))
		Disp(ent, "^3Failed to spawn entity.");
	else
		Disp(ent, va("^3Entity spawned as number ^2%i^3.", spawn->s.number));
}

void Cmd_PlaceCannon_f (gentity_t *ent, int iArg){
	placeinfront(ent, "classname,emplaced_eweb,count,60000,spawnflags,1,");
}

void SP_random_spot (gentity_t *ent);
void Cmd_PlaceRandomSpot_f (gentity_t *ent, int iArg){
	gentity_t *spot;
	if(!(spot = trySpawn(va("classname,random_spot,origin,%s,angle,%.0f", vtos2(ent->client->ps.origin), ent->client->ps.viewangles[YAW])))){
		Disp(ent, "^3Failed to spawn entity.");
		return;
	}
	else
		Disp(ent, va("^3Entity spawned as number ^2%i^3.", spot->s.number));

	if(Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
		Lmd_Entities_SetSaveable(spot->Lmd.spawnData, qtrue);
}

void listRemaps(gentity_t *ent, int offset);
qboolean removeRemap(int index);
void Cmd_Remap_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int argc = trap_Argc();
	if(argc == 1){
		Disp(ent, "^3Usage: remap {^2list ^6[starting offset]^3} {^2remove ^5<index>^3} {^2replace ^5<old shader> <new shader>^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "list") == 0){
		trap_Argv(2, arg, sizeof(arg));
		listRemaps(ent, atoi(arg));
		return;
	}
	else if(Q_stricmp(arg, "remove") == 0){
		int i;
		trap_Argv(2, arg, sizeof(arg));
		i = atoi(arg);
		if(argc < 3 || i < 0 || (arg == 0 && !(arg[0] == '0' && arg[1] == 0))){
			Disp(ent, "^3You must enter the index of the remap to remove.");
			return;
		}
		if(!removeRemap(i))
			Disp(ent, "^3Index out of range.");
		else
			Disp(ent, "^2Remap removed.");
	}
	else if(Q_stricmp(arg, "replace") == 0){
		char old[MAX_QPATH];
		if(argc < 4){
			Disp(ent, "^3You must enter the old shader and the new shader to replace it with.");
			return;
		}
		trap_Argv(2, old, sizeof(old));
		trap_Argv(3, arg, sizeof(arg));
		AddRemap(old, arg, level.time * 0.001);
		trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
		Disp(ent, "^2Remap added.");
	}
	else
		Disp(ent, "^3Uknown argument.  Use the command without args for its usage.");


	//trap_G2API_InitGhoul2Model: one of the args is a custom shader, check it?
}

void Cmd_ShowEntities_f (gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int i, len, val, count = 0;
	gentity_t *point;
	vec3_t dir;
	if (trap_Argc() < 2)
		return;
	VectorSet(dir, -90, 0, 0);
	trap_Argv(1,arg,sizeof(arg));
	val = atoi(arg);
	len = strlen(arg);

	for (i = MAX_CLIENTS; i < level.num_entities && count < 20; i++) {
		point = &g_entities[i];
		if (!point->inuse || (Q_strncmp(arg, point->classname, len) && i != val))
			continue;
		count++;
		gentity_t *te = G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),point->s.origin, dir);
		te->r.svFlags |= SVF_SINGLECLIENT;
		te->r.singleClient = ent->s.number;
	}
}

void Cmd_Spawnstring_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	char arg2[MAX_STRING_CHARS];
	gentity_t *targ;
	int argc = trap_Argc();
	trap_Argv(1, arg, sizeof(arg));
	trap_Argv(2, arg2, sizeof(arg2));

	if(argc < 2){
		Disp(ent, "^3Usage: /spawnstring {view [entity number]} {edit [entity number] <key> <value>} {deletekey <key name>}");
		return;
	}

	if(Q_stricmp(arg, "view") == 0){
		char *str;
		int slen;
		if(argc >= 3){
			int i = atoi(arg2);	
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else
			targ = AimAnyTarget(ent, 2048);

		if(!targ || !targ->inuse){
			Disp(ent, "^3Invalid target.");
			return;
		}
		else if(!targ->Lmd.spawnData) {
			Disp(ent, "^3Target is not a custom entity.");
			return;
		}

		slen = Lmd_Entites_GetSpawnstringLen(targ->Lmd.spawnData);
		str = (char *)G_Alloc(slen);
		Lmd_Entities_getSpawnstring(targ->Lmd.spawnData, str, slen);
		Disp(ent, va("^2%s", str));
		G_Free(str);
	}
	else if(Q_stricmp(arg, "edit") == 0){
		SpawnData_t *curSpawn, *backupSpawn;
		//gentity_t *newent;
		//spawnstring edit <index> <key> <value>
		if(argc < 4){
			Disp(ent, "^3Invalid arguments, expected \'^2spawnstring edit <index> <key> <value>.");
			return;
		}
		if(argc >= 5){
			int i = atoi(arg2);
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else{
			targ = AimAnyTarget(ent, 2048);
		}

		if(!targ || !targ->inuse || !targ->Lmd.spawnData){
			Disp(ent, "^3Invalid target.");
			return;
		}

		trap_Argv(2 + (argc >= 5), arg, sizeof(arg));
		trap_Argv(3 + (argc >= 5), arg2, sizeof(arg2));

		curSpawn = targ->Lmd.spawnData;

		backupSpawn = cloneSpawnstring(curSpawn);
		Lmd_Entities_setSpawnstringKey(curSpawn, arg, arg2);

		if(spawnEntity(targ, curSpawn) == NULL){
			Disp(ent, "^3Entity failed to respawn, reverting...");
			if(spawnEntity(targ, backupSpawn) == NULL)
				Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
		}
		else{
			Disp(ent, va("^3Entity modified."));
			removeSpawnstring(backupSpawn);
		}
	}
	else if(Q_stricmp(arg, "deletekey") == 0){
		SpawnData_t *curSpawn, *backupSpawn;
		if(argc > 3){
			int i = atoi(arg2);
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else
			targ = AimAnyTarget(ent, 2048);

		if(!targ || !targ->inuse || !targ->Lmd.spawnData){
			Disp(ent, "^3Invalid target.");
			return;
		}

		trap_Argv(2 + (argc > 3), arg, sizeof(arg));

		curSpawn = targ->Lmd.spawnData;

		backupSpawn = cloneSpawnstring(curSpawn);
		Lmd_Entities_deleteSpawnstringKey(curSpawn, arg);

		if(spawnEntity(targ, curSpawn) == NULL){
			Disp(ent, "^3Entity failed to respawn, reverting...");
			if(spawnEntity(targ, backupSpawn) == NULL)
				Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
			else
				removeSpawnstring(curSpawn);
		}
		else{
			Disp(ent, va("^3Entity modified."));
			removeSpawnstring(backupSpawn);
		}
	}
	else
		Disp(ent, "^3Invalid argument.");
}

BG_field_t *getField (const char *fieldName);
void getSpawnstringPairs(SpawnData_t *spawnData, KeyPairSet_t *set);
void dispEntityInfo (gentity_t *ent, gentity_t *tEnt){
	KeyPairSet_t set;
	int i;
	memset(&set, 0, sizeof(set));
	DispContiguous(ent, "^3---------------------------------");
	DispContiguous(ent, va("%s ^3(^2%i^3)", FormattedEntString(tEnt), tEnt->s.number));
	if(tEnt->Lmd.group)
		DispContiguous(ent, va("^5group: ^2%s", tEnt->Lmd.group));
	DispContiguous(ent, va("^3constant origin: (^2%s^3)", vtos2(tEnt->s.origin)));
	DispContiguous(ent, va("^3current origin: (^2%s^3)", vtos2(tEnt->r.currentOrigin)));

	if (tEnt->model)
		DispContiguous(ent, va("^3model: ^2%s", tEnt->model));

	if (tEnt->model && tEnt->model[0] == '*')
		DispContiguous(ent, va("^3absmin: (^2%s^3)\n^3absmax: (^2%s^3)", vtos2(tEnt->r.absmin), vtos2(tEnt->r.absmax)));
	else if(VectorCompare(tEnt->r.mins, vec3_origin) == qfalse || VectorCompare(tEnt->r.maxs, vec3_origin) == qfalse)
		DispContiguous(ent, va("^3mins: (^2%s^3)\n^3maxs: (^2%s^3)", vtos2(tEnt->r.mins), vtos2(tEnt->r.maxs)));
	
	if(tEnt->Lmd.spawnData) {
		getSpawnstringPairs(tEnt->Lmd.spawnData, &set);
		for(i = 0; i < set.count; i++) {
			if(Q_stricmp(set.pairs[i].key, "group") == 0 || Q_stricmp(set.pairs[i].key, "origin") == 0 ||
				Q_stricmp(set.pairs[i].key, "classname") == 0 || Q_stricmp(set.pairs[i].key, "model") == 0 ||
				Q_stricmp(set.pairs[i].key, "mins") == 0 || Q_stricmp(set.pairs[i].key, "maxs") == 0) {
					continue;
			}
			DispContiguous(ent, va("^3%s: ^2%s", set.pairs[i].key, set.pairs[i].value));

			if(Q_stricmpn(set.pairs[i].key, "target", 6) == 0 && (set.pairs[i].key[6] == 0 || (set.pairs[i].key[6] >= '1' && set.pairs[i].key[6] <= '6'))) {
				gentity_t *t = NULL;
				while ( (t = G_Find (t, FOFS(targetname), set.pairs[i].value)) != NULL ) {
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}
			}
			else if(Q_stricmp(set.pairs[i].key, "targetname") == 0) {
				gentity_t *t = NULL;
				qboolean hasOne = qfalse;

				//t1
				while ( (t = G_Find (t, FOFS(target), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t2
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target2), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target2"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t3
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target3), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target3"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t4
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target4), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target4"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t5
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target5), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target5"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t6
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target6), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target6"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}
			}
		}
	}

	DispContiguous(ent, NULL);
}

void Cmd_Trace_f (gentity_t *ent, int iArg) {
	gentity_t *tEnt;
	if (trap_Argc() > 1) {
		int entnr;
		char arg[16];
		trap_Argv(1, arg, sizeof(arg));
		entnr = atoi(arg);
		if(entnr == 0 && !(arg[0] == '0' && arg[1] == 0)) {
			Disp(ent, "^3Invalid entity number");
			return;
		}
		tEnt = GetEnt(entnr);
	}
	else {
		if (!ent)
			return;

		G_PlayEffectID(G_EffectIndex("env/hevil_bolt"), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);
	}
	if (!tEnt || !tEnt->inuse){
		Disp(ent, "^3Entity not found");
		return;
	}
	dispEntityInfo(ent,tEnt);
}

void Cmd_UseEnt_f(gentity_t *ent, int iArg){
	gentity_t *tEnt;
	char arg[MAX_STRING_CHARS] = "";
	if(trap_Argc() > 1){
		trap_Argv(1, arg, sizeof(arg));
		if(iArg == 0){
			int num = atoi(arg);
			if(num < MAX_GENTITIES)
				tEnt = &g_entities[num];
			else
				tEnt = Lmd_logic_entity(num - MAX_GENTITIES);
			if(!tEnt || !tEnt->inuse){
				Disp(ent, "^3Invalid entity.");
				return;
			}
		}
	}
	else{
		G_PlayEffectID(G_EffectIndex("env/hevil_bolt"),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);
		if(!tEnt || !tEnt->inuse){
			Disp(ent, "^3Invalid entity.");
			return;
		}
		if(iArg == 1){
			if(!tEnt->targetname){
				Disp(ent, "^3This entity has no targetname.");
				return;
			}
			Q_strncpyz(arg, tEnt->targetname, sizeof(arg));
		}
	}
	if(iArg == 0) {
		GlobalUse(tEnt, ent, ent);
		Disp(ent, "^2Entity used.");
	}
	else if(iArg == 1) {
		G_UseTargets2(ent, ent, arg);
		Disp(ent, "^2Targetname triggered.");
	}
}

void DiagnoseBuilding(gentity_t *ent) {
	//TODO: check for number of random spots, random spots in solids, suchlike.
}

void Cmd_Entityinfo_t(gentity_t *ent, int iArg);

cmdEntry_t entityCommandEntries[] = {
	{"Blowup","[entity number]\nTarget entity is blown up. (Unstable)", Cmd_BlowUp_f, 0, qtrue, 1, 0, 0},
	{"BlueSpawnPoint","\nMakes the current position a spawn point for blue team players.", Cmd_PlayerSpawnPoint_f, TEAM_BLUE, qtrue, 1, 0, 0},
	{"Bounds", "[model | entity number]\nGet the default bounding box of a model, or show the bounding box of an entity.", Cmd_Bounds_f, 0, qtrue, 1, 0, 0},
	{"DelAll", "<field or classname> [value]\nDelete all entities with the given key/value or classname.  Only deletes editable entities.", Cmd_DelAll_f, 0, qtrue, 1, 0, 0},
	{"DelEnt", "[entity number]\nDeletes the specified entity number.  If no entity number is given, delete the ent you are aiming at.", Cmd_Delent_f, 0, qtrue, 1, 0, 0},
	{"Disable", "[entity number]\nDisable or enable the specified entity.  If no entity is given, the entity being targeted will be affected.", Cmd_DisableEnt_f, 0, qtrue, 2, 0, 0},
	{"EntityInfo", "[classname]\nSearches for and gets information about the given entity.  If no classname is given, then the entity being targeted will be used.  If a partial entity name is given, it lists all entities that match the given name.  Otherwise it lists all spawnflags and keys of the found entity.", Cmd_Entityinfo_t, 0, qtrue, 1, 0, 0},
	{"EntityList", "[offset] [key or classname] [value]\nShow all entities, or search entities by spawnstring keys.", Cmd_Entitylist_f, 0, qtrue, 1, 0, 0},
	{"GetEnt", "[entity number]\nMoves the entity into your current position.", Cmd_GetEnt_f, 0, qtrue, 1, 0, 0},
	{"GotoEnt", "[entity number]\nTeleports you to the entity's current position.", Cmd_GetEnt_f, 1, qtrue, 2, 0, 0},
	{"JailSpawnPoint", "\nMakes the current position a jail spawn point.", Cmd_PlayerSpawnPoint_f, TEAM_JAILED, qtrue, 1, 0, 0},
	{"MapEnts", "{^2count^3} {^2save ^5<name>^3} {^2load ^5<name>^3}\nCount, save, and load map entities.", Cmd_Mapents_f, 0, qtrue, 1, 0, 0},
	{"Nearby", "[<distance> [filter]]\nLists all nearby entities.  If the filter arg is given, only entities with the classnames matching the front-partial value will be listed.", Cmd_Nearby_f, 0, qtrue, 1, 0, 0},
	{"NewMap","<mapname>\nLoad a new map.", Cmd_NewMap_f, 0, qtrue, 2, 0, 0},
	{"NextMap","\nGo to the next map in the rotation, if any.", Cmd_NextMap_f, 0, qtrue, 2, 0, 0},
	{"Nudge", "[entity index] <x y z>\nPush the specified entity in the given direction.  If no entity is specified, than the entity being aimed at will be affected.", Cmd_Nudge_f, 0, qtrue, 1, 0, 0},
	{"Place", "<classname> <dist> [field],[value], ...\nSpawn anything. It will spawn at the distance <dist> from the surface you are aiming at. ", Cmd_Place_f, 1, qtrue, 1, 0, 0},
	{"PlaceCannon", "\nSpawns an emplaced gun.", Cmd_PlaceCannon_f, 0, qtrue, 2, 0, (1 << GT_SIEGE)|(1 << GT_BATTLE_GROUND)},
	{"PlaceRandomSpot", "\nPlace a spot for random placements.", Cmd_PlaceRandomSpot_f, 0, qtrue, 1, 0, 0},
	{"PlayerSpawnPoint", "\nMakes the current position a spawn point for players.", Cmd_PlayerSpawnPoint_f, TEAM_FREE, qtrue, 1, 0, 0},
	{"RedSpawnPoint","\nMakes the current position a spawn point for red team players.", Cmd_PlayerSpawnPoint_f, TEAM_RED, qtrue, 1, 0, 0},
	{"Remap", "{^2list ^6[starting offset]^3} {^2remove ^5<index>^3} {^2replace ^5<old shader> <new shader>^3}\nRemap a shader, or list all remaps.  Changes are not permenant.", Cmd_Remap_f, 0, qtrue, 2, 0, 0},
	{"Show", "<entity class name|entity number>\nShow the location of entities.", Cmd_ShowEntities_f, 0, qtrue, 1, 0, 0},
	{"SpawnString", "<view, edit>\nView or edit a spawnstring on an existing entity.", Cmd_Spawnstring_f, 0, qtrue, 1, 0, 0},
	{"Trace", "[entitynumber]\nGet info on entity with number [entitynumber]. If no argument is provided, the target in sight will be investigated.", Cmd_Trace_f, 0, qtrue, 1, 0, 0},
	{"UseEnt", "[number]\nTarget entity is activated.", Cmd_UseEnt_f, 0, qtrue, 1, 0, 0},
	{"UseTarg", "<targetname>\nUse a group of entities by targetname.  If no argument is provided, the targetname of the entity in sight will be used.", Cmd_UseEnt_f, 1, qtrue, 1, 0, 0},
	{NULL}
};
