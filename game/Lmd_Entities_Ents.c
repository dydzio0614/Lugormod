
#include "g_local.h"

#include "Lmd_Entities_Public.h"

#include "Lmd_KeyPairs.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Property.h"
#include "Lmd_Accounts_CustomSkills.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Professions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Inventory.h"
#include "Lmd_Time.h"
#include "Lmd_Interact.h"

int EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount, qboolean logical);

//for fake body
#include "../ghoul2/G2.h"

KeyPairSet_t* getAccCustSkills(int nickIndex);


unsigned char model_frames [MAX_MODELS];
vec3_t model_mins [MAX_MODELS];
vec3_t model_maxs [MAX_MODELS];

void animate_model (gentity_t *self){
	if (self->spawnflags & 2){
		self->s.frame = ((4 + level.time/100) % (self->genericValue4));
		self->nextthink = level.time + 10;
		return;
	}

	if (self->s.frame != self->genericValue5) {
		if (self->s.frame < self->genericValue5) {
			self->s.frame++;
		} else {
			self->s.frame--;
		}
		//self->nextthink = level.time + (50 * self->modelScale[0]);
		self->nextthink = level.time + 75;
		return;
	} 
	self->think = NULL;
}


char* fixpath (char *str){
	char *h, *t;
	t = h = str;
	for (;*h && h - str < MAX_STRING_CHARS;h++) {
		if (*h == '\\'){
			*h = '/';
		}
	}
	h = str;
	if (*h == '/') {
		h++;
	}
	if (Q_strncmp(h, "models/", 7) == 0) {
		h += 7;
	}

	for (;*h 
		&& h - str < MAX_STRING_CHARS 
		&& Q_strncmp(h, ".md3", 4);
	h++) {
		*(t++) = *h;
	}
	*t = '\0';
	//Com_Printf("info: %s\n", str);
	return str;
}

void FixBox(vec3_t mins,vec3_t maxs, vec3_t angles){
	if (VectorLength(angles) == 0) {
		return;
	}
	vec3_t sides[6];
	int i,j;
	vec3_t corners[8];

	AngleVectors(angles, sides[0],sides[1],sides[2]);
	for (i = 0; i < 3;i++) {
		VectorCopy(sides[i],sides[i+3]);
		VectorScale(sides[i],maxs[i],sides[i]);
		VectorScale(sides[i+3],mins[i],sides[i+3]);
	}

	for (i = 0; i < 8;i++) {
		VectorAdd(sides[i % 6], sides[(i+(i>5?2:1)) % 6], corners[i]);
		VectorAdd(corners[i], sides[(i + (i>5?4:2)) % 6], corners[i]);
	}


	VectorCopy(corners[0],mins);
	VectorCopy(corners[0],maxs);
	for (i = 0;i < 8;i++) {
		for (j = 0;j < 3;j++) {
			if (maxs[j] < corners[i][j]) {
				maxs[j] = corners[i][j];
			}
			if (mins[j] > corners[i][j]) {
				mins[j] = corners[i][j];
			}
		}
	}
}

entityInfoData_t model_key_description = {"Model", "The model.  If the model is a glm model, you need to include the \'.glm\' extention.  You do not need the \'map_objects\' folder in the path."};
entityInfoData_t hitbox_key_description = {"Mins/Maxs", "Hitbox data.  Both values are vectors containing the x, y, and z sizes from the model's origin.  Normally, the Mins data is negitive.  If no mins or maxs are specified, then a hitbox is automatically calculated to cover the entire model."};
qboolean SpawnEntModel(gentity_t *ent, qboolean isSolid, qboolean isAnimated){
	qboolean hasModel = qfalse;
	ent->modelScale[1] = ent->modelScale[2] = ent->modelScale[0];
	ent->s.iModelScale = (short)(100 * ent->modelScale[0]);
	if(ent->model && ent->model[0]){
		//Brushmodel
		if(ent->model[0] == '*') {
			trap_SetBrushModel(ent, ent->model);
		}
		else{
			char *model = G_NewString(ent->model);
			hasModel = qtrue;
			VectorCopy(ent->s.angles, ent->s.apos.trBase);
			if (strstr(model, ".glm")){
				ent->s.modelindex = G_ModelIndex(va("models/%s", fixpath(model)));
				ent->s.modelGhoul2 = 1;
				G_SpawnVector( "maxs", "8 8 16", ent->r.maxs);
				G_SpawnVector( "mins", "-8 -8 0", ent->r.mins);
			}
			else{
				ent->s.modelindex = G_ModelIndex(va("models/%s.md3", fixpath(model)));
				vec3_t maxs, mins;
				if(model_frames[ent->s.modelindex]){
					VectorCopy(model_mins[ent->s.modelindex], mins);
					VectorCopy(model_maxs[ent->s.modelindex], maxs);
					FixBox(mins,maxs,ent->s.angles);
				}
				else{
					VectorSet(mins, 8, 8, 16);
					VectorSet(maxs,-8,-8, 0);
				}

				G_SpawnVector( "maxs", vtos2(maxs), ent->r.maxs);
				G_SpawnVector( "mins", vtos2(mins), ent->r.mins);
				if (isAnimated && (model_frames[ent->s.modelindex] > 1)){
					ent->genericValue4 = model_frames[ent->s.modelindex];
					ent->think = animate_model;
					ent->nextthink = level.time;
				}
			}
			G_Free(model);
		}
	}
	if (ent->modelScale[0] > 0) {
		VectorScale(ent->r.mins, ent->modelScale[0], ent->r.mins);
		VectorScale(ent->r.maxs, ent->modelScale[0], ent->r.maxs);
	}
	if(isSolid && !VectorCompare(vec3_origin, ent->r.mins) && !VectorCompare(vec3_origin, ent->r.maxs)){
		ent->r.contents = CONTENTS_SOLID;
		ent->clipmask = MASK_SOLID;
	}
	else{
		ent->r.contents = 0;
	}
	return hasModel;
}

entityInfoData_t usable_key_description[] = {
	{"Profession", "A bitmask of professions. Values: 1 - Jedi, 2 - Merc"},
	{"Level", "Minimum player level."},
	{"MaxLevel", "Maximum player level."},
	{"AdminLevel", "Minimum admin level."},
	{"PlayerFlags", "A bitmask of flags set by the lmd_flagplayer entity."},
	{"Property", "Check if the player can access the property, any rank will work."},
	{"CustomSkill", "Name of a custom skill to check for."},
	{"CustomSkillValue", "Value to compare."},
	{"CustomSkillCompare", "Comparison type.  -1 text, 0 greater or equal, 1 lesser."},
	{"RequireCredits", "If positive, the player must have this many credits or more.  If negitive, the player must have less than this."},
	{NULL, NULL}
};
void PlayerUsableGetKeys(gentity_t *ent){
	G_SpawnInt("profession", "0", &ent->Lmd.UseReq.profession);
	G_SpawnInt("level", "0", &ent->Lmd.UseReq.level);
	G_SpawnInt("maxlevel", "0", &ent->Lmd.UseReq.levelMax);
	G_SpawnInt("adminLevel", "0", &ent->Lmd.UseReq.authLevel);
	G_SpawnInt("playerflags", "0", &ent->Lmd.UseReq.flags);
	G_SpawnString("customskill", "", &ent->Lmd.UseReq.customSkill.skill);
	G_SpawnString("customskillvalue", "", &ent->Lmd.UseReq.customSkill.value);
	G_SpawnInt("customskillcompare", "0", &ent->Lmd.UseReq.customSkill.compare);
	G_SpawnString("property", "", &ent->Lmd.UseReq.prop);
	G_SpawnInt("requirecredits", "", &ent->Lmd.UseReq.credits);
}

char* ProfessionName(int prof);
qboolean PlayerUseableCheck(gentity_t *self, gentity_t *activator){
	int activatorLevel;
	int activatorProf;
	if(!activator || !activator->client)
		return qtrue;

	activatorLevel = PlayerAcc_Prof_GetLevel(activator);
	activatorProf = PlayerAcc_Prof_GetProfession(activator);
	if (self->Lmd.UseReq.authLevel != 0) {
		int rank = Auths_GetPlayerRank(activator);
		if (rank == 0) {
			return qfalse;
		}

		// < 0 means 'any rank'.  larger ranks are lower
		if (self->Lmd.UseReq.authLevel > 0 && rank > self->Lmd.UseReq.authLevel) {
			return qfalse;
		}
	}

	if((self->Lmd.UseReq.profession < 0 && activatorProf != PROF_NONE) || (self->Lmd.UseReq.profession > 0 &&
		(activatorProf <= PROF_BOT || !(self->Lmd.UseReq.profession & (1 << (activatorProf - 3))))))
			return qfalse;

	if (self->Lmd.UseReq.level > 0 && (activatorLevel < self->Lmd.UseReq.level || 
		(self->Lmd.UseReq.levelMax >= self->Lmd.UseReq.level && activatorLevel > self->Lmd.UseReq.levelMax)))
			return qfalse;

	if (self->Lmd.UseReq.level < 0 && -self->Lmd.UseReq.level < activatorLevel)
		return qfalse;

	if(self->Lmd.UseReq.flags > 0 && !(activator->client->Lmd.playerFlags & self->Lmd.UseReq.flags))
		return qfalse;

	if((self->Lmd.UseReq.credits > 0 && (PlayerAcc_GetCredits(activator) < self->Lmd.UseReq.credits)) ||
		(self->Lmd.UseReq.credits < 0 && (PlayerAcc_GetCredits(activator) >= -self->Lmd.UseReq.credits))) {
			return qfalse;
	}

	if(self->Lmd.UseReq.customSkill.skill && self->Lmd.UseReq.customSkill.skill[0]) {
		char *value = PlayerAcc_Custom_GetValue(activator, self->Lmd.UseReq.customSkill.skill);
		if(!value)
			value = "";
		if(self->Lmd.UseReq.customSkill.compare >= 0) {
			int ours = atoi(value);
			int comp = atoi(self->Lmd.UseReq.customSkill.value);
			//using if(!()) for clarity of purpose
			switch(self->Lmd.UseReq.customSkill.compare) {
			default:
			case 0:
				if(!(ours >= comp))
					return qfalse;
				break;
			case 1:
				if(!(ours < comp))
					return qfalse;
				break;
			}
		}
		else if(Q_stricmp(value, self->Lmd.UseReq.customSkill.value) != 0)
			return qfalse;
	}
	if(self->Lmd.UseReq.prop && self->Lmd.UseReq.prop[0] && Accounts_Property_GetPlayerPropRank(activator, self->Lmd.UseReq.prop) == PROPRANK_NONE)
		return qfalse;
	return qtrue;
}


//======================================================
//lmd_light
//======================================================
void use_lmd_light(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(self->genericValue3 == 1){
		self->s.constantLight = self->genericValue2;
		self->genericValue3 = 0;
	}
	else{
		self->s.constantLight = self->genericValue1;
		self->genericValue3 = 1;

	}
}

const entityInfoData_t lmd_light_keys[] = {
	{"Light", "The intensity of the light when turned on."},
	{"Color", "The colors in decimal precent form (0 to 1, 0.5 would be 50%) for the color to display.  Values are red, green, and blue, in that order.  Example: color,1 0 1, would be yellow."},
	{"Offlight", "Same as light, but when toggled off."},
	{"Offcolor", "Same as color, but when toggled off."},
	{NULL, NULL}
};

entityInfo_t lmd_light_info = {
	"This entity lights up an area.  It can be toggled between two colors.",
	NULL,
	lmd_light_keys
};
void lmd_light(gentity_t *ent){

	float light;
	//int r, g, b, i;
	int c, i;
	vec3_t color;

	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "lmd_light") != 0){
		ent->classname = "lmd_light";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_light");
	}
	//qboolean	lightSet, colorSet;
	G_SpawnFloat( "light", "100", &light );
	G_SpawnVector( "color", "1 1 1", color );
	if(light <= 0){
		EntitySpawnError("lmd_light must have a light value greater than zero.");
		G_FreeEntity(ent);
		return;
	}

	for(i = 0;i<3;i++){
		c = color[i] * 255;
		if(c > 255)
			c = 255;
		ent->s.constantLight |= (c << (8*i));
	}
	i = light / 4;
	if(i > 255){
		i = 255;
	}
	ent->s.constantLight |= (i << 24);
	ent->genericValue2 = ent->s.constantLight;



	G_SpawnFloat( "offlight", "0", &light );
	G_SpawnVector( "offcolor", "0 0 0", color );
	if(light > 0){
		for(i = 0;i<3;i++){
			c = color[i] * 255;
			if(c > 255)
				c = 255;
			ent->genericValue1 |= (c << (8*i));
		}
		i = light / 4;
		if(i > 255){
			i = 255;
		}
		ent->genericValue1 |= (i << 24);
	}

	ent->use = use_lmd_light;

	trap_LinkEntity(ent);

}

//======================================================
//lmd_gravity
//======================================================
void reset_lmd_gravity(gentity_t *ent){
	trap_Cvar_Set("g_gravity", va("%i", ent->genericValue1));

}

void Use_lmd_gravity(gentity_t *ent, gentity_t *other, gentity_t *activator){
	if(ent->spawnflags & 2){
		ent->genericValue1 = trap_Cvar_VariableIntegerValue("g_gravity");
		if(ent->wait > 0){
			ent->think = reset_lmd_gravity;
			ent->nextthink = level.time + ent->wait;
		}
		trap_Cvar_Set("g_gravity", va("%i", ent->count));
		return;
	}
	if(!activator->client)
		return;

	activator->client->Lmd.customGravity.value = ent->count;
	if(ent->wait > 0)
		activator->client->Lmd.customGravity.time = level.time + ent->wait;
	else
		activator->client->Lmd.customGravity.time = 0;

}

