


#include "g_local.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"
#include "Lmd_Time.h"
#include "Lmd_Checksum.h"
#include "Lmd_IPs.h"
#include "BG_Fields.h"

#include "Lmd_Professions.h"

#define SECCODE_LENGTH 6

qboolean IsValidName(char *name);

struct Account_s{
	char *username;

	char *name;
	unsigned int pwChksum;
	char *secCode;

	int id;
	int logins;
	unsigned int lastLogin;
	IP_t lastIP;
	
	int time;
	int score;
	int credits;
	int flags;

	struct {
		unsigned int count; // In case data is added after the account was allocated.
		void **data;
	} data;

	int modifiedTime;
};

#define	ACCOUNTOFS(x) ((int)&(((Account_t *)0)->x))

struct {
	unsigned int count;
	accDataModule_t **categories;
} AccountDataTypes;

int Lmd_Accounts_AddDataCategory(accDataModule_t *category) {
	int newIndex = Lmd_Arrays_AddArrayElement((void **)&AccountDataTypes.categories, sizeof(accDataModule_t*), &AccountDataTypes.count);
	AccountDataTypes.categories[newIndex] = category;
	return newIndex;
}

void* Lmd_Accounts_GetAccountCategoryData(Account_t *acc, int categoryIndex) {
	if(!acc)
		return NULL;
	if (categoryIndex < 0 || categoryIndex >= AccountDataTypes.count) {
		G_Error("GetAccountCategoryData: Index out of range");
	}

	return acc->data.data[categoryIndex];
}


qboolean Accounts_Parse_Password(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*)target;
	if(Q_stricmpn(value, "0x", 2) == 0){
		//skip the starting '0x'
		acc->pwChksum = HexToInt((value + 2));
	}
	else {
		acc->pwChksum = Checksum(value);
	}
	return true;
}

DataWriteResult_t Accounts_Write_Password(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Q_strncpyz(value, va("0x%06x", acc->pwChksum), valueSize);
	return DWR_COMPLETE;
}

qboolean Accounts_Parse_LastLogin(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*)target;
	acc->lastLogin = Time_ParseString(value);
	return qtrue;
}

DataWriteResult_t Accounts_Write_LastLogin(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Time_ToString(acc->lastLogin, value, valueSize);
	return DWR_COMPLETE;
}

qboolean Accounts_Parse_LastIP(char *key, char *value, void *target, void *args) {
	// TODO: Parse this in ip module.
	Account_t *acc = (Account_t*)target;
	Lmd_IPs_ParseIP(value, acc->lastIP);
	return qtrue; // Even if we failed to parse the ip, this is still our key.
}

DataWriteResult_t Accounts_Write_LastIP(void *target, char *key, int keySize, char *value, int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Q_strncpyz(value, Lmd_IPs_IPToString(acc->lastIP), valueSize);
	return DWR_COMPLETE;
}

void Accounts_Update_Override(char **key, char **value) {
	if (Q_stricmp(*key, "prof_merc_jetpack") == 0)
		*key = "prof_merc_fuel";
	else if (Q_stricmp(*key, "prof_merc_strength") == 0)
		*key = "prof_merc_forceresist";
}

qboolean Accounts_Parse_Modules(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*) target;

	Accounts_Update_Override(&key, &value);

	int i;
	for (i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *module = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i];
		if (module->numDataFields > 0 &&
			Lmd_Data_Parse_KeyValuePair(key, value, dataPtr, module->dataFields, module->numDataFields))
		{
			return qtrue;
		}
	}

	return qfalse;
}

struct AccountsWriteModulesState {
	int moduleIndex;
	int dataFieldIndex;
	void *dataFieldState;
};

