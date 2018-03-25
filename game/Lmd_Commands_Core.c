

#include "g_local.h"

#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Data.h"
#include "Lmd_Professions.h"
#include "Lmd_Arrays.h"
#include "Lmd_Commands_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Console.h"

extern vmCvar_t lmd_adminloglevels;
extern vmCvar_t lmd_admindefaultlogfile;

extern cmdEntry_t playerCommandEntries[];
extern cmdEntry_t accountCommandEntries[];
extern cmdEntry_t professionCommandEntries[];
extern cmdEntry_t adminCommandEntries[];
extern cmdEntry_t IPCommandEntries[];
extern cmdEntry_t banCommandEntries[];
extern cmdEntry_t entityCommandEntries[];
extern cmdEntry_t groupCommandEntries[];
extern cmdEntry_t cheatCommandEntries[];
extern cmdEntry_t npcCommandEntries[];
#ifdef LMD_EXPERIMENTAL
#ifdef LMD_NEW_FORCEPOWERS
extern cmdEntry_t forceDebugCommands[];
#endif
#ifdef LMD_NEW_WEAPONS
extern cmdEntry_t weaponDebugCommands[];
#endif
#endif

extern cmdEntry_t authCommandEntries[];

cmdCategory_t Categories[] = {
	{"Player", "General player commands", 2, playerCommandEntries},
	{"Account", "General account commands", 2, accountCommandEntries},
	{"Profession", "Profession commands", 1, professionCommandEntries},
	{"Admin", "Administrative commands", 6, adminCommandEntries},
	{"IPs", "View and search connect IPs.  Useful for banning players.", 5, IPCommandEntries},
	{"Bans", "Prevent players from connecting to the server", 6, banCommandEntries},
	{"Auth", "Admin authentication commands", 5, authCommandEntries},
	{"Entities", "Entity creation and manipulation commands", 3, entityCommandEntries},
	{"Entity Grouping", "Commands for importing, exporting, and manipulating groups of entities.", 3, groupCommandEntries},
	{"Cheats", "Cheat commands for various attributes.", 6, cheatCommandEntries},
	{"Npc Commands", "Control npc behavior", 4, npcCommandEntries},
#ifdef LMD_EXPERIMENTAL
#ifdef LMD_NEW_FORCEPOWERS
	{"Force debug", "Debug forcepower settings.", 1, forceDebugCommands},
#endif
#ifdef LMD_NEW_WEAPONS
	{"Weapon debug", "Debug weapon settings.", 1, weaponDebugCommands},
#endif
#endif

	{NULL}
};

void Commands_Init(){
	cmdCategory_t *c = Categories;
	cmdEntry_t *e;
	while(c->name) {
		e = c->entries;
		while(e->name) {
			e->category = c;
			e++;
		}
		c++;
	}
}

cmdCategory_t *Commands_GetCategory(const char *name){
	cmdCategory_t *c = Categories;
	while(c->name){
		if(Q_stricmp(c->name, name) == 0)
			return c;
		c++;
	}
	return NULL;
}

cmdEntry_t *Commands_GetEntry(const char *name){
	cmdCategory_t *c = Categories;
	cmdEntry_t *e;
	while(c->name){
		e = c->entries;
		while(e->name){
			if(Q_stricmp(e->name, name) == 0)
				return e;
			e++;
		}
		c++;
	}
	return NULL;
}

extern vmCvar_t g_cmdDisable;
extern vmCvar_t g_gametype;
qboolean Commands_PlayerCanUseCommand(gentity_t *ent, cmdEntry_t *cmd){
	if(cmd->isAdmin) {
		if(cmd->level == -2) {
			if(!Auths_PlayerHasAdmin(ent))
				return qfalse;
		}
	}
	else {
		if(cmd->profession > 0 && PlayerAcc_Prof_GetProfession(ent) != cmd->profession)
			return qfalse;
		if(cmd->level && PlayerAcc_Prof_GetLevel(ent) < cmd->level)
			return qfalse;
	}
	if(cmd->disable & g_cmdDisable.integer)
		return qfalse;
	if(cmd->gametype & (1 << g_gametype.integer))
		return qfalse;
	//level of -1 will still get here, so will work for admin setting.
	return qtrue;
}

qboolean Commands_PlayerHasCatgory(gentity_t *ent, cmdCategory_t *category){
	cmdEntry_t *entry = category->entries;
	while(entry->name){
		if(Commands_PlayerCanUseCommand(ent, entry) && Auths_PlayerHasCommand(ent, entry))
			return qtrue;
		entry++;
	}
	return qfalse;
}

