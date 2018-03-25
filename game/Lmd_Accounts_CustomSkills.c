

#include "g_local.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_KeyPairs.h"

int AccCustomDataIndex = -1;
#define CUSTDATA(acc) (KeyPairSet_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccCustomDataIndex)


qboolean Accounts_CustomSkill_Parse(char *key, char *value, void *target, void *args)
{
	KeyPairSet_t *set = (KeyPairSet_t *)target;
	if(Q_strncmp("skill_custom_", key, 13) != 0)
		return qfalse;
	key += 13;
	Lmd_Pairs_New(set, key, value);
	return qtrue;
}

typedef struct CustomSkillWriteState_s {
	int index;
} CustomSkillWriteState_t;

DataWriteResult_t Accounts_CustomSkill_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args)
{
	KeyPairSet_t *set = (KeyPairSet_t *)target;
	
	void* statePtr = *writeState;
	if (statePtr == NULL) {
		statePtr = *writeState = G_Alloc(sizeof(CustomSkillWriteState_t));
	}

	CustomSkillWriteState_t *state = (CustomSkillWriteState_t *)statePtr;

	if (state->index >= set->count) {
		G_Free(state);
		return DWR_NODATA;
	}

	Q_strcat(key, keySize, set->pairs[state->index].key);
	Q_strncpyz(value, set->pairs[state->index].value, valueSize);

	state->index++;

	if (state->index >= set->count) {
		G_Free(state);
		return DWR_COMPLETE;
	}

	return DWR_CONTINUE;
}

#define CustomSkillFields_Base(_m) \
	_m##_PREF(skill_custom_, Accounts_CustomSkill_Parse, Accounts_CustomSkill_Write, NULL)

CustomSkillFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(CustomSkillFields)
CustomSkillFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int CustomSkillFields_Count = DATAFIELDS_COUNT(CustomSkillFields);

void freeCust(void *data){
	KeyPairSet_t *set = (KeyPairSet_t *)data;
	Lmd_Pairs_Clear(set);
}

accDataModule_t Accounts_Custom = {
	// dataFields
	CustomSkillFields,

	// numDataFields
	CustomSkillFields_Count,

	// dataSize
	sizeof(KeyPairSet_t),

	// allocData
	NULL,

	// freeData
	freeCust,
};


void Accounts_CustomSkills_Register() {
	AccCustomDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Custom);
}


char* Accounts_Custom_GetValue(Account_t *acc, char *key) {
	KeyPairSet_t *set = CUSTDATA(acc);
	if(!set)
		return NULL;
	int i = Lmd_Pairs_FindKey(set, key);
	if(i < 0)
		return NULL;
	return set->pairs[i].value;
}

void Accounts_Custom_SetValue(Account_t *acc, char *key, char *value) {
	if(!acc)
		return;
	KeyPairSet_t *set = CUSTDATA(acc);
	int i = Lmd_Pairs_FindKey(set, key);
	if(i < 0) {
		Lmd_Pairs_New(set, key, value);
	}
	else {
		G_Free(set->pairs[i].value);
		set->pairs[i].value = G_NewString2(value);
	}
	Lmd_Accounts_Modify(acc);
}

void Accounts_Custom_Clear(Account_t *acc) {
	if(!acc)
		return;
	KeyPairSet_t *set = CUSTDATA(acc);
	Lmd_Pairs_Clear(set);
}

void Accounts_Custom_ClearAll() {
	int i, max = Accounts_Count();
	for(i = 0; i < max; i++) {
		Accounts_Custom_Clear(Accounts_Get(i));
	}
}