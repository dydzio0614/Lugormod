
#include "g_local.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"

gentity_t* AimAnyTarget (gentity_t *ent, int length);

void Cmd_NPC_f(gentity_t *ent);
void lmdCmd_NPC_f(gentity_t *ent, int iArg){
	Cmd_NPC_f(ent);
}

void Cmd_NpcBehavior_f(gentity_t *ent, int iArg) {
	gentity_t *targ = NULL;
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if(trap_Argc() <= 2) {
		targ = AimAnyTarget(ent, 1024);
	}
	else {
		int i = atoi(arg);
		if(i > 0 && i < MAX_GENTITIES)
			targ = &g_entities[i];
		trap_Argv(2, arg, sizeof(arg));
	}
	if(!targ || !targ->NPC) {
		Disp(ent, "^3Not an npc.");
		return;
	}
	if(targ->client->playerTeam != ent->client->playerTeam) {
		Disp(ent, "^3This npc is not an ally.");
		return;
	}
	bState_t behavior;
	if(Q_stricmp(arg, "guard") == 0) {
		behavior = BS_STAND_GUARD;
	}
	else if(Q_stricmp(arg, "cinematic") == 0) {
		behavior = BS_CINEMATIC;
	}
	else if(Q_stricmp(arg, "hunt") == 0) {
		behavior = BS_HUNT_AND_KILL;
	}
	else if(Q_stricmp(arg, "flee") == 0) {
		behavior = BS_FLEE;
	}
	else {
		Disp(ent, "^3Current supported behavior states (not all work for all npc types):\n"
			"guard: protect target.  Must set goal first\n"
			"cinematic: do nothing\n"
			"hunt: chase enemy.  Must set enemy first\n"
			"flee: retreat from enemy.");
	}
	targ->NPC->behaviorState = behavior;
}

void Cmd_NpcClear_f(gentity_t *ent, int iArg) {
	gentity_t *targ = NULL;
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if(!arg[0]) {
		targ = AimAnyTarget(ent, 1024);
	}
	else {
		int i = atoi(arg);
		if(i > 0 && i < MAX_GENTITIES)
			targ = &g_entities[i];
	}
	if(!targ || !targ->NPC) {
		Disp(ent, "^3Not an npc.");
		return;
	}
	if(targ->client->playerTeam != ent->client->playerTeam || targ->client->enemyTeam == ent->client->enemyTeam) {
		Disp(ent, "^3This npc is not an ally.");
		return;
	}
	targ->NPC->behaviorState = BS_DEFAULT;
	targ->client->leader = NULL;
	targ->enemy = NULL;
}



void Cmd_NpcFollow_f(gentity_t *ent, int iArg) {
	gentity_t *targ = NULL;
	targ = AimAnyTarget(ent, 1024);
	if(!targ || !targ->NPC) {
		Disp(ent, "^3Not an npc.");
		return;
	}
	if(targ->client->playerTeam != ent->client->playerTeam) {
		Disp(ent, "^3This npc is not an ally.");
		return;
	}
	targ->NPC->behaviorState = BS_FOLLOW_LEADER;
	targ->client->leader = ent;
	targ->NPC->followDist = 80;
}

void Cmd_NpcTeam_f(gentity_t *ent, int iArg) {
	gentity_t *targ = NULL;
	char arg[MAX_STRING_CHARS];
	if(trap_Argc() <= 3) {
		Disp(ent, "^3Usage: ^2npcteam <friendlyteam | enemyteam> <team>\n"
			"^3Team can be:\n"
			"^2free\n"
			"^2enemy\n"
			"^2player\n"
			"^2neutral\n");
	}
	targ = AimAnyTarget(ent, 1024);
	if(!targ || !targ->NPC) {
		Disp(ent, "^3Not an npc.");
		return;
	}
	int team;
	trap_Argv(2, arg, sizeof(arg));
	if(Q_stricmp(arg, "free") == 0) {
		team = NPCTEAM_FREE;
	}
	else if(Q_stricmp(arg, "enemy") == 0) {
		team = NPCTEAM_ENEMY;
	}
	else if(Q_stricmp(arg, "player") == 0) {
		team = NPCTEAM_PLAYER;
	}
	else if(Q_stricmp(arg, "neutral") == 0) {
		team = NPCTEAM_NEUTRAL;
	}
	else {
		Disp(ent, "^3Invalid team.");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "friendlyteam") == 0) {
		targ->client->playerTeam = team;
	}
	else if(Q_stricmp(arg, "enemyteam") == 0) {
		targ->client->enemyTeam = team;
	}
	else
		Disp(ent, "^3First argument must be 'friendlyteam' or 'enemyteam'"); 
}