qboolean Auths_CanUseCommand(gentity_t *ent, cmdEntry_t *cmd);
void Auths_CommandUsed(gentity_t *ent, cmdEntry_t *cmd);
qboolean Commands_TryUse(gentity_t *ent, cmdEntry_t *cmd){
	if(!Auths_PlayerHasCommand(ent, cmd) || !Commands_PlayerCanUseCommand(ent, cmd))
		return qfalse;
	if(Auths_CanUseCommand(ent, cmd)) {
		cmd->func(ent, cmd->iArg);
		Auths_CommandUsed(ent, cmd);
	}
	return qtrue;
}

qboolean Lmd_Command(gentity_t *ent, const char *cmd) {
	/*
	cmdEntry_t *e = Commands_GetEntry(cmd);
	if(!e)
		return qfalse;
	*/

	cmdCategory_t *c = Categories;
	cmdEntry_t *e, *found = NULL;
	int cmdLen = strlen(cmd);
	int count = 0;
	while(c->name){
		e = c->entries;
		while(e->name){
			if(!Auths_PlayerHasCommand(ent, e) || !Commands_PlayerCanUseCommand(ent, e)) {
				e++;
				continue;
			}
				
			if(Q_stricmpn(e->name, cmd, cmdLen) == 0) {
				if (strlen(e->name) == cmdLen) {
					// Exact match.
					count = 1;
					found = e;
					goto execCommand;
				}
				count++;
				if (!found) {
					found = e;
				}
				else {
					DispContiguous(ent, va(CT_B"%s", found->name));
					found = e;
				}
			}
			e++;
		}
		c++;
	}

	execCommand:

	if (count > 1) {
		DispContiguous(ent, va(CT_B"%s", found->name));
		DispContiguous(ent, va(CT_V"%i"CT_B" commands match that command.", count));
		DispContiguous(ent, NULL);
		return qfalse;
	}

	if (!found) {
		return qfalse;
	}

	if (Q_stricmp(found->name, cmd) != 0) {
		Disp(ent, va("%s %s", found->name, ConcatArgs(1)));
	}

	return Commands_TryUse(ent, found);
}

void Cmd_Help_ShowCmd(gentity_t *ent, authCmdEntry_t *entry){
	Disp(ent, va("^2%s", entry->cmd->name));
	if(entry->restr > 0){
		Disp(ent, va("^3Requires another online admin of rank ^2%i", entry->restr));
	}
	if(entry->cost > 0){
		Disp(ent, va("^3Command point cost: ^2%i", entry->restr));
	}
	Disp(ent, va("^3%s", entry->cmd->descr));
}

int Cmd_Help_ListCmds(gentity_t *ent, cmdCategory_t *category){
	int c = 0;
	cmdEntry_t *entry = category->entries;
	while(entry->name){
		if(Auths_GetPlayerFileCmdEntry(ent, entry, NULL) && Commands_PlayerCanUseCommand(ent, entry)){
			DispContiguous(ent, va("^2%-25s ^3%s", entry->name, entry->descr));
			c++;
		}
		entry++;
	}
	DispContiguous(ent, NULL);
	return c;
}

void Cmd_Help_ListCategories(gentity_t *ent){
	cmdCategory_t *category = Categories;
	while(category->name){
		if(Commands_PlayerHasCatgory(ent, category)){
			DispContiguous(ent, va("^%i%-25s ^3%s", category->color, category->name, category->descr));
		}
		category++;
	}
	DispContiguous(ent, NULL);
}

extern vmCvar_t Lugormod_Version;
void Cmd_Help_f(gentity_t *ent){
	char *arg = ConcatArgs(1);
	cmdCategory_t *category = NULL;
	authCmdEntry_t *entry = NULL;
	if(arg[0]){
		category = Commands_GetCategory(arg);
		if(!category){
			cmdEntry_t *e = Commands_GetEntry(arg);
			if(e)
				entry = Auths_GetPlayerFileCmdEntry(ent, e, NULL);
			if(!entry || !Commands_PlayerCanUseCommand(ent, e)){
				Disp(ent, "^3There is no category or command by that name.");
				return;
			}
		}
	}
	if(entry){
		Cmd_Help_ShowCmd(ent, entry);
	}
	else if(category){
		if(Cmd_Help_ListCmds(ent, category) <= 0){
			Disp(ent, "^3You do not have any commands in this category");
			return;
		}
	}
	else {
		Disp(ent, va("^2Lugormod %s -- ^3Lugormod.RoboPhredDev.Net\n", Lugormod_Version.string));
		Cmd_Help_ListCategories(ent);
	}
}

