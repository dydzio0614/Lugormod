

#include "g_local.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Data.h"
#include "Lmd_Professions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_Bans.h"
#include "Lmd_Time.h"

#define STANDARD_BEAM "env/hevil_bolt"

gentity_t *ClientFromArg (gentity_t *to, int argNum);
gentity_t* AimAnyTarget (gentity_t *ent, int length);
gentity_t* AimTarget (gentity_t *ent, int length);
int ClientNumberFromString( gentity_t *to, char *s );

void Professions_SetDefaultSkills(Account_t *acc, int prof);
void ClientCleanName(const char *in, char *out, int outSize);
qboolean IsValidPlayerName(char *name, gentity_t *ent, qboolean isRegister);
void Cmd_AccountEdit_f(gentity_t *ent, int iArg){
	Account_t *acc;
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	if(trap_Argc() < 4){
		Disp(ent, "^3Usage: Accountedit ^2<username or id> <stat> <value>\n"
			"^3Stats are:\n"
			"^2Name\n"
			"^2Credits\n"
			"^2Profession\n"
			"^2Level\n"
			"^2Score");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	//accIndex = getAccountIndexByName(arg); //dont check for name, what if someone sets their name to someone elses username?
	//if(accIndex == -1)
	acc = Accounts_GetById(atoi(arg));
	if(!acc)
		acc = Accounts_GetByUsername(arg);
	if(!acc){
		Disp(ent, "^3Unable to find account.");
		return;
	}
	trap_Argv(2, arg, sizeof(arg));
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "name") == 0){
		ClientCleanName((const char *)val, arg, sizeof(arg));
		if(!IsValidPlayerName(arg, NULL, qfalse)){
			Disp(ent, "^3That name is invalid or already in use.");
			return;
		}

		Accounts_SetName(acc, arg);
		Disp(ent, "^2Name changed.");
	}
	else if(Q_stricmp(arg, "credits") == 0){
		int v = atoi(val);
		if(v == 0 && !(val[0] == '0' && val[1] == 0)){
			Disp(ent, "^3Invalid credit amount.");
			return;
		}
		if(v < 0){
			Disp(ent, "^3Invalid amount, credits must be greater than or equal to zero.");
			return;
		}
		Accounts_SetCredits(acc, v);
		Disp(ent, "^2Credits set.");
	}
	else if(Q_stricmp(arg, "profession") == 0){
		int p = -1;
		if(Q_stricmp(val, "none") == 0)
			p = PROF_NONE;
		else if(Q_stricmp(val, "god") == 0)
			p = PROF_ADMIN;
		else if(Q_stricmp(val, "jedi") == 0)
			p = PROF_JEDI;
		else if(Q_stricmp(val, "merc") == 0)
			p = PROF_MERC;
		else{
			Disp(ent, "^3Invalid profession, valid values:\n^2None\n^2God\n^2Jedi\n^2Merc");
			return;
		}
		Accounts_Prof_SetProfession(acc, p);
		Professions_SetDefaultSkills(acc, p);
		Disp(ent, "^2Profession changed.");
	}
	else if(Q_stricmp(arg, "level") == 0){
		int v = atoi(val);
		if(v == 0 && !(val[0] == '0' && val[1] == 0)){
			Disp(ent, "^3Invalid level.");
			return;
		}
		if(v <= 0){
			Disp(ent, "^3Invalid level, level must be greater than zero.");
			return;
		}
		if(v > 40){
			Disp(ent, "^3Invalid level, level must be less than 40.");
			return;
		}
		Accounts_Prof_SetLevel(acc, v);
		Disp(ent, "^2Level set.");
	}
	else if(Q_stricmp(arg, "score") == 0){
		int v = atoi(val);
		if(v == 0 && !(val[0] == '0' && val[1] == 0)){
			Disp(ent, "^3Invalid score.");
			return;
		}
		if(v < 0){
			Disp(ent, "^3Invalid score, score must be greater than or equal to zero.");
			return;
		}
		Accounts_SetScore(acc, v);
		Disp(ent, "^2Score set.");
	}
	else{
		Disp(ent, "^3Invalid stat.  Use the command without args to view the available stats.");
	}
}

void GetStats (gentity_t *ent, Account_t *acc);
void Cmd_AccountInfo_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	Account_t *acc;
	if(trap_Argc() == 1){
		Disp(ent, "^3Usage: accountinfo ^2<player name, username, or user id>");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	acc = Accounts_GetById(atoi(arg));
	if(!acc)
		acc = Accounts_GetByUsername(arg);
	if(!acc)
		acc = Accounts_GetByName(arg);
	if(!acc){
		Disp(ent, "^3Unable to find account.");
		return;
	}
	GetStats(ent, acc);
}

int g_announceTime = 0;
char *g_announceMessage = NULL;
void Cmd_Announce_f (gentity_t *ent, int iArg)
{
	if (trap_Argc() < 3) {
		Disp(ent, "^3Usage: announce ^2<time> <msg>");
		return;
	}
	if (g_announceMessage) {
		G_Free(g_announceMessage);
	}

	char arg[5];
	int time;

	trap_Argv(1, arg, sizeof(arg));
	time = atoi(arg);
	if (time > 10) {
		time = 10;
	}
	time *= 1000;

	g_announceTime = level.time + time;

	g_announceMessage = G_NewString(ConcatArgs(2));
}