const entityInfoData_t lmd_gravity_spawnflags[] = {
	{"2", "Change the gravity for the entire level."},
	{NULL, NULL}
};
const entityInfoData_t lmd_gravity_keys[] = {
	{"Count", "New gravity value"},
	{"Time", "Number of seconds to change the gravity for.  0 for no time limit."},
	{NULL, NULL}
};

entityInfo_t lmd_gravity_info = {
	"Change the gravity for a specific player.",
	lmd_gravity_spawnflags,
	lmd_gravity_keys
};

void lmd_gravity(gentity_t *ent){
	float f;
	if(ent->count < 0){
		//invalid gravity set
		EntitySpawnError("lmd_gravity must have a count greater than or equal to zero.");
		G_FreeEntity( ent );
		return;
	}
	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "target_gravity_change") != 0){
		ent->classname = "lmd_gravity";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_gravity");
	}
	G_SpawnFloat("time", "0", &f);
	ent->wait = floor(f * 1000.0f);	

	ent->use = Use_lmd_gravity;

	trap_LinkEntity( ent );

}

//======================================================
//lmd_speed
//======================================================

void reset_lmd_speed(gentity_t *ent){
	trap_Cvar_Set("g_speed", va("%i", ent->genericValue1));

}

void Use_lmd_speed(gentity_t *ent, gentity_t *other, gentity_t *activator){
	if(ent->spawnflags & 2){
		ent->genericValue1 = trap_Cvar_VariableIntegerValue("g_speed");
		if(ent->wait > 0){
			ent->think = reset_lmd_speed;
			ent->nextthink = level.time + ent->wait;
		}
		trap_Cvar_Set("g_speed", va("%i", ent->count));
		return;
	}
	if(!activator->client)
		return;

	activator->client->Lmd.customSpeed.value = ent->count;
	if(ent->wait > 0)
		activator->client->Lmd.customSpeed.time = level.time + ent->wait;
	else
		activator->client->Lmd.customSpeed.time = 0;
}

const entityInfoData_t lmd_speed_spawnflags[] = {
	{"2", "Change the speed for the entire level."},
	{NULL, NULL}
};
const entityInfoData_t lmd_speed_keys[] = {
	{"Count", "New speed value"},
	{"Time", "Number of seconds to change the speed for.  0 for no time limit."},
	{NULL, NULL}
};

entityInfo_t lmd_speed_info = {
	"Change the speed for a specific player.",
	lmd_speed_spawnflags,
	lmd_speed_keys
};

void lmd_speed(gentity_t *ent) {
	float f;
	if(ent->count < 0){
		EntitySpawnError("lmd_speed must have a count value greater than or equal to zero.");
		G_FreeEntity( ent );
		return;
	}
	G_SpawnFloat("time", "0", &f);
	ent->wait = floor(f * 1000.0f);	

	ent->use = Use_lmd_speed;

	trap_LinkEntity( ent );
}

//======================================================
//lmd_toggle
//======================================================
void use_lmd_toggle(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(self->painDebounceTime > level.time)
		return;

	switch(self->genericValue1){
		case 0:
			G_UseTargets2( self, activator, self->target );
			break;
		case 1:
			G_UseTargets2( self, activator, self->target2 );
			break;
		case 2:
			G_UseTargets2( self, activator, self->target3 );
			break;
		case 3:
			G_UseTargets2( self, activator, self->target4 );
			break;
		case 4:
			G_UseTargets2( self, activator, self->target5 );
			break;
		case 5:
			G_UseTargets2( self, activator, self->target6 );
			break;
	}
	self->genericValue1++;
	if(self->genericValue1 == self->count)
		self->genericValue1 = 0;
	self->painDebounceTime = level.time + self->wait;
}

const entityInfoData_t lmd_toggle_keys[] = {
	{"Count", "The highest target number you are using.  Must be greater than 1."},
	{"Target-Target6", "Targets to fire."},
	{NULL, NULL}
};

entityInfo_t lmd_toggle_info = {
	"Targets 1 through \'count\' will be used in order.  One target will be fired per trigger.",
	NULL,
	lmd_toggle_keys
};


void lmd_toggle(gentity_t *ent){
	ent->use = use_lmd_toggle;
	ent->genericValue1 = 0;
	ent->wait *= 1000; //msec

	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "lmd_toggle") != 0){
		ent->classname = "lmd_toggle";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_toggle");
	}

	if(ent->count < 2){
		EntitySpawnError("lmd_toggle must have a count value greater than or equal to 2.");
		G_FreeEntity(ent);
	}
}

//======================================================
//lmd_mover
//======================================================
void use_lmd_mover(gentity_t *ent, gentity_t *other, gentity_t *activator){
	if(ent->genericValue1 == 0){
		//turn us on
		VectorCopy(ent->genericVec1, ent->s.pos.trDelta);
		VectorCopy(ent->genericVec3, ent->s.apos.trDelta);
		ent->s.pos.trType = (trType_t)ent->genericValue11;
		ent->s.apos.trType = (trType_t)ent->genericValue9;
		ent->s.pos.trDuration = ent->genericValue15;
		ent->s.apos.trDuration = ent->genericValue13;
		ent->genericValue1 = 1;
	}
	else{
		//turn us off
		VectorCopy(ent->genericVec2, ent->s.pos.trDelta);
		VectorCopy(ent->genericVec4, ent->s.apos.trDelta);
		ent->s.pos.trType = (trType_t)ent->genericValue10;
		ent->s.apos.trType = (trType_t)ent->genericValue8;
		ent->s.pos.trDuration = ent->genericValue14;
		ent->s.apos.trDuration = ent->genericValue12;
		ent->genericValue1 = 0;
	}
}

const entityInfoData_t lmd_mover_keys[] = {
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"OnDelta", "The movement offset when turned on.  This is a vector, and needs the x, y, and z values specified (like mins/maxs/origin and others).  This is the origin/angles the entity should be at once 'duration' completes."},
	{"OnDuration", "The movement time when turned on.  The object will move Delta units every this many milliseconds.  This may repeat if the right movement type is set."},
	{"OnType", "The movement type when turned on."},
	{"OffDelta", "The movement/rotation offset when turned on.  This is a vector, and needs the x, y, and z values specified (like mins/maxs/origin and others).  This is the origin/angles the entity should be at once 'duration' completes."},
	{"OffDuration", "The movement time when turned on.  The object will move Delta units every this many milliseconds.  This may repeat if the right movement type is set."},
	{"OffType", "The movement type when turned on."},

	{"OnADelta", "The angular rotation offset when turned on.  This is a vector, and needs the x, y, and z values specified (like mins/maxs/origin and others).  This is the origin/angles the entity should be at once 'duration' completes."},
	{"OnADuration", "The angular movement time when turned on.  The object will move Delta units every this many milliseconds.  This may repeat if the right movement type is set."},
	{"OnAType", "The angular movement type when turned on."},
	{"OffADelta", "The angular movement/rotation offset when turned on.  This is a vector, and needs the x, y, and z values specified (like mins/maxs/origin and others).  This is the origin/angles the entity should be at once 'duration' completes."},
	{"OffADuration", "The angular movement time when turned on.  The object will move Delta units every this many milliseconds.  This may repeat if the right movement type is set."},
	{"OffAType", "The angular movement type when turned on."},
	{NULL, NULL}
};

entityInfo_t lmd_mover_info = {
	"A model able to make use of any combination of movement patterns.  Always spawns with its on movement settings.\n"
	"Movement types: 0: Stationary.  1: Interpolation.  2: Linear (no acceleration), move forever.  4: Linear (no aceleration), stop when the duration has passed.  5: Sine, accelerate and reverse after the duration.  6: Gravity.",
	NULL,
	lmd_mover_keys
};
void lmd_mover(gentity_t *ent){
	/*
	[b]lmd_mover[/b]

	[u]Keys[/u]
	Uses standard mins/maxs/model keys.

	ondelta: movement offset when toggled 'on'
	offdelta: movement offset when toggled 'off'
	onadelta: rotation offset when toggled 'on'
	offadelta: rotation offset when toggled 'off'

	onduration: movement duration when toggled 'on'
	offduration: movement duration when toggled 'off'
	onaduration: rotation duration when toggled 'on'
	offaduration: rotation duration when toggled 'off'
	ontype: movement type when toggled 'on'
	offtype: movement type when toggled 'off'
	onatype: rotation type when toggled 'on'
	offatype: rotation type when toggled 'off'

	[u]Examples[/u]
	A rotating object that rotates once every 2 seconds and can be toggled, Starts on:
	onatype: 2
	onaduration: 1000
	onadelta: 0 180 0

	A door that moves 100 units up in 2.5 seconds.  Assume that the door was spawned at location '0 0 0'
	ontype: 3
	ondelta: 0 0 100 //You will need to make this the door origin, with the z value increased by 100
	onduration: 2500

	An object that rotates once every 2 seconds when 'on', and bobbs up and down 30 units every second when 'off'. Starts off:
	onatype: 2
	onaduration: 1000
	onadelta: 0 180 0
	offtype: 5
	offdelta: 0 0 30
	offtime: 1000
	*/

	//DO NOT USE GENERICVALUE4, ravensoft used it to tell the damage system that the object is a bbrush

	if(ent->model && ent->model[0] == '*')
		trap_SetBrushModel( ent, ent->model );
	else{
		SpawnEntModel(ent, ent->spawnflags & 1, qfalse);
		//Hack to get a mover scaled
		ent->s.modelindex2 = ent->s.modelindex;
		ent->s.modelindex = G_ModelIndex("models/items/datapad.glm");
	}
	//in theory, level.time is 0 at this point, but this might be placed by a lugormod cmd
	ent->s.pos.trTime = ent->s.apos.trTime = level.time;

	G_SpawnVector("ondelta", "0 0 0", ent->genericVec1);
	G_SpawnVector("offdelta", "0 0 0", ent->genericVec2);
	G_SpawnVector("onadelta", "0 0 0", ent->genericVec3);
	G_SpawnVector("offadelta", "0 0 0", ent->genericVec4);

	G_SpawnInt("onduration", "0", &ent->genericValue15);
	G_SpawnInt("offduration", "0", &ent->genericValue14);
	G_SpawnInt("onaduration", "0", &ent->genericValue13);
	G_SpawnInt("offaduration", "0", &ent->genericValue12);

	G_SpawnInt("ontype", "0", &ent->genericValue11);
	G_SpawnInt("offtype", "0", &ent->genericValue10);
	G_SpawnInt("onatype", "0", &ent->genericValue9);
	G_SpawnInt("offatype", "0", &ent->genericValue8);

	if(ent->genericValue8 < 0 || ent->genericValue8 > 6 ||
		ent->genericValue9 < 0 || ent->genericValue9 > 6 ||
		ent->genericValue10 < 0 || ent->genericValue10 > 6 ||
		ent->genericValue11 < 0 || ent->genericValue11 > 6) {
		EntitySpawnError("lmd_mover has no movement information.  Removing...");
		G_FreeEntity(ent);
		return;
	}

	VectorCopy(ent->genericVec1, ent->s.pos.trDelta);
	VectorCopy(ent->genericVec3, ent->s.apos.trDelta);
	ent->s.pos.trType = (trType_t)ent->genericValue11;
	ent->s.apos.trType = (trType_t)ent->genericValue9;
	ent->s.pos.trDuration = ent->genericValue15;
	ent->s.apos.trDuration = ent->genericValue13;
	ent->genericValue1 = 1;

	//needed?
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);
	VectorCopy(ent->s.apos.trBase, ent->r.currentAngles);

	VectorCopy(ent->s.origin, ent->pos1);

	ent->use = use_lmd_mover;

	ent->s.eType = ET_MOVER;

	trap_LinkEntity(ent);
}

//======================================================
//lmd_remap
//======================================================
qboolean removeRemap(int index);
void use_lmd_remap(gentity_t *self, gentity_t *other, gentity_t *user){
	if(self->genericValue1 == 0)
		self->genericValue2 = AddRemap(self->target, self->target2, (level.time * 0.001) + self->delay);
	else
		removeRemap(self->genericValue2);
	if(self->spawnflags & 1)
		self->genericValue1 = !self->genericValue1;
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
}

const entityInfoData_t lmd_remap_spawnflags[] = {
	{"1", "Toggle the remap on and off when used."},
	NULL
};
const entityInfoData_t lmd_remap_keys[] = {
	{"Old", "Shader to replace."},
	{"New", "Shader to use in place of the old one."},
	{NULL, NULL}
};
entityInfo_t lmd_remap_info = {
	"Replace a texture or shader in the game with a new texture/shader.  This affects all textures, including map textures, models, sabers, and the ui interface.",
	lmd_remap_spawnflags,
	lmd_remap_keys
};

void lmd_remap(gentity_t *ent){
	/*
	[b]lmd_remap[/b]
	Change a texture on the map.  Use of this will greatly increase the connection time to anyone joining after the remap took place.  Make sure to increase the connection timeout cvar before using this.
	Quake 3 uses 'shaders', which are basicly textures with additional information.  You can try to remak a shader by using the texture name, but it is best to remap it using its shader name.  Shader names are stored INSIDE the .shader files, and are not the shader files themselvs.  I might eventally finish my pk3 scanner and release it for easy finding of shaders.

	Give this entity a targetname to activate it.  If no targetname is given, then the entity will activate the moment it is spawned.

	[u]Spawnflags[/u]
	1: if set, using this toggles the remap on and off.  Otherwise using it will just turn it on, even if it already was.  Leave this off if you plan on using multiple remaps for the same textures.

	[u]Keys[/u]
	old: old shader to replace
	new: new shader
	*/


	if(ent->target)
		G_Free(ent->target);
	if(ent->target2)
		G_Free(ent->target2);
	G_SpawnString("old", NULL, &ent->target);
	G_SpawnString("new", NULL, &ent->target2);
	if(!ent->target || !ent->target[0] || !ent->target2 || !ent->target2[0]){
		EntitySpawnError("lmd_remap must have the keys \'old\' and \'new\'");
		G_FreeEntity(ent);
		return;
	}
	if(ent->targetname)
		ent->use = use_lmd_remap;
	else
		use_lmd_remap(ent, ent, ent);

	ent->r.svFlags = SVF_NOCLIENT;
}