void Cmd_SetGoal_f(gentity_t *ent, int iArg) {
	gentity_t *targ = NULL;
	gentity_t *set;
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if(!arg[0]) {
		targ = AimAnyTarget(ent, 1024);
	}
	else {
		int i = atoi(arg);
		if(i > 0 && i < MAX_GENTITIES)
			targ = &g_entities[i];
		trap_Argv(2, arg, sizeof(arg));
	}
	if(!targ) {
		Disp(ent, "^3Not an entity.");
		return;
	}
	if(!iArg && !targ->NPC) {
		Disp(ent, "^3Entity must be npc");
		return;
	}
	if(targ->client->playerTeam != ent->client->playerTeam || targ->client->enemyTeam == ent->client->enemyTeam) {
		Disp(ent, "^3This npc is not friendly towards you.");
		return;
	}
	if(!arg[0])
		set = ent;
	else {
		int i = atoi(arg);
		if((i == 0 && !(arg[0] == '0' && arg[1] == 0)) || i < 0 || i >= MAX_GENTITIES) {
			Disp(ent, "^3Invalid target index");
			return;
		}
		set = &g_entities[i];
		if(!set->inuse) {
			Disp(ent, "^3Target not in use");
			return;
		}
	}
	if(iArg)
		targ->enemy = set;
	else {
		targ->NPC->goalEntity = set;
	}
}

gentity_t *NPC_SpawnType   (gentity_t *ent,  char  *npc_type, char  *targetname, qboolean isVehicle );
void Cmd_SpawnVehicle_f (gentity_t *ent, int iArg) {
	char NPC_type[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2){
		return;
	}

	trap_Argv(1, NPC_type, sizeof(NPC_type)); //Ufo: was missing

	NPC_SpawnType( ent, NPC_type, NULL, qtrue);
}

cmdEntry_t npcCommandEntries[] = {
	{"npc", "Spawn, list, or kill npcs and vehicles.", lmdCmd_NPC_f, 0, qtrue, 1, 0, 0},
	{"npcbehavior", "[npc entity number] <behavior>.  Set behavior state.", Cmd_NpcBehavior_f, 0, qtrue, 5, 0, 0, 0, qfalse},
	{"npcclear", "[npc entity number].  Reset behavior state, leader, and enemy.", Cmd_NpcClear_f, 0, qtrue, 5, 0, 0, 0, qfalse},
	{"npcfollow", "Tell the npc to follow you.  They will only listen if they are on your team.", Cmd_NpcFollow_f, 0, qtrue, 5, 0, 0, 0, qfalse},	
	{"npcteam", "Set the npc's team.", Cmd_NpcTeam_f, 0, qtrue, 2, 0, 0, 0, qfalse},	
	{"setnpcgoal", "[entity number] [enemy number].  Set the npc's goal.", Cmd_SetGoal_f, 0, qtrue, 5, 0, 0, 0, qfalse},
	{"setenemy", "[entity number] [enemy number].  Set the npc's enemy.  Also works on turrets.", Cmd_SetGoal_f, 1, qtrue, 5, 0, 0, 0, qfalse},
	{"spawn","spawn a NPC_Vehicle.", Cmd_SpawnVehicle_f, 0, qtrue, 3, 0, (1 << GT_SIEGE)|(1 << GT_BATTLE_GROUND)},

	{NULL}
};