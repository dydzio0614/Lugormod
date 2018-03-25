
#include "g_local.h"

#include "Lmd_Console.h"

#include "Lmd_Data.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Commands_Core.h"
#include "Lmd_Prof_Core.h"
#include "Lmd_Confirm.h"
#include "Lmd_Time.h"
#include "BG_Fields.h"

int AccProfessionDataDataIndex = -1;
#define PROFDATA(acc) (profData_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccProfessionDataDataIndex)


#define LEVEL_REDUCE 5
#define LEVEL_POINTS 5
#define MIN_LEVEL_POINTS 3

//This is multiplied by (level) X (the next level)
#define LEVEL_COST 25

#define MAX_LEVEL  40

extern vmCvar_t lmd_accLevelDiscount_time;
extern vmCvar_t lmd_accLevelDiscount_maxTime;

//Max reduction time for level cost calculations.
// 60*60*24*5
#define LEVEL_TIME_THRESH (lmd_accLevelDiscount_maxTime.integer)
//Reduce cost at 1 credit every 5 minutes (300 seconds).
// 60*5
#define LEVEL_TIME_REDUCE (lmd_accLevelDiscount_time.integer)

#define MAX_SKILL_LVL 5


extern profession_t noProf;
extern profession_t godProf;
extern profession_t botProf;
extern profession_t mercProf;
extern profession_t jediProf;

profession_t *Professions[] = {
	&noProf, //none
	&godProf, //admin
	&botProf, //bot
	&jediProf, //jedi
	&mercProf, //merc
};

typedef struct profData_s{
	int profession;
	int level;
	int lastLevelUp;
	void *data;
}profData_t;
#define	PROFDATAOFS(x) ((int)&(((profData_t *)0)->x))


void SetSkillParents(profSkill_t *skill) {
	int i;
	for(i = 0; i < skill->subSkills.count; i++) {
		skill->subSkills.skill[i].parent = skill;
		SetSkillParents(&skill->subSkills.skill[i]);
	}
}

void Prof_Init() {
	//Set up parent relationships in the skills
	int i;
	for(i = 0; i < NUM_PROFESSIONS; i++) {
		if(Professions[i]) {
			SetSkillParents(&Professions[i]->primarySkill);
			Professions[i]->primarySkill.parent = NULL;
		}
	}
}

qboolean Accounts_Profs_Parse(char *key, char *value, void *target, void *args) {
	profData_t *profData = (profData_t *)target;
	if(Q_stricmpn(key, "prof_", 5) != 0)
		return qfalse;

	void **alloc = (void **)profData->data;
	int i;
	char *profName = key + 5;
	char *skillName;
	int len;
	for(i = 0; i < NUM_PROFESSIONS; i++)
	{
		if(!Professions[i] || !Professions[i]->key)
			continue;

		len = strlen(Professions[i]->key);

		if(Q_stricmpn(profName, Professions[i]->key, len) != 0 || profName[len] != '_')
			continue;

		if(!Professions[i]->data.fields)
			break;

		skillName = profName + (len + 1);
		Lmd_Data_Parse_KeyValuePair(skillName, value, (void *)alloc[i], Professions[i]->data.fields, Professions[i]->data.count);
		break;
	}

	return qtrue;
}

typedef struct ProfessionWriteState_s {
	int index;
	int dataFieldIndex;
	void *dataFieldState;
} ProfessionWriteState_t;

DataWriteResult_t Accounts_Profs_Write(void *target, char key[], int keySize, char value[], int valueSize,void **writeState, void *args)
{
	profData_t *profData = (profData_t *)target;

	profession_t *prof = Professions[profData->profession];
	int dataFieldsCount = prof->data.count;

	if (dataFieldsCount == 0) {
		// This prof has no data, dont bother allocating anything
		return DWR_NODATA;
	}

	void* statePtr = *writeState;
	if (statePtr == NULL) {
		statePtr = *writeState = G_Alloc(sizeof(ProfessionWriteState_t));
	}

	ProfessionWriteState_t *state = (ProfessionWriteState_t *)statePtr;

	if (state->index >= dataFieldsCount) {
		G_Free(state);
		return DWR_NODATA;
	}

	nextField:
	if (state->index < dataFieldsCount) {
		// Write the field.
		const DataField_t *field = &prof->data.fields[state->index];
		if (field->write) {
			void *dataPtr = profData->data;

			Q_strcat(key, keySize, va("%s_%s", prof->key, field->key));
			DataWriteResult_t result = field->write(dataPtr, key, keySize, value, valueSize, &state->dataFieldState, field->writeArgs);
			if (result == DWR_COMPLETE || result == DWR_NODATA) {
				// We are done with this field
				state->index++;
			}

			if (result == DWR_NODATA) {
				goto nextField;
			}

			return DWR_CONTINUE;
		}
		else {
			state->index++;
			goto nextField;
		}
	}
	else {
		G_Free(state);
		return DWR_NODATA;
	}
}

