
#include "Lmd_Accounts_Public.h"

#include "ip_t.h"

#define LMDAPI_ACCOUNTS_VERSION_1 (0)

#define LMDAPI_ACCOUNTS_VERSION_CURRENT LMDAPI_ACCOUNTS_VERSION_1

typedef struct LmdApi_Accounts_v1_s {
	/*
	Registers an account module.
	Account modules provide the basis of all account based information, and provide
	support for saving and loading of data.

	Upon successful registration, the function will return a number > -1.
	Pass this number to getAccountModuleData(account, index) to fetch
	the data.

	See the Lmd_Accounts_Stats example for a sample implementation.

	Modules must be registered before GAME_INIT is called, otherwise
	accounts will not load their data, and data corruption will occur.
	*/
	int (*registerAccountModule)(accDataModule_t *module, int moduleVersion);

	// Gets the account data pointer for the specified account and module index.
	void* (*getAccountModuleData)(AccountPtr_t accPtr, int moduleIndex);

	// Gets the number of accounts.
	unsigned int (*getAccountsCount)();

	// Gets an account by its ID
	AccountPtr_t (*findAccountByID)(int id);
	
	// Gets an account by its username
	AccountPtr_t (*findAccountByUserName)(char *username);

	// Gets an account by the associated player name.  Color codes are included in the check.
	AccountPtr_t (*findAccountByPlayerName)(char *playerName);

	// Gets an account by its index in the account array.
	// Valid values are between 0 and getNumberOfAccounts().
	AccountPtr_t (*findAccountByIndex)(unsigned int index);

	// Gets the ID of the account.
	int (*getAccountID)(AccountPtr_t acc);

	// Gets the username of the account
	char* (*getAccountUsername)(AccountPtr_t acc);

	// Sets the password of the account.
	void (*setAccountPassword)(AccountPtr_t acc, char *password);

	// Gets the security code of the account.
	char* (*getAccountSecCode)(AccountPtr_t acc);

	// Sets the security code of the account.
	void (*clearAccountSecCode)(AccountPtr_t acc);

	// Gets the credit count of the account
	int (*getAccountCredits)(AccountPtr_t acc);

	// Sets the credit count of the account.
	void (*setAccountCredits)(AccountPtr_t acc, int credits);

	// Gets the score of the account.
	int (*getAccountScore)(AccountPtr_t acc);

	// Sets the score of the account.
	void (*setAccountScore)(AccountPtr_t acc, int score);

	// Gets the time spent logged in to this account.
	int (*getAccountTime)(AccountPtr_t acc);

	// Gets the flags of the account.
	int (*getAccountFlags)(AccountPtr_t acc);

	// Adds or removes flags of the account.
	// Set the flag to negitive to remove it.
	void (*modifyAccountFlags)(AccountPtr_t acc, int flags);

	// Gets the number of logins made to the account.
	int (*getAccountLogins)(AccountPtr_t acc);

	// Gets the last login made to the account.
	int (*getAccountLastLogin)(AccountPtr_t acc);

	// Gets the last IP to log in to the account.
	void (*getAccountLastIP)(AccountPtr_t acc, IP_t outIP);
} LmdApi_Accounts_v1_t;


// Represents the current memory function list at the time of compile.
#define LmdApi_Accounts_t LmdApi_Accounts_v1_t

#ifdef LUGORMOD
const void *LmdApi_Get_Accounts(unsigned int version);
#else
// Fetch the current function list at the time of compile.
// May return NULL if the version is no longer supported.
#define LmdApi_GetCurrent_Accounts(getAPIFunc) (LmdApi_Memory_t*) getAPIFunc(LMDAPI_ACCOUNTS_VERSION_CURRENT)
#endif