gentity_t *makeFakeBody(gentity_t *ent){
	vec3_t tempVec = {0, 0, 0};//vec3_origin;
	char uinfo[MAX_INFO_STRING];
	char ModelName[MAX_QPATH] = "kyle";
	char SkinName[MAX_QPATH] = "default";
	char fullSkinName[MAX_QPATH];
	char GLAName[MAX_QPATH];
	char *s;
	int skinHandle, handle;
	int i;
	gentity_t *body = G_Spawn();
	if(!body)
		return NULL;

	trap_GetUserinfo(ent->s.number, uinfo, sizeof(uinfo));

	G_CreateFakeClient(body->s.number, &body->client);
	//Ravensoft had this here somewhere (dont remember where), but it cannot be right...
	//memset ( body->client, 0, sizeof(*body->client) );
	body->playerState = &body->client->ps;

	//what if we do this?
	/*cr1
	s = Info_ValueForKey(uinfo, "model");
	if(s)
		Q_strncpyz(ModelName, s, sizeof(ModelName));
	i = strlen(ModelName);
	for(i;i>0;i--){
		if(ModelName[i] == '/'){
			ModelName[i] = 0;
			Q_strncpyz(SkinName, &ModelName[i + 1], sizeof(SkinName));
			break;
		}
	}


	int oldnum = body->s.number;
	body->s = ent->s;
	//cr2
	body->s.eType = ET_NPC;
	//end cr2
	SetupGameGhoul2Model(body, ModelName, SkinName);
	body->s.number = oldnum;
	*/	
	//end cr1
	int oldnum = body->s.number;
	body->s = ent->s;
	body->s.number = oldnum;
	body->client->ps.persistant[PERS_TEAM] = body->client->sess.sessionTeam = ent->client->sess.sessionTeam;

	body->s.teamowner = ent->client->sess.sessionTeam;

	///*c2
	body->s.eType = ET_NPC;
	//*/

	///*c1
	s = Info_ValueForKey(uinfo, "model");
	if(s)
		Q_strncpyz(ModelName, s, sizeof(ModelName));
	i = strlen(ModelName);
	for(i;i>0;i--){
		if(ModelName[i] == '/'){
			ModelName[i] = 0;
			Q_strncpyz(SkinName, &ModelName[i + 1], sizeof(SkinName));
			break;
		}
	}
	//SetupGameGhoul2Model(body, ModelName, SkinName);

	///*

	if(strchr(SkinName, '|'))
		Q_strncpyz(fullSkinName, va("models/players/%s/|%s", ModelName, SkinName), sizeof(fullSkinName));
	else
		Q_strncpyz(fullSkinName, va("models/players/%s/model_%s.skin", ModelName, SkinName), sizeof(fullSkinName));

	skinHandle = trap_R_RegisterSkin(fullSkinName);
	Q_strncpyz(ModelName, va("models/players/%s/model.glm", ModelName), sizeof(ModelName));
	handle = trap_G2API_InitGhoul2Model(&body->ghoul2, ModelName, 0, skinHandle, -20, 0, 0);

	if (handle<0){ //Huh. Guess we don't have this model. Use the default.
		if (body->ghoul2 && trap_G2_HaveWeGhoul2Models(body->ghoul2))
			trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
		body->ghoul2 = NULL;
		trap_G2API_DuplicateGhoul2Instance(precachedKyle, &body->ghoul2);
		Q_strncpyz(ModelName, "models/players/kyle/model.glm", sizeof(ModelName));
		Q_strncpyz(SkinName, "default", sizeof(SkinName));
		//Q_strncpyz(fullSkinName, va("models/players/kyle/model_default.skin", ModelName, SkinName), sizeof(fullSkinName));

	}
	trap_G2API_SetSkin(body->ghoul2, 0, skinHandle, skinHandle);

	GLAName[0] = 0;
	trap_G2API_GetGLAName(body->ghoul2, 0, GLAName);

	if (!GLAName[0] || (!strstr(GLAName, "players/_humanoid/"))){ //a bad model
		trap_G2API_CleanGhoul2Models(&(body->ghoul2));
		body->ghoul2 = NULL;
		trap_G2API_DuplicateGhoul2Instance(precachedKyle, &body->ghoul2);
		Q_strncpyz(ModelName, va("models/players/kyle/model.glm"), sizeof(ModelName));
		Q_strncpyz(SkinName, "default", sizeof(SkinName));
	}

	body->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

	if (SkinName[0]) //append it after a *
		Q_strcat(ModelName, sizeof(ModelName), va("*%s", SkinName));

	body->s.modelindex = G_ModelIndex(ModelName);

	trap_G2API_AttachInstanceToEntNum(body->ghoul2, body->s.number, qtrue);


	trap_G2API_AddBolt(body->ghoul2, 0, "*r_hand");
	trap_G2API_AddBolt(body->ghoul2, 0, "*l_hand");

	//rhand must always be first bolt. lhand always second. Whichever you want the
	//jetpack bolted to must always be third.
	trap_G2API_AddBolt(body->ghoul2, 0, "*chestg");

	//claw bolts
	trap_G2API_AddBolt(body->ghoul2, 0, "*r_hand_cap_r_arm");
	trap_G2API_AddBolt(body->ghoul2, 0, "*l_hand_cap_l_arm");

	trap_G2API_SetBoneAnim(body->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
	trap_G2API_SetBoneAngles(body->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
	trap_G2API_SetBoneAngles(body->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time);

	if (trap_G2API_AddBolt(body->ghoul2, 0, "lower_lumbar") == -1)//check now to see if we have this bone for setting anims and such
		body->noLumbar = qtrue;


	/*
	SetupGameGhoul2Model(body, ModelName, SkinName);
	trap_G2API_GetGLAName(body->ghoul2, 0, GLAName);
	if(Q_stricmp(GLAName, "
	*/
	/*
	trap_G2API_DuplicateGhoul2Instance(ent->ghoul2, &body->ghoul2);
	trap_G2API_AttachInstanceToEntNum(body->ghoul2, body->s.number, qtrue);
	body->localAnimIndex = 0;
	//*/
	
	

	//G_SetAnim(body, NULL, SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
	//G_SetAnim(body, NULL, SETANIM_LEGS, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);

	body->takedamage = qtrue;
	body->health = Q3_INFINITE;
	body->damageRedirect = qtrue;
	body->damageRedirectTo = ent->s.number;

	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);
	VectorCopy(ent->r.currentOrigin, body->s.origin);
	G_SetOrigin(body, body->s.origin);
	body->r.contents = ent->r.contents;

	body->classname = "fakebody";

	trap_LinkEntity(body);
	
	return body;
}

void removePlayerFromBody(gentity_t *player){
	gentity_t *body = &g_entities[player->r.ownerNum];
	if(!body->inuse || Q_stricmp(body->classname, "fakebody") != 0)
		body = NULL;
	if(player && player->client && player->client->pers.connected == CON_CONNECTED){
		player->r.ownerNum = 0; //should be somthing else?  entitynum_none?
	}
}

void camera_disconnect(gentity_t *camera, gentity_t *player, gentity_t *body, vec3_t origin);
void FakeBodyThink(gentity_t *self){
	gentity_t *camera = GetEnt(self->genericValue1);
	if(!camera->inuse || (self->message && Q_stricmp(camera->classname, self->message) != 0))
		camera = NULL;
	if(!camera || !self->activator->inuse || !self->activator->client || self->activator->client->pers.connected != CON_CONNECTED){
		//removePlayerFromBody(self->activator);
		if(camera)
			camera->activator = NULL;
		camera_disconnect(camera, self->activator, self, NULL);
		return;
	}
	self->nextthink = level.time + FRAMETIME;
}

gentity_t* attachPlayerToBody(gentity_t *player){
	gentity_t *body = makeFakeBody(player);
	if(!body)
		return NULL;

	/*
	player->r.svFlags |= SVF_NOCLIENT;
	player->s.eFlags |= EF_NODRAW;
	player->client->ps.eFlags |= EF_NODRAW;
	player->r.contents = 0;
	player->clipmask = 0;
	player->client->Lmd.flags |= SNF_FREEZE;
	VectorCopy(newOrigin, player->client->ps.origin);
	player->client->ps.origin[2] -= 36;
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	*/

	player->r.ownerNum = body->s.number;
	body->activator = player;
	body->think = FakeBodyThink;
	body->nextthink = level.time + FRAMETIME;
	return body;
}

void lmd_body_think(gentity_t *self){
	if(!self->activator || !self->activator->client || self->activator->health <= 0 || 
		self->activator->client->pers.connected != CON_CONNECTED){
			removePlayerFromBody(self->activator);
			G_FreeEntity(self->chain);
			self->activator = NULL;
			return;
	}
	if(!self->chain->inuse || Q_stricmp(self->chain->classname, "fakebody") != 0){
		removePlayerFromBody(self->activator);
		self->activator = NULL;
		return;
	}
	self->nextthink = level.time + FRAMETIME;
}

void use_lmd_body(gentity_t *self, gentity_t *other, gentity_t *activator){
	if (self->activator){
		removePlayerFromBody(self->activator);
		G_FreeEntity(self->chain);
		return;
	}

	if (!activator->client)
		return;

	if(activator->client->ps.forceHandExtend != HANDEXTEND_NONE) //don't use if busy doing something else
		return;

	if(self->target && self->target[0])
		G_UseTargets( self, activator );
	
	gentity_t *body = attachPlayerToBody(activator);
	if(!body)
		return;

	self->genericValue1 = body->s.number;
	body->genericValue1 = self->s.number;
	body->message = G_NewString2("lmd_body");

	VectorCopy(activator->client->ps.origin, self->s.origin2);
	VectorCopy(activator->client->ps.viewangles, self->s.angles2);

	self->activator = activator;
	self->think = lmd_body_think;
	self->delay = level.time + 800;
	self->nextthink = level.time + FRAMETIME; //give a delay so we dont instantly quit
	self->chain = body;
}

entityInfo_t lmd_body_info = {
	"A false player body.  When a player uses this, a clone of their model will appear where they are, doing their last animation.  Damage to the body will transfer to the player.  If the player is not also teleported, they will be stuck inside the newly spawned body.\n"
	"This is unstable, and may crash clients while leaving the server operational.",
	NULL,
	NULL
};


void lmd_body(gentity_t *ent){
	/*
	[b]lmd_body[/b]
	Basically the same fake body as left behind by the misc_camera.  The player that activates this will get a fake body left behind with their model and team settings.  Damage to this fake body will be forwarded to the player.  This entity will NOT move the player, so unless a target_teleporter or such is used at the same time (perferably under the same targetname), then the player will be stuck inside the fake body and will not be able to move.
	You might need a target_delay with 0.01 wait time (1 game frame) for the teleporter, otherwise the entity might be triggered after the teleport and the fake body will appear in the teleport dest with the player.
	When the entity is used again (by any player or entity), the body is removed.  The player that first used it is not teleported, but left wherever they are.
	*/
	ent->use = use_lmd_body;
}

void lmd_pwterminal_interact( gentity_t *self, gentity_t *activator ){
	char pass[MAX_STRING_CHARS];
	if(PlayerUseableCheck(self, activator) == qfalse)
		return;
	trap_Argv(1, pass, sizeof(pass));
	if(!pass[0])
	{
		Disp(activator, "^3Usage: ^2/interact <password>");
		return;
	}
	G_UseTargets2(self, activator, self->target4);

	if(Q_stricmp(self->target3, pass) == 0)
		G_UseTargets2(self, activator, self->target);
	else
		G_UseTargets2(self, activator, self->target2);
}

void lmd_pwterminal_use( gentity_t *self, gentity_t *other, gentity_t *activator ){
	if(PlayerUseableCheck(self, activator) == qfalse)
		return;
	trap_SendServerCommand(activator-g_entities, va("cp \"%s\n^3Enter the password with ^2/interact <password>\"", self->message));
	G_UseTargets2(self, activator, self->target5);
	return;

}

const entityInfoData_t lmd_pwterminal_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Message", "Text to show when used."},
	{"Password", "The correct password."},
	{"Target", "Target to fire when correct password is entered."},
	{"Target2", "Target to fire when incorrect password is entered."},
	{"Target4", "Target to fire when any password is given."},
	{"Target5", "Target to fire when used."},
	{"randPw", "Generates a new, random password of the same length, everytime the entity is (re)spawned."},
	{NULL, NULL}
};

entityInfo_t lmd_pwterminal_info = {
	 "A terminal that fires its target when the correct password is given.  Players can enter a password using \'/interact <password>\'.",
	NULL,
	lmd_pwterminal_keys
};



void lmd_pwterminal(gentity_t *ent){
	/*
	[b]lmd_pwterminal[/b]
	Player must know the right password to use this.
	Passwored is entered though the player command "/interact"

	[u]Keys[/u]
	Uses standard mins/maxs/model keys.

	message: This string will be displayed when the player uses the terminal, in addition to instructions how to use it.
	target: target to fire when pass is right
	target2: target to fire when pass is wrong
	target3: the password
	target4: fired whenever it recieves a password.

	*/
	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "t2_pwterminal") == 0){
		ent->classname = "lmd_pwterminal";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_pwterminal");
	}

	PlayerUsableGetKeys(ent);

	if(!ent->message)
		ent->message = ""; //to stop it printing "(null)"

	if(!ent->target3 || !ent->target3[0])
		G_SpawnString("password", "", &ent->target3);
	if(!ent->target3 || !ent->target3[0]) {
		EntitySpawnError("lmd_pwterminal must have a password key");
		G_FreeEntity(ent);
		return;
	}
	
	//Ufo:
	G_SpawnString("index", "", &ent->fullName);
	G_SpawnInt("randPw", "0", &ent->genericValue7);

	if (ent->genericValue7)
	{
		for(int i = 0; i < strlen(ent->target3); i++) {
			int r = Q_irand(0, 2);
			if(r == 0)
				ent->target3[i] = Q_irand('1', '9'); //skip 0, might look like O
			else if(r == 1)
				ent->target3[i] = Q_irand('A', 'K'); //in jka font, I looks like l
			else if(r == 2)
				ent->target3[i] = Q_irand('M', 'Z');
		}
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "password", ent->target3);
	}

	// allow to ride movers
	//ent->s.groundEntityNum = tr.entityNum;

	SpawnEntModel(ent, qtrue, qfalse);

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	ent->use = lmd_pwterminal_use;
	ent->interact = lmd_pwterminal_interact;
	ent->genericValue1 = -1; //player
	ent->healingclass = ""; //msg

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	trap_LinkEntity(ent);
}