#define ProfsFields_Base(_m) \
	_m##_PREF(prof_, Accounts_Profs_Parse, Accounts_Profs_Write, NULL) \
	_m##_AUTO(profession, PROFDATAOFS(profession), F_INT) \
	_m##_AUTO(level, PROFDATAOFS(level), F_INT) \
	_m##_AUTO(lastLevelUp, PROFDATAOFS(lastLevelUp), F_INT)

ProfsFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(ProfsFields)
ProfsFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int ProfsFields_Count = DATAFIELDS_COUNT(ProfsFields);


void Lmd_Prof_Alloc(void *target) {
	profData_t *profData = (profData_t *)target;
	//Init the loader by giving us an array of all professions
	void **alloc = (void**)G_Alloc(sizeof(void *) * NUM_PROFESSIONS);
	int i;
	for(i = 0; i < NUM_PROFESSIONS; i++) {
		if (!Professions[i] || Professions[i]->data.size == 0) {
			alloc[i] = NULL;
			continue;
		}

		alloc[i] = (void *) G_Alloc(Professions[i]->data.size);
		memset(alloc[i], 0, Professions[i]->data.size);
	}

	profData->data = alloc;
}

void Lmd_Prof_AccountLoaded(AccountPtr_t accPtr, void *target){
	Account_t *acc = (Account_t*)accPtr;
	//Remove the unused data.
	profData_t *profData = (profData_t *)target;
	void **alloc = (void **)profData->data;
	void *keep = alloc[profData->profession];
	int i;
	for(i = 0; i < NUM_PROFESSIONS; i++) {
		if(i == profData->profession)
			continue;
		if(alloc[i]) {
			Lmd_Data_FreeFields(alloc[i], Professions[i]->data.fields, Professions[i]->data.count);
			G_Free(alloc[i]);
		}
	}

	G_Free(profData->data);
	profData->data = keep;

	//Make sure they have a valid level up time
	int now = Time_Now();
	if(profData->lastLevelUp <= 0 || profData->lastLevelUp > now)
		profData->lastLevelUp = now;
}


void Lmd_Prof_Free(void *target) {
	profData_t *profData = (profData_t *)target;
	if(profData->data)
		G_Free(profData->data);
}

accDataModule_t Accounts_Profession = {
	// dataFields
	ProfsFields,

	// numDataFields
	ProfsFields_Count,

	// dataSize
	sizeof(profData_t),

	// allocData
	Lmd_Prof_Alloc,

	// freeData
	Lmd_Prof_Free,

	// Load completed
	Lmd_Prof_AccountLoaded
};

void Accounts_Prof_Register() {
	AccProfessionDataDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Profession);
}

void Accounts_Prof_ClearData(Account_t *acc) {
	profData_t *data = PROFDATA(acc);
	if(Professions[data->profession]->data.fields) {
		Lmd_Data_FreeFields(data->data, Professions[data->profession]->data.fields, Professions[data->profession]->data.count);
		memset(data->data, 0, Professions[data->profession]->data.size);
		Lmd_Accounts_Modify(acc);
	}
}

void* Accounts_Prof_GetFieldData(Account_t *acc) {
	if(!acc)
		return NULL;
	profData_t *data = PROFDATA(acc);
	return data->data;
}
void Accounts_Prof_SetModified(Account_t *acc) {
	Lmd_Accounts_Modify(acc);
}

int Accounts_Prof_GetProfession(Account_t *acc) {
	if(!acc)
		return 0;
	profData_t *data = PROFDATA(acc);
	return data->profession;
}

//FIXME: should be a profession callback.
int Jedi_GetAccSide(Account_t *acc);
qboolean PlayerAcc_Prof_CanUseProfession(gentity_t *ent) {
	if(!ent->client->pers.Lmd.account)
		return qfalse;
	int prof = Accounts_Prof_GetProfession(ent->client->pers.Lmd.account);
	if(prof == PROF_JEDI) {
		//Check if we are not the opposite, since someone with nothing set should become their profession.
		if(ent->client->ps.trueNonJedi)
			return qfalse;
		//Ufo: added missing g_forceBasedTeams check
		if(g_gametype.integer >= GT_TEAM && g_forceBasedTeams.integer) {
			int side = Jedi_GetAccSide(ent->client->pers.Lmd.account);
			if(side != 0) {
				if(side == FORCE_DARKSIDE && ent->client->sess.sessionTeam != TEAM_RED)
					return qfalse;
				else if(side == FORCE_LIGHTSIDE && ent->client->sess.sessionTeam != TEAM_BLUE)
					return qfalse;
			}
		}
	}
	else if(prof == PROF_MERC) {
		//Check if we are not the opposite, since someone with nothing set should become their profession.
		if(ent->client->ps.trueJedi)
			return qfalse;
	}
	return qtrue;
}