DataWriteResult_t Accounts_Write_Modules(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;

	struct AccountsWriteModulesState *state;
	accDataModule_t *module;

	if (*writeState == NULL) {
		state = (struct AccountsWriteModulesState *)G_Alloc(sizeof(struct AccountsWriteModulesState));
		state->moduleIndex = -1;
		*writeState = state;

		nextModule:
		// Find a good category
		state->dataFieldIndex = 0;
		state->dataFieldState = NULL;
		while (++state->moduleIndex < AccountDataTypes.count) {
			module = AccountDataTypes.categories[state->moduleIndex];

			if (module->numDataFields > 0) {
				// Found a category that wants to save.
				break;
			}
		}

	}
	else {
		state = (struct AccountsWriteModulesState *)*writeState;
	}


	if (state->moduleIndex >= AccountDataTypes.count) {
		// No more modules
		G_Free(state);
		return DWR_NODATA;
	}

	module = AccountDataTypes.categories[state->moduleIndex];

	nextField:
	if (state->dataFieldIndex < module->numDataFields) {
		// Write the field.
		const DataField_t *field = &module->dataFields[state->dataFieldIndex];
		if (field->write) {
			void *dataPtr = acc->data.data[state->moduleIndex];

			Q_strncpyz(key, field->key, keySize);
			DataWriteResult_t result = field->write(dataPtr, key, keySize, value, valueSize, &state->dataFieldState, field->writeArgs);
			if (result == DWR_COMPLETE || result == DWR_NODATA) {
				// We are done with this field
				state->dataFieldIndex++;
			}

			if (result == DWR_NODATA) {
				goto nextField;
			}

			return DWR_CONTINUE;
		}
		else {
			state->dataFieldIndex++;
			goto nextField;
		}
	}

	goto nextModule;
}

#define AccountFields_Base(_m) \
	_m##_AUTO(name, ACCOUNTOFS(name), F_QSTRING) \
	_m##_FUNC(password, Accounts_Parse_Password, Accounts_Write_Password, NULL)	\
	_m##_AUTO(seccode, ACCOUNTOFS(secCode), F_QSTRING) \
	_m##_AUTO(id, ACCOUNTOFS(id), F_INT) \
	_m##_AUTO(logins, ACCOUNTOFS(logins), F_INT) \
	_m##_FUNC(lastlogin, Accounts_Parse_LastLogin, Accounts_Write_LastLogin, NULL) \
	_m##_FUNC(lastip, Accounts_Parse_LastIP, Accounts_Write_LastIP, NULL) \
	_m##_AUTO(time, ACCOUNTOFS(time), F_INT) \
	_m##_AUTO(score, ACCOUNTOFS(score), F_INT) \
	_m##_AUTO(credits, ACCOUNTOFS(credits), F_INT) \
	_m##_AUTO(flags, ACCOUNTOFS(flags), F_INT) \
	_m##_DEFL(Accounts_Parse_Modules, Accounts_Write_Modules, NULL)

AccountFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(AccountFields)
AccountFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int AccountFields_Count = DATAFIELDS_COUNT(AccountFields);

struct {
	unsigned int count;
	Account_t **accounts;
}AccList;

unsigned int nextId = 1;

unsigned int Accounts_Count() {
	return AccList.count;
}

Account_t* Accounts_Get(unsigned int i) {
	assert(i < AccList.count);
	if( i < 0 || i >= AccList.count ) {
		return NULL;
	}
	return AccList.accounts[i];
}

Account_t *Accounts_GetById(int id) {
	int i;
	for(i = 0; i < AccList.count; i++) {
		if(AccList.accounts[i]->id == id)
			return AccList.accounts[i];
	}
	return NULL;
}

Account_t *Accounts_GetByUsername(char *str) {
	int i;
	for(i = 0; i < AccList.count; i++) {
		if(Q_stricmp(AccList.accounts[i]->username, str) == 0)
			return AccList.accounts[i];
	}
	return NULL;
}

Account_t *Accounts_GetByName(char *str) {
	int i;
	for(i = 0; i < AccList.count; i++) {
		if(Q_stricmpname(AccList.accounts[i]->name, str) == 0)
			return AccList.accounts[i];
	}
	return NULL;
}

gentity_t *Accounts_GetPlayerByAcc(Account_t *acc) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!g_entities[i].inuse || !g_entities[i].client)
			continue;
		if(g_entities[i].client->pers.Lmd.account == acc)
			return &g_entities[i];
	}
	return NULL;
}

Account_t *allocAccount(){
	int i;
	Account_t *acc = (Account_t *)G_Alloc(sizeof(Account_t));
	acc->data.count = AccountDataTypes.count;
	acc->data.data = (void **)malloc(sizeof(void *) * AccountDataTypes.count);
	for(i = 0; i < AccountDataTypes.count; i++) {
		accDataModule_t *category = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i] = G_Alloc(category->dataSize);
		memset(dataPtr, 0, category->dataSize);
		if(category->allocData) {
			category->allocData(dataPtr);
		}
	}

	return acc;
}

