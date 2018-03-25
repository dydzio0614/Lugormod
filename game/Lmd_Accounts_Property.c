

#include "g_local.h"
#include "Lmd_Time.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Property.h"
#include "Lmd_Arrays.h"
#include "Lmd_Entities_Public.h"
#include "Lmd_EntityCore.h"
#include "Lmd_EntityUtil.h"

int AccPropertyDataIndex = -1;
#define PROPDATA(acc) (propData_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccPropertyDataIndex)


struct propValues_s{
	char *name;
	int color;
} propValues[] = {
	{"None", 1},
	{"Owner", 5},
	{"Caretaker", 4},
	{"Guest", 2},
};

typedef struct propertyKey_s{
	char *key;
	unsigned int rank;
	unsigned int expireDay;
	qboolean rented;
	int adderId;
}propertyKey_t;

typedef struct propData_s{
	unsigned int count;
	propertyKey_t *keys;
}propData_t;

qboolean Accounts_Property_Parse(char *key, char *value, void *target, void *args) {

	propData_t *propData = (propData_t *)target;
	unsigned int rank = 0, adder = 0;
	char expireDayStr[MAX_STRING_CHARS];
	unsigned int expireDay = 0;
	char prop[MAX_STRING_CHARS];
	char buf[MAX_STRING_CHARS];
	qboolean rented = qfalse;

	expireDayStr[0] = 0;
	if(sscanf((const char *)value, "\"%1000[^\"]\" %u %u %s", prop, &rank, &adder, expireDayStr) < 2 || rank >= PROPRANK_MAX){
		return qtrue; //it was ment for us, but invalid.
	}
	else if(expireDayStr[0]){
		char buf2[MAX_STRING_CHARS];
		buf[0] = 0;
		buf2[0] = 0;
		if(sscanf((const char *)expireDayStr, "%s %s", buf, buf2) == 2) {
			if(Q_stricmp(buf2, "rented") == 0)
				rented = qtrue;
		}
		if(buf[0]) {
			if(buf[0] == '-') { //old ver used a negative number for rental.
				rented = qtrue;
				expireDay = Time_ParseString(&buf[1]);
			}
			else
				expireDay = Time_ParseString(buf);
		}
	}

	if(!prop[0])
		return qtrue;

	int i = Lmd_Arrays_AddArrayElement((void **)&propData->keys, sizeof(propertyKey_t), &propData->count);
	propData->keys[i].key = G_NewString2(prop);
	propData->keys[i].rank = rank;
	propData->keys[i].adderId = adder;
	propData->keys[i].rented = rented;
	propData->keys[i].expireDay = expireDay;

	return qtrue;
}

typedef struct PropertyWriteState_s {
	int index;
} PropertyWriteStateState_t;

DataWriteResult_t Accounts_Property_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	propData_t *propData = (propData_t *)target;

	void* statePtr = *writeState;
	if (statePtr == NULL) {
		statePtr = *writeState = G_Alloc(sizeof(PropertyWriteStateState_t));
	}

	PropertyWriteStateState_t *state = (PropertyWriteStateState_t *)statePtr;

	if (state->index >= propData->count) {
		G_Free(state);
		return DWR_NODATA;
	}


	char expireDay[MAX_STRING_CHARS];
	Time_ToString(propData->keys[state->index].expireDay, expireDay, sizeof(expireDay));

	char *modifiers = "";
	if(propData->keys[state->index].rented) {
		modifiers = "rented";
	}

	Q_strncpyz(value, va("\"%s\" %u %u %s %s", propData->keys[state->index].key, propData->keys[state->index].rank, 
		propData->keys[state->index].adderId, expireDay, modifiers), valueSize);
	
	state->index++;

	if (state->index >= propData->count) {
		G_Free(state);
		return DWR_COMPLETE;
	}

	return DWR_CONTINUE;
}

#define PropertyFields_Base(_m) \
	_m##_FUNC(property, Accounts_Property_Parse, Accounts_Property_Write, NULL)

PropertyFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(PropertyFields)
PropertyFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int PropertyFields_Count = DATAFIELDS_COUNT(PropertyFields);

void Accounts_Property_Free(void *data){
	propData_t *propData = (propData_t *)data;
	int i;
	for(i = 0; i < propData->count; i++){
		G_Free(propData->keys[i].key);
	}
	Lmd_Arrays_RemoveAllElements((void **)&propData->keys);
}