void Accounts_Prof_SetProfession(Account_t *acc, int value) {
	if(!acc)
		return;

	profData_t *data = PROFDATA(acc);
	
	Accounts_Prof_ClearData(acc);

	G_Free(data->data);
	data->profession = value;
	data->data = G_Alloc(Professions[value]->data.size);
	memset(data->data, 0, Professions[value]->data.size);
	Lmd_Accounts_Modify(acc);
}

int Accounts_Prof_GetLevel(Account_t *acc) {
	if(!acc)
		return 0;
	profData_t *data = PROFDATA(acc);
	return data->level;
}

void Accounts_Prof_SetLevel(Account_t *acc, int value) {
	if(!acc)
		return;
	profData_t *data = PROFDATA(acc);
	data->level = value;
	data->lastLevelUp = Time_Now();
	Lmd_Accounts_Modify(acc);
}

int Accounts_Prof_GetLastLevelup(Account_t *acc) {
	if(!acc)
		return 0;
	profData_t *data = PROFDATA(acc);
	return data->lastLevelUp;
}

#if 0
int PlayerAcc_Prof_GetSkill(gentity_t *ent, int prof, int skill) {
	//For gametypes.
	//The team is set before a spawn, so we will never be a spectator and spawning.
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		//duplicated from Accounts_Prof_GetSkill, but we need to confirm it here.
		if(PlayerAcc_Prof_GetProfession(ent) != prof)
			return 0;

		if(!PlayerAcc_Prof_CanUseProfession(ent))
			return 0;
			
	}
	return Accounts_Prof_GetSkill(ent->client->pers.Lmd.account, prof, skill);
}

void Accounts_Prof_SetSkill(Account_t *acc, int prof, int skill, int value) {
	if(!acc)
		return;
	if(Accounts_Prof_GetProfession(acc) != prof)
		return;
	return Professions[prof]->setSkillValue(acc, skill, value);

}
#endif

char* Professions_GetName(int prof){
	return Professions[prof]->name;
}

void Professions_PlayerSpawn(gentity_t *ent)
{
	if(PlayerAcc_Prof_CanUseProfession(ent)) {
		int prof = PlayerAcc_Prof_GetProfession(ent);
		if(Professions[prof]->spawn)
			Professions[prof]->spawn(ent);
	}
}

void SetDefaultSkills(Account_t *acc, int prof, profSkill_t *skill) {
	int i;
	for(i = 0; i < skill->subSkills.count; i++) {
		SetDefaultSkills(acc, prof, &skill->subSkills.skill[i]);
	}
	if (skill->setValue && skill->levels.min > 0) {
		skill->setValue(acc, skill, skill->levels.min);
	}
}

void Professions_SetDefaultSkills(Account_t *acc, int prof){
	SetDefaultSkills(acc, prof, &Professions[prof]->primarySkill);

}

//FIXME: use grouping values.
int Professions_TotalSkillPoints(int prof, int level) {
	int p;
	if (level > LEVEL_REDUCE * (LEVEL_POINTS - MIN_LEVEL_POINTS)) {
		p = ((((LEVEL_POINTS - MIN_LEVEL_POINTS) * (LEVEL_POINTS - MIN_LEVEL_POINTS + 1)) / 2) * LEVEL_REDUCE);
		p += level * MIN_LEVEL_POINTS;
	}
	else {
		int q;
		q = (int)floor((float)(level - 1) / LEVEL_REDUCE);
		q = LEVEL_POINTS - q;
		p = (((LEVEL_POINTS * (LEVEL_POINTS + 1)) / 2) - ((q * (q + 1))/2)) * LEVEL_REDUCE;
		p += q * (1 + ((level - 1) % LEVEL_REDUCE));
	}

	return p;
}

int Professions_SkillCost(profSkill_t *skill, int level) {
	//If we have a min level, dont count it
	if(level <= skill->levels.min)
		return 0;
	level -= skill->levels.min;
	if(skill->points.type == SPT_TRIANGULAR)
		return (level * (level + 1)) / 2;
	return 0;

	////linear
	//return skill->points.cost + (skill->points.multiplier * level);
}

