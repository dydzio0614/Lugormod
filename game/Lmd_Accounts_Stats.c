

#include "g_local.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"

int AccStatDataIndex = -1;
#define STATDATA(acc) (statData_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccStatDataIndex)

typedef struct statData_s{
	int kills;
	int deaths;
	int shots;
	int hits;
	int duels;
	int duelsWon;
	int stashes;
} statData_t;
#define	STATOFS(x) ((int)&(((statData_t *)0)->x))



#define StatFields_Base(_m) \
	_m##_AUTO(kills, STATOFS(kills), F_INT) \
	_m##_AUTO(deaths, STATOFS(deaths), F_INT) \
	_m##_AUTO(shots, STATOFS(shots), F_INT) \
	_m##_AUTO(hits, STATOFS(hits), F_INT) \
	_m##_AUTO(duels, STATOFS(duels), F_INT) \
	_m##_AUTO(duelsWon, STATOFS(duelsWon), F_INT) \
	_m##_AUTO(stashes, STATOFS(stashes), F_INT)

StatFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(StatsFields)
StatFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int StatsFields_Count = DATAFIELDS_COUNT(StatsFields);


accDataModule_t Accounts_Stats = {
	// dataFields
	StatsFields,

	// numDataFields
	StatsFields_Count,

	// dataSize
	sizeof(statData_t),

	// allocData
	NULL,

	// freeData
	NULL,

	// Load completed
	NULL
};

void Accounts_Stats_Register() {
	AccStatDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Stats);
}


int Accounts_Stats_GetDuels(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->duels;
}

void Accounts_Stats_SetDuels(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->duels = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetDuelsWon(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->duelsWon;
}

void Accounts_Stats_SetDuelsWon(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->duelsWon = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetKills(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->kills;
}

void Accounts_Stats_SetKills(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->kills = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetDeaths(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->deaths;
}

void Accounts_Stats_SetDeaths(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->deaths = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetShots(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->shots;
}

void Accounts_Stats_SetShots(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->shots = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetHits(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->hits;
}

void Accounts_Stats_SetHits(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->hits = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_Stats_GetStashes(Account_t *acc) {
	if(!acc)
		return 0;
	statData_t *statData = STATDATA(acc);
	return statData->stashes;
}

void Accounts_Stats_SetStashes(Account_t *acc, int value) {
	if(!acc)
		return;
	statData_t *statData = STATDATA(acc);
	statData->stashes = value;
	Lmd_Accounts_Modify(acc);
}