void freeAccount(Account_t *acc) {
	int i;

	Lmd_Data_FreeFields((void*)acc, AccountFields, AccountFields_Count);

	for(i = 0; i < AccountDataTypes.count; i++) {
		accDataModule_t *module = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i];

		if (dataPtr) {
			Lmd_Data_FreeFields(dataPtr, module->dataFields, module->numDataFields);
			G_Free(dataPtr);
		}
	}

	Lmd_Arrays_RemoveAllElements((void **)&acc->data.data);
	G_Free(acc->username);
	G_Free(acc);
}

void addAccount(Account_t *acc) {
	int i = Lmd_Arrays_AddArrayElement((void **)&AccList.accounts, sizeof(Account_t *), &AccList.count);
	AccList.accounts[i] = acc;
	if(acc->id >= nextId)
		nextId = acc->id + 1;
}

void removeAccount(Account_t *acc) {
	int i, index = -1;
	for(i = 0; i < AccList.count; i++) {
		if(AccList.accounts[i] == acc) {
			index = i;
			break;
		}
	}
	assert(index > -1);
	Lmd_Arrays_RemoveArrayElement((void **)&AccList.accounts, index, sizeof(Account_t *), &AccList.count);
}

void deleteAccount(Account_t *acc) {
	Lmd_Data_DeleteFile("accounts", va("%s.uac", acc->username));
	freeAccount(acc);
}

void Lmd_Accounts_Player_Logout(gentity_t *ent);
void Accounts_Delete(Account_t *acc) {
	gentity_t *player = Accounts_GetPlayerByAcc(acc);
	if(player)
		Lmd_Accounts_Player_Logout(player);
	removeAccount(acc);
	deleteAccount(acc);
}

void Accounts_Save(Account_t *acc)
{
	fileHandle_t f = Lmd_Data_OpenDataFile("accounts", va("%s.uac", acc->username), FS_WRITE);
	Lmd_Data_WriteToFile_LinesDelimited(f, AccountFields, AccountFields_Count, (void *)acc);
	trap_FS_FCloseFile(f);

	acc->modifiedTime = 0;
}

void updatePlayer(gentity_t *ent);
void Accounts_SaveAll(qboolean force){
	int i;
	gentity_t *player;
	for(i = 0; i < AccList.count; i++){
		if(AccList.accounts[i]->modifiedTime > 0 && (force || 
			level.time - AccList.accounts[i]->modifiedTime > 60000)) {
				player = Accounts_GetPlayerByAcc(AccList.accounts[i]);
				if(player)
					updatePlayer(player);
				Accounts_Save(AccList.accounts[i]);
		}
	}
}

extern vmCvar_t lmd_accBaseDays;
extern vmCvar_t lmd_accLevelDays;
extern vmCvar_t lmd_accMaxDays;

//Should this be in the profession area?
int accountLiveTime(int level) {
	int keep = lmd_accBaseDays.integer + (level * lmd_accLevelDays.integer);
	if(keep == 0)
		return 0;
	if(lmd_accMaxDays.integer && keep > lmd_accMaxDays.integer)
		keep = lmd_accMaxDays.integer;
	return keep;
}

qboolean accountLiveTimeCheck(Account_t *acc) {
	int now = Time_Days(Time_Now());
	int keep = accountLiveTime(Accounts_Prof_GetLevel(acc));
	if(now - Time_Days(acc->lastLogin) > keep)
		return qfalse;
	return qtrue;
}

qboolean validateNewAccount(Account_t *acc) {
	if(acc->id <= 0)
		return qfalse;
	if(Accounts_GetById(acc->id) != NULL)
		return qfalse;

	if(!accountLiveTimeCheck(acc) && Auths_AccHasAdmin(acc) == qfalse)
		return qfalse;

	return qtrue;
}

qboolean parseAccount(char *name, char *buf){
	Account_t *acc = allocAccount();
	if(g_developer.integer > 0)
		Com_Printf("Loading account: %s\n", name);
	acc->username = G_NewString2(name); //guarenteed to be unique, since it's a filename.
	char *str = buf;
	Lmd_Data_Parse_LineDelimited(&str, (void *) acc, AccountFields, AccountFields_Count);
	if(validateNewAccount(acc)) {
		addAccount(acc);
	}
	else {
		deleteAccount(acc);
	}

	return qtrue;
}

unsigned int Accounts_Load(){
	unsigned int result = Lmd_Data_ProcessFiles("accounts", ".uac", parseAccount, Q3_INFINITE);
	int i, a;
	for(i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *module = AccountDataTypes.categories[i];
		if(module->accLoadCompleted == NULL)
			continue;
		for(a = 0; a < AccList.count; a++) {
			module->accLoadCompleted(AccList.accounts[a], AccList.accounts[a]->data.data[i]);
		}
	}

	return result;
}