void Cmd_CancelVote_f (gentity_t *ent, int iArg)
{
	//Check if vote is in progress
	if ( level.voteTime + VOTE_TIME < level.time) {
		Disp(ent, "There is no vote in progress.");
		return;
	}
	level.voteYes = 0;
	level.voteNo  = 1;
	level.voteTime = level.time - VOTE_TIME;
}

void Cmd_Freeze_f(gentity_t *ent, int iArg){
	gentity_t *targ;
	if (trap_Argc() > 1){
		targ = ClientFromArg(ent, 1);
	}
	else{
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		targ = AimTarget(ent, 8192);
	}

	if(!targ || !targ->inuse || !targ->client){
		Disp(ent, "^3Invalid player specified.");
		return;
	}

	if(Auths_Inferior(ent, targ)) {
		Disp(ent, "^3You are inferior to that player.");
		return;
	}

	if(targ->r.ownerNum != ENTITYNUM_NONE){
		Disp(ent, "^3You cannot freeze or unfreeze this player, they are in a camera or turret.");
		return;
	}
	if(targ->client->Lmd.flags & SNF_FREEZE){
		targ->client->Lmd.flags &= ~SNF_FREEZE;
		Disp(ent, "^3Player ^2unfrozen.");
		trap_SendServerCommand(targ->s.number, "cp \"^2You have been unfrozen.");
	}
	else{
		targ->client->Lmd.flags |= SNF_FREEZE;
		Disp(ent, "^3Player ^1frozen.");
		trap_SendServerCommand(targ->s.number, "cp \"^3You have been frozen.");
	}
}

void Cmd_Teleport_f (gentity_t *ent, int iArg){
	//RoboPhred: general changes throughout this func for /sendto

	gentity_t *tEnt, *fEnt = ent;
	char otherindex[MAX_TOKEN_CHARS];
	char index2[MAX_TOKEN_CHARS];
	int i;
	vec3_t    t, tv, viewAngles, to;
	vec_t dist;
	trace_t tr;

	if(trap_Argc() < 2) {
		Disp(ent, "^3No player specified.");
		return;
	}

	trap_Argv(1, otherindex, sizeof(otherindex));
	trap_Argv(2, index2, sizeof(index2));

	if (!otherindex[0]){
		Disp(ent, "^3The second argument must be a player name or number.\n");
		return;
	}

	if(iArg >= 2){
		i = ClientNumberFromString(ent, otherindex);

		if (i < 0 || i >= MAX_CLIENTS){
			Disp(ent, "^3Invalid target player.");
			return;
		}
		fEnt = &g_entities[i];

		if(!index2[0]){
			Disp(ent, "^3Usage: Sendto ^2<teleport player> <target player>");
			return;
		}
		i = ClientNumberFromString(ent, index2);
		if(i < 0){
			Disp(ent, "^3Invalid destination player.");
			return;
		}
	}
	else{
		i = ClientNumberFromString(ent, otherindex);
		if(i < 0){
			Disp(ent, "^3Invalid target.");
			return;
		}
	}

	tEnt = &g_entities[i];

	if(fEnt->client->ps.m_iVehicleNum){
		Disp(ent, va("^3Unable to teleport%s are in a vehicle.", (iArg == 1)?", you":" target player, they"));
		return;
	}
	if(tEnt->client->ps.m_iVehicleNum){
		Disp(ent, va("^3Unable to teleport%s to target.  The target is in a vehicle.", (iArg == 1)?"":" player"));
		return;
	}
	if(duelInProgress(&fEnt->client->ps)){
		Disp(ent, va("^3Unable to teleport%s are in a duel.", (iArg == 1)?", you":" target player, they"));
		return;
	}

	if(tEnt->s.number == fEnt->s.number){
		Disp(ent, "^3Target and destination players are the same.");
		return;
	}
	if(iArg == 0){
		tEnt = ent;
		fEnt = &g_entities[i];
	}

	if (Auths_Inferior(ent, fEnt))
		return;

	if(fEnt->r.ownerNum != ENTITYNUM_NONE){
		Disp(ent, va("^3%s in a camera or turret.", (iArg == 1)?"You are":"Target player is"));
		return;
	}

	dist = fEnt->r.maxs[0] + tEnt->r.maxs[0] + 30;
	VectorCopy(tEnt->r.currentOrigin, t);
	AngleVectors(tEnt->client->ps.viewangles, tv, NULL, NULL);
	tv[2] = 0;
	VectorNormalize(tv);
	VectorMA(t, dist, tv, t);
	t[2] += 64;
	//RoboPhred: why only ent?  Need to get this back now that I have /sendto
	trap_Trace(&tr, tEnt->client->ps.origin, fEnt->r.mins, fEnt->r.maxs, t, tEnt->s.number, fEnt->clipmask);
	//trap_Trace(&tr, tEnt->client->ps.origin, fEnt->r.mins,fEnt->r.maxs, t, tEnt->s.number, /*fEnt->clipmask*/ent->clipmask);
	if(!fEnt->client->noclip && tr.fraction != 1.0f){
		Disp(ent, "^3Target area is blocked.");
		return;
	}
	VectorCopy (t,to);
	to[2] -= 4096;

	trap_Trace(&tr, t, fEnt->r.mins,fEnt->r.maxs, to, tEnt->s.number, fEnt->clipmask);
	if (!fEnt->client->noclip && tr.fraction == 1.0f){
		Disp(ent, "^3Target area is in mid air.");                
		return;
	}

	VectorCopy(tEnt->client->ps.viewangles, viewAngles);
	viewAngles[YAW] += 180;
	TeleportPlayer(fEnt, tr.endpos, viewAngles, qfalse);
	VectorSet(fEnt->client->ps.velocity, 0, 0, 0);
}

