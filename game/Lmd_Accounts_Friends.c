

#include "g_local.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"

int AccFriendsDataIndex = -1;
#define FRIENDDATA(acc) (friendData_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccFriendsDataIndex)


typedef struct friendData_s{
	unsigned int count;
	int *friends;
}friendData_t;

int getFriendIndex(friendData_t *data, int id) {
	unsigned int i;
	for(i = 0; i < data->count; i++) {
		if(data->friends[i] == id)
			return i;
	}
	return -1;
}

void removeFriendIndex(friendData_t *data, int index) {
	if(index < 0 || index > data->count)
		return;
	Lmd_Arrays_RemoveArrayElement((void **)&data->friends, index, sizeof(int), &data->count);
}

qboolean addFriend(friendData_t *data, int id) {
	if(getFriendIndex(data, id) >= 0)
		return qfalse;
	unsigned int i = Lmd_Arrays_AddArrayElement((void **)&data->friends, sizeof(int), &data->count);
	data->friends[i] = id;
	return qtrue;
}

qboolean Accounts_Friends_Parse(char *key, char *value, void *target, void *args)
{
	friendData_t *friendData = (friendData_t *)target;
	int id = atoi(value);
	if(id <= 0)
		return qfalse;
	addFriend(friendData, id);
	return qtrue;
}

typedef struct FriendsWriteState_s {
	int index;
} FriendsWriteState_t;

DataWriteResult_t Accounts_Friends_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args)
{
	friendData_t *friendData = (friendData_t *)target;

	void* statePtr = *writeState;
	if (statePtr == NULL) {
		statePtr = *writeState = G_Alloc(sizeof(FriendsWriteState_t));
	}

	FriendsWriteState_t *state = (FriendsWriteState_t *)statePtr;

	if (state->index >= friendData->count) {
		G_Free(state);
		return DWR_NODATA;
	}

	Q_strncpyz(value, va("%i", friendData->friends[state->index]), valueSize);

	state->index++;

	if (state->index >= friendData->count) {
		G_Free(state);
		return DWR_COMPLETE;
	}

	return DWR_CONTINUE;
}

#define FriendsFields_Base(_m) \
	_m##_FUNC(friend, Accounts_Friends_Parse, Accounts_Friends_Write, NULL)

FriendsFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(FriendsFields)
FriendsFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int FriendsFields_Count = DATAFIELDS_COUNT(FriendsFields);

void Accounts_Friends_AccountLoaded(AccountPtr_t accPtr, void *data) {
	Account_t *acc = (Account_t*)accPtr;
	friendData_t *d = (friendData_t *)data;
	int i;
	for(i = 0; i < d->count; i++) {
		if(!Accounts_GetById(d->friends[i])) {
			removeFriendIndex(d, i);
			i--;
		}
	}
}

void Accounts_Friends_Free(void *target){
	friendData_t *friendData = (friendData_t *)target;
	Lmd_Arrays_RemoveAllElements((void **)&friendData->friends);
}

accDataModule_t Accounts_Friends = {
	// dataFields
	FriendsFields,

	// numDataFields
	FriendsFields_Count,

	// dataSize
	sizeof(friendData_t),

	// allocData
	NULL,

	// freeData
	Accounts_Friends_Free,

	// Load completed
	Accounts_Friends_AccountLoaded
};


void Accounts_Friends_Register() {
	AccFriendsDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Friends);
}



unsigned int Accounts_Friends_Count(Account_t *acc) {
	friendData_t *friendData = FRIENDDATA(acc);
	if (!friendData) {
		return 0;
	}
	return friendData->count;
}

qboolean Accounts_Friends_IsFriend(Account_t *acc, int otherId) {
	friendData_t *friendData = FRIENDDATA(acc);
	int i;

	if (!friendData) {
		return qfalse;
	}

	for(i = 0; i < friendData->count; i++) {
		if(friendData->friends[i] == otherId)
			return qtrue;
	}
	return qfalse;
}

void listFriends(gentity_t *ent, Account_t *account) {
	int i, count = Accounts_Count(), f;
	Account_t *acc;
	friendData_t *data;
	char buf[MAX_STRING_CHARS] = "";
	int bufUsed = 0;
	int len;
	int id = Accounts_GetId(account);
	char *str;
	for(i = 0; i < count; i++) {
		acc = Accounts_Get(i);
		data = FRIENDDATA(acc);
		for(f = 0; f < data->count; f++) {
			if(data->friends[f] == id) {
				str = va("^5%s\n", Accounts_GetUsername(acc));
				len = strlen(str);
				if(bufUsed + len >= sizeof(buf)) {
					buf[bufUsed - 1] = 0; //remove linefeed
					Disp(ent, buf);
					buf[0] = 0;
					bufUsed = 0;
				}
				Q_strcat(buf, sizeof(buf), str);
				bufUsed += len;
			}
		}
	}
	if(bufUsed > 0) {
		buf[bufUsed - 1] = 0; //remove linefeed
		Disp(ent, buf);
	}
}

