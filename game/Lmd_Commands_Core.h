
#ifndef LMD_COMMANDS_CORE_H
#define LMD_COMMANDS_CORE_H

#include "Lmd_Commands_Public.h"

cmdEntry_t *Commands_GetEntry(const char *name);
cmdCategory_t *Commands_GetCategory(const char *name);
qboolean Commands_PlayerCanUseCommand(gentity_t *ent, cmdEntry_t *cmd);
extern cmdCategory_t Categories[];


#endif