void Cmd_GotoPoint_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	if(argc == 1){
		trace_t tr;
		vec3_t origin, dir;

		VectorCopy(ent->client->renderInfo.eyePoint, origin);
		AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
		VectorNormalize(dir);
		VectorMA(origin, 26635, dir, origin);
		trap_Trace(&tr, ent->client->renderInfo.eyePoint, NULL, 
			NULL, origin, ent->s.number, ent->clipmask);
		if(tr.fraction >= 1){
			Disp(ent, "^3Target is too far away!");
			return;
		}
		VectorCopy(tr.endpos, origin);
		VectorMA(origin, 50, tr.plane.normal, origin);
		VectorSet(ent->client->ps.velocity, 0, 0, 0);
		VectorCopy(origin, ent->client->ps.origin);
		trap_LinkEntity(ent);
		Disp(ent, "^2Teleported to crosshair.");
	}
	else{
		vec3_t loc;
		vec_t vec;
		char arg[MAX_STRING_CHARS];
		int i;
		if(argc < 4){
			Disp(ent, "^3Usage: Gotopoint ^2<x pos> <y pos> <z pos>");
			return;
		}
		for(i = 0;i<3;i++){
			trap_Argv(i + 1, arg, sizeof(arg));
			vec = atoi(arg);
			if(vec == 0 && !(arg[0] == '0' && arg[1] == 0)){
				Disp(ent, "^3Invalid argument, expected a number.");
				return;
			}
			loc[i] = vec;
		}
		TeleportPlayer(ent, loc, ent->client->ps.viewangles, qfalse);
		Disp(ent, "^2Teleported to cordinates.");
	}
}

void Cmd_HideAdminStatus_f(gentity_t *ent, int iArg){
	if(ent->client->pers.Lmd.persistantFlags & SPF_HIDEADMIN){
		ent->client->pers.Lmd.persistantFlags &= ~SPF_HIDEADMIN;
		Disp(ent, "^3Admin status: ^2Visible");
	}
	else{
		ent->client->pers.Lmd.persistantFlags |= SPF_HIDEADMIN;
		Disp(ent, "^3Admin status: ^6Hidden");
	}
}

void jailPlayer(gentity_t *targ, int time){
	char *targMsg;
	gentity_t *spawnPoint;
	vec3_t newOrigin, newAngles;
	if(time > 0 || time == -1){
		char timeStr[MAX_STRING_CHARS];
		if(time == -1){
			Q_strncpyz(timeStr, "indefinitely", sizeof(timeStr));	
			targ->client->pers.Lmd.jailTime = level.time + Q3_INFINITE; //Ufo: Q3_INFINITE is insufficient for leveltime dependant stuff
		}
		else{
			Q_strncpyz(timeStr, va("for %g minutes", ((float)(time) / 1000.0f / 60.0f)), sizeof(timeStr));	
			targ->client->pers.Lmd.jailTime = level.time + time;
		}

		if(targ->client->ps.m_iVehicleNum) {
			gentity_t *veh = &g_entities[targ->client->ps.m_iVehicleNum];
			veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)targ, qtrue);
		}
		
		spawnPoint = SelectSpawnPoint(targ, newOrigin, newAngles );
		TeleportPlayer(targ, newOrigin, newAngles, qfalse);
		if(spawnPoint)
			G_UseTargets(spawnPoint, targ);

		targMsg = va("^3You have been jailed %s.\n^3If you leave the server while in jail\n"
			"^3you will be temporarily banned from this server.", timeStr);
		trap_SendServerCommand(targ->s.number, va("cp \"%s\"", targMsg));
		Disp(targ, targMsg);
		trap_SendServerCommand(-1, va("print \"%s ^3has been jailed.\n\"", targ->client->pers.netname));
	}
	else{
		trap_SendServerCommand(targ->s.number, "cp \"^3You have been unjailed.");
		targ->client->pers.Lmd.jailTime = 0;
		trap_SendServerCommand(-1, va("print \"%s ^3has been unjailed.\n\"", targ->client->pers.netname));

		spawnPoint = SelectSpawnPoint(targ, newOrigin, newAngles);
		TeleportPlayer(targ, newOrigin, newAngles, qfalse);
		if(spawnPoint)
			G_UseTargets(spawnPoint, targ);
	}
}