Account_t *Accounts_New(char *username, char *name, char *password) {
	if(Accounts_GetByUsername(username) || Accounts_GetByName(name))
		return NULL;
	Account_t *acc = allocAccount();
	acc->username = G_NewString2(username);
	acc->name = G_NewString2(name);
	acc->modifiedTime = level.time;
	acc->pwChksum = Checksum(password);
	acc->id = nextId;
	int i;
	for(i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *category = AccountDataTypes.categories[i];
		if(category->accLoadCompleted == NULL)
			continue;
		category->accLoadCompleted(acc, acc->data.data[i]);
	}
	addAccount(acc);
	return acc;
}

void Lmd_Accounts_Modify(Account_t *acc) {
	if(acc->modifiedTime == 0)
		acc->modifiedTime = level.time;
}

/*
=======================================================================================
This could be moved into a seperate account module file.
Probably should be, too.
=======================================================================================
*/

int Accounts_GetId(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->id;
}

char* Accounts_GetUsername(Account_t *acc) {
	if(!acc)
		return NULL;
	return acc->username;
}

char* Accounts_GetName(Account_t *acc) {
	if(!acc)
		return NULL;

	// Cant use this, as it will say false if the name is already in use, which it will be.
	if (IsValidName(acc->name) == qfalse) {
		return "Padawan";
	}

	return acc->name;
}

void Accounts_SetName(Account_t *acc, char *name) {
	if(!acc)
		return;

	if (IsValidName(name) == qfalse) {
		name = "Padawan";
	}

	G_Free(acc->name);
	acc->name = G_NewString2(name);
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetPassword(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->pwChksum;
}

void Accounts_SetPassword(Account_t *acc, char *password) {
	if(!acc)
		return;
	acc->pwChksum = Checksum(password);
	Lmd_Accounts_Modify(acc);
}

char* Accounts_GetSeccode(Account_t *acc) {
	if(!acc)
		return "";
	return acc->secCode;
}

void Accounts_ClearSeccode(Account_t *acc) {
	if(!acc)
		return;
	G_Free(acc->secCode);
	acc->secCode = NULL;
	Lmd_Accounts_Modify(acc);
}

char* Accounts_NewSeccode(Account_t *acc) {
	if(!acc) {
		assert(!"New security code for no account.");
		return "";
	}
	char code[SECCODE_LENGTH];
	int i;
	int r;
	for(i = 0; i < SECCODE_LENGTH; i++) {
		r = Q_irand(0, 1);
		if(r == 0)
			code[i] = Q_irand('1', '9'); //skip 0, might look like O
		else if(r == 1) { 
			if(Q_irand(0, 1) == 0) //in jka font, I looks like l
				code[i] = Q_irand('A', 'K');
			else
				code[i] = Q_irand('M', 'Z');
		}
	}
	code[SECCODE_LENGTH - 1] = 0;
	G_Free(acc->secCode);
	acc->secCode = G_NewString2(code);
	Lmd_Accounts_Modify(acc);
	return acc->secCode;
}

int Accounts_GetCredits(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->credits;
}

void Accounts_SetCredits(Account_t *acc, int value) {
	if(!acc)
		return;
	if(value < 0)
		value = 0;
	acc->credits = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetScore(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->score;
}

void Accounts_SetScore(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->score = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetTime(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->time;
}

void Accounts_SetTime(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->time = value;
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetFlags(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->flags;
}

void Accounts_AddFlags(Account_t *acc, int flags) {
	if(!acc)
		return;

	if(flags > 0) {
		acc->flags |= flags;
	}
	else {
		acc->flags &= ~(-flags);
	}

	Lmd_Accounts_Modify(acc);
}

int Accounts_GetLogins(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->logins;
}

void Accounts_SetLogins(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->logins = value;
	Lmd_Accounts_Modify(acc);
}

void Accounts_GetLastIp(Account_t *acc, IP_t value) {
	memcpy(value, acc->lastIP, sizeof(IP_t));
}

void Accounts_SetLastIp(Account_t *acc, IP_t value) {
	if(!acc)
		return;
	memcpy(acc->lastIP, value, sizeof(IP_t));
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetLastLogin(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->lastLogin;
}

void Accounts_SetLastLogin(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->lastLogin = value;
	Lmd_Accounts_Modify(acc);
}