accDataModule_t Accounts_Property = {
	// dataFields
	PropertyFields,

	// numDataFields
	PropertyFields_Count,

	// dataSize
	sizeof(propData_t),

	// allocData
	NULL,

	// freeData
	Accounts_Property_Free,
};

void Accounts_Property_Register() {
	AccPropertyDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Property);
}


void Accounts_Property_SetAccess(Account_t *acc, char *key, unsigned int rank, int adder, unsigned int expireDay, qboolean rented);
void checkExpiredProperty(Account_t *acc) {
	if(!acc)
		return;
	propData_t *data = PROPDATA(acc);
	int i;
	int now = Time_Days(Time_Now());
	for(i = 0;i < data->count; i++) {
		if(data->keys[i].expireDay > 0 && data->keys[i].expireDay - now <= 0) {
			Accounts_Property_SetAccess(acc, data->keys[i].key, PROPRANK_NONE, 0, 0, qfalse);
		}
	}
}

unsigned int Accounts_Property_GetAccess(Account_t *acc, char *key, int *adder, unsigned int *expireDay, qboolean *rented){
	if(!acc)
		return PROPRANK_NONE;
	propData_t *propData = PROPDATA(acc);
	propertyKey_t *propKey = NULL;
	if(expireDay)
		(*expireDay) = 0;
	if(adder)
		(*adder) = 0;
	if(rented)
		(*rented) = qfalse;

	int i;
	checkExpiredProperty(acc);
	for(i = 0; i < propData->count; i++) {
		if(Q_stricmp(propData->keys[i].key, key) == 0) {
			propKey = &propData->keys[i];
			break;
		}
	}
	if(propKey == NULL)
		return PROPRANK_NONE;
	if(expireDay)
		(*expireDay) = propKey->expireDay;
	if(adder)
		(*adder) = propKey->adderId;
	if(rented)
		(*rented) = propKey->rented;
	return propKey->rank;
}

void removePropertyByAdder(char *key, int adder){
	unsigned int i, i2;
	propData_t *propData;
	Account_t *acc;
	int count = Accounts_Count();
	for(i = 0; i < count; i++){
		acc = Accounts_Get(i);
		propData = PROPDATA(acc);
		for(i2 = 0; i2 < propData->count; i2++){
			if(Q_stricmp(propData->keys[i2].key, key) == 0 &&
				propData->keys[i2].adderId == adder)
			{
				Accounts_Property_SetAccess(acc, propData->keys[i2].key, PROPRANK_NONE, 0, 1, qfalse);
			}
		}
	}
}

void Accounts_Property_SetAccess(Account_t *acc, char *key, unsigned int rank, int adder, unsigned int expireDay, qboolean rented){
	if(!acc)
		return;
	propData_t *propData = PROPDATA(acc);
	if (rank < 0 || rank >= PROPRANK_MAX) {
		rank = PROPRANK_NONE;
	}

	int i, index = -1;
	for(i = 0; i < propData->count; i++) {
		if(Q_stricmp(propData->keys[i].key, key) == 0) {
			index = i;
			break;
		}
	}
	if(rank <= PROPRANK_NONE || rank >= PROPRANK_MAX){
		if(index > -1){
			Lmd_Arrays_RemoveArrayElement((void **)&propData->keys, index, sizeof(propertyKey_t),
				&propData->count);
		}
		if(expireDay > 0)
		{
			removePropertyByAdder(key, Accounts_GetId(acc));
		}
	}
	else{
		if(index == -1){
			index = Lmd_Arrays_AddArrayElement((void **)&propData->keys, sizeof(propertyKey_t),
				&propData->count);
			propData->keys[i].key = G_NewString2(key);
		}
		propData->keys[i].rank = rank;
		propData->keys[i].adderId = adder;
		propData->keys[i].expireDay = expireDay;
		propData->keys[i].rented = rented;
	}
	Lmd_Accounts_Modify(acc);
}