void Cmd_Jail_f(gentity_t *ent, int iArg){
	gentity_t *tEnt;
	qboolean argShift = qfalse;
	if (trap_Argc() > 1) {
		tEnt = ClientFromArg(ent, 1);
		argShift = qtrue;
	}
	else{
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimTarget(ent, 8192);
	}

	if(!tEnt || !tEnt->inuse || !tEnt->client){
		Disp(ent, "^3Invalid player specified.");
		return;
	}
	if (Auths_Inferior (ent, tEnt)) {
		Disp(ent, "^3You cannot jail that player.");
		return;
	}

	if(!(tEnt->client->pers.Lmd.jailTime > level.time)){
		int time = -1;
		float fTime;
		char arg[MAX_STRING_CHARS];
		if(trap_Argc() >= 2 + argShift){
			trap_Argv(1 + argShift, arg, sizeof(arg));
			if(arg[0]){
				fTime = atof(arg);
				if(fTime <= 0.0f){
					Disp(ent, "^3Invalid time given. Time is the time in minutes and must be greater than zero.");
					return;
				}
				time = (int)floor(fTime * 60.0f * 1000.0f);
			}
			else
				Disp(ent, "^3No jail time given, defaulting to infinite.");
		}
		else
			Disp(ent, "^3Jail time defaulting to infinite.");
		jailPlayer(tEnt, time);
	}
	else {
		jailPlayer(tEnt, 0);
	}
}

void WP_SaberAddG2Model(gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void JediMasterUpdate (gentity_t *self);
void Cmd_JmQuit_f (gentity_t *ent, int iArg) {
	if (g_gametype.integer != GT_JEDIMASTER) {
		return;
	}
	int clientNum = ent->s.number;
	if (ent->client->ps.isJediMaster) {
		vec3_t a;
		gentity_t *saberent = &g_entities[ent->client->ps.saberIndex];
		//Shouldn't I be able to use hurl here?
		AngleVectors(ent->client->ps.viewangles, a,NULL,NULL);

		trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

		WP_SaberAddG2Model( saberent, ent->client->saber[0].model, ent->client->saber[0].skin );

		saberent->enemy = NULL;                
		saberent->flags = FL_BOUNCE_HALF;
		saberent->s.eFlags &= ~(EF_NODRAW);

		saberent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");

		saberent->s.modelGhoul2 = 1;
		saberent->s.g2radius = 20;
		saberent->s.eType = ET_MISSILE;
		saberent->s.weapon = WP_SABER;
		saberent->s.pos.trType = TR_GRAVITY;
		saberent->s.pos.trTime = level.time;
		VectorSet( saberent->r.maxs, 3, 3, 3 );
		VectorSet( saberent->r.mins, -3, -3, -3 );
		saberent->r.contents = CONTENTS_TRIGGER;
		saberent->clipmask = MASK_SOLID;
		saberent->isSaberEntity = qtrue; 
		saberent->bounceCount = -5;
		saberent->physicsObject = qtrue;

		G_SetOrigin(saberent, ent->client->lastSaberBase_Always); //use this as opposed to the right hand bolt,
		//because I don't want to risk reconstructing the skel again to get it here. And it isn't worth storing.
		VectorNormalize(a);

		saberent->s.pos.trDelta[0] = a[0]*256;
		saberent->s.pos.trDelta[1] = a[1]*256;
		saberent->s.pos.trDelta[2] = 256;

		trap_LinkEntity(saberent);
		saberent->nextthink = level.time + 50;


		//--------------------------------------------
		ent->client->ps.isJediMaster = qfalse;
		ent->maxHealth = 0;
		JediMasterUpdate(ent);
		//This should not be hardcoded
		if (ent->health > 125) {
			ent->health = 125;
		}
		//ent->client->ps.stats[STAT_ARMOR] = 25;
		ent->client->ps.stats[STAT_WEAPONS] &= 
			~(1 << WP_SABER);
		ent->client->ps.stats[STAT_WEAPONS] |= 
			(1 << WP_MELEE);
		ent->client->ps.stats[STAT_WEAPONS] |= 
			(1 << WP_BRYAR_PISTOL);
		ent->client->ps.weapon = WP_MELEE;

	} else {
		trap_SendServerCommand( clientNum, "print \"You are not the Jedi Master!\n\"");
	}
	return;
}

void listAdmins(gentity_t *ent);
void Cmd_ListAdmins_f(gentity_t *ent, int iArg){
	listAdmins(ent);
}

void Cmd_ToggleSPF_f(gentity_t *ent, int flag){
	gentity_t *tEnt = NULL;
	char cmd[MAX_TOKEN_CHARS];
	trap_Argv(0,cmd,sizeof(cmd));

	if (trap_Argc() > 1)
		tEnt = ClientFromArg(ent, 1);
	else if(!ent)
		return;
	else{
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimTarget(ent, 8192);
	}

	if (!tEnt || !tEnt->client || !tEnt->inuse) {
		return;
	}

	if (Auths_Inferior(ent, tEnt)){
		return;
	}
	if ( tEnt->client->pers.Lmd.persistantFlags & flag) {
		Disp(ent, va("^3%s ^1OFF", cmd));
		tEnt->client->pers.Lmd.persistantFlags &= ~flag;
		if (flag == SPF_SHUTUP) {
			trap_SendServerCommand(tEnt-g_entities, "cp \"You are no longer muted.\"");
		} else if (flag == SPF_NOCALL) {
			trap_SendServerCommand(tEnt-g_entities, "cp \"Calling votes is no longer disabled for you.\"");
		}
	} else {
		Disp(ent,va("^3%s ^2ON",cmd));
		tEnt->client->pers.Lmd.persistantFlags |= flag;
		if (flag == SPF_SHUTUP) {
			trap_SendServerCommand(tEnt-g_entities, "cp \"You have been muted.\nIf you leave the server while muted\nyou will be temporarily banned from this server.\"");
		} else if (flag == SPF_NOCALL) {
			trap_SendServerCommand(tEnt-g_entities, "cp \"Calling votes has been disabled for you.\nIf you leave the server while this is disabled\nyou will be temporarily banned from this server.\"");
		}
	}
}

#ifdef LMD_EXPERIMENTAL
void Cmd_NPCCmd_f(gentity_t *ent, int iArg) {
	if(trap_Argc() < 2) {
		Disp(ent, "^3TODO: help text");
		return;
	}

	gentity_t *targ;
	targ = AimAnyTarget(ent, 1024);
	if(!targ || !targ->NPC) {
		Disp(ent, "^3Not an npc.");
		return;
	}
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "follow") == 0) {
		//TODO: if another arg, follow that index
		gentity_t *followTarg = ent;

		targ->NPC->behaviorState = BS_FOLLOW_LEADER;
		targ->client->leader = followTarg;
		targ->NPC->followDist = 80;
	}
	else if(Q_stricmp(arg, "default") == 0) {
		targ->NPC->behaviorState = BS_DEFAULT;
	}
	else if(Q_stricmp(arg, "cinematic") == 0) {
		targ->NPC->behaviorState = BS_CINEMATIC;
	}
	else if(Q_stricmp(arg, "walkspeed") == 0) {
		trap_Argv(2, arg, sizeof(arg));
		if(arg[0])
			targ->NPC->stats.walkSpeed = atoi(arg);
		else
			Disp(ent, va("^3Walk speed: %i", targ->NPC->stats.walkSpeed));
	}
	else if(Q_stricmp(arg, "runspeed") == 0) {
		trap_Argv(2, arg, sizeof(arg));
		if(arg[0])
			targ->NPC->stats.runSpeed = atoi(arg);
		else
			Disp(ent, va("^3Run speed: %i", targ->NPC->stats.runSpeed));
	}
	else if(Q_stricmp(arg, "playerteam") == 0) {
		trap_Argv(2, arg, sizeof(arg));
		if(arg[0])
			targ->client->playerTeam = atoi(arg);
		else
			Disp(ent, va("^3Player team: %i", targ->client->playerTeam));
	}
	else if(Q_stricmp(arg, "enemyteam") == 0) {
		trap_Argv(2, arg, sizeof(arg));
		if(arg[0])
			targ->client->enemyTeam = atoi(arg);
		else
			Disp(ent, va("^3Enemy team: %i", targ->client->enemyTeam));
	}
}
#endif