int Professions_UsedSkillPoints(Account_t *acc, int prof, profSkill_t *skill){
	//Count upward from the current location and total up all skill points.

	int i, sum = 0;

	if (!acc) {
		return 0;
	}

	if(skill == NULL)
		skill = &Professions[prof]->primarySkill;

	if (skill->getValue) {
		int level = skill->getValue(acc, skill) - skill->levels.min;
		sum = Professions_SkillCost(skill, level);
	}

	for(i = 0; i < skill->subSkills.count; i++) {
		sum += Professions_UsedSkillPoints(acc, prof, &skill->subSkills.skill[i]);
	}
	return sum;
}

int Professions_AvailableSkillPoints(Account_t *acc, int prof, profSkill_t *skill, profSkill_t **parent){
	int level = 0;

	if (!acc) {
		return 0;
	}

	//Trace backwards from the given skill to find the next point group.
	// TODO: Support reuse of a single skill def on multiple parents?
	while(skill = skill->parent) {
		if(skill->subSkills.pointGroup) {
			//point groups must have an associated skill.
			assert(skill->getValue);
			if (skill->getValue) {
				level = skill->getValue(acc, skill);
			}
			break;
		}
		else if(!skill->parent) {
			level = Accounts_Prof_GetLevel(acc);
			break;
		}
	}

	if(parent)
		*parent = skill;

	return Professions_TotalSkillPoints(prof, level) - Professions_UsedSkillPoints(acc, prof, skill);
}

int recallDroppedCredits(gentity_t *ent);
qboolean Professions_ChooseProf(gentity_t *ent, int prof){
	int playerProfession = PlayerAcc_Prof_GetProfession(ent);
	int myLevel = PlayerAcc_Prof_GetLevel(ent);
	gentity_t *t = NULL;
	int flags = PlayerAcc_GetFlags(ent);

	if (!ent->client->pers.Lmd.account) {
		Disp(ent, "^3You need to be registered to do this. Type '\\help register' for more information.");
		return qfalse;
	}

	if (prof < 0 || prof >= NUM_PROFESSIONS) {
		Disp(ent, "^3That is not a valid profession.");
		return qfalse;
	}

	if (prof == playerProfession) {
		Disp(ent, "^3Your profession was not changed.");
		return qfalse;
	}

	if(flags & ACCFLAGS_NOPROFCRLOSS){
		PlayerAcc_AddFlags(ent, -ACCFLAGS_NOPROFCRLOSS);
		Disp(ent, "^3Your free profession change has been used up.");
	}
	else
		PlayerAcc_SetCredits(ent, PlayerAcc_GetCredits(ent) / 2);

	PlayerAcc_Prof_SetProfession(ent, prof);
	PlayerAcc_Prof_SetLevel(ent, 1);

	recallDroppedCredits(ent);

	PlayerAcc_SetScore(ent, 10);
	Professions_SetDefaultSkills(ent->client->pers.Lmd.account, prof);
	Disp(ent, va("^3Your profession is now: ^2%s", Professions[prof]->name));
	ent->client->ps.fd.forceDoInit = qtrue;
	ent->flags &= ~FL_GODMODE;
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR){
		ent->client->ps.persistant[PERS_SCORE]++;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
	}
	return qtrue;
}

void Profession_UpdateSkillEffects(gentity_t *ent, int prof){
	if(prof == PROF_JEDI)
		WP_InitForcePowers(ent);
}

qboolean Lmd_Prof_SkillIsLeveled(Account_t *acc, int prof, profSkill_t *skill) {
	if (skill->getValue) {
		int value = skill->getValue(acc, skill);
		if (value > skill->levels.min) {
			return qtrue;
		}
	}

	if(skill->subSkills.count > 0) {
		int i;
		for(i = 0; i < skill->subSkills.count; i++) {
			if(Lmd_Prof_SkillIsLeveled(acc, prof, &skill->subSkills.skill[i]))
				return qtrue;
		}
	}
	return qfalse;
}

qboolean Lmd_Prof_SkillIsAchieveable(Account_t *acc, int prof, profSkill_t *skill, profSkill_t **blocker) {
	profSkill_t *parent;
	//track down the tree to find any skills that block our path from being achived.
	while((parent = skill->parent)) {
		int i;
		for(i = 0; i < parent->subSkills.count; i++) {
			if(&parent->subSkills.skill[i] == skill)
				continue;
			if(parent->subSkills.skill[i].unique & skill->unique && Lmd_Prof_SkillIsLeveled(acc, prof, &parent->subSkills.skill[i])) {
				if(blocker)
					*blocker = &parent->subSkills.skill[i];
				return qfalse;
			}
		}
		skill = parent;
	}
	return qtrue;
}