void Accounts_Property_ViewAccount(gentity_t *ent, Account_t *acc){
	if(!acc)
		return;
	propData_t *data = PROPDATA(acc);
	unsigned int count = ((ent)?75:INT_MAX), i;
	checkExpiredProperty(acc);
	int now = Time_Now();
	char *adderName;
	char *days;
	for(i = 0; i < data->count; i++){
		if(data->keys[i].adderId) {
			Account_t *adder = Accounts_GetById(data->keys[i].adderId);
			if(adder)
				adderName = Accounts_GetUsername(adder);
			else
				adderName = "Unknown";
		}
		else
			adderName = "";
		if(data->keys[i].expireDay) {
			days = va("%i", Time_Days(data->keys[i].expireDay - now));
		}
		else
			days = "";
		Disp(ent, va("^%i%-15s ^2%-20s ^3%-20s ^5%s", propValues[data->keys[i].rank].color,
			propValues[data->keys[i].rank].name, data->keys[i].key, adderName, days));
		count--;
		if(count == 0)
			break;
	}
	if(count == 0)
		Disp(ent, "^3Only the first 75 properties were listed.");
}

unsigned int Accounts_Property_GetAccessCount(char *key, int rentCheck) {
	propData_t *data;
	int i, d, num = Accounts_Count();
	int count = 0;
	for(i = 0; i < num; i++) {
		data = PROPDATA(Accounts_Get(i));
		for(d = 0; d < data->count; d++) {
			if(Q_stricmp(data->keys[d].key, key) != 0)
				continue;
			if(rentCheck != -1 && (rentCheck == 1) != data->keys[d].rented)
				continue;
			count++;
		}
	}
	return count;
}

void Accounts_Property_ViewKey(gentity_t *ent, char *key){
	unsigned int count = ((ent)?75:INT_MAX), i, p;
	Account_t *acc;
	int numAccounts = Accounts_Count();
	char *username;
	char *adderName;
	char *days;
	int now = Time_Now();
	propData_t *data;
	for(i = 0; i < numAccounts; i++){
		acc = Accounts_Get(i);
		username = Accounts_GetUsername(acc);
		data = PROPDATA(acc);
		for(p = 0; p < data->count; p++){
			if(!Q_stricmp(data->keys[p].key, key)){
				if(data->keys[p].adderId) {
					Account_t *adder = Accounts_GetById(data->keys[p].adderId);
					if(adder)
						adderName = Accounts_GetUsername(adder);
					else
						adderName = "Unknown";
				}
				else
					adderName = "";
				if(data->keys[p].expireDay) {
					days = va("%i", Time_Days(data->keys[p].expireDay - now));
				}
				else
					days = "";
				Disp(ent, va("^%i%-15s ^2%-20s ^3%-20s ^5%s", propValues[data->keys[p].rank].color,
					propValues[data->keys[p].rank].name, username, adderName, days));
				count--;
			}
			if(count == 0)
				break;
		}
		if(count == 0)
			break;
	}
	if(count == 0)
		Disp(ent, "^3Only the first 75 players were listed.");
}

qboolean Inventory_Quest_CheckAccess(gentity_t *player, char *prop);
qboolean Factions_CheckPlayerFactionProperty(int id, char *propName);
int Accounts_Property_GetPlayerPropRank(gentity_t *ent, char *prop){
	if(Inventory_Quest_CheckAccess(ent, prop))
		return PROPRANK_GUEST;
	if(Factions_CheckPlayerFactionProperty(PlayerAcc_GetId(ent), prop))
		return PROPRANK_GUEST;
	return PlayerAcc_Property_GetAccess(ent, prop, NULL, NULL, NULL);
}

void removePropertyByAdder(char *key, int adder);
void Cmd_Property_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(argc <= 1){
		Disp(ent, va("^3Usage: %s {^2list ^5%cproperty name%c^3} {^2setrank ^5<property name> <player username or id> <rank> [days]^3}\n"
			"{^2removeadded ^5<property> <player username or id>}\n"
			"^3Rank can be:\n%s^4Caretaker\n^2Guest\n^1None",
			iArg?"propadmin":"property", iArg?'<':'[', iArg?'>':']', iArg?"^5Owner\n":""));
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp("list", arg) == 0){
		if(argc >= 3){
			int rank;
			trap_Argv(2, arg, sizeof(arg));
			rank = PlayerAcc_Property_GetAccess(ent, arg, NULL, NULL, NULL);
			if(!iArg && rank == PROPRANK_NONE)
			{
				Disp(ent, "^3Unknown property.");
				return;
			}
			Disp(ent, va("^3Players with access to the ^2%s^3 property:\n"
				"^5Rank          ^2Username             ^3Adder\n"
				"^3========================================================================", arg));
			Accounts_Property_ViewKey(ent, arg);
			return;
		}
		else if(iArg){
			Disp(ent, "^3You must give a property name to list.");
			return;
		}
		Disp(ent, "^3Properties you have access to:\n"
			"^5Rank          ^2Property             ^3Adder\n"
			"^3========================================================================");
		Accounts_Property_ViewAccount(ent, ent->client->pers.Lmd.account);
	}
	else if(Q_stricmp("setrank", arg) == 0){
		unsigned int myRank, newRank;
		char propKey[MAX_STRING_CHARS];
		Account_t *targ;
		int expireDay;
		int targRank;
		int myId = PlayerAcc_GetId(ent);
		int targAdder;
		int adderRank;
		if(argc < 5){
			Disp(ent, va("^2%s setrank ^5<property name> <player username or id> <rank> [days]\n"
				"^3Set a player's property rank", iArg?"propadmin":"property"));
			return;
		}
		trap_Argv(2, propKey, sizeof(arg));
		myRank = PlayerAcc_Property_GetAccess(ent, propKey, NULL, NULL, NULL);
		if(!iArg && (myRank == PROPRANK_NONE || myRank > PROPRANK_CARETAKER)){
			Disp(ent, "^3You are not allowed to change ranks on this property.");
			return;
		}
		trap_Argv(3, arg, sizeof(arg));
		targ = Accounts_GetByUsername(arg);
		if(!targ){
			Disp(ent, "^3Unable to find player with that username.");
			return;
		}
		//Ufo: FIXME - figure why renter doesn't work rather than omitting it
		targRank = Accounts_Property_GetAccess(targ, propKey, &targAdder, NULL, NULL);
		adderRank = Accounts_Property_GetAccess(Accounts_GetById(targAdder), propKey, NULL, NULL, NULL);
		if(!iArg && targRank != PROPRANK_NONE && targRank <= myRank){
			Disp(ent, "^3You cannot modify players equal or above your rank.");
			return;
		}
		if(!iArg && targRank != PROPRANK_NONE && targAdder != myId && myRank >= adderRank){
			Disp(ent, "^3You cannot modify players added by another property admin.");
			return;
		}
/*
		if(!iArg && ((targExp && targRented) || (adderExp && adderRented))){
			Disp(ent, "^3You cannot modify a player who is renting the terminal or was added by a renter.");
			return;
		}
*/

		trap_Argv(4, arg, sizeof(arg));
		if(Q_stricmp(arg, "none") == 0)
			newRank = PROPRANK_NONE;
		else if(iArg && Q_stricmp(arg, "owner") == 0)
			newRank = PROPRANK_OWNER;
		else if(Q_stricmp(arg, "caretaker") == 0)
			newRank = PROPRANK_CARETAKER;
		else if(Q_stricmp(arg, "guest") == 0)
			newRank = PROPRANK_GUEST;
		else{
			Disp(ent, "^3Invalid property rank.  Use the command without arguments to view the valid property ranks.");
			return;
		}
		if(!iArg && newRank != PROPRANK_NONE && myRank >= newRank){
			Disp(ent, "^3You cannot set a player to a rank equal or higher to your own.");
			return;
		}
		if(targRank == PROPRANK_NONE)
			expireDay = 0;
		else{
			trap_Argv(5, arg, sizeof(arg));
			expireDay = atoi(arg);
			if(expireDay > 0)
				expireDay = Time_Now() + Time_DaysToTime(expireDay);
			else
				expireDay = 0;
		}
		Accounts_Property_SetAccess(targ, propKey, newRank, (iArg)?0:myId, expireDay, qfalse);
		Disp(ent, va("^3Property access modified for player ^7%s^3.", Accounts_GetName(targ)));
	}
	else if(Q_stricmp("removeadded", arg) == 0){
		int myRank;
		int myId = PlayerAcc_GetId(ent);
		int targId;
		char propKey[MAX_STRING_CHARS];
		Account_t *targ;
		int targRank;
		int adderRank;
		int targAdder;
		if(argc < 4){
			Disp(ent, va("^2%s removeadded ^5<property> <player username or id>\n"
				"^3This command removes all players added by the specified player.", iArg?"propadmin":"property"));
			return;
		}

		trap_Argv(2, propKey, sizeof(arg));
		myRank = PlayerAcc_Property_GetAccess(ent, propKey, NULL, NULL, NULL);
		if(!iArg && (myRank == PROPRANK_NONE || myRank > PROPRANK_CARETAKER)){
			Disp(ent, "^3You are not allowed to change ranks on this property.");
			return;
		}
		trap_Argv(3, arg, sizeof(arg));
		targ = Accounts_GetByUsername(arg);
		if(!targ){
			Disp(ent, "^3Unable to find player with that username.");
			return;
		}
		targId = Accounts_GetId(targ);
		//Ufo: FIXME - figure why renter doesn't work rather than omitting it
		targRank = Accounts_Property_GetAccess(targ, propKey, &targAdder, NULL, NULL);
		adderRank = Accounts_Property_GetAccess(Accounts_GetById(targAdder), propKey, NULL, NULL, NULL);
		if(!iArg && targRank != PROPRANK_NONE && myId != targId && targRank <= myRank){
			Disp(ent, "^3You cannot modify players equal or above your rank.");
			return;
		}
		if(!iArg && targRank != PROPRANK_NONE && myId != targId && targAdder != myId && myRank >= adderRank){
			Disp(ent, "^3You cannot modify players added by another property admin.");
			return;
		}
/*
		if(!iArg && ((targExp && targRented) || (adderExp && adderRented))){
			Disp(ent, "^3You cannot modify a player who is renting the terminal or was added by a renter.");
			return;
		}
*/

		removePropertyByAdder(propKey, targId);
		Disp(ent, va("^3All players added by %s have been removed.", Accounts_GetUsername(targ)));
	}
	else
		Disp(ent, "^3Unknown argument.");
}