void Cmd_PassVote_f (gentity_t *ent, int iArg)
{
	//Check if vote is in progress

	if ( level.voteTime + VOTE_TIME < level.time) {
		Disp(ent, "There is no vote in progress.");
		return;
	}

	level.voteYes = 64;
	level.voteNo  = 0;
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
}

extern vmCvar_t lmd_penaltyAddTime;
extern vmCvar_t lmd_penaltyRemoveTime;
extern vmCvar_t lmd_penaltyJailCount;
extern vmCvar_t lmd_penaltyJailTime;
extern vmCvar_t lmd_penaltyTmpbanCount;
extern vmCvar_t lmd_penaltyTmpbanTime;

void updatePenalties(gentity_t *ent) {
	if(lmd_penaltyRemoveTime.integer <= 0)
		return;
	if(ent->client->sess.Lmd.penalties.count > 0 && ent->client->sess.Lmd.penalties.lastDebounce + (lmd_penaltyRemoveTime.integer * 1000) <= level.time) {
		ent->client->sess.Lmd.penalties.count--;
		ent->client->sess.Lmd.penalties.lastDebounce = level.time;
	}
}

void Cmd_Penalize_f(gentity_t *ent, int iArg) {
	gentity_t *targ;
	if(trap_Argc() < 2) {
		Disp(ent, "^3Usage: /penalize ^2<player>^3");
		return;
	}
	targ = ClientFromArg(ent, 1);
	if (!targ || Auths_Inferior(ent, targ)) {
		Disp(ent, "^3You cannot penalize this player.");
		return;
	}
	int nextPenalty = targ->client->sess.Lmd.penalties.lastPenalty + (lmd_penaltyAddTime.integer * 1000);
	if(nextPenalty > level.time) {
		char duration[MAX_STRING_CHARS];
		Time_DurationString((nextPenalty - level.time) / 1000, duration, sizeof(duration));
		Disp(ent, va("^3You must wait ^2%s^3 seconds before adding another penalty to this player.", duration));
		return;
	}
	targ->client->sess.Lmd.penalties.count++;
	targ->client->sess.Lmd.penalties.lastDebounce = targ->client->sess.Lmd.penalties.lastPenalty = level.time;
	if(lmd_penaltyTmpbanCount.integer > 0 && targ->client->sess.Lmd.penalties.count == lmd_penaltyTmpbanCount.integer) {
		if(lmd_penaltyTmpbanTime.value < 0) {
			Disp(ent, "^3The player has been banned.");
			Lmd_Bans_BanPlayer(ent);
		}
		else {
			Disp(ent, "^3The player has been temporarily banned.");
			Lmd_Bans_TempBanPlayer(targ, (int)floor(lmd_penaltyTmpbanTime.value * 60.0f * 1000.0f), NULL);
		}
	}
	else if(lmd_penaltyJailCount.integer > 0 && targ->client->sess.Lmd.penalties.count == lmd_penaltyJailCount.integer) {
		float time = (int)floor(lmd_penaltyJailTime.value * 60.0f * 1000.0f);
		if(time < 0)
			time = 0;
		Disp(ent, "^3The player has been jailed.");
		jailPlayer(targ, time);
	}
	else {
		Disp(ent, va("^3The player has been penalized, they now have ^2%i^3 penalties.", targ->client->sess.Lmd.penalties.count));
	}
}