void Cmd_SkillSelect_List(gentity_t *ent, int prof, profSkill_t *parent) {
	int i;
	char *s;
	char c;
	profSkill_t *group = NULL;
	int points;
	int level;
	int cost;
	profSkill_t *skill;
	profSkill_t *blocker = NULL;
	qboolean allBlocked = qfalse;

	Account_t *acc = ent->client->pers.Lmd.account;

	if (!acc) {
		return;
	}


	//Pass a child of the parent, to get the points available inside of it.
	points = Professions_AvailableSkillPoints(acc, prof, &parent->subSkills.skill[0], &group);

	if(Lmd_Prof_SkillIsAchieveable(acc, prof, parent, &blocker) == qfalse) {
		allBlocked = qtrue;
	}

	if(allBlocked)
		s = va("^3(^1Blocked by ^2%s^3)", blocker->name);
	else if (parent->getValue && parent->levels.max > parent->levels.min) {
		// Skill has levels
		level = parent->getValue(acc, parent);
		s = va("^3(^2%i^3 out of ^2%i^3)", level, parent->levels.max);
	}
	else {
		s = "";
	}

	Disp(ent, va("^3Skills for ^%c%s^3 %s", (blocker) ? '1' : '2', parent->name, s));


	for(i = 0; i < parent->subSkills.count; i++) {
		skill = &parent->subSkills.skill[i];
		
		if(allBlocked) {
			c = '1';
			s = "";
		}
		else if(Lmd_Prof_SkillIsAchieveable(acc, prof, skill, &blocker) == qfalse) {
			c = '1';
			s = va("^3(^1Blocked by ^2%s^3)", blocker->name);
		}
		else if (skill->getValue && skill->levels.max > skill->levels.min) { // if(skill->index >= 0) {
			level = skill->getValue(acc, skill);
			cost = Professions_SkillCost(skill, level);
			if(level >= skill->levels.max)
				c = '3';
			else if(points < cost)
				c = '1';
			else
				c = '2';

			s = va("^3(^2%i^3 out of ^2%i^3)", level, skill->levels.max);
		}
		else {
			c = '2';
			s = "";
		}

		Disp(ent, va("^%c%-20s %s", c, skill->name, s));
	}

	if(group) {
		if(group->setValue)
			// TODO: Not garenteed that this is the case.
			s = va(" ^3for use inside the ^2%s^3 skill.\nLeveling up the ^2%s^3 skill will give you more points for these skills", group->name, group->name);
		else
			s = va(" ^3for use inside the ^2%s^3 skill", group->name);
	}
	else
		s = "";

	if(points > 0)
		Disp(ent, va("^3You have ^2%i^3 skill point%s available%s.", points, (points != 1) ? "s" : "", s));
	else
		Disp(ent, va("^3You have no skill points available%s.", s));
}

void Cmd_SkillSelect_Level(gentity_t *ent, int prof, profSkill_t *skill, qboolean down) {
	
	Account_t *acc = ent->client->pers.Lmd.account;

	if (!acc) {
		return;
	}

	if(!skill->getValue || !skill->setValue || skill->levels.max <= skill->levels.min) {
		Disp(ent, "^3This skill cannot be leveled.");
		return;
	}

	profSkill_t *blocker;
	if(Lmd_Prof_SkillIsAchieveable(acc, prof, skill, &blocker) == qfalse) {
		Disp(ent, va("^3You cannot level this skill while ^2%s^3 is leveled.", blocker->name));
		return;
	}

	int level = skill->getValue(acc, skill);
	if(down) {
		if(!skill->levels.canRemove) {
			Disp(ent, "^3This skill cannot be leveled down.");
			return;
		}
		if(level <= skill->levels.min) {
			Disp(ent, "^3This skill is already at its lowest level.");
			return;
		}
		level--;
	}
	else {
		if(level >= skill->levels.max) {
			Disp(ent, "^3This skill is already at its highest level.");
			return;
		}
		level++;

		int points = Professions_AvailableSkillPoints(acc, prof, skill, NULL);
		if(points < level) {
			Disp(ent, va("^3It takes ^2%i^3 points to level up this skill.", level));
			return;
		}

		if (!skill->canSetValue(acc, skill, level)) {
			// TODO: Better message, explain why it can't be leveled.
			Disp(ent, va("^3This skill cannot be leveled up at this time."));
			return;
		}

		if(level >= skill->levels.max) {
			Disp(ent, "^3This skill is now at its highest level.");
		}
		else {
			points -= level;
			int nextLevel = level + 1;
			if(points >= level + 1) {
				int i;
				const char **descr = skill->levelDescriptions;
				int cost = Professions_SkillCost(skill, level + 1) - Professions_SkillCost(skill, level);
				Disp(ent, va("^3You can increase this skill again.  It will cost ^2%i^3 points, leaving you with ^2%i^3 point%s left.",
					cost, points - cost, (points - cost == 1) ? "" : "s"));

				for(i = 0; i < level; i++) {
					if(descr == NULL)
						break;
					descr++;
				}
				if(*descr != NULL)
					Disp(ent, va("^2Next level: ^5%s", *descr));
			}
			else
				Disp(ent, "^3You do not have enough points to increase this skill another time.");
		}
	}

	skill->setValue(acc, skill, level);
	Disp(ent, va("^3The ^2%s^3 skill is now at level ^2%i^3.", skill->name, level));
	Profession_UpdateSkillEffects(ent, prof);
}