qboolean G_FindEntityTeam(gentity_t *e){
	//adapted from G_FindTeams(void)
	gentity_t *e2;
	int i = 0;
	int c2 = 0;
	if (!e->inuse)
		return qfalse;
	if (!e->team)
		return qfalse;
	if (e->r.contents == CONTENTS_TRIGGER)
		return qfalse;//triggers NEVER link up in teams!

	if (e->flags & FL_TEAMSLAVE)
		e->flags &= ~FL_TEAMSLAVE;
	e->teammaster = e;

	for (i = 0, e2 = &g_entities[0]; i < level.num_entities ; i++,e2++){
		if(e == e2)
			continue;

		if (!e2->inuse)
			continue;
		if (!e2->team)
			continue;
		//override currently existing teams.
		//if (e2->flags & FL_TEAMSLAVE)
		if (!Q_stricmp(e->team, e2->team))
		{
			c2++;
			e2->teamchain = e->teamchain;
			e->teamchain = e2;
			e2->teammaster = e;
			e2->flags |= FL_TEAMSLAVE;

			// make sure that targets only point at the master
			if(e2->targetname && !e2->isAutoTargeted) {
				G_Free(e->targetname);
				e->targetname = e2->targetname;
				e2->targetname = NULL;
			}
		}
	}
	if(c2 > 0)
		return qtrue;
	else
		return qfalse;
}
void Blocked_Door(gentity_t *ent, gentity_t *other);
void InitMover( gentity_t *ent );
void Think_MatchTeam( gentity_t *ent );
void Think_SpawnNewDoorTrigger( gentity_t *ent );
/*
#define MOVER_START_ON		1
#define MOVER_FORCE_ACTIVATE	2
#define MOVER_CRUSHER		4
#define MOVER_TOGGLE		8
#define MOVER_LOCKED		16
#define MOVER_GOODIE		32
#define MOVER_PLAYER_USE	64
#define MOVER_INACTIVE		128
*/
const entityInfoData_t lmd_door_spawnflags[] = {
	{"1", "Reverse open and close positions."},
	{"2", "Activate by force push or pull."},
	{"4", "Crush players when closing."},
	{"8", "Toggle on/off: Stay open or closed until next used."},
	{"16", "Start locked.  Do not open or close until it is used once.  If the shader has a second animation stage, it will show its second stage until unlocked."},
	{"64", "Players can press the use key to trigger this."},
	{"128", "Start disabled.  Do not trigger until used by a target_activate."},
	NULL
};
const entityInfoData_t lmd_door_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Target", "Fired when the door starts moving from the closed position to the open position."},
	{"OpenTarget", "Fired after reaching the \'open\' position."},
	{"Target2", "Fired when it starts moving from the open position to the closed position."},
	{"CloseTarget", "Fire after reaching the \'closed\' position."},
	{"TargetName", "Trigger when an entity uses this.  If not specified, the door will open when someone gets close to it."},
	{"Movement", "Vector cordinates containing the distance to move in each direction.  Default is 0 0 100."},
	{"Speed", "Movement speed (100 default)."},
	{"Wait", "Seconds wait before returning to the closed position (3 default, -1 = never return)."},
	{"Delay", "How many seconds to wait after it is used before moving - default is none."},
	{"Dmg", "Damage to inflict when blocked (2 default, set to negative for no damage).  Requires the \'Crush players\' spawnflag."},
	{"Color", "Constantly emit this color. Buggy."},
	{"Light", "Emit light at this intensity.  Buggy."},
	{"Health", "If set, the door must be shot to open."},
	{"Linear", "If non-0, then move at a constant speed, rather than accelerating and decelerating."},
	{"TeamAllow", "Even if locked, this team can always open and close it just by walking up to it.  Values are: 0 - none, 1 - red, 2 - blue."},
	{"VehOpen", "If non-0, vehicles/players riding vehicles can open this door by getting close."},
	{NULL, NULL}
};

entityInfo_t lmd_door_info = {
	"The lugormod equivalent of func_door.  An entity that moves between 2 states: open and closed.  This can also be used as an elevator.",
	lmd_door_spawnflags,
	lmd_door_keys
};

void lmd_door(gentity_t *ent){
	vec3_t abs_movedir;

	if(ent->team)
		G_FindEntityTeam( ent );

	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "t2_door") != 0){
		ent->classname = "lmd_door";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_door");
	}
	G_SpawnInt("vehopen", "0", &ent->genericValue14);


	SpawnEntModel(ent, qtrue, qfalse);
	//Hack to get a mover scaled
	ent->s.modelindex2 = ent->s.modelindex;
	ent->s.modelindex = G_ModelIndex("models/items/datapad.glm");

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;

	ent->wait *= 1000;

	ent->delay *= 1000;

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );
	if(ent->damage < 0)
		ent->damage = 0;

	G_SpawnInt( "teamallow", "0", &ent->alliedTeam );

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	G_SpawnVector("movement", "0 0 100", abs_movedir);
	VectorAdd(ent->pos1, abs_movedir, ent->pos2);

	// First angles at start
	VectorCopy(ent->s.angles, ent->pos3);

	G_SpawnVector("moveangles", "0 0 0", abs_movedir);
	VectorAdd(ent->s.angles, abs_movedir, ent->pos4);

	ent->blocked = Blocked_Door;

	// if "start_open", reverse position 1 and 2
	if(ent->spawnflags & 1){ //MOVER_START_ON
		vec3_t	temp;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );

		VectorCopy(ent->pos4, temp);
		VectorCopy(ent->pos3, ent->pos4);
		VectorCopy(temp, ent->pos3);
	}

	if ( ent->spawnflags & 16 ) //MOVER_LOCKED
	{//a locked door, set up as locked until used directly
		ent->s.eFlags |= EF_SHADER_ANIM;//use frame-controlled shader anim
		ent->s.frame = 0;//first stage of anim
	}
	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( !(ent->flags&FL_TEAMSLAVE) ) 
	{
		int health;

		G_SpawnInt( "health", "0", &health );
		
		if ( health ) 
		{
			ent->takedamage = qtrue;
		}
		
		if (ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) //so we know it's push/pullable on the client
			ent->s.bolt1 = 1;

		if ( !(ent->spawnflags & 16/*MOVER_LOCKED*/) && (ent->targetname || health || ent->spawnflags & 64/*MOVER_PLAYER_USE*/ ||
			ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/)){
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		} 
		else 
		{//locked doors still spawn a trigger
			ent->think = Think_SpawnNewDoorTrigger;
		}
	}
}

void lmd_terminal_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	char msg[MAX_STRING_CHARS] = "";
	int i = 0;
	if(self->genericValue1 > level.time)
		return;
	self->genericValue1 = level.time + 800;
	if(!PlayerUseableCheck(self, activator))
		return;
	if(self->message){
		Q_strcat(msg, sizeof(msg), va("%s\n^5==============================\n", self->message));
	}

	for(i = 0;i<self->count;i++){
		Q_strcat(msg, sizeof(msg), va("^2%i^3: %s\n", i + 1, self->GenericStrings[i]));
	}
	Q_strcat(msg, sizeof(msg), "^5==============================\n^3Type ^2/interact <cmd>^3 to use a command.");

	if(self->spawnflags & 1 || self->spawnflags & 2)
		trap_SendServerCommand(activator->s.number, va("cp \"%s\"", msg));
	if(!(self->spawnflags & 2)){
		if(!(self->spawnflags & 1))
			trap_SendServerCommand(activator->s.number, va("cp \"%s\n^5Output sent to console\"", self->message));
		Disp(activator, msg);
	}
	G_UseTargets2(self, activator, self->GenericStrings[7]);
}

void lmd_terminal_interact(gentity_t *self, gentity_t *activator){
	char arg1[MAX_STRING_CHARS];
	if(!PlayerUseableCheck(self, activator))
		return;
	trap_Argv(1, arg1, sizeof(arg1));
	int i = atoi(arg1);
	if(!arg1[0])
	{
		lmd_terminal_use(self, activator, activator);
		return;
	}
	//(i == 0 && !(arg1[0] == '0' && arg1[1] == 0)) || 
	if(i <= 0 || i > self->count){
		Disp(activator, va("^3Unknown interaction command, expected number between 1 and %i", self->count));
		return;
	}
	switch(i){
		case 1:
			G_UseTargets2(self, activator, self->target);
			break;
		case 2:
			G_UseTargets2(self, activator, self->target2);
			break;
		case 3:
			G_UseTargets2(self, activator, self->target3);
			break;
		case 4:
			G_UseTargets2(self, activator, self->target4);
			break;
		case 5:
			G_UseTargets2(self, activator, self->target5);
			break;
		case 6:
			G_UseTargets2(self, activator, self->target6);
			break;
		default:
			Disp(activator, "^3Unknown interaction command.");
			return;
	}
	G_UseTargets2(self, activator, self->GenericStrings[8]);
	Disp(activator, "^2Command successful.");
}

const entityInfoData_t lmd_terminal_spawnflags[] = {
	{"1", "Send the output to the player's screen."},
	{"2", "Do not send the output to the player's console."},
	NULL
};
const entityInfoData_t lmd_terminal_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Message", "Message to display when used."},
	{"UseTarget", "Target to fire when the player presses the use key on this."},
	{"GlobalTarget", "Targe to fire when any command is used."},
	{"Cmd-Cmd6", "Name of each command."},
	{"Target-Target6", "Target to use for each command."},
	NULL
};

entityInfo_t lmd_terminal_info = {
	"List up to 6 commands to the player.  The player can activate a command by using \'/interact <command number>\'.",
	lmd_terminal_spawnflags,
	lmd_terminal_keys
};

void lmd_terminal(gentity_t *ent){
	/*
	[b]lmd_terminal[/b]
	Generic interactive terminal.  Will display the targets and descriptions in the console or screen, and will let the player use "/interact" to trigger the target of their choise.

	[u]Spawnflags[/u]
	1: display the possible commands on the screen.

	[u]Keys[/u]
	message: This is the message to display when the terminal is used.  Use this as a description.
	usetarget: target to fire when used
	globaltarget: target to fire when any 


	cmd:
	cmd2:
	cmd3:
	cmd4:
	cmd5:
	cmd6:
	All of the above cmd keys will display as the description for the target when the player does /interact.

	target:
	target2:
	target3:
	target4:
	target5:
	target6:
	All of the above target keys are the targetnames that will be used when the player does "/interact <1 to 6>".
	*/


	char *s = NULL;
	int i;

	PlayerUsableGetKeys(ent);

	G_SpawnString("usetarget", "", &ent->GenericStrings[7]);
	G_SpawnString("globaltarget", "", &ent->GenericStrings[8]);


	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "t2_terminal") == 0){
		ent->classname = "lmd_terminal";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_terminal");
	}

	if(G_SpawnString("cmd", "NULL", &s)){
		ent->GenericStrings[0] = G_NewString2(s);
		ent->count = 1;
	}
	for(i = 1;i<=5;i++){
		if(G_SpawnString(va("cmd%i", i+1), "NULL", &s)){
			ent->GenericStrings[i] = G_NewString2(s);
			ent->count = i + 1;
		}
		else
			break;
	}
	if(ent->count == 0){
		EntitySpawnError("lmd_terminal must have one or more 'cmd' keys for the commands.");
		G_FreeEntity(ent);
		return;
	}

	SpawnEntModel(ent, qtrue, qfalse);

	ent->use = lmd_terminal_use;
	ent->interact = lmd_terminal_interact;

	ent->r.svFlags |= SVF_PLAYER_USABLE;

	if(ent->wait <= 0){
		ent->wait = 3;
	}

	if(!ent->message)
		ent->message = "";

	trap_LinkEntity( ent );
}