int SumCredits(void){
	int n;
	int sum = 0;
	gentity_t *ent;
	for ( n = 0; n < ENTITYNUM_MAX_NORMAL; n++){
		ent = &g_entities[n];
		//Possible crash here showing up in the logs on Q_stricmp.
		//And anyway, this is just common sense
		if(!ent->inuse)
			continue;
		//This might be a sign of more severe things though, since ent->classname should NEVER be invalid.

		switch (ent->s.eType){
			case ET_PLAYER:
				if (!ent->client){
					continue;
				}
				sum += PlayerAcc_GetCredits(ent);
				break;
			case ET_GENERAL:
				if (Q_stricmp("credits", ent->classname)) {
					continue;
				}
				sum += ent->count;
				break;
			default:
				break;
		}
	}
	return sum;
}

unsigned int totalAccountCredits(){ 
	int i, num = Accounts_Count();
	int count = 0;
	for(i = 0; i < num; i++) {
		count += Accounts_GetCredits(Accounts_Get(i));
	}
	return count;
}

void Cmd_Playerlist_f (gentity_t *ent, int iArg){
	int i, from, to, n;
	gentity_t *pEnt;
	char dstr[MAX_TOKEN_CHARS] = "";
	char name[MAX_NAME_LENGTH];
	qboolean shade = qfalse;
	char *str;

	if (trap_Argc() < 2) {
		from = 0;
		to = MAX_CLIENTS;
	}
	else{
		char arg[MAX_TOKEN_CHARS];
		trap_Argv(1, arg, sizeof(arg));
		from = ClientNumberFromString(ent, arg);
		if (from < 0 || from > MAX_CLIENTS) {
			return;
		}
		to = from + 1;
	}

	Disp(ent, "------------------------------------------------------------------------------------------------");
	Disp(ent, "Ind Player                       Mute Caps Call King Iodl Jail Credits lvl Votes Id   Username");

	for (i = from; i < to; i++) {
		pEnt = &g_entities[i];
		if(!pEnt->inuse || !pEnt->client){
			continue;
		}
		if(shade)
			Q_strcat(dstr, sizeof(dstr),"^3");
		else
			Q_strcat(dstr, sizeof(dstr),"^7");
		shade = !shade;

		Q_strcat(dstr, sizeof(dstr), va("%-2i  ", i));
		Q_strncpyz(name, pEnt->client->pers.netname, sizeof(name));
		Q_CleanStr(name);
		Q_strcat(dstr, sizeof(dstr), va("%-28s ", name));
		if(pEnt->client->pers.connected != CON_CONNECTED){
			Q_strcat(dstr, sizeof(dstr), "----------------------------------------------------------------");
			Disp(ent, va("%s", dstr));
			dstr[0] = 0;
			continue;
		}
		for (n = 0;n < 5;n++){
			if (pEnt->client->pers.Lmd.persistantFlags & (1 << n))
				Q_strcat(dstr, sizeof(dstr), "**** ");
			else
				Q_strcat(dstr, sizeof(dstr), "     ");
		}
		if (pEnt->client->pers.Lmd.jailTime > level.time)
			Q_strcat(dstr, sizeof(dstr), "**** ");
		else
			Q_strcat(dstr, sizeof(dstr), "     ");
		Q_strcat(dstr, sizeof(dstr), va("%-7i ", PlayerAcc_GetCredits(pEnt)));
		Q_strcat(dstr, sizeof(dstr), va("%-3i ", PlayerAcc_Prof_GetLevel(pEnt)));
		Q_strcat(dstr, sizeof(dstr), va("%-5i ", pEnt->client->pers.voteCount));
		Q_strcat(dstr, sizeof(dstr), va("%-4i ", PlayerAcc_GetId(pEnt)));
		str = PlayerAcc_GetUsername(pEnt);
		if(!str)
			str = "";
		Q_strcat(dstr, sizeof(dstr), str);
		Disp(ent, dstr);
		dstr[0] = 0;
	}
	Disp(ent, "------------------------------------------------------------------------------------------------");
	Disp(ent, va("^3Credits in the world: ^2CR %i", SumCredits()));
	Disp(ent, va("^3Credits in accounts: ^2CR %u", totalAccountCredits()));
	Disp(ent, va("^3Number of registered accounts: ^2%i", Accounts_Count()));
}