void Cmd_SkillSelect(gentity_t *ent, int prof, profSkill_t *skill, int depth) {
	//depth is the current argument depth that matches our skill
	//skills jetpack: 1
	//skills jetpack fuel: 2
	char arg[MAX_STRING_CHARS];
	Account_t *acc = ent->client->pers.Lmd.account;
	if (!acc) {
		return;
	}

	trap_Argv(depth + 1, arg, sizeof(arg));
	if(arg[0]) {
		if(Q_stricmp(arg, "up") == 0) {
			Cmd_SkillSelect_Level(ent, prof, skill, qfalse);
		}
		else if(Q_stricmp(arg, "down") == 0) {
			Cmd_SkillSelect_Level(ent, prof, skill, qtrue);
		}
		else if(skill->subSkills.count > 0) {
			int index = -1, i;
			int arglen = strlen(arg);
			for(i = 0; i < skill->subSkills.count; i++) {
				if(Q_stricmpn(skill->subSkills.skill[i].name, arg, arglen) == 0){
					if(index == -1)
						index = i;
					else {
						index = -2;
						break;
					}
				}
			}
			if(index == -1)
				Disp(ent, va("^3\'^2%s^3\' does not match any skills.", arg));
			else if(index == -2)
				Disp(ent, va("^3Multiple skills match \'^2%s^3\'.", arg));
			else
				Cmd_SkillSelect(ent, prof, &skill->subSkills.skill[index], depth + 1);
			return;
		}
	}
	else {
		profSkill_t *blocker;
		int i;
		int level = 0;
		char *cmd = ConcatArgs(0);

		if (skill->getValue) {
			level = skill->getValue(acc, skill);
		}

		Disp(ent, va("^2%s", skill->name));

		if(skill->subSkills.count > 0) {
			//Display the help text before the subskill list.
			Disp(ent, va("^5%s", skill->baseDescription));
			Cmd_SkillSelect_List(ent, prof, skill);
			Disp(ent, va("^3Use ^2/%s <skill>^3 to manipulate skills within this skill.", cmd));
		}
		else {
			if(Lmd_Prof_SkillIsAchieveable(acc, prof, skill, &blocker) == qfalse) {
				Disp(ent, va("^3You cannot level this skill while ^2%s^3 is leveled.", blocker->name));
			}
			else if(skill->getValue) {
				//Only show if it was not already shown by SkillSelect_List
				Disp(ent, va("^3Level ^2%i^3 out of ^2%i^3", level, skill->levels.max));
			}
			Disp(ent, va("^5%s", skill->baseDescription));
		}

		if(level < skill->levels.max) {
			const char **descr = skill->levelDescriptions;
			for(i = 0; i < level; i++) {
				if(descr == NULL)
					break;
				descr++;
			}
			if(*descr != NULL)
				Disp(ent, va("^2Next level: ^5%s", *descr));
		}

		if(skill->setValue && skill->levels.max > skill->levels.min) {
			int points = Professions_AvailableSkillPoints(acc, prof, skill, NULL);
			int cost = Professions_SkillCost(skill, level + 1) - Professions_SkillCost(skill, level);

			if(level < skill->levels.max) {
				if(points >= level + 1)
					Disp(ent, va("^3Use ^2/%s up^3 to increase the ^2%s^3 skill.  It will cost ^2%i^3 point%s, leaving you with ^2%i^3 point%s left.",
						cmd, skill->name, cost, (cost == 1) ? "" : "s", points - cost, (points - cost == 1) ? "" : "s"));
				else
					Disp(ent, va("^3You do not have enough points to increase the ^2%s^3 skill.", skill->name));
			}
			else
				Disp(ent, va("^3The ^2%s^3 skill is at its maximum level", skill->name));
			if(skill->levels.canRemove) {
				if(level > skill->levels.min)
					Disp(ent, va("^3Use ^2/%s down^3 to decrease the ^2%s^3 skill.  You will regain ^2%i^3 point%s, bringing you up to ^2%i^3 point%s total.",
					cmd, skill->name, cost, (cost == 1) ? "" : "s", points + cost, (points + cost == 1) ? "" : "s"));
				else
					Disp(ent, va("^3The ^2%s^3 skill is at its minimum level.", skill->name));
			}
		}
	}
}

