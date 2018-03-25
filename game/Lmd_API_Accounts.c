
#include <stdlib.h>


#include "Lmd_API_Accounts.h" 

#include "Lmd_Accounts_Core.h" 
#include "Lmd_Accounts_Data.h"

int Lmd_Accounts_AddDataCategory(accDataModule_t *category);
int LmdApi_Accounts_RegisterAccountModule(accDataModule_t *module, int moduleVersion) {
	if (moduleVersion != LMDAPI_ACCOUNTS_DATAMODULE_VERSION_CURRENT) {
		return -1;
	}
	return Lmd_Accounts_AddDataCategory(module);
}

void* Lmd_Accounts_GetAccountCategoryData(Account_t *acc, int categoryIndex);
void *LmdApi_Accounts_GetAccountModuleData(AccountPtr_t accPtr, int moduleIndex) {
	Account_t *acc = (Account_t*)accPtr;
	return Lmd_Accounts_GetAccountCategoryData(acc, moduleIndex);
}

AccountPtr_t LmdApi_Accounts_GetById(int id) 
{ 
	return (AccountPtr_t) Accounts_GetById(id); 
} 

AccountPtr_t LmdApi_Accounts_GetByUsername(char* str) 
{ 
	return (AccountPtr_t) Accounts_GetByUsername(str); 
} 

AccountPtr_t LmdApi_Accounts_GetByName(char* str) 
{ 
	return (AccountPtr_t) Accounts_GetByName(str); 
} 

AccountPtr_t LmdApi_Accounts_Get(unsigned int i) 
{ 
	return (AccountPtr_t) Accounts_Get(i); 
} 

int LmdApi_Accounts_GetId(AccountPtr_t acc) 
{ 
	return Accounts_GetId((Account_t*) acc); 
} 

char* LmdApi_Accounts_GetUsername(AccountPtr_t acc) 
{ 
	return Accounts_GetUsername((Account_t*) acc); 
} 

char* LmdApi_Accounts_GetName(AccountPtr_t acc) 
{ 
	return Accounts_GetName((Account_t*) acc); 
} 

void LmdApi_Accounts_SetName(AccountPtr_t acc, char* name) 
{ 
	Accounts_SetName((Account_t*) acc, name); 
} 

void LmdApi_Accounts_SetPassword(AccountPtr_t acc, char* password) 
{ 
	Accounts_SetPassword((Account_t*) acc, password); 
} 

int LmdApi_Accounts_GetCredits(AccountPtr_t acc) 
{ 
	return Accounts_GetCredits((Account_t*) acc); 
} 

void LmdApi_Accounts_SetCredits(AccountPtr_t acc, int value) 
{ 
	Accounts_SetCredits((Account_t*) acc, value);
} 

int LmdApi_Accounts_GetScore(AccountPtr_t acc) 
{ 
	return Accounts_GetScore((Account_t*) acc); 
} 

void LmdApi_Accounts_SetScore(AccountPtr_t acc, int value) 
{ 
	Accounts_SetScore((Account_t*) acc, value); 
} 

int LmdApi_Accounts_GetTime(AccountPtr_t acc) 
{ 
	return Accounts_GetTime((Account_t*) acc); 
} 

char* LmdApi_Accounts_GetSeccode(AccountPtr_t acc) 
{ 
	return Accounts_GetSeccode((Account_t*) acc); 
}

void LmdApi_Accounts_ClearSeccode(AccountPtr_t acc) {
	Accounts_ClearSeccode((Account_t*)acc);
}

int LmdApi_Accounts_GetFlags(AccountPtr_t acc) 
{ 
	return Accounts_GetFlags((Account_t*) acc); 
} 

void LmdApi_Accounts_AddFlags(AccountPtr_t acc, int value) 
{ 
	Accounts_AddFlags((Account_t*) acc, value); 
} 

int LmdApi_Accounts_GetLogins(AccountPtr_t acc) 
{ 
	return Accounts_GetLogins((Account_t*) acc); 
} 

void LmdApi_Accounts_GetLastIp(AccountPtr_t acc, IP_t ip) 
{ 
	Accounts_GetLastIp((Account_t*) acc, ip); 
} 

int LmdApi_Accounts_GetLastLogin(AccountPtr_t acc) 
{ 
	return Accounts_GetLastLogin((Account_t*) acc); 
} 

LmdApi_Accounts_v1_t lmd_api_accounts_v1 = { 
	// Registers an account module for saving and loading of account data.
	LmdApi_Accounts_RegisterAccountModule,

	// Gets the data for a module.
	LmdApi_Accounts_GetAccountModuleData,

	// Gets the number of accounts. 
	Accounts_Count,

	// Gets an account by its ID 
	LmdApi_Accounts_GetById,

	// Gets an account by its username 
	LmdApi_Accounts_GetByUsername,

	// Gets an account by the associated player name.  Color codes are included in the check. 
	LmdApi_Accounts_GetByName,

	// Gets an account by its index in the account array. 
	// Valid values are between 0 and getNumberOfAccounts(). 
	LmdApi_Accounts_Get,

	// Gets the ID of the account. 
	LmdApi_Accounts_GetId,

	// Gets the username of the account 
	LmdApi_Accounts_GetUsername,

	// Sets the password of the account. 
	LmdApi_Accounts_SetPassword, 

	// Gets the security code of the account. 
	LmdApi_Accounts_GetSeccode,

	// Clears the security code of the account. 
	LmdApi_Accounts_ClearSeccode, 

	// Gets the credit count of the account 
	LmdApi_Accounts_GetCredits,

	// Sets the credit count of the account. 
	LmdApi_Accounts_SetCredits,

	// Gets the score of the account. 
	LmdApi_Accounts_GetScore,

	// Sets the score of the account. 
	LmdApi_Accounts_SetScore, 

	// Gets the time spent logged in to this account. 
	LmdApi_Accounts_GetTime,

	// Gets the flags of the account. 
	LmdApi_Accounts_GetFlags, 

	// Adds or removes flags of the account. 
	// Set the flag to negitive to remove it. 
	LmdApi_Accounts_AddFlags, 

	// Gets the number of logins made to the account. 
	LmdApi_Accounts_GetLogins, 

	// Gets the last login made to the account. 
	LmdApi_Accounts_GetLastLogin,

	// Gets the last IP to log in to the account. 
	LmdApi_Accounts_GetLastIp,
}; 

const void *LmdApi_Get_Accounts(unsigned int version) {
	if (version == LMDAPI_ACCOUNTS_VERSION_1) {
		return &lmd_api_accounts_v1;
	}

	return NULL;
}