void listDataFriends(gentity_t *ent, friendData_t *data) {
	char buf[MAX_STRING_CHARS] = "";
	int bufUsed = 0;
	int len;
	char *str;
	Account_t *acc;
	int i;
	for(i = 0; i < data->count; i++) {
		acc = Accounts_GetById(data->friends[i]);
		if(!acc) {
			len = getFriendIndex(data, data->friends[i]);
			removeFriendIndex(data, len);
			continue;
		}
		str = va("^5%s\n", Accounts_GetUsername(acc));
		len = strlen(str);
		if(bufUsed + len >= sizeof(buf)) {
			buf[bufUsed - 1] = 0; //remove linefeed
			Disp(ent, buf);
			buf[0] = 0;
			bufUsed = 0;
		}
		Q_strcat(buf, sizeof(buf), str);
		bufUsed += len;
	}
	if(bufUsed) {
		buf[bufUsed - 1] = 0; //remove linefeed
		Disp(ent, buf);
	}
}

void Cmd_Friends_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(!ent->client->pers.Lmd.account) {
		//I need to combine buddy system with this eventually...
		Disp(ent, "^3You must be logged in to use this.");
		return;
	}
	if(argc <= 1){
		Disp(ent, "^3Usage: friends {^2list^3} {^2add ^5<username>^3} {^2remove ^5<index>^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	friendData_t *data = FRIENDDATA(ent->client->pers.Lmd.account);
	if(Q_stricmp("list", arg) == 0){
		Disp(ent, 
			"^3Your friends:\n"
			"^5    ^2Username                         ^3Name\n"
			"^3======================================");
		listDataFriends(ent, data);
		Disp(ent, 
			"^3You are in the friends list of:\n"
			"^2Username                         ^3Name\n"
			"^3======================================");
		listFriends(ent, ent->client->pers.Lmd.account);
		return;
	}
	if(argc < 3){
		Disp(ent, "^3Not enough arguments.");
		return;
	}
	if(Q_stricmp("add", arg) == 0){
		Account_t *acc;
		int accId;
		trap_Argv(2, arg, sizeof(arg));
		acc = Accounts_GetByUsername(arg);
		if(!acc){
			Disp(ent, "^3Unable to find player with that username.");
			return;
		}
		accId = Accounts_GetId(acc);
		if(Accounts_Friends_IsFriend(ent->client->pers.Lmd.account, accId))
			Disp(ent, "^3This player is already in your friends list.");
		else{
			addFriend(data, accId);
			Lmd_Accounts_Modify(ent->client->pers.Lmd.account); //Ufo: was missing
			Disp(ent, va("%s ^3has been added to your friends list.", Accounts_GetUsername(acc)));
		}
	}
	else if(Q_stricmp("remove", arg) == 0){
		Account_t *acc, *selectedAcc;
		int i, selectedIndex = -1;
		char compare[MAX_STRING_CHARS];
		trap_Argv(2, arg, sizeof(arg));
		Q_strlwr(arg);
		for(i = 0; i < data->count; i++) {
			acc = Accounts_GetById(data->friends[i]);
			Q_strncpyz(compare, Accounts_GetUsername(acc), sizeof(compare));
			Q_CleanStr2(compare);
			if(Q_stricmp(compare, arg) == 0) {
				selectedIndex = i;
				selectedAcc = acc;
				break; //exact match, use this one.
			}
			if(strstr(compare, arg)) {
				if(selectedIndex == -1) {
					selectedIndex = i;
					selectedAcc = acc;
				}
				else {
					Disp(ent, "^3More than one entry matches that name.");
				}
			}
		}
		if (selectedIndex < 0) {
			Disp(ent, "^3Invalid index.");
			return;
		}
		removeFriendIndex(data, selectedIndex);
		Lmd_Accounts_Modify(ent->client->pers.Lmd.account); //Ufo: was missing
		if(selectedAcc) {
			Disp(ent, va("%s ^3has been removed from your friends list.", Accounts_GetUsername(selectedAcc)));
		}
	}
	else
		Disp(ent, "^3Unknown argument.  Use the command without any arguments to see a list of valid arguments.");
}