extern vmCvar_t g_maxForceLevel;
void Cmd_SkillSelect_f(gentity_t *ent, int iArg){
	//Ufo:
	if(ent->client->ps.duelInProgress) {
		Disp(ent, "^3You cannot select skills at this time.");
		return;
	}

	int prof = PlayerAcc_Prof_GetProfession(ent);

	if(!Professions[prof]->primarySkill.subSkills.count) {
		Disp(ent, "^3This profession has no skills.");
		return;
	}
	Disp(ent, "^4===========================================");
	Cmd_SkillSelect(ent, prof, &Professions[prof]->primarySkill, 0);
	Disp(ent, "^4===========================================");
}

void Cmd_ResetSkills_f (gentity_t *ent, int iArg){
	Account_t *acc = ent->client->pers.Lmd.account;
	int credits = 0;
	int myCredits = PlayerAcc_GetCredits(ent);
	int prof = PlayerAcc_Prof_GetProfession(ent);
	int used;
	
	if (!acc) {
		return;
	}

	used = Professions_UsedSkillPoints(acc, prof, &Professions[prof]->primarySkill);

	if(Professions[prof]->primarySkill.subSkills.count == 0) {
		Disp(ent, "^3This profession has no skills.");
		return;
	}

	if (trap_Argc() > 1) {
		credits = atoi(ConcatArgs(1));
	}
	if (myCredits < credits) {
		credits = myCredits;
	}

	if(used == 0) {
		Disp(ent, "^3All your skills are already at their lowest level.");
		return;
	}

	int cost = used * 200;

	if (credits < cost) {
		Disp(ent,va("^3The cost to reset your skills is ^2CR %i^3.", cost));
		return;
	}

	PlayerAcc_SetCredits(ent, myCredits - cost);
	Accounts_Prof_ClearData(ent->client->pers.Lmd.account);
	Professions_SetDefaultSkills(ent->client->pers.Lmd.account, prof);
	Profession_UpdateSkillEffects(ent, prof);

	Disp(ent, "^3Your skills have been reset.");
}

//FIXME: should be replaced by Profession_SkillCost
int Professions_LevelCost(int prof, int level, int time){
	int nextlevel = level + 1;
	int cost = (int)(LEVEL_COST * (nextlevel * (nextlevel + 1)));

	// Allow negative?
	if (LEVEL_TIME_REDUCE != 0) {
		if(LEVEL_TIME_THRESH > 0 && time > LEVEL_TIME_THRESH)
			time = LEVEL_TIME_THRESH;
		cost -= (int)floor((float)time / LEVEL_TIME_REDUCE);
	}

	if (cost < LEVEL_COST * nextlevel * 2) {
		cost = LEVEL_COST * nextlevel * 2;
	}
	return cost;
}

typedef struct Cmd_BuyLevel_Confirm_Data_s {
	int cost;
	int level;
}Cmd_BuyLevel_Confirm_Data_t;

void Cmd_BuyLevel_Confirm(gentity_t *ent, void *dataptr) {
	Cmd_BuyLevel_Confirm_Data_t *data = (Cmd_BuyLevel_Confirm_Data_t *)dataptr;
	int newCr = PlayerAcc_GetCredits(ent) - data->cost;
	if(newCr < 0) {
		Disp(ent, va("^3You no longer have enough credits to level up. You need ^2%i^3 more.", -newCr));
		return;
	}

}

void Cmd_BuyLevel_f(gentity_t *ent, int iArg){
	int playerLevel = PlayerAcc_Prof_GetLevel(ent);
	int myCreds = PlayerAcc_GetCredits(ent), cost;
	int flags = PlayerAcc_GetFlags(ent);
	int prof = PlayerAcc_Prof_GetProfession(ent);
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if( prof == PROF_ADMIN ){
		Disp(ent, "^3The god profession has no levels.");
		return;
	}

	if ( playerLevel <= 0 ){
		Disp(ent, "^3You are not logged in.");
		return;
	}

	if (playerLevel >= Professions[prof]->primarySkill.levels.max) {
		Disp(ent, va("^3You have reached the maximum level %i.", Professions[prof]->primarySkill.levels.max));
		return;
	}


	if(flags & ACCFLAGS_NOPROFCRLOSS){
		PlayerAcc_AddFlags(ent, -ACCFLAGS_NOPROFCRLOSS);
	}

	cost = Professions_LevelCost(prof, playerLevel, Time_Now() - Accounts_Prof_GetLastLevelup(ent->client->pers.Lmd.account));


	int resCr = PlayerAcc_GetCredits(ent) - cost;
	if(resCr < 0){
		Disp(ent, va("^3The next level is ^2%i^3 and costs ^2CR %i^3.  You need ^2%i^3 more credit%s.", playerLevel + 1, cost, -resCr, (resCr != -1) ? "s" : ""));
		return;
	}

	if (Q_stricmp("confirm", arg) == 0) {
		PlayerAcc_SetCredits(ent, resCr);
		PlayerAcc_Prof_SetLevel(ent, playerLevel + 1);
		Disp(ent, va("^3You are now at level ^2%i^3.", playerLevel + 1));
		WP_InitForcePowers(ent);
		return;
	}

	Disp(ent, va(
		CT_B"The next level costs "CT_V"%i"CT_B" credits.  Leveling up will leave you with "CT_V"%i"CT_B" credits.\n"
		"Use \'"CT_C"BuyLevel confirm"CT_B"\' to level up .",
		cost,
		resCr));
}