void RenamePlayer( gentity_t *ent, char *Name);
void Cmd_RenamePlayer_f(gentity_t *ent, int iArg){
	int i;
	char arg[MAX_STRING_CHARS];
	char *name;
	if(trap_Argc() < 3){
		Disp(ent, "^3Usage: Rename ^5<player name or client number> <new name>");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	i = ClientNumberFromString(ent, arg);
	if(i < 0)
		return; //ClientNumberFromString gives the player the error message.
	name = ConcatArgs(2);
	if(IsValidPlayerName(name, NULL, qfalse) == qfalse) {
		Disp(ent, "^3Name is not valid.");
		return;
	}

	RenamePlayer(&g_entities[i], name);
	Disp(ent, "^3Player name changed.");
}

void Cmd_Strip_f (gentity_t *ent, int iArg){
	if (trap_Argc() < 3) {
		Disp(ent, "^3Usage: strip ^2<player> <'items'|'weapons'|'force'|'all'>");
		return;
	}
	gentity_t *targ;
	char arg[MAX_TOKEN_CHARS];

	if (!(targ = ClientFromArg(ent, 1))) {
		return;
	}

	if (Auths_Inferior(ent,targ)) {
		return;
	}

	trap_Argv(2, arg, sizeof (arg));
	qboolean stripall = qfalse;

	if (Q_stricmp(arg, "all") == 0) {
		stripall = qtrue;
	}
	if (Q_stricmp (arg, "weapons") == 0 || stripall) {
		targ->client->ps.stats[STAT_WEAPONS] = 1;
		targ->client->ps.weapon = WP_NONE;

		//stop client game from attempting to force a saber to the player.
		targ->client->ps.saberHolstered = 2;
		targ->client->ps.trueJedi = qfalse;
	} 

	if (Q_stricmp (arg, "items") == 0 || stripall) {
		targ->client->jetPackOn = qfalse;
		targ->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;

	} 
	if (Q_stricmp (arg, "force") == 0 || stripall) {
		targ->client->ps.fd.forcePowersKnown &= (1 << FP_LEVITATION) | (1 << FP_SABER_DEFENSE) | (1 << FP_SABER_OFFENSE);

		//This clears and resets the powers, loading them from the info string.
		//WP_InitForcePowers( targ );
	} 

	return;
}

void Cmd_TeamOther_f (gentity_t *ent, int iArg)
{
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		Disp(ent, "^3Usage: ^2<player> <team>");
		return;
	}
	gentity_t *tEnt = ClientFromArg(ent, 1);
	if(!tEnt) {
		return;
	}
	//Ufo:
	if (Auths_Inferior(ent, tEnt)) {
		Disp(ent, "^3You cannot change this player's team");
		return;
	}

	if ( trap_Argc() < 3 ) {
		oldTeam = tEnt->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_DUEL && tEnt->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
		Disp(ent, "Cannot switch teams in Duel");
		return;
	}

	if (g_gametype.integer == GT_POWERDUEL)
	{ //don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		//trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		Disp(ent, "Cannot switch teams in Power Duel");
		return;
	}

	//Lugormod for now don't allow team switch in private duels
	if (duelInProgress(&tEnt->client->ps)) {
		Disp(ent, "This player is in a duel.");
		return;
	}

	trap_Argv( 2, s, sizeof( s ) );

	SetTeam( tEnt, s );

	tEnt->client->switchTeamTime = level.time + 5000;
}

extern vmCvar_t g_tmpBanTime;
void Cmd_TmpBan_f (gentity_t *ent, int iArg) {
	gentity_t *tEnt;
	char *msg = "";
	if (trap_Argc() < 2){
		G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimTarget(ent, 8192);
	}
	else {
		tEnt = ClientFromArg(ent, 1);
		msg = ConcatArgs(2);
	}
	if (!tEnt || !tEnt->inuse) {
		Disp(ent, "^3Player not found.");
		return;
	}
	if (Auths_Inferior(ent, tEnt)) {
		Disp(ent, "^3You are inferior to this admin.");
		return;
	}
	Lmd_Bans_TempBanPlayer(tEnt, 0, msg);
}

void Cmd_Position_f (gentity_t *ent, int iArg);
void Cmd_AddLocation_f (gentity_t *ent, int iArg);
void Cmd_Factions_f(gentity_t *ent, int iArg);
void HiScore (gentity_t  *ent, int field);
void Cmd_Property_f(gentity_t *ent, int iArg);
void Cmd_Say2_f (gentity_t *ent, int iArg);

