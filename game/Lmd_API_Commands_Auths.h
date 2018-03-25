

#include "Lmd_Commands_Auths_Public.h"

#include "Lmd_Accounts_Public.h"

#define LMDAPI_COMMANDS_AUTHS_VERSION_1 (0)

#define LMDAPI_COMMANDS_AUTHS_VERSION_CURRENT LMDAPI_COMMANDS_AUTHS_VERSION_1

typedef struct LmdApi_Commands_Auths_v1_s {
	// Gets the authfile with the given name, or NULL it not found.
	authFile_t* (*getAuthByName)(char *name);


	// Permissions

	// Returns true if the player has access to the given auth flag.
	qboolean (*playerHasAuthFlag)(gentity_t *ent, int flag);

	// Returns true only if the player has temporary admin (ignores account admin).
	qboolean (*playerHasAdminTemp)(gentity_t *ent);

	// Returns true if the player has the authfile in their temporary auths list (ignores account auths).
	qboolean (*playerHasAuthTemp)(gentity_t *player, authFile_t *auth);

	// Returns true if the account has access to the given auth flag.
	qboolean (*accountHasAuthFlag)(AccountPtr_t acc, int flag);

	// Returns true if the account has any authfiles.
	qboolean (*accountHasAdmin)(AccountPtr_t acc);

	// Returns true if the account has a specific authfile.
	qboolean (*accountHasAuth)(AccountPtr_t acc, authFile_t *auth);


	// Ranks

	// Gets the best auth rank of all auths (account and temporary) of the player.
	int (*playerAuthRank)(gentity_t *player);

	// Checks if the player is inferior to another player, using both account and temporary auths.
	qboolean (*playerInferiorTo)(gentity_t *player, gentity_t *target);

	// Check if the player is inferior to an account, using the player's account and temporary auths.
	qboolean (*playerIneferiorToAccount)(gentity_t *player, AccountPtr_t target);

	// Gets the auth rank of the account.
	int (*accountAuthRank)(AccountPtr_t acc);


	// Modification

	// Grants the authfile to the account.
	void (*grantAccountAuth)(AccountPtr_t acc, authFile_t *auth);

	// Removes the authfile from the account.  Returns false if the account did not have the authfile.
	qboolean (*removeAccountAuth)(AccountPtr_t acc, authFile_t *auth);

	// Grant a temporary auth to the player.
	void (*grantPlayerAuthTemp)(gentity_t *player, authFile_t *auth);

	// Remove a temporary auth from the player.  Returns false if the player did not have the temporary auth.
	qboolean (*removePlayerAuthTemp)(gentity_t *player, authFile_t *auth);
} LmdApi_Commands_Auths_v1_t;


// Represents the current memory function list at the time of compile.
#define LmdApi_Commands_Auths_t LmdApi_Commands_Auths_v1_t

#ifdef LUGORMOD
const void *LmdApi_Get_Commands_Auths(unsigned int version);
#else
// Fetch the current function list at the time of compile.
// May return NULL if the version is no longer supported.
#define LmdApi_GetCurrent_Commands_Auths() (LmdApi_Commands_Auths_t*) LmdApi_Get_Commands_Auths(LMDAPI_COMMANDS_AUTHS_VERSION_CURRENT)
#endif