void Profession_DisplayProfs(gentity_t *ent){
	int prof = PlayerAcc_Prof_GetProfession(ent);
	int i;
	Disp(ent, va("^3Your current profession is: ^2%s", Professions[prof]->name));
	Disp(ent, "^3Available professions are:");
	for(i = 0;i < NUM_PROFESSIONS;i++) {
		if((i == PROF_ADMIN && !Auths_PlayerHasAuthFlag(ent, AUTH_GODPROF)) || i == PROF_BOT) {
			continue;
		}
		Disp(ent, va("^3%s", Professions[i]->name));
	}
}

void Cmd_Profession_f (gentity_t *ent, int iArg){
	char arg[MAX_TOKEN_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if (arg[0]) {
		int i;
		int prof = -1;
		int arglen = strlen(arg);
		for(i = 0; i < NUM_PROFESSIONS; i++) {
			if((i == PROF_ADMIN && !Auths_PlayerHasAuthFlag(ent, AUTH_GODPROF)) || i == PROF_BOT) {
				continue;
			}
			if(Q_stricmpn(arg, Professions[i]->name, arglen) == 0) {
				if(prof >= 0) {
					prof = -1;
					break;
				}
				prof = i;
			}
		}
		if(prof >= 0) {
			Professions_ChooseProf(ent, prof);
			return;
		}
		Disp(ent, "^3Unknown profession.");
	}
	Profession_DisplayProfs(ent);
}

void Cmd_Cortosis_f(gentity_t *ent, int iArg);
void Cmd_Flame_f(gentity_t *ent, int iArg);
void Cmd_Ionlysaber_f(gentity_t *ent, int iArg);
void Cmd_MercWeapon_f(gentity_t *ent, int iArg);
void Cmd_Ysalamiri_f (gentity_t *ent, int iArg);

cmdEntry_t professionCommandEntries[] = {
	{"buylevel","Buys a level in your current profession if [cost] is enough to buy the next level.\nOtherwise your current level, and the cost to buy the next level will be displayed.", Cmd_BuyLevel_f, 0, qfalse, 1, 129, 0, 0},
	{"cortosis", "Equips an armor that turns off hostile lightsabers and lowers incoming splash damage. Prevents usability of heavy splash weapons.", Cmd_Cortosis_f, 0, qfalse, 0, 64, ~(1 << GT_FFA), PROF_MERC},
	{"flame", "Shoots out a spew of flames.", Cmd_Flame_f, 0, qfalse, 1, 257, 0, PROF_MERC},
	{"ionlysaber", "You can't use forcepowers other than heal or drain - but you're also immune to them. Greatly reduces received splash damage.", Cmd_Ionlysaber_f, 0, qfalse, 0, 64, ~(1 << GT_FFA), PROF_JEDI},
	{"profession", "Choose a profession. ^1You will start from level one and lose your score and half your money if you choose a new profession.", Cmd_Profession_f, 0, qfalse, 1, 256, 0, 0},
	{"resetskills", "Reset your skills. This costs money; if no argument is provided the cost will be displayed.", Cmd_ResetSkills_f, 0, qfalse, 2, 257, 0, 0},
	{"skills", "View and raise your profession skills. You can only raise skill levels if you have unallocated skill points.\nIf no argument is provided, your current skill levels will be listed.", Cmd_SkillSelect_f, 0, qfalse, 1, 257,0, 0},
	{"weapons", "Select or unselect a weapon.", Cmd_MercWeapon_f, 0, qfalse, 1, 257, 0, PROF_MERC},
#ifndef LMD_EXPERIMENTAL
	{"ysalamiri","Use your Ysalamiri.  You can use the 'challenge to duel' button instead of this command.", Cmd_Ysalamiri_f, 0, qfalse, 0, 257,0, PROF_MERC},
#endif
	{NULL},
};