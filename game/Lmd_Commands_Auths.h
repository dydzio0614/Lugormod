

#ifndef LMD_COMMANDS_AUTHS_H
#define LMD_COMMANDS_AUTHS_H

#include "Lmd_Commands_Auths_Public.h"

qboolean Auths_AccHasAuthFlag(Account_t *ent, int auth);
qboolean Auths_PlayerHasAuthFlag(gentity_t *ent, int auth);

qboolean Auths_PlayerHasAdmin(gentity_t *ent);
qboolean Auths_PlayerHasTempAdmin(gentity_t *ent);
int Auths_GetPlayerRank(gentity_t *ent);
qboolean Auths_Inferior(gentity_t *ent, gentity_t *targ);
qboolean Auths_InferiorToAcc(gentity_t *ent, Account_t *targ);

qboolean Auths_RemoveTempAdmin(gentity_t *ent, char *authfile);

qboolean Auths_AccHasAdmin(Account_t *acc);
int Auths_GetRank(Account_t *acc);

char* Auths_QuickAuthList(Account_t *acc);
char* Auths_QuickPlayerAuthList(gentity_t *ent);

qboolean Auths_PlayerHasTempFile(gentity_t *ent, authFile_t *file);

qboolean Auths_AccHasFile(Account_t *acc, authFile_t *file);

void Auths_AddAccAuthfile(Account_t *acc, authFile_t *file);
qboolean Auths_RemoveAccAuthfile(Account_t *acc, authFile_t *file);

authFile_t *Auths_GetFile(char *name);

void Auths_AddTempAuthfile(gentity_t *ent, authFile_t *file);
qboolean Auths_RemoveTempAuthfile(gentity_t *ent, authFile_t *file);

typedef struct authFileData_s{
	authFile_t *file;
	int points;
}authFileData_t;

qboolean Auths_PlayerHasCommand(gentity_t *ent, cmdEntry_t *cmd);
authCmdEntry_t *Auths_GetPlayerFileCmdEntry(gentity_t *ent, cmdEntry_t *cmd, authFileData_t **foundData);

void Auths_DispAdmins(char* msg);

#endif