qboolean lmd_rentterminal_pay (gentity_t *self, int credits, gentity_t *activator)
{
	int min = 0;
	int sec = 0;
	char msg[MAX_STRING_CHARS];
	int curSeconds = 0;

	if(self->timestamp)
	{
		curSeconds = (self->timestamp - level.time) / 1000;
		if(self->activator == activator)
		{
			if(!(self->spawnflags & 2))
			{
				Disp(activator, "^3This terminal does not allow adding more credits while it is being rented.");
				return qfalse;
			}
		}
		else
		{
			Disp(activator, "^3This terminal is currently being rented by someone else.");
			return qfalse;
		}
	}

	sec = (int)floor((float)self->genericValue1 * ((float)credits / (float)self->count));
	if(self->genericValue2 > 0 && (sec + curSeconds) > self->genericValue2)
	{
		int Msec, Mmin;
		Q_strncpyz(msg, "^3That credit amount goes beyond the max allowed time of ", sizeof(msg));

		Msec = self->genericValue2;
		Mmin = (int)floor((float)Msec / 60.0f);
		Msec -= Mmin * 60;
		if(Mmin > 0)
			Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", Mmin, (Mmin != 1)?"s":""));
		if(Msec > 0)
		{
			if(Mmin > 0)
				Q_strcat(msg, sizeof(msg), " and ");
			Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", Msec, (Msec != 1)?"s":""));
		}
		Q_strcat(msg, sizeof(msg), va(".\n"
			"^3Please enter a credit amount less than or equal to ^2%i^3", (int)floor((float)((self->genericValue2 - curSeconds) / (float)self->genericValue1) * (float)self->count )));
		Disp(activator, msg);
		return qfalse;
	}

	if(self->genericValue3 > 0 && (sec + curSeconds) < self->genericValue3)
	{
		int Msec, Mmin;
		Q_strncpyz(msg, "^3That credit amount goes beyond the min allowed time of ", sizeof(msg));

		Msec = self->genericValue3;
		Mmin = (int)floor((float)Msec / 60.0f);
		Msec -= Mmin * 60;
		if(Mmin > 0)
			Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", Mmin, (Mmin != 1)?"s":""));
		if(Msec > 0)
		{
			if(Mmin > 0)
				Q_strcat(msg, sizeof(msg), " and ");
			Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", Msec, (Msec != 1)?"s":""));
		}
		Q_strcat(msg, sizeof(msg), va(".\n"
			"^3Please enter a credit amount greater than or equal to ^2%i^3", (int)floor((float)((self->genericValue3 - curSeconds) / (float)self->genericValue1) * (float)self->count )));
		Disp(activator, msg);
		return qfalse;
	}

	self->activator = activator;
	if(self->timestamp == 0)
	{
		self->timestamp = level.time;
		G_UseTargets2(self, activator, self->target3);
	}
	self->timestamp += sec * 1000;

	sec = (self->timestamp - level.time) / 1000;
	min = (int)floor((float)sec / 60.0f);
	sec -= min * 60;
	Q_strncpyz(msg, "^3Payment recieved.  The terminal will expire in ", sizeof(msg));
	if(min > 0)
		Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", min, (min != 1)?"s":""));
	if(sec > 0)
	{
		if(min > 0)
			Q_strcat(msg, sizeof(msg), " and ");
		Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", sec, (sec != 1)?"s":""));
	}
	//Q_strcat(msg, sizeof(msg), ".\n");

	Disp(activator, msg);

	//sec = minutes * (credts / cost)
	//(sec / minutes) = credits / cost
	//TODO: test to see if it gives change properly.
	PlayerAcc_SetCredits(activator, PlayerAcc_GetCredits(activator) + (int)floor(((float)sec / (float)self->genericValue1) * (float)self->count));

	return qtrue;
}

void lmd_rentterminal_examine (gentity_t *self, gentity_t *activator)
{
	char msg[MAX_STRING_CHARS] = "";
	int sec;
	int min;

	if(self->message)
		Disp(activator, self->message); //send this as a seperate disp, in case the msg makes us hit MAX_STRING_CHARS

	if(self->timestamp)
	{
		sec = (self->timestamp - level.time) / 1000;
		min = (int)floor((float)sec / 60.0f);
		sec -= min * 60;
		if(self->activator == activator)
			Q_strcat(msg, sizeof(msg), "^2You are renting this terminal.  ^3It will expire in ");
		else
			//Ufo: useful to know
			Q_strcat(msg, sizeof(msg), va("^3This terminal is being rented by ^7%s^3.  ^3It will expire in ", self->activator->client->pers.netname));

		if(min > 0)
			Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", min, (min != 1)?"s":""));
		if(sec > 0)
		{
			if(min > 0)
				Q_strcat(msg, sizeof(msg), " and ");
			Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", sec, (sec != 1)?"s":""));
		}
		Q_strcat(msg, sizeof(msg), ".\n");
	}
	else
		Q_strcat(msg, sizeof(msg), "^3Use \'^2/pay^3\' to rent this terminal.\n");

	sec = self->genericValue1;
	min = (int)floor((float)sec / 60.0f);
	sec -= min * 60;
	Q_strcat(msg, sizeof(msg), va("^3This terminal costs ^2%i^3 credits for every ", self->count));
	if(min > 0)
		Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", min, (min != 1)?"s":""));
	if(sec > 0)
	{
		if(min > 0)
			Q_strcat(msg, sizeof(msg), " and ");
		Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", sec, (sec != 1)?"s":""));
	}
	Q_strcat(msg, sizeof(msg), ".\n");


	if(self->genericValue3 > 0)
	{
		sec = self->genericValue3;
		min = (int)floor((float)sec / 60.0f);
		sec -= min * 60;
		Q_strcat(msg, sizeof(msg), "^3The min rental time is ");
		if(min > 0)
			Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", min, (min != 1)?"s":""));
		if(sec > 0)
		{
			if(min > 0)
				Q_strcat(msg, sizeof(msg), " and ");
			Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", sec, (sec != 1)?"s":""));
		}
		Q_strcat(msg, sizeof(msg), ".\n");
	}

	msg[strlen(msg)-1] = 0;
	Disp(activator, msg);
	msg[0] = 0;

	if(self->genericValue2 > 0)
	{
		sec = self->genericValue2;
		min = (int)floor((float)sec / 60.0f);
		sec -= min * 60;
		Q_strcat(msg, sizeof(msg), "^3The max rental time is ");
		if(min > 0)
			Q_strcat(msg, sizeof(msg), va("^2%i^3 minute%s", min, (min != 1)?"s":""));
		if(sec > 0)
		{
			if(min > 0)
				Q_strcat(msg, sizeof(msg), " and ");
			Q_strcat(msg, sizeof(msg), va("^2%i^3 second%s", sec, (sec != 1)?"s":""));
		}
		Q_strcat(msg, sizeof(msg), ".\n");
	}

	Q_strcat(msg, sizeof(msg), va(
		"^3The rent %s ^3expire if you die.\n"
		"^3The terminal %s ^3more credits after the first payment before it expires.",
		(self->spawnflags & 1)?"^1will":"^2will not",
		(self->spawnflags & 2)?"^2accepts":"^1does not accept"));

	Disp(activator, msg);
}
void lmd_rentterminal_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	char msg[MAX_STRING_CHARS] = "";
	int sec = 0, min = 0;
	if(self->message)
		Q_strcat(msg, sizeof(msg), va("%s\n", self->message));

	Q_strcat(msg, sizeof(msg), "^3This is a rentable terminal.\n");
	if(self->timestamp > 0)
	{
		if(activator == self->activator)
		{
			Q_strcat(msg, sizeof(msg), "^2You are renting this terminal.\n");
			G_UseTargets(self, activator);
		}
		else
		{
			Q_strcat(msg, sizeof(msg), "^1This terminal is currently being rented.\n");
			G_UseTargets2(self, activator, self->target4);
		}
	}
	else
	{
		Q_strcat(msg, sizeof(msg), "^5This terminal is not currently being rented.\n");
		G_UseTargets2(self, activator, self->target5);
	}

	Q_strcat(msg, sizeof(msg), "^3Use \'^2/examine^3\' on this terminal for\n^3more information.");
	trap_SendServerCommand(activator->s.number, va("cp \"%s\"", msg));
}

void lmd_rentterminal_think(gentity_t *ent)
{
	if(ent->timestamp > 0)
	{
		int timeLeft = (ent->timestamp - level.time) / 1000;

		if(!ent->activator || !ent->activator->client || ent->activator->client->pers.connected != CON_CONNECTED ||
			//Ufo: wrong spawnflag
			(ent->spawnflags & 1 && ent->activator->health <= 0))
		{
			ent->activator = NULL;
		}

		if(ent->activator && ent->spawnflags & 4 && (timeLeft == 30 || timeLeft == 15 || timeLeft == 5))
		{
			char msg[MAX_STRING_CHARS] = "";
			if(ent->message)
				Q_strncpyz(msg, va("%s\n", ent->message), sizeof(msg));
			Q_strcat(msg, sizeof(msg), va("^3You have ^2%i^3 seconds left.", timeLeft));
			trap_SendServerCommand(ent->activator->s.number, va("cp \"%s\"", msg));
		}
		if(ent->timestamp <= level.time || ent->activator == NULL)
		{
			ent->timestamp = 0;
			if(ent->activator != NULL)
			{
				char msg[MAX_STRING_CHARS] = "";
				if(ent->message)
					Q_strncpyz(msg, va("%s\n", ent->message), sizeof(msg));
				Q_strcat(msg, sizeof(msg), "^1Your rent has expired.");
				trap_SendServerCommand(ent->activator->s.number, va("cp \"%s\"", msg));
			}
			G_UseTargets2(ent, ent->activator ? ent->activator : ent, ent->target2);
			ent->activator = NULL;
		}
	}
	ent->nextthink = level.time + FRAMETIME;
}

const entityInfoData_t lmd_rentterminal_spawnflags[] = {
	{"1", "The rent will expire when the player dies."},
	{"2", "Player will be able to pay more money after the initial payment to increase the rental time."},
	{"4", "Warn the player when there are 30, 15, and 5 seconds left before the rent runs out."},
	NULL
};
const entityInfoData_t lmd_rentterminal_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Message", "The message to be displayed when the terminal is used."},
	{"Count", "Cost to rent this terminal."},
	{"Minutes", "Number of minutes to gain when payed \'count\' number of credits."},
	{"Wait", "Minimum time to wait between payments, if spawnflag 2 is set."},
	{"MaxTime", "Maximum number of minutes a player can rent."},
	{"MinTime", "Minimum number of minutes a player can rent."},
	{"Target", "Target to fire when used by its current renter."},
	{"Target2", "Target to fire when the rent runs out."},
	{"Target3", "Target to fire when first rented."},
	{"Target4", "Target to fire when used by someone who is not the renter while being rented."},
	{"Target5", "Target to fire when used by anyone when not being rented."},
	{NULL, NULL}
};

entityInfo_t lmd_rentterminal_info = {
	"A terminal that can be rented.  Players pay money using \'pay <credits>\', and then can trigger the terminal's target as many times as they want before the rent runs out.",
	lmd_rentterminal_spawnflags,
	lmd_rentterminal_keys
};


void lmd_rentterminal(gentity_t *ent)
{
	/*
	[b]lmd_rentterminal[/b]
	Same as t2_rentterminal.
	Player pays money to rent this, and can use it as long as their rent doesnt expire.  The player will be informed when their deadline is getting near, and when it expires.  The player who currently holds it is able to pay more money later on.
	Player will be warned at 1 minute, 30 seconds, 15 seconds, and 5 seconds that their rent is running out.

	If the player disconnects or switches teams or spectates, than they forfit the terminal instantly.

	[u]Spawnflags[/u]
	1: The rental will expire of the player dies.
	2: Player will be able to pay more money after their first payment to keep ownership.
	4: Give a warning to the player when there is 30/15/5 seconds left of the rent.

	[u]Keys[/u]
	message: This is the message to be displayed when the teminal is used.  Use this as a description
	count: price per minute.
	minutes: time in minutes to own this (decmals accepted, will be rounded to the nearest second)
	wait: time to wait between payments, if spawnflag 2 is set.
	maxtime: Players will not be able to buy more than this.  Value in minutes.
	mintime: Players will not be able to buy less than this.  Value in minutes.
	target: target to fire when used by the owner
	target2: target to fire when rent runs out (ex: target to a target_teleporter to teleport the player to a location outside the house of which the door is controled by this terminal, to make sure they are not staying in it.
	target3: uses this when rented
	target4: target to fire when used by someone other than the renter while being rented
	target5: target to fire when used by someone and not currently rented

	minutes: GenericValue1
	maxtime: GenericValue2
	mintime: GenericValue3

	expire time: timestamp
	renter: activator
	*/

	float time;


	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "t2_rentterminal") != 0){
		ent->classname = "lmd_rentterminal";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_rentterminal");
	}

	G_SpawnFloat("minutes", "25", &time);
	ent->genericValue1 = time * 60.0f;

	G_SpawnFloat("maxtime", "0", &time);
	ent->genericValue2 = time * 60.0f;

	G_SpawnFloat("mintime", "0", &time);
	ent->genericValue3 = time * 60.0f;

	PlayerUsableGetKeys(ent);

	ent->use = lmd_rentterminal_use;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->examine = lmd_rentterminal_examine;
	ent->pay = lmd_rentterminal_pay;
	ent->think = lmd_rentterminal_think;
	ent->nextthink = level.time + FRAMETIME;

	SpawnEntModel(ent, qtrue, qfalse);

	trap_LinkEntity(ent);
}

void scaleEntity(gentity_t *scaleEnt, int scale);
void lmd_scale_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(self->wait > 0){
		activator->client->Lmd.AutoScale.value = activator->s.iModelScale;
		activator->client->Lmd.AutoScale.time = level.time + self->wait;
	}
	scaleEntity(activator, self->count);
}

const entityInfoData_t lmd_scale_keys[] = {
	{"Scale", "The scale between 1 and 1024 to set the player to.  A scale of 0 will set them to their default scale.  Default 0."},
	{"Time", "Number of seconds for the scale to last.  0 for unlimited.  Default 0."},
	{NULL, NULL}
};

entityInfo_t lmd_scale_info = {
	"Changes a player's scale.",
	NULL,
	lmd_scale_keys
};


void lmd_scale(gentity_t *ent){
	float f = 0;
	G_SpawnInt("scale", "0", &ent->count);
	G_SpawnFloat("time", "0", &f);
	ent->wait = floor(f * 1000.0f);

	if(ent->count < 0) {
		EntitySpawnError("lmd_scale cannot have a negitive scale.");
		return;
	}
	if(ent->wait < 0) {
		EntitySpawnError("lmd_scale cannot have a negitive time.");
	}

	ent->use = lmd_scale_use;

}

//Ufo: adding new options:
const entityInfoData_t lmd_playereffect_keys[] = {
	{"effect", "1: Invincible, 2: Electrocution, 3: Fall to death, 4: Jail, 5: Godmode, 6: Shield, 7: Notarget, 8: Invisible, 9. Undying."},
	{"wait", "Time to play the effect for.  Default 30."},
	{NULL, NULL}
};