cmdEntry_t adminCommandEntries[] = {
	{"accountedit", "Modify a player account.", Cmd_AccountEdit_f, 0, qtrue, 1, 0, 0},
	{"accountinfo", "Get account information on the specified player", Cmd_AccountInfo_f, 0, qtrue, 4, 0, 0},
	{"addloc", "Add a description of the current location.", Cmd_AddLocation_f, 0, qtrue, 1, 0, 0},
	{"announce", "Show a message on the center of player's screens.", Cmd_Announce_f, 0, qtrue, 3, 0, 0},
	{"cancelvote","If a vote is in progress, it will fail.", Cmd_CancelVote_f, 0, qtrue, 3, 0, 0},
	{"factionadmin", "View and edit player factions." , Cmd_Factions_f, 1, qtrue, 1, 0, 0}, 
	{"freeze", "Freeze a player in place.  If no player given, affect the targeted player.  Use again to unfreeze them.", Cmd_Freeze_f, 0, qtrue, 3, 0, 0},
	{"gethere", "Teleport a player in front of you.", Cmd_Teleport_f, 0, qtrue, 3, 0,(1 << GT_SIEGE)|(1 << GT_CTF)|(1 << GT_CTY)|(1 << GT_BATTLE_GROUND)|(1 << GT_SABER_RUN)},
	{"goto", "Teleport to infront of <player>.", Cmd_Teleport_f, 1, qtrue, 4, 0, (1 << GT_SIEGE)|(1 << GT_CTF)|(1 << GT_CTY)|(1 << GT_BATTLE_GROUND)|(1 << GT_SABER_RUN)},
	{"gotopoint", "Teleports you to the given cordinates.  If no cordinates specified, it teleports you to the location you are aiming at.", Cmd_GotoPoint_f, 0, qtrue, 4, 0, (1 << GT_SIEGE)|(1 << GT_CTF)|(1 << GT_CTY)|(1 << GT_BATTLE_GROUND)|(1 << GT_SABER_RUN)},
	{"hicredits","Display top ten wealthiest players.", HiScore, 2, qtrue, 1, 129, 0},
	{"hideadmin", "Toggles your visibility in the admin list.", Cmd_HideAdminStatus_f, 0, qtrue, 2, 0, 0},
	{"jail", "Jail a player. If no argument is provided, the target in sight will be jailed.", Cmd_Jail_f, 0, qtrue, 4, 0, 0},//~(1 << GT_FFA)},
	{"jmquit", "Quit being Jedi Master in Jedi Master game type.", Cmd_JmQuit_f, 0, qtrue, 2, 0, ~(1 << GT_JEDIMASTER)},
	{"listadmins","List all admins.", Cmd_ListAdmins_f, 0, qtrue, 4, 1, 0},
	{"location", "Describes the current location of the specified player (if a locations file exists for the map).", Cmd_Position_f, 0, qtrue, 4, 0, 0},
	{"nocaps", "Makes the specified player talk in lowercase only. If no argument is provided, the target in sight will be chosen.", Cmd_ToggleSPF_f, SPF_UNCAP, qtrue, 4, 0, 0},
	{"novote", "Disable the ability to call votes for the specified player. If no argument is provided, the target in sight will be chosen.", Cmd_ToggleSPF_f, SPF_NOCALL, qtrue, 4, 0, 0},
#ifdef LMD_EXPERIMENTAL
	{"npccmd", "Set NPC behavior actions and other settings.", Cmd_NPCCmd_f, 0, qtrue, 1, 0, 0},
#endif
	{"passvote", "If a vote is in progress, it will pass.", Cmd_PassVote_f, 0, qtrue, 2, 0, 0},
	{"penalize", "Mark this player as misbehaving.  After so many penalties, the player may be jailed or temporaraly banned.\nThis can only be used once on a player within a certain time period, and the player's penalties drop with time.", Cmd_Penalize_f, 0, qtrue, 2, 0, 0},
	{"playerlist", "Get info on the specified player, or all players.", Cmd_Playerlist_f, 0, qtrue, 4, 0, 0},
	{"propadmin", "Grant and remove player access to properties.", Cmd_Property_f, 1, qtrue, 1, 0, 0},
	{"renameplayer", "Change a player's name.", Cmd_RenamePlayer_f, 0, qtrue, 3, 0, 0},
	{"say_admins", "Send a message to all logged in admins.", Cmd_Say2_f, SAY_ADMINS, qtrue, 4, 0, 0},
	{"sendto", "Teleport one player to another.", Cmd_Teleport_f, 2, qtrue, 4, 0, (1 << GT_SIEGE)|(1 << GT_CTF)|(1 << GT_CTY)|(1 << GT_BATTLE_GROUND)|(1 << GT_SABER_RUN)},
	{"shutup", "Mute the specified player. If no argument is provided, the target in sight will be muted.", Cmd_ToggleSPF_f, SPF_SHUTUP, qtrue, 4, 0, 0},
	{"strip","Remove all items, weapons, force powers or all of them from <player>.", Cmd_Strip_f, 0, qtrue, 3, 0, ~(1 << GT_FFA)},
	{"teamother", "Put the specified player in the given team.", Cmd_TeamOther_f, 0, qtrue, 3, 0, 0},
	{"tmpban", "Kick and temporary ban the specified player. If no argument is provided, the target in sight will be temporarily banned.", Cmd_TmpBan_f, 0, qtrue, 3, 0, 0},

	{NULL},
};

//const int adminCmdTableSize = sizeof(adminCommandEntries) / sizeof(cmdEntry_t);