void lmd_propertyterminal_use(gentity_t *self, gentity_t *other, gentity_t *activator){
	unsigned int rank = PROPRANK_NONE;
	//int adder;
	if(self->painDebounceTime > level.time)
		return;
	self->painDebounceTime = level.time + self->wait;
	rank = Accounts_Property_GetPlayerPropRank(activator, self->GenericStrings[0]);
	if(rank == PROPRANK_NONE){
		trap_SendServerCommand(activator->s.number, va("cp \"%s%s%s\"", (self->message)?self->message:"", (self->message)?"\n":"", self->GenericStrings[2]));
		G_UseTargets2(self, activator, self->target2);
	}
	else{
		trap_SendServerCommand(activator->s.number, va("cp \"%s%s%s\"", (self->message)?self->message:"", (self->message)?"\n":"", self->GenericStrings[1]));
		G_UseTargets(self, activator);
	}
}

unsigned int Accounts_Property_GetAccessCount(char *key, int rentCheck);
qboolean lmd_propertyterminal_pay(gentity_t *self, int credits, gentity_t *activator){
	unsigned int expireDay;
	qboolean renter;
	int now = Time_Now();
	int daysAdd, curDays = 0;
	int rank = PlayerAcc_Property_GetAccess(activator, self->GenericStrings[0], NULL, &expireDay, &renter);
	daysAdd = (int)floor((float)credits / (float)self->count);
	if(expireDay > 0)
		curDays = Time_Days(expireDay - now);
	if(rank == PROPRANK_NONE)
	{
		if(self->genericValue4 > 0 && Accounts_Property_GetAccessCount(self->GenericStrings[0], 1) >= self->genericValue4)
		{
			Disp(activator, "^3There are no more rentals available for this property.");
			return qfalse;
		}
	}
	else if(renter)
	{
		if(!(self->spawnflags & 1))
		{
			Disp(activator, "^3This property does not allow adding more days after the first payment.");
			return qfalse;
		}
	}
	else
	{
		Disp(activator, "^3You already have non-rental access to this property.");
		return qfalse;
	}

	if(curDays + daysAdd > self->genericValue2)
	{
		Disp(activator, va("^3The credit amount goes past the maximum of ^2%i^3 day%s.  Please enter an amount less than or equal to ^2%i^3.",
			self->genericValue2, (self->genericValue2 != 1)?"s":"", self->count * (self->genericValue2 - curDays)));
		return qfalse;
	}

	if(curDays + daysAdd < self->genericValue1)
	{
		Disp(activator, va("^3The credit amount is lower than the minimum of ^2%i^3 day%s.  Please enter an amount greater than or equal to ^2%i^3.",
			self->genericValue1, (self->genericValue1 != 1)?"s":"", self->count * (self->genericValue1 - curDays)));
		return qfalse;
	}

	PlayerAcc_Property_SetAccess(activator, self->GenericStrings[0], self->genericValue3, 0, now + Time_DaysToTime(daysAdd), qtrue);
	Disp(activator, va("^3Added ^2%i^3 day%s.", daysAdd, (daysAdd != 1)?"s":""));
	if(curDays > 0)
		Disp(activator, va("^3You can now use this terminal for ^2%i^3 day%s.", curDays + daysAdd, (curDays + daysAdd != 1)?"s":""));
	PlayerAcc_SetCredits(activator, PlayerAcc_GetCredits(activator) + (credits - (daysAdd * self->count)));

	return qtrue;
}

void lmd_propertyterminal_examine(gentity_t *self, gentity_t *activator)
{
	unsigned int expireDay;
	qboolean renter;
	int adderId;
	Account_t *adder;
	int now = Time_Now();
	int rank = PlayerAcc_Property_GetAccess(activator, self->GenericStrings[0], &adderId, &expireDay, &renter);
	adder = Accounts_GetById(adderId);
	int renters = Accounts_Property_GetAccessCount(self->GenericStrings[0], 1);
	char msg[MAX_STRING_CHARS];

	if(self->message)
		Disp(activator, self->message);

	if(rank == PROPRANK_NONE)
		Q_strncpyz(msg, "^3You do not have access to this property terminal.\n", sizeof(msg));
	else
	{
		char *rankStr;
		switch(rank)
		{
		case PROPRANK_OWNER:
			rankStr = "^4owner";
			break;
		case PROPRANK_CARETAKER:
			rankStr = "^5caretaker";
			break;
		case PROPRANK_GUEST:
			rankStr = "^3guest";
			break;
		default:
			rankStr = "^1unknown";
			break;
		}
		Q_strncpyz(msg, va("^3You have access to this property terminal with a rank of %s^3.\n", rankStr), sizeof(msg));
		if(adderId > 0)
			Q_strcat(msg, sizeof(msg), va("^3You were added by: ^7%s ^3(^2%s^3).\n", Accounts_GetName(adder), Accounts_GetUsername(adder)));
		if(renter)
			Q_strcat(msg, sizeof(msg), "^3You are renting this terminal.\n");
		if(expireDay)
		{
			int diff = Time_Days(expireDay - now);
			Q_strcat(msg, sizeof(msg), va("^3Your access expires in ^2%i^3 day%s.\n", diff, (diff != 1)?"s":""));
		}
	}
	if(self->count > 0)
	{
		Q_strcat(msg, sizeof(msg), "^3This terminal is rentable.\n");
		Q_strcat(msg, sizeof(msg), va("^3Cost is ^2%i^3 credits per day.\n", self->count));

		Q_strcat(msg, sizeof(msg), va("^3You %s ^3add more days after the first payment.\n", (self->spawnflags & 1)?"^2can":"^1cannot"));
		if(self->genericValue1 > 0)
			Q_strcat(msg, sizeof(msg), va("^3Min rental days: ^2%i\n", self->genericValue1));
		if(self->genericValue2 > 0)
			Q_strcat(msg, sizeof(msg), va("^3Max rental days: ^2%i\n", self->genericValue2));
		Q_strcat(msg, sizeof(msg), va("^3There %s currently ^2%i^3 ", (renters > 1)?"are":"is", renters));
		if(self->genericValue4 > 0)
			Q_strcat(msg, sizeof(msg), va("out of ^2%i^3 ", self->genericValue4));
		//Q_strcat(msg, sizeof(msg), va("user%s renting this terminal.\n", ((self->genericValue4 == 0 && renters != 1) || self->genericValue4 != 1)?"s":""));
		Q_strcat(msg, sizeof(msg), "users renting this terminal.\n");
	}

	msg[strlen(msg) - 1] = 0;
	Disp(activator, msg);
}

const entityInfoData_t lmd_propertyterminal_spawnflags[] = {
	{"1", "If \'count\' greater than zero, allow the renter to pay more money after the first payment to increase their time."},
	{NULL, NULL}
};
const entityInfoData_t lmd_propertyterminal_keys[] = {
	{"#UKEYS", NULL},
	{"#MODEL", NULL},
	{"#HITBOX", NULL},
	{"Property", "The name of the property to check access to."},
	{"Target", "Target to fire if the user is a member of the property."},
	{"Target2", "Target to fire if the user is not a member of the property."},
	{"Count", "Number of credits per day this terminal costs to rent.  If not set, then this terminal will not be rentable.  Players can pay money to rent the terminal with \'/pay <credits>\'"},
	{"MinDays", "If \'count\', the minimum number of days this player can rent the property for."},
	{"MaxDays", "If \'count\', the maximum number of days this player can rent the property for."},
	{"MaxRenters", "If \'count\', the maximum number of renters that this terminal will allow.  Defaults to 1."},
	{"Rank", "If \'count\', the property rank to give to new renters.  Values are: 1 - owner, 2 - caretaker, 3 - guest."},
	{"GrantMessage", "Message to display when user has access."},
	{"DenyMessage", "Message to display when user does not have access."},
	{NULL, NULL},
};

entityInfo_t lmd_propertyterminal_info = {
	"Fires target if the player is a member of the specified property, target2 if they are not.  If set, players may rent property access via this terminal.",
	lmd_propertyterminal_spawnflags,
	lmd_propertyterminal_keys
};

void lmd_propertyterminal(gentity_t *ent){
	/*
	Spawnflags:
		1: if "count" > 0, allow payments after the initial payment.
	Keys:
		count: if set, allow players to pay "count" credits for 1 day of usage.
	*/


	//useless, since converted ones will not have the property keys.
	if(ent->Lmd.spawnData && Q_stricmp(ent->classname, "t2_propertyterminal") != 0){
		ent->classname = "lmd_propertyterminal";
		Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "classname", "lmd_propertyterminal");
	}
	
	SpawnEntModel(ent, qtrue, qfalse);

	if(!G_SpawnString("property", "", &ent->GenericStrings[0])){
		EntitySpawnError("lmd_propertyterminal must have a property key.");
		G_FreeEntity(ent);
		return;
	}

	G_SpawnString("grantmessage", "^2Access Granted.", &ent->GenericStrings[1]);
	G_SpawnString("denymessage", "^1Access Denied.", &ent->GenericStrings[2]);


	if(ent->wait <= 0)
		ent->wait = 0.7;
	ent->wait = floor(ent->wait * 1000.0f);

	if(ent->count > 0)
	{
		//ent->count as price per day
		G_SpawnInt("mindays", "0", &ent->genericValue1);
		G_SpawnInt("maxdays", "0", &ent->genericValue2);
		G_SpawnInt("maxrenters", "1", &ent->genericValue4);
		G_SpawnInt("rank", "0", &ent->genericValue3);
		if(ent->genericValue3 <= PROPRANK_NONE || ent->genericValue3 > PROPRANK_GUEST)
			ent->genericValue3 = PROPRANK_GUEST;
		ent->pay = lmd_propertyterminal_pay;
	}

	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->use = lmd_propertyterminal_use;
	ent->examine = lmd_propertyterminal_examine;

	trap_LinkEntity(ent);
}

const entityInfoData_t lmd_property_keys[] = {
	{"Property", "The property to set access to."},
	{"Rank", "If \'count\', the property rank to give to new renters.  Values are: 1 - owner, 2 - caretaker, 3 - guest, 0 - remove access."},
};

entityInfo_t lmd_property_info = {
	"Sets or removes a property access rank on the activator.",
	NULL,
	lmd_property_keys
};

void lmd_property_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	PlayerAcc_Property_SetAccess(activator, self->GenericStrings[0], self->genericValue3, NULL, NULL, qfalse);
}

void lmd_property(gentity_t *ent) {
	if (!G_SpawnString("property", "", &ent->GenericStrings[0])) {
		EntitySpawnError("lmd_property must have a property key.");
		G_FreeEntity(ent);
		return;
	}

	if (!G_SpawnInt("rank", "0", &ent->genericValue3)) {
		EntitySpawnError("lmd_property must have a rank key");
		G_FreeEntity(ent);
		return;
	}

	if (ent->genericValue3 < 0 || ent->genericValue3 >= PROPRANK_MAX) {
		EntitySpawnError("Invalid rank");
		G_FreeEntity(ent);
		return;
	}

	ent->use = lmd_property_use;
}
