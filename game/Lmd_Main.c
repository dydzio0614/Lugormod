
#include "g_local.h"
#include "Lmd_EntityCore.h"

//#define LMD_MEMORY_DEBUG


#ifdef LMD_MEMORY_DEBUG
#include <crtdbg.h>
#endif

void Accounts_SaveAll(qboolean force);

unsigned int Factions_Load(void);
unsigned int Accounts_Load();
void Commands_Init();
unsigned int Auths_Load();
void loadLingoFilter();
int LoadLocationData();
#ifdef LMD_USEMAPDATA
void Lmd_LoadMapData();
#endif
void Bans_Load();
void LoadMapDefaults(void);
void Lmd_IPs_Init();
void Prof_Init();

void Accounts_CustomSkills_Register();
void Accounts_Friends_Register();
void Accounts_Property_Register();
void Accounts_Stats_Register();
void Accounts_Auths_Register();
void Accounts_Inventory_Register();
void Accounts_Prof_Register();

void Lmd_Startup(void) {
	Lmd_IPs_Init();
	Commands_Init();
	Prof_Init();

	// What exactly does this include?
	//v * 2 & 2
	//1 | 2 | yes
	//2 | 4 | no
	//3 | 6 | yes
	//4 | 8 | no
	//5 | 10 | yes
	G_Printf("^5Loading default entities...\n");
	LoadMapDefaults();
	if (((g_cmdDisable.integer) * 2 & (1 << 1)) == 0){
		G_Printf("^5Registering account plugins...\n");
		Accounts_CustomSkills_Register();
		Accounts_Friends_Register();
		Accounts_Property_Register();
		Accounts_Stats_Register();
		Accounts_Auths_Register();
		Accounts_Inventory_Register();
		Accounts_Prof_Register();

		G_Printf("^5Loading authfiles...\n");
		G_Printf("%u loaded.\n", Auths_Load());

		G_Printf("^5Loading accounts...\n");
		G_Printf("%u loaded.\n", Accounts_Load());

		G_Printf("^5Loading factions...\n");
		G_Printf("%u loaded.\n", Factions_Load());
	}

#ifdef LMD_USEMAPDATA
	Lmd_LoadMapData();
#endif
	G_Printf("^5Loading language filter...\n");
	loadLingoFilter();
	G_Printf("^5Loading location data...\n");
	LoadLocationData();
	G_Printf("^5Loading bans...\n");
	Bans_Load();
}

void jailPlayer(gentity_t *targ, int time);
void PlayerItem_Scan(gentity_t *player);
void Confirm_Check(gentity_t *ent);
void Interact_Check(gentity_t *ent);
void Inventory_Player_Think(gentity_t *player);
void updatePenalties(gentity_t *ent);
void Chicken (gentity_t *chicken);
void checkKingTimer(gentity_t *ent);
void CheckRestrictAll(gentity_t *player);
void Cmd_Kill_f(gentity_t *ent);
void Lmd_PlayerThink(gentity_t *ent){

	//Ufo:
	if (!ent || !ent->inuse || !ent->client || ent->s.number >= MAX_CLIENTS)
		return;

	if (ent->client->Lmd.thinkDelay > level.time)
		return;

#ifndef LMD_NEW_JETPACK
	if (ent->client->Lmd.restrict & 4)
		Jetpack_Off(ent);
#endif

	if (ent->client->Lmd.restrict & 8 && ent->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL){
		Cmd_Kill_f(ent);
	}

	if (ent->client->Lmd.restrict & 32 && ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] > 0) {
		ent->client->Lmd.backupJumpLevel = ent->client->ps.fd.forcePowerLevel[FP_LEVITATION];
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 0;
	}

	if (ent->client->Lmd.backupJumpLevel && !(ent->client->Lmd.restrict & 32)) {
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = ent->client->Lmd.backupJumpLevel;
		ent->client->Lmd.backupJumpLevel = 0;
	}

	if(ent->client->pers.Lmd.jailTime > 0 && ent->client->pers.Lmd.jailTime < level.time){
		jailPlayer(ent, 0);
	}

	if(gameMode(GMF_CLOAKING)) {
		ent->client->ps.powerups[PW_CLOAKED] = INT_MAX;
	}

	CheckRestrictAll(ent);

	checkKingTimer(ent);

	Chicken(ent);   

	Inventory_Player_Think(ent);
	PlayerItem_Scan(ent);

	Confirm_Check(ent);
	Interact_Check(ent);

	updatePenalties(ent);
	
	ent->client->Lmd.thinkDelay = level.time + FRAMETIME;
}


//Ufo: discarded
#ifdef _NAKEN
void naken_run(void);
#endif
void Factions_Save(qboolean full);
int LoadEntitiesData(const char *filename);
extern qboolean isSavingMap;
void Lmd_IPs_Run();

//void Inventory_Think();

#ifdef LMD_EXPERIMENTAL
void CheckJediItemSpawn();
#endif

void Lmd_RunFrame(void){

	Accounts_SaveAll(qfalse);

	Factions_Save(qfalse);

	Lmd_IPs_Run();

#ifdef LMD_EXPERIMENTAL
	CheckJediItemSpawn();
#endif

	//Inventory_Think();

//Ufo: discarded
#ifdef _NAKEN
	naken_run();
#endif
}

void ShutdownEntitySystem();
void Auths_Shutdown();
void Lmd_Shutdown(void){

	SaveEntitiesData("previous");
	ShutdownEntitySystem();

	Accounts_SaveAll(qtrue);
	Factions_Save(qtrue);

	Auths_Shutdown();
}