entityInfo_t lmd_playereffect_info = {
	"When triggered, this entity will use its target when enabled, and use its target2 when disabled.",
	NULL,
	lmd_playereffect_keys
};

void jailPlayer(gentity_t *targ, int time);
void lmd_playereffect_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	switch(ent->genericValue1) {
	case 1:
		activator->client->ps.eFlags |= EF_INVULNERABLE;
		activator->client->invulnerableTimer = level.time + ent->wait;
		break;
	case 2:
		activator->client->ps.electrifyTime = level.time + ent->wait;
		break;
	case 3:
		activator->client->ps.fallingToDeath = level.time;
		activator->client->Lmd.fallingToDeathReset = level.time + ent->wait;
		break;
	case 4:
		jailPlayer(activator, ent->wait);
		break;
	case 5:
		activator->flags |= FL_GODMODE;
		activator->client->Lmd.godTime = level.time + ent->wait;
		break;
	case 6:
		activator->flags |= FL_SHIELDED;
		activator->client->Lmd.shieldTime = level.time + ent->wait;
		break;
	case 7:
		activator->flags |= FL_NOTARGET;
		activator->client->Lmd.notargetTime = level.time + ent->wait;
		break;
	case 8:
		activator->r.svFlags |= SVF_NOCLIENT;
		activator->s.eFlags |= EF_NODRAW;
		activator->client->ps.eFlags |= EF_NODRAW;
		activator->client->Lmd.invisibleTime = level.time + ent->wait;
		break;
	case 9:
		activator->flags |= FL_UNDYING;
		activator->client->Lmd.undyingTime = level.time + ent->wait;
		break;
	}
}

void lmd_playereffect(gentity_t *ent) {
	G_SpawnInt("effect", "1", &ent->genericValue1);
	ent->use = lmd_playereffect_use;
	if (!ent->wait) {
		ent->wait = 30;
	}
	ent->wait *= 1000; //Ufo: was missing
}

void lmd_gate_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(self->flags & FL_INACTIVE)
		G_UseTargets2(self, activator, self->target2);
	else
		G_UseTargets(self, activator);

}

const entityInfoData_t lmd_gate_spawnflags[] = {
	{"1", "Start off"},
	{NULL, NULL}
};
const entityInfoData_t lmd_gate_keys[] = {
	{"Target", "Target to fire when triggered and enabled."},
	{"Target2", "Target to fire when triggered and disabled."},
	{NULL, NULL}
};

entityInfo_t lmd_gate_info = {
	"When triggered, this entity will use its target when enabled, and use its target2 when disabled.",
	lmd_gate_spawnflags,
	lmd_gate_keys
};

void lmd_gate(gentity_t *ent){
	if(ent->spawnflags & 1)
		ent->flags |= FL_INACTIVE;
	ent->use = lmd_gate_use;
}

void lmd_drop_object_trypickup(gentity_t *ent, gentity_t *other){

	if(ent->pain_debounce_time > level.time)
		return;
	if (other->health < 1)
		return;
	if ( other->client->pers.connected != CON_CONNECTED)
		return;

	if (duelInProgress(&other->client->ps))
		return;
		
	//Ufo:
	if (other->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL)
		return;

	if (other->client->invulnerableTimer > level.time)
		return;

	if ( other->client->ps.pm_type == PM_SPECTATOR )
	{//spectators don't pick stuff up
		return;
	}

	ent->pain_debounce_time = level.time + 500;

	if(PlayerUseableCheck(ent, other) == qfalse)
		return;

	if(ent->genericValue11 == other->s.number && ent->genericValue10 > level.time)
		return;
	
	if(ent->count > 0 && other->client->pers.Lmd.account)
	{
		trap_SendServerCommand(other->s.number, va("cp \"^3You picked up CR ^2%i^3.\"", ent->count));
		PlayerAcc_SetCredits(other, PlayerAcc_GetCredits(other) + ent->count);
	}
	if(ent->noise_index)
		G_Sound(other, CHAN_AUTO, ent->noise_index);
	G_UseTargets(ent, other);
	G_FreeEntity(ent);
}

void lmd_drop_object_touch(gentity_t *ent, gentity_t *other, trace_t *trace) {
	lmd_drop_object_trypickup(ent, other);
}

void lmd_drop_object_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	lmd_drop_object_trypickup(ent, activator);
}

void lmd_drop_object_think(gentity_t *ent){
	if(ent->painDebounceTime > 0 && level.time >= ent->painDebounceTime){
		G_UseTargets2(ent, ent->activator, ent->target2);
		G_FreeEntity(ent);
		return;
	}
	ent->nextthink = level.time + FRAMETIME;
}

void lmd_drop_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(self->painDebounceTime > level.time)
		return;

	gentity_t *dropped = G_Spawn();
	vec3_t dir;
	if(!dropped)
		return;
	if(self->model && self->model[0] == '*'){
		trap_SetBrushModel(dropped, self->model);
	}
	else{
		//work damn you
		//dropped->s.modelindex = G_ModelIndex("models/map_objects/factory/catw2_b.md3");
		dropped->s.modelindex = self->genericValue2;
		dropped->s.modelGhoul2 = self->s.modelGhoul2;
		VectorCopy(self->r.mins, dropped->r.mins);
		VectorCopy(self->r.maxs, dropped->r.maxs);
		VectorCopy(self->modelScale, dropped->modelScale);
		dropped->s.iModelScale = self->s.iModelScale;
	}

	dropped->classname = "lmd_drop_object";
	dropped->s.eType = ET_GENERAL;
	dropped->count = self->count;
	dropped->parent = NULL;

	vec3_t dropOrigin = {0, 0, 0};

	if (self->spawnflags & 1 && activator) {
		if (activator->client && activator->inuse) {
			vec3_t forward, viewAngles;
			VectorCopy(activator->client->ps.viewangles, viewAngles);
			viewAngles[0] = 0;
			AngleVectors(viewAngles, forward, NULL, NULL);
			VectorMA(activator->r.currentOrigin, self->genericValue3, forward, dropOrigin);
		}
		else {
			VectorCopy(activator->s.origin, dropOrigin);
		}
	}
	else {
		VectorCopy(self->s.origin, dropOrigin);
	}

	G_SetOrigin(dropped, dropOrigin);

	dropped->activator = activator;

	AngleVectors(self->s.angles, dir, NULL, NULL);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(vec3_origin, self->genericValue1, dir, dropped->s.pos.trDelta);
	dropped->s.apos.trBase[YAW] = self->s.angles[YAW];
	dropped->s.angles[YAW] = self->s.angles[YAW];

	dropped->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;

	dropped->flags = FL_DROPPED_ITEM|FL_BOUNCE;
	dropped->physicsObject = qtrue;
	dropped->physicsBounce = 0.50;
	dropped->bounceCount = 8;
	dropped->r.ownerNum = dropped->s.number;

	if(self->spawnflags & 2){
		dropped->use = lmd_drop_object_use;

		dropped->r.svFlags |= SVF_PLAYER_USABLE;

		dropped->clipmask = MASK_PLAYERSOLID;
		dropped->r.contents = CONTENTS_SOLID;
	}
	else{
		dropped->touch = lmd_drop_object_touch;

		dropped->r.contents = CONTENTS_TRIGGER;
		dropped->clipmask = MASK_PLAYERSOLID;
	}
	
	if(self->delay > 0)
		dropped->painDebounceTime = level.time + self->delay;
	
	memcpy(&dropped->Lmd.UseReq, &self->Lmd.UseReq, sizeof(dropped->Lmd.UseReq));
	dropped->noise_index = self->noise_index;

	dropped->target = G_NewString2(self->target);
	dropped->target2 = G_NewString2(self->target2);

	dropped->think = lmd_drop_object_think;
	dropped->nextthink = level.time + FRAMETIME;

	trap_LinkEntity(dropped);

	self->painDebounceTime = level.time + self->wait;
}

const entityInfoData_t lmd_drop_spawnflags[] = {
	{"1", "Spawn the item at the player's position."},
	{"2", "Player must use the item to pick it up rather than touching it."},
	{NULL, NULL}
};
const entityInfoData_t lmd_drop_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Target", "Target to fire when picked up."},
	{"Target2", "Target to fire when time runs out."},
	{"Noise", "Sound to make when being picked up"},
	{"Count", "Number of credits to give the player.  If this is set and no noise key is specified, then the noise key defaults to sound/interface/secret_area.wav"},
	{"Velocity", "Speed at which to toss this item.  Default 50."},
	{"Offset", "Offset to drop from the player if spawnflag 4 is set.  Default 64."},
	{"Angles", "Angles to launch this item at"},
	{"Time", "Number of seconds to stay for.  Set this to -1 for no time limit.  Default 30."},
	{"Wait", "Number of seconds to wait between triggerings."},
	{NULL, NULL}
};

entityInfo_t lmd_drop_info = {
	"Create and launch a model that can be picked up by players.",
	lmd_drop_spawnflags,
	lmd_drop_keys
};

void lmd_drop(gentity_t *ent){
	char *t = NULL;
	if(!ent->model || !ent->model[0]){
		if(ent->model)
			G_Free(ent->model);
		ent->model = G_NewString2("models/items/datapad.glm");
	}
	if(G_SpawnString("noise", NULL, &t))
	{
		ent->noise_index = G_SoundIndex(t);
		G_Free(t);
	}
	else if(ent->count > 0)
	{
		ent->noise_index = G_SoundIndex("sound/interface/secret_area.wav");
	}
	SpawnEntModel(ent, qfalse, qfalse);
	ent->genericValue2 = ent->s.modelindex;
	ent->s.modelindex = 0;
	ent->r.bmodel = qfalse;
	PlayerUsableGetKeys(ent);
	G_SpawnInt("velocity", "50", &ent->genericValue1);
	G_SpawnInt("offset", "64", &ent->genericValue3);

	G_SpawnInt("time", "30", &ent->delay);
	if(ent->delay == 0)
		ent->delay = 30;
	ent->delay *= 1000.0f;

	ent->wait *= 1000.0f;

	ent->use = lmd_drop_use;
}

void lmd_flagplayer_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(!activator->client)
		return;
	if(self->count > 0)
		activator->client->Lmd.playerFlags |= self->count;
	else
		activator->client->Lmd.playerFlags &= ~(-self->count);
}

const entityInfoData_t lmd_flagplayer_keys[] = {
	{"Flags", "Bitmask of values to add to the player.  If value is negitive, the flags will be removed."},
	{NULL, NULL}
};

entityInfo_t lmd_flagplayer_info = {
	"Set temporary values on a player.  These flags can be used by any entity that supports the usability keys.  Flags are cleared on player death.  This is unrelated to the capture the flag gametype.",
	NULL,
	lmd_flagplayer_keys
};


void lmd_flagplayer(gentity_t *ent){
	if(!G_SpawnInt("flags", "0", &ent->count)){
		G_FreeEntity(ent);
		return;
	}
	ent->use = lmd_flagplayer_use;
}

qboolean Inventory_RunSpawner(gentity_t *spawner, gentity_t *activator);
qboolean Inventory_SetupSpawner(gentity_t *ent);
void lmd_iobject_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	if(!activator->client || activator->s.number >= MAX_CLIENTS)
		return;
	Inventory_RunSpawner(self, activator);
}

const entityInfoData_t lmd_iobject_keys[] = {
	{"Type", "The inventory type to use."},
	{"Name", "The name the item should use.  Defaults to the type name"},
	{"NoDelete", "Don't let the user delete this item."},
	{"NoCombine", "Do not let this item combine with other items, if applicable."},
	{"...", "Extra item-specific keys."},
	NULL
};

entityInfo_t lmd_iobject_info = {
	"Gives the user the specified inventory object.",
	NULL,
	lmd_iobject_keys
};

void lmd_iobject(gentity_t *ent){
	if(!G_SpawnString("type", "", &ent->fullName))
		G_SpawnString("item", "", &ent->fullName);
	if(!Inventory_SetupSpawner(ent)){
		EntitySpawnError("Invalid item in lmd_iobject ");
		G_FreeEntity(ent);
		return;
	}
	ent->use = lmd_iobject_use;
}


/*
qboolean CheckRestrictDamage(gentity_t *player) {
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for(i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if(Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE) && ent->spawnflags & 1)
			return qtrue;
	}
	return qfalse;
}

qboolean CheckRestrictForcepowers(gentity_t *player) {
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for(i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if(Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE) && ent->spawnflags & 2)
			return qtrue;
	}
	return qfalse;
}

qboolean CheckRestrictJetpack(gentity_t *player) {
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for(i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if(Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE) && ent->spawnflags & 4)
			return qtrue;
	}
	return qfalse;
}

qboolean CheckRestrictDuel(gentity_t *player) {
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for(i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if(Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE) && ent->spawnflags & 8)
			return qtrue;
	}
	return qfalse;
}

qboolean CheckRestrictWeaponFire(gentity_t *player) {
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for (i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if (Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE) && ent->spawnflags & 16)
			return qtrue;
	}
	return qfalse;
}
*/

//Ufo:
int EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount, qboolean logical);
void CheckRestrictAll(gentity_t *player) {
	player->client->Lmd.restrict = 0;
	int entList[MAX_GENTITIES];
	int count = EntitiesInBox(player->r.absmin, player->r.absmax, entList, MAX_GENTITIES, qtrue);
	int i;
	gentity_t *ent;
	for (i = 0; i < count; i++) {
		ent = GetEnt(entList[i]);
		if (Q_stricmp(ent->classname, "lmd_restrict") == 0 && !(ent->flags & FL_INACTIVE))
			player->client->Lmd.restrict |= ent->spawnflags;
	}
}

const entityInfoData_t lmd_restrict_spawnflags[] = {
	{"1", "Players in this area will not take damage."},
	{"2", "Players in this area will not be able to use forcepowers."},
	{"4", "Players in this area will not be able to use their jetpack."},
	{"8", "Players in this area will not be able to duel.  Existing duels will be broken if a player enters it."},
	{ "16", "Players in this area will not be able to fire weapons.  Players may still see the weapon fire animation." },
	{"128", "Start disabled.  Must be used by a target_activate to have any effect."},
	{NULL, NULL}
};
const entityInfoData_t lmd_restrict_keys[] = {
	{"#HITBOX", NULL},
	NULL
};

entityInfo_t lmd_restrict_info = {
	"Restrict certain player actions or events within the given area.",
	lmd_restrict_spawnflags,
	lmd_restrict_keys
};

void lmd_restrict(gentity_t *ent) {

	G_SpawnVector("mins", "0 0 0", ent->r.mins);
	G_SpawnVector("maxs", "0 0 0", ent->r.maxs);

	ent->r.contents = CONTENTS_TRIGGER;
	ent->r.svFlags = SVF_NOCLIENT;

	if(!ent->spawnflags || (ent->spawnflags == 128)) {
		EntitySpawnError("lmd_restrict with no spawnflags will not restrict anything.  Removing...");
		G_FreeEntity(ent);
		return;
	}

	if(ent->spawnflags & 128)
		ent->flags |= FL_INACTIVE;

	trap_LinkEntity(ent);
}

void lmd_playercheck_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	if(PlayerUseableCheck(self, activator))
		G_UseTargets(self, activator);
	else
		G_UseTargets2(self, activator, self->target2);
}

const entityInfoData_t lmd_playercheck_keys[] = {
	{"#UKEYS", NULL},
	{"Target", "Target to fire if the player meets the usability keys."},
	{"Target2", "Target to fire if the player does not meet the usability keys."},
	NULL
};

entityInfo_t lmd_playercheck_info = {
	"Fires its target if the user meets the usability keys, target2 if not.",
	NULL,
	lmd_playercheck_keys
};


void lmd_playercheck(gentity_t *ent){
	PlayerUsableGetKeys(ent);
	ent->use = lmd_playercheck_use;
}

void lmd_chance_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	int num = Q_irand(1, self->count);
	if(num <= self->genericValue1) {
		G_UseTargets(self, activator);
	}
	else if(num <= self->genericValue2) {
		G_UseTargets2(self, activator, self->target2);
	}
	else if(num <= self->genericValue3) {
		G_UseTargets2(self, activator, self->target3);
	}
	else if(num <= self->genericValue4) {
		G_UseTargets2(self, activator, self->target4);
	}
	else if(num <= self->genericValue5) {
		G_UseTargets2(self, activator, self->target5);
	}
	else if(num <= self->genericValue6) {
		G_UseTargets2(self, activator, self->target6);
	}
}

const entityInfoData_t lmd_chance_keys[] = {
	{"Count", "The maximum random number to generate."},
	{"Chance, Chance2, ..., Chance6", "If the random number is less than or equal to the given value, then fire the relevant target."},
	{NULL, NULL}
};

entityInfo_t lmd_chance_info = {
	 "Fire one of six targets based on a random number.",
	NULL,
	lmd_chance_keys
};

void lmd_chance(gentity_t *ent) {

	if(ent->count < 1){
		EntitySpawnError("lmd_chance must have a count key greater than or equal to 1.");
		G_FreeEntity(ent);
		return;
	}

	G_SpawnInt("chance", "", &ent->genericValue1);
	G_SpawnInt("chance2", "", &ent->genericValue2);
	G_SpawnInt("chance3", "", &ent->genericValue3);
	G_SpawnInt("chance4", "", &ent->genericValue4);
	G_SpawnInt("chance5", "", &ent->genericValue5);
	G_SpawnInt("chance6", "", &ent->genericValue6);

	ent->use = lmd_chance_use;
}

//FIXME: Using ravensoft's wrong info for these.  Find the real values!
const entityInfoData_t lmd_train_spawnflags[] = {
	{"1", "Start moving as soon as the map loads."},
	{"2", "Toggle movemen when used."},
	{"4", "Stop when blocked."},
	{"32", "Keep moving when blocked, crush through the blocking player."},
	{"64", "Usable by players."},
	{"128", "Start disabled."},
	NULL
};
const entityInfoData_t lmd_train_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Speed", "Movement speed.  Default 100."},
	{"Dmg", "Damage to inflict when blocked."},
	{"Target", "The first path_corner to move to."},
	{"Color", "Vector of red, blue, and green values."},
	{"Light", "Intensity of emitted color."},
	{"Wait", "Time to wait between uses, if spawnflag 64 is set."},
	{NULL, NULL}
};

entityInfo_t lmd_train_info = {
	 "A train that moves between path_corner target points.",
	lmd_train_spawnflags,
	lmd_train_keys
};


void Reached_Train( gentity_t *ent );
void Think_SetupTrainTargets( gentity_t *ent );
void lmd_train (gentity_t *self) {
	VectorClear (self->s.angles);

	if (self->spawnflags & 4) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	//RoboPhred
	if(lmd_enforceentwait.integer && self->spawnflags & 64 && self->wait < lmd_enforceentwait.integer)
		self->wait = lmd_enforceentwait.integer; 

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf ("func_train without a target at %s\n", vtos(self->r.absmin));
		G_FreeEntity( self );
		return;
	}

	//RoboPhred: trains without targetnames start on, and we give it a targetname if it doesnt have one, so
	if(self->targetname)
		self->spawnflags |= 1; //start on
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	SpawnEntModel(self, qtrue, qfalse);
	//Hack to get a mover scaled
	self->s.modelindex2 = self->s.modelindex;
	self->s.modelindex = G_ModelIndex("models/items/datapad.glm");
}

void lmd_customskill_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if(!activator->client)
		return;
	if(ent->spawnflags & 1) {
		PlayerAcc_Custom_SetValue(activator, ent->fullName, ent->message);
	}
	else {
		char *val = PlayerAcc_Custom_GetValue(activator, ent->fullName);
		if(!val)
			val = "0";
		PlayerAcc_Custom_SetValue(activator, ent->fullName, va("%i", atoi(val) + atoi(ent->message)));
	}
}

const entityInfoData_t lmd_customskill_spawnflags[] = {
	{"1", "Direct set.  Use this to override the value or set it to a string."},
	{NULL, NULL},
};
const entityInfoData_t lmd_customskill_keys[] = {
	{"Skill", "The custom skill name to set."},
	{"Value", "If spawnflag 1 is not set, then this is added to the origional as a non-decimal number.  Else, it is directly set."},
	{NULL, NULL},
};

entityInfo_t lmd_customskill_info = {
	"Set a custom skill value to a registered player.  No affect on players who are not logged in.  No affect on profession skills.",
	lmd_customskill_spawnflags,
	lmd_customskill_keys
};

void lmd_customskill(gentity_t *ent){
	if(!G_SpawnString("skill", "", &ent->fullName) || !G_SpawnString("value", "", &ent->message)) {
		EntitySpawnError("lmd_customskill must have both a skill key and value key.");
		G_FreeEntity(ent);
		return;
	}
	ent->use = lmd_customskill_use;
}


const entityInfoData_t lmd_event_spawnflags[] = {
	{"1", "For DeathTarget and KillTarget, Both the attacker and target must be inside the trigger"},
	{"2", "DeathTarget is only called for a combat death (No suicides or untriggered entity damage)."},
	{"128", "Start deactivated."},
	{NULL, NULL}
};
const entityInfoData_t lmd_event_keys[] = {
	{"#UKEYS", NULL},
	{"#HITBOX", NULL},
	{"Target", "Target to fire if a player enters this area.  The trigger will not refire on this player until they exit and re-enter"},
	{"Exittarget", "Target to fire when a player exits this area.  This will never be called without Target firing first."},
	//{"Attacktarget", "Target to fire when a player uses attack or alt attack."},
	//{"Forcetarget", "Target to fire when a player uses an active forcepower (IE any but jump)."},
	{"DeathTarget", "Target to fire when a player dies."}, //target5
	{"KillTarget", "Target to fire when a player kills another player."}, //target6
	{NULL, NULL}
};

entityInfo_t lmd_event_info = {
	"Trigger targetnames on various player events.",
	lmd_event_spawnflags,
	lmd_event_keys
};

/*
	genericValue1: remembered clients 1
	genericValue2: remembered clients 2
*/

void lmd_event_think(gentity_t *ent) {
	if (!(ent->flags & FL_INACTIVE)) //Ufo: was missing
	{
		int entitylist[MAX_GENTITIES];
		int count = trap_EntitiesInBox(ent->r.absmin, ent->r.absmax, entitylist, MAX_GENTITIES);
		int i, val;
		int touched[2] = {0};
		gentity_t *targ;
	
		for(i = 0; i < count; i++) {
			if(entitylist[i] >= MAX_CLIENTS)
				continue;
			if(!g_entities[entitylist[i]].inuse || !g_entities[entitylist[i]].client)
				continue;
			if(g_entities[entitylist[i]].health <= 0)
				continue;
			if(entitylist[i] < 16) //Ufo: was <=, client no. 16 wouldn't trigger
				touched[0] |= (1 << entitylist[i]);
			else
				touched[1] |= (1 << (entitylist[i] - 16));
		}
		
	
		for(i = 0; i < 16; i++) {
			val = (1 << i);
	
			//0-15 (gv1)
			targ = &g_entities[i];
			if(PlayerUseableCheck(ent, targ)) {
				//In list but not touched
				if((ent->genericValue1 & val) && !(touched[0] & val)) {
					//Exited
					G_UseTargets2(ent, targ, ent->target2);
				}
	
				//Touched but not in list
				if((touched[0] & val) && !(ent->genericValue1 & val)) {
					//Entered
					G_UseTargets(ent, targ);
				}
			}
	
			//16-31 (gv2)
			targ = &g_entities[i + 16];
			if(PlayerUseableCheck(ent, targ)) {
				//In list but not touched
				if((ent->genericValue2 & val) && !(touched[1] & val)) {
					//Exited
					G_UseTargets2(ent, targ, ent->target2);
				}
	
				//Touched but not in list
				if((touched[1] & val) && !(ent->genericValue2 & val)) {
					//Entered
					G_UseTargets(ent, targ);
				}
			}
		}
	
		ent->genericValue1 = touched[0];
		ent->genericValue2 = touched[1];
	}

	ent->nextthink = level.time + FRAMETIME;
}

void lmd_event_playerhurt(gentity_t *player, gentity_t *attacker) {
	
}

void lmd_event_playerkilled(gentity_t *player, gentity_t *attacker, int meansOfDeath) {
	gentity_t *trig = NULL;
	qboolean playerIn, attackerIn;
	//Tecnically we only need logical ents here.
	while(trig = IterateEnts(trig)) {
		if(Q_stricmp(trig->classname, "lmd_event") != 0)
			continue;
		if (trig->flags & FL_INACTIVE) //Ufo: was missing
			continue;
		playerIn = attackerIn = qfalse;
		if(trap_EntityContact(trig->r.absmin, trig->r.absmax, player))
			playerIn = qtrue;
		if(trap_EntityContact(trig->r.absmin, trig->r.absmax, attacker))
			attackerIn = qtrue;

		if (trig->spawnflags & 2) {
			if (player == attacker || attacker == NULL || meansOfDeath == MOD_SUICIDE) {
				continue;
			}
		}

		//player died
		if(playerIn && (!(trig->spawnflags & 1) || attackerIn) && PlayerUseableCheck(trig, player))
			G_UseTargets2(trig, player, trig->target5);

		//attacker killed
		if(attackerIn && (!(trig->spawnflags & 1) || playerIn) && PlayerUseableCheck(trig, attacker))
			G_UseTargets2(trig, attacker, trig->target6);
	}
}

void lmd_event(gentity_t *ent) {
	if(ent->spawnflags & 512)
		ent->flags |= FL_INACTIVE;

	ent->think = lmd_event_think;
	ent->nextthink = level.time + FRAMETIME;

	PlayerUsableGetKeys(ent);

	G_SpawnString("exittarget", "", &ent->target2);
	G_SpawnString("deathtarget", "", &ent->target5);
	G_SpawnString("killtarget", "", &ent->target6);

	G_SpawnVector("mins", "0 0 0", ent->r.mins);
	G_SpawnVector("maxs", "0 0 0", ent->r.maxs);

	ent->r.contents = CONTENTS_TRIGGER;
	ent->r.svFlags = SVF_NOCLIENT;

	trap_LinkEntity(ent);
}

void lmd_interact_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	Interact_Set(activator, self, self->s.origin, self->radius, self->message);
}

void lmd_interact_interact(gentity_t *self, gentity_t *activator){
	char arg[MAX_STRING_CHARS];
	int i, index = -1;
	if(!PlayerUseableCheck(self, activator))
		return;

	trap_Argv(1, arg, sizeof(arg));

	for(i = 0; i < 6; i++) {
		if(Q_stricmp(arg, self->GenericStrings[i]) == 0) {
			index = i;
			break;
		}
	}

	if(index < 0){
		if(!(self->spawnflags & 1))
			Disp(activator, "^3That command is not valid in this interaction.");
		G_UseTargets2(self, activator, self->GenericStrings[7]);
		return;
	}
	switch(index){
		case 0:
			G_UseTargets2(self, activator, self->target);
			break;
		case 1:
			G_UseTargets2(self, activator, self->target2);
			break;
		case 2:
			G_UseTargets2(self, activator, self->target3);
			break;
		case 3:
			G_UseTargets2(self, activator, self->target4);
			break;
		case 4:
			G_UseTargets2(self, activator, self->target5);
			break;
		case 5:
			G_UseTargets2(self, activator, self->target6);
			break;
	}
	G_UseTargets2(self, activator, self->GenericStrings[8]);
}

const entityInfoData_t lmd_interact_spawnflags[] = {
	{"1", "Password mode.  Do not show the player the commands or tell them when the command is invalid."},
	NULL
};
const entityInfoData_t lmd_interact_keys[] = {
	{"#UKEYS", NULL},
	{"Message", "Message to display while interacting."},
	{"Radius", "How far away from this entity the player may wander before the interaction is canceled."},
	{"GlobalTarget", "Targe to fire when any command is used."},
	{"UnknownTarget", "Targe to fire when any command is used."},
	{"Cmd, Cmd2, ..., Cmd6", "Name of each command."},
	{"Target, Target2, ..., Target6", "Target to use for each command."},
	{NULL, NULL},
};

entityInfo_t lmd_interact_info = {
	"List up to 6 commands to the player.  The player can activate a command by using \'/interact <command name>\'.\n"
	"Note that this uses names instead of numbers.",
	lmd_interact_spawnflags,
	lmd_interact_keys
};

void lmd_interact(gentity_t *ent) {
	char *s = NULL;
	int i;
	char msg[MAX_STRING_CHARS] = "";
	const int max_cmds = 5;

	PlayerUsableGetKeys(ent);

	G_SpawnString("unknowntarget", "", &ent->GenericStrings[7]);
	G_SpawnString("globaltarget", "", &ent->GenericStrings[8]);
	G_SpawnString("message", "", &ent->message);

	if(G_SpawnString("cmd", "NULL", &s)){
		ent->GenericStrings[0] = G_NewString2(s);
		ent->count = 1;
	}
	for(i = 1; i <= max_cmds; i++){
		if(G_SpawnString(va("cmd%i", i+1), "NULL", &s)){
			ent->GenericStrings[i] = G_NewString2(s);
			ent->count = i + 1;
		}
		else
			break;
	}

	if(ent->count <= 0){
		EntitySpawnError("lmd_interact must have one or more 'cmd' keys for the commands.");
		G_FreeEntity(ent);
		return;
	}

	if (ent->count > max_cmds) {
		EntitySpawnError(va("lmd_interact cannot have more than %i commands.", max_cmds));
		G_FreeEntity(ent);
		return;
	}

	if(ent->message)
		Q_strcat(msg, sizeof(msg), ent->message);
	if(!(ent->spawnflags & 1)) {
		if(ent->message)
			Q_strcat(msg, sizeof(msg), "\n");
		Q_strcat(msg, sizeof(msg), "^5==============================\n");
		for(i = 0;i<ent->count;i++)
			Q_strcat(msg, sizeof(msg), va("%s\n", ent->GenericStrings[i]));
		Q_strcat(msg, sizeof(msg), "^5==============================\n^3Use the \'^2/interact <cmd>^3\' command to interact.");
	}
	G_Free(ent->message);
	ent->message = G_NewString(msg);

	ent->use = lmd_interact_use;
	ent->interact = lmd_interact_interact;

	ent->r.svFlags |= SVF_NOCLIENT;

	trap_LinkEntity( ent );
}

//Ufo:

void target_modify_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	gentity_t* ent = NULL;
	SpawnData_t* backupSpawn;
	while (ent = G_Find(ent, FOFS(targetname), self->target))
	{
		backupSpawn = cloneSpawnstring(ent->Lmd.spawnData);
		if (*self->target2 && *self->target3)
			Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, self->target2, self->target3);
		if (!spawnEntity(ent, ent->Lmd.spawnData))
			spawnEntity(ent, backupSpawn);
		else
			removeSpawnstring(backupSpawn);
	}
}

const entityInfoData_t target_modify_keys[] = {
	{"Target", "Target entity to edit or reset."},
	{"Key", "SpawnString key."},
	{"Value", "SpawnString value."},
	{NULL, NULL},
};
entityInfo_t target_modify_info = {
	"Modifies all entities on a map with the given targetname. It can change any specified key of the entity.\n"
	"This has the same effect as /spawnstring edit, and results in the entity respawning.\n"
	"If no key or value specified, it will only respawn the entity.",
	NULL,
	target_modify_keys
};

void sp_target_modify(gentity_t* self)
{
	G_SpawnString("key", "", &self->target2);
	G_SpawnString("value", "", &self->target3);

	self->use = target_modify_use;
}

void lmd_cskill_compare_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	if (activator->s.number >= MAX_CLIENTS)
		return;
	Account_t* acc = activator->client->pers.Lmd.account;
	if (!acc)
		return;
	int r1, r2;
	if (Accounts_Custom_GetValue(acc, self->target4))
		r1 = atoi(Accounts_Custom_GetValue(acc, self->target4));
	else
		r1 = 0;
	if (Accounts_Custom_GetValue(acc, self->target5))
		r2 = atoi(Accounts_Custom_GetValue(acc, self->target5));
	else
		r2 = 0;
	if (r1 > r2)
		G_UseTargets2(self, activator, self->target);
	else if (r1 < r2)
		G_UseTargets2(self, activator, self->target2);
	else //if (r1 == r2)
		G_UseTargets2(self, activator, self->target3);
}

const entityInfoData_t lmd_cskill_compare_keys[] = {
	{"Skill1", "Skill to compare."},
	{"Skill2", "Skill to compare."},
	{"Target", "Target to fire if Skill1 is greater."},
	{"Target2", "Target to fire if Skill2 is greater."},
	{"Target3", "Target to fire if both are equal."},
	{NULL, NULL},
};
entityInfo_t lmd_cskill_compare_info = {
	"Compares values of two customskills, and fires respective target.",
	NULL,
	lmd_cskill_compare_keys
};

void lmd_cskill_compare(gentity_t* self)
{
	if (!G_SpawnString("skill1", "", &self->target4) || !G_SpawnString("skill2", "", &self->target5)){
		G_FreeEntity(self);
		return;
	}

	self->use = lmd_cskill_compare_use;
}

void lmd_countcheck_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	int count;
	int result = 0;
	int touched[2] = { 0 };
	int entitylist[MAX_GENTITIES];
	if (!self->r.mins[0] && !self->r.mins[1] && !self->r.mins[2] && !self->r.maxs[0] && !self->r.maxs[1] && !self->r.maxs[2])
	{
		count = -1;
	}
	else
	{
		count = trap_EntitiesInBox(self->r.absmin, self->r.absmax, entitylist, MAX_GENTITIES);
		for (int i = 0; i < count; i++) {
			if (entitylist[i] >= MAX_CLIENTS)
				continue;
			if (entitylist[i] < 16)
				touched[0] |= (1 << entitylist[i]);
			else
				touched[1] |= (1 << (entitylist[i] - 16));
		}
	}
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* ent = GetEnt(i);
		if (!ent || !ent->client || ent->health < 1 || ent->client->pers.connected != CON_CONNECTED || ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			continue;
		if (PlayerUseableCheck(self, ent) && (count == -1 || ((i < 16) ? (touched[0] & (1 << i)) : (touched[1] & (1 << (i - 16))))))
			result++;
	}
	if (result >= self->count)
		G_UseTargets2(self, activator, self->target);
	else
		G_UseTargets2(self, activator, self->target2);
}

const entityInfoData_t lmd_countcheck_keys[] = {
	{"#UKEYS", NULL},
	{"#HITBOX", NULL},
	{"Count", "Required count of players."},
	{"Target", "Target to fire if greater or equal."},
	{"Target2", "Target to fire if less."},
	{NULL, NULL},
};
entityInfo_t lmd_countcheck_info = {
	"Fires target or target2 depending on the current count of players who meet the requirements.",
	NULL,
	lmd_countcheck_keys
};

void lmd_countcheck(gentity_t* self)
{
	if (self->spawnflags & 128)
		self->flags |= FL_INACTIVE;

	PlayerUsableGetKeys(self);

	G_SpawnVector("mins", "0 0 0", self->r.mins);
	G_SpawnVector("maxs", "0 0 0", self->r.maxs);

	self->r.contents = CONTENTS_TRIGGER;
	self->r.svFlags = SVF_NOCLIENT;

	self->use = lmd_countcheck_use;

	trap_LinkEntity(self);
}

void lmd_iterateplayers_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	int count;
	int touched[2] = {0};
	int entitylist[MAX_GENTITIES];
	if (!self->r.mins[0] && !self->r.mins[1] && !self->r.mins[2] && !self->r.maxs[0] && !self->r.maxs[1] && !self->r.maxs[2])
	{
		count = -1;
	}
	else
	{
		count = trap_EntitiesInBox(self->r.absmin, self->r.absmax, entitylist, MAX_GENTITIES);
		for (int i = 0; i < count; i++) {
			if (entitylist[i] >= MAX_CLIENTS)
				continue;
			if (entitylist[i] < 16)
				touched[0] |= (1 << entitylist[i]);
			else
				touched[1] |= (1 << (entitylist[i] - 16));
		}
	}
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* ent = GetEnt(i);
		if (!ent || !ent->client || ent->health < 1 || ent->client->pers.connected != CON_CONNECTED || ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			continue;
		if (PlayerUseableCheck(self, ent) && (count == -1 || ((i < 16) ? (touched[0] & (1 << i)) : (touched[1] & (1 << (i - 16))))))
			G_UseTargets2(self, ent, self->target);
	}
}

const entityInfoData_t lmd_iterateplayers_keys[] = {
	{"#UKEYS", NULL},
	{"#HITBOX", NULL},
	{"Target", "Target to fire for any connected player who meets the requirements."},
	{NULL, NULL},
};
entityInfo_t lmd_iterateplayers_info = {
	"Iterates among all connected players and fires its target for them.",
	NULL,
	lmd_iterateplayers_keys
};

void lmd_iterateplayers(gentity_t* self)
{
	if (self->spawnflags & 128)
		self->flags |= FL_INACTIVE;

	PlayerUsableGetKeys(self);

	G_SpawnVector("mins", "0 0 0", self->r.mins);
	G_SpawnVector("maxs", "0 0 0", self->r.maxs);

	self->r.contents = CONTENTS_TRIGGER;
	self->r.svFlags = SVF_NOCLIENT;

	self->use = lmd_iterateplayers_use;
	
	trap_LinkEntity(self);
}

void target_heal_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	if (!activator->client)
		return;
	if (self->spawnflags & 1)
	{
		activator->health = self->genericValue1;
		activator->client->ps.stats[STAT_ARMOR] = self->genericValue2;
	}
	else
	{
		activator->health += self->genericValue1;
		activator->client->ps.stats[STAT_ARMOR] += self->genericValue2;
	}
	if (activator->client->ps.stats[STAT_ARMOR] > activator->client->ps.stats[STAT_MAX_HEALTH])
		activator->client->ps.stats[STAT_ARMOR] = activator->client->ps.stats[STAT_MAX_HEALTH];
	else if (activator->client->ps.stats[STAT_ARMOR] < 0)
	{
		activator->client->ps.stats[STAT_ARMOR] = 0;
		if (self->spawnflags & 2)
			activator->health += self->genericValue2;
	}
	if (activator->health > activator->client->ps.stats[STAT_MAX_HEALTH])
		activator->health = activator->client->ps.stats[STAT_MAX_HEALTH];
	activator->client->ps.stats[STAT_HEALTH] = activator->health;
	if (activator->health < 1)
	{
		activator->flags &= ~FL_GODMODE;
		player_die(activator, activator, activator, 100000, MOD_TRIGGER_HURT);
	}
	Add_Ammo(activator, AMMO_BLASTER, self->genericValue3);
	Add_Ammo(activator, AMMO_POWERCELL, self->genericValue3);
	Add_Ammo(activator, AMMO_METAL_BOLTS, self->genericValue3);
}

const entityInfoData_t target_heal_keys[] = {
	{"Health", "Health points to acquire."},
	{"Armor", "Shield points to acquire."},
	{"Ammo", "Ammo to acquire. Does not affect every ammo type. Use a value of 16777216 to set maximum a player can carry."},
	{NULL, NULL},
};
entityInfo_t target_heal_info = {
	"Heals or damages the user by a specified value of health or armor.",
	NULL,
	target_heal_keys
};

void sp_target_heal(gentity_t* self)
{
	G_SpawnInt( "health", "0", &self->genericValue1 );
	G_SpawnInt( "armor", "0", &self->genericValue2 );
	G_SpawnInt( "ammo", "0", &self->genericValue3 );
	self->use = target_heal_use;
}

void target_animate_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	if (!activator->client)
		return;
	if (self->genericValue1 == -1 && self->genericValue2 == -1) {
		activator->client->ps.torsoTimer = 0;
		activator->client->ps.legsTimer = 0;
		return;
	}
	if (self->genericValue1 != -1 && activator->client->ps.torsoAnim != self->genericValue1) {
		G_SetAnim(activator, SETANIM_TORSO, self->genericValue1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		activator->client->ps.torsoTimer = self->genericValue3 ? self->genericValue3 : Q3_INFINITE;
	}
	if (self->genericValue2 != -1 && activator->client->ps.legsAnim != self->genericValue2) {
		G_SetAnim(activator, SETANIM_LEGS, self->genericValue2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		activator->client->ps.legsTimer = self->genericValue4 ? self->genericValue4 : Q3_INFINITE;
	}
}

const entityInfoData_t target_animate_keys[] = {
	{"torsoAnim", "Upper animation to be played."},
	{"legsAnim", "Lower animation to be played."},
	{"torsoTime", "Time in milliseconds to play the upper animation for."},
	{"legsTime", "Time in milliseconds to play the lower animation for."},
	{NULL, NULL},
};
entityInfo_t target_animate_info = {
	"Makes the user play specified animation.",
	NULL,
	target_animate_keys
};

extern stringID_table_t animTable[MAX_ANIMATIONS+1];
void sp_target_animate(gentity_t* self)
{
	char *arg;

	G_SpawnString("torsoAnim", "", &arg);
	self->genericValue1 = GetIDForString(animTable, arg);
	G_Free(arg);

	G_SpawnString("legsAnim", "", &arg);
	self->genericValue2 = GetIDForString(animTable, arg);
	G_Free(arg);

	G_SpawnInt( "torsoTime", "0", &self->genericValue3 );
	G_SpawnInt( "legsTime", "0", &self->genericValue4 );

	self->use = target_animate_use;
}