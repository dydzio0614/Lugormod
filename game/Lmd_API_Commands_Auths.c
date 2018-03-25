

#include "q_shared.h"

#include "Lmd_API_Commands_Auths.h"
#include "Lmd_Commands_Auths.h"

#include "g_local.h"

qboolean LmdApi_Commands_Auths_AccountHasAdmin(AccountPtr_t acc) {
	return Auths_AccHasAdmin((Account_t *)acc);
}

qboolean LmdApi_Commands_Auths_AccountHasAuthFlag(AccountPtr_t acc, int flag) {
	return Auths_AccHasAuthFlag((Account_t*)acc, flag);
}

qboolean LmdApi_Commands_Auths_AccountHasAuth(AccountPtr_t acc, authFile_t *auth) {
	return Auths_AccHasFile((Account_t *)acc, auth);
}

qboolean LmdApi_Commands_Auths_PlayerInferiorToAccount(gentity_t *ent, AccountPtr_t acc) {
	return Auths_InferiorToAcc(ent, (Account_t*)acc);
}

int LmdApi_Commands_Auths_AccountRank(AccountPtr_t acc) {
	return Auths_GetRank((Account_t *)acc);
}

void LmdApi_Commands_Auths_GrantAccountAuth(AccountPtr_t acc, authFile_t *file) {
	Auths_AddAccAuthfile((Account_t *)acc, file);
}

qboolean LmdApi_Commands_Auths_RemoveAccountAuth(AccountPtr_t acc, authFile_t *file) {
	return Auths_RemoveAccAuthfile((Account_t*)acc, file);
}

LmdApi_Commands_Auths_v1_t api_commands_auths_v1 = {
	// Gets the authfile with the given name, or NULL it not found.
	Auths_GetFile,


	// Permissions

	// Returns true if the player has access to the given auth flag.
	Auths_PlayerHasAuthFlag,

	// Returns true only if the player has temporary admin (ignores account admin).
	Auths_PlayerHasTempAdmin,


	// Returns true if the player has the authfile in their temporary auths list (ignores account auths).
	Auths_PlayerHasTempFile,

	// Returns true if the account has access to the given auth flag.
	LmdApi_Commands_Auths_AccountHasAuthFlag,

	// Returns true if the account has any authfiles.
	LmdApi_Commands_Auths_AccountHasAdmin,

	// Returns true if the account has a specific authfile.
	LmdApi_Commands_Auths_AccountHasAuth,


	// Ranks

	// Gets the best auth rank of all auths (account and temporary) of the player.
	Auths_GetPlayerRank,

	// Checks if the player is inferior to another player, using both account and temporary auths.
	Auths_Inferior,

	// Check if the player is inferior to an account, using the player's account and temporary auths.
	LmdApi_Commands_Auths_PlayerInferiorToAccount,

	// Gets the auth rank of the account.
	LmdApi_Commands_Auths_AccountRank,


	// Modification

	// Grants the authfile to the account.
	LmdApi_Commands_Auths_GrantAccountAuth,

	// Removes the authfile from the account.  Returns false if the account did not have the authfile.
	LmdApi_Commands_Auths_RemoveAccountAuth,

	// Grant a temporary auth to the player.
	Auths_AddTempAuthfile,

	// Remove a temporary auth from the player.  Returns false if the player did not have the temporary auth.
	Auths_RemoveTempAuthfile,
};

const void *LmdApi_Get_Commands_Auths(unsigned int version) {
	if (version == LMDAPI_COMMANDS_AUTHS_VERSION_1) {
		return &api_commands_auths_v1;
	}

	return NULL;
}