#include "g_local.h"

//RoboPhred

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Property.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Professions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_Arrays.h"
#include "Lmd_Time.h"

gentity_t *G_GetJediMaster (void);
char	  *ConcatArgs      (int        start );
gentity_t *G_PlayEffect    (int fxID, vec3_t org, vec3_t ang);
int        ClientNumberFromString (gentity_t *to, char *s );
void       PM_SetPMViewAngle (playerState_t *ps, vec3_t angle, usercmd_t *ucmd);
void       SP_info_player_start (gentity_t *ent);
qboolean   saberKnockOutOfHand (gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);
void       StandardSetBodyAnim (gentity_t *self, int anim, int flags);
void       G_Say           (gentity_t *ent, gentity_t *target, int mode, const char *chatText );
void       Cmd_ForceChanged_f( gentity_t *ent );
void       TossClientWeapon (gentity_t *self, vec3_t direction, float speed);
void       BG_CycleInven   (playerState_t *ps, int direction); //bg_misc.c
void       Jedi_Decloak    (gentity_t *self );
void       HiScore         (gentity_t *ent, int field);
void       Cmd_Say_f       (gentity_t *ent, int mode, qboolean arg0 );

extern gentity_t  *g_bestKing;
extern int         g_bestKingScore;

/*
=============
G_NewString2

Builds a copy of the string, without translating \n to real linefeeds
Lugormod
=============
*/
char *G_NewString2( const char *string ) {
	char	*newb, *new_p;
	int		i, len;

	if (!string) {
		return NULL;
	}

	len = strlen(string) + 1;

	newb = (char *) G_Alloc( len );

	new_p = newb;
	for (i = 0; i < len; i++) {
		*new_p++ = string[i];
	}

	return newb;
}

/*
==================
Cmd_AdminInfo_f
Lugormod
==================
*/
void Cmd_AdminInfo_f ( gentity_t *ent, int iArg ) {
	int i;
	gentity_t *targ;

	for(i = 0; i < MAX_CLIENTS; i++) {
		targ = &g_entities[i];
		if(!targ->inuse || !targ->client || targ->client->pers.connected != CON_CONNECTED)
			continue;
		if(!Auths_PlayerHasAdmin(targ))
			continue;
		if(targ->client->pers.Lmd.persistantFlags & SPF_HIDEADMIN && ent && Auths_Inferior(ent, targ))
			continue;
		Disp(ent, va("%s:\t^2%i\t%s", targ->client->pers.netname, Auths_GetPlayerRank(targ), Auths_QuickPlayerAuthList(targ)));
	}
}

/*
==================
AimAnyTarget

==================
*/
gentity_t* AimAnyTarget (gentity_t *ent, int length){
	trace_t tr;
	vec3_t fPos,maxs,mins;

	AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);
	VectorSet( mins, -8, -8, -8 );
	VectorSet( maxs, 8, 8, 8 );

	fPos[0] = ent->client->renderInfo.eyePoint[0] + fPos[0]*length;
	fPos[1] = ent->client->renderInfo.eyePoint[1] + fPos[1]*length;
	fPos[2] = ent->client->renderInfo.eyePoint[2] + fPos[2]*length;

	trap_Trace(&tr,ent->client->renderInfo.eyePoint, mins, maxs, fPos, ent->s.number, ent->clipmask);
	if (tr.entityNum >= ENTITYNUM_MAX_NORMAL) {
		return NULL;
	}
	return &g_entities[tr.entityNum];
}


/*
==================
AimTarget

==================
*/
gentity_t* AimTarget (gentity_t *ent, int length)
{
	gentity_t *tEnt;

	tEnt = AimAnyTarget(ent, length);
	if (tEnt
		&& tEnt->inuse
		&& tEnt->client 
		&& tEnt->client->pers.connected == CON_CONNECTED)
	{
		return tEnt;
	}
	return NULL;

}

/*
==================
ClientFromArg
Lugormod
==================
*/
gentity_t *ClientFromArg (gentity_t *to, int argNum) 
{

	if (trap_Argc() > argNum)
	{
		int clNum;
		char Arg[MAX_STRING_CHARS];

		trap_Argv( argNum, Arg, sizeof( Arg ) );

		clNum = ClientNumberFromString(to,Arg);
		if (clNum >= 0 && clNum < MAX_GENTITIES)
		{
			return &g_entities[clNum];
		}
	}
	return NULL;
}

/*
==================
Cmd_IgnoreClient_f
Lugormod
==================
*/
void Cmd_IgnoreClient_f (gentity_t *ent, int iArg) 
{
	gentity_t *iEnt;
	int clientNum = ent->s.number;
	int i = (int)floor((float)clientNum / 16);
	int j = clientNum % 16;
	int k;
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	k = atoi(arg);
	if(k == -1){
		qboolean allIgnored = qtrue;
		for (k = 0; k < MAX_CLIENTS; k++) {
			iEnt = &g_entities[k];
			if (!iEnt->inuse || !iEnt->client) {
				continue;
			}
			if(!(iEnt->client->pers.Lmd.ignoredindex[i] & (1 << j))){
				allIgnored = qfalse;
				break;
			}
		}
		if(allIgnored){
			for (k = 0; k < MAX_CLIENTS; k++) {
				iEnt = &g_entities[k];
				if (!iEnt->inuse || !iEnt->client) {
					continue;
				}
				iEnt->client->pers.Lmd.ignoredindex[i] &= ~(1 << j);
			}
		}
		else{
			for (k = 0; k < MAX_CLIENTS; k++) {
				iEnt = &g_entities[k];
				if (!iEnt->inuse || !iEnt->client) {
					continue;
				}
				iEnt->client->pers.Lmd.ignoredindex[i] |= (1 << j);
			}
		}
	}
	else{
		iEnt = ClientFromArg(ent, 1);
		if (iEnt && iEnt->inuse && iEnt->client) {
			//Ufo: no reason to restrict usage on ourselves
			/*
			if (iEnt->s.number == clientNum) {
				return;
			}
			*/

			iEnt->client->pers.Lmd.ignoredindex[i] ^= (1 << j);

		}
	}
	Disp(ent, "You are currently ignoring:");

	for (k = 0; k < MAX_CLIENTS; k++) {
		iEnt = &g_entities[k];
		if (!iEnt->inuse || !iEnt->client) {
			continue;
		}
		if (iEnt->client->pers.Lmd.ignoredindex[i] & (1 << j)){
			Disp(ent, iEnt->client->pers.netname);
		}
	}
}

/*
==================
Cmd_IgnoreClient_f
Lugormod
==================
*/

qboolean isBuddy(gentity_t *ent, gentity_t *other){
	int i;
	int j;
	
	if(!ent || !ent->client){
		return qfalse;
	}

	if(!other){
		if(ent->client->pers.Lmd.buddyindex[0] > 0 || ent->client->pers.Lmd.buddyindex[1] > 0)
			return qtrue;
		return qfalse;
	}

	j = other->s.number % 16;
	i = (int)floor((float)other->s.number / 16);

	if((ent->client->pers.Lmd.buddyindex[i] & (1 << j)) != 0)
		return qtrue;

	return qfalse;
}


void Cmd_BuddyClient_f (gentity_t *ent, int iArg) 
{
	gentity_t *iEnt;
	iEnt = ClientFromArg(ent, 1);

	int k,i ,j,clientNum;
	if (iEnt && iEnt->inuse && iEnt->client) {
		if (iEnt == ent) {
			return;
		}

		clientNum = iEnt->s.number;
		i = (int)floor((float)clientNum / 16);
		j = clientNum % 16;

		ent->client->pers.Lmd.buddyindex[i] ^= (1 << j);
	}
	Disp(ent, "Your current buddies:");

	for (k = 0; k < MAX_CLIENTS; k++) {
		iEnt = &g_entities[k];
		if (!iEnt->inuse || !iEnt->client) {
			continue;
		}
		i = (int)floor((float)k / 16);
		j = k % 16;

		if (ent->client->pers.Lmd.buddyindex[i] & (1 << j)){
			Disp(ent, iEnt->client->pers.netname);
		}
	}
}

/*
==================
Cmd_Scale_f
Lugormod
==================
*/
void scaleEntity(gentity_t *scaleEnt, int scale){
	if (!scaleEnt || !scaleEnt->client) {
		return;
	}
	scale &= 1023;
	float fs;
	//float os;
	if (!scale) {
		fs = 1.0f;
	} else {
		fs = scale/100.0000f;
	}

	//if (scaleEnt->client->ps.weapon == WP_SABER && !scaleEnt->client->ps.saberHolstered){
	//        Cmd_ToggleSaber_f(scaleEnt);
	//}
	//os = scaleEnt->modelScale[2];
	scaleEnt->s.iModelScale = scale;
	if (scaleEnt->client) {
		scaleEnt->client->ps.iModelScale = scale;
	}

	scaleEnt->modelScale[0] = scaleEnt->modelScale[1] = scaleEnt->modelScale[2] = fs;

	if (scaleEnt->NPC) {
		if (scaleEnt->m_pVehicle) {
			if (scaleEnt->m_pVehicle->m_pDroidUnit) {
				scaleEntity((gentity_t*)scaleEnt->m_pVehicle->m_pDroidUnit, scale);
			}

			//scaleEnt->m_pVehicle->m_pVehicleInfo->cameraOverride = qtrue;
			//scaleEnt->m_pVehicle->m_pVehicleInfo->cameraRange = 80 * fs;
			//scaleEnt->client->ps.crouchheight = (int)(CROUCH_MAXS_2 * fs);
			//scaleEnt->client->ps.standheight = (int)(DEFAULT_MAXS_2 * fs);
		}
		//scale NPC:s here ?

	} else if (scaleEnt->client) {
		scaleEnt->client->ps.crouchheight = (int)((CROUCH_MAXS_2 + 24) * fs - 24);
		scaleEnt->client->ps.standheight = (int)((DEFAULT_MAXS_2 + 24) * fs - 24);
		if (scaleEnt->client->ps.saberEntityNum > MAX_CLIENTS) {
			scaleEntity(&g_entities[scaleEnt->client->ps.saberEntityNum], scale);
		}
	}
}

/*
==================
Cmd_Challenge_f
Lugormod
==================
*/
void Cmd_Challenge_f(gentity_t *ent, int iArg){

	char *dt;
	char *p;
	//if (ent->client->ps.duelInProgress) {
	if (duelInProgress(&ent->client->ps)) {
		return;
	}

	if (trap_Argc() < 2) {
		Disp(ent, "^3Usage: /challenge ^2<types ...>\n"
			"Valid types are: power, force, fullforce, training, tiny, titan, bet, hibet");
		return;
	}
	dt = ConcatArgs(1);
	ent->client->Lmd.duel.duelType = 0;
	ent->client->Lmd.duel.duelBet = 0;

	if (Q_wordsInLine("melee ", dt, &p) && Auths_PlayerHasAuthFlag(ent, AUTH_BETACMDS)) {
		ent->client->Lmd.duel.duelType |= DT_MELEE;
	}
	if (Q_wordsInLine("force ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_FORCE;
	}
	if (Q_wordsInLine("fullforce ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_FULL_FORCE;
	}
	if (Q_wordsInLine("training ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_TRAINING;
	}
	if (Q_wordsInLine("power ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_POWER;
	}
	if (Q_wordsInLine("tiny ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_TINY;
	}
	if (Q_wordsInLine("titan ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_TITAN;
	}
	if (Q_wordsInLine("bet ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_BET;
		ent->client->Lmd.duel.duelBet = 10;
	}
	if (Q_wordsInLine("hibet ", dt, &p)) {
		ent->client->Lmd.duel.duelType |= DT_BET;
		if(ent->client->Lmd.duel.duelBet == 10)
			ent->client->Lmd.duel.duelBet = 500;
		else
			ent->client->Lmd.duel.duelBet = 100;

	}
	if (ent->client->Lmd.duel.duelType == 0) {
		return;
	}
	//trap_SendServerCommand(ent->s.number, 
	//                       va("print \"userint3: %i\n\"",
	//                          ent->client->Lmd.duel.duelType));


	Cmd_EngageDuel_f(ent);
}


//RoboPhred
qboolean PlayerItem_CanSpawnItem(gentity_t *player){
	unsigned int i;
	for(i = 0;i<sizeof(player->client->Lmd.spawnedEnts) / sizeof(player->client->Lmd.spawnedEnts[0]);i++){
		if(player->client->Lmd.spawnedEnts[i] == NULL)
			return qtrue;
	}
	return qfalse;
}

qboolean PlayerItem_Spawn(gentity_t *player, gentity_t *item){
	unsigned int i;
	for(i = 0;i<sizeof(player->client->Lmd.spawnedEnts) / sizeof(player->client->Lmd.spawnedEnts[0]);i++){
		if(player->client->Lmd.spawnedEnts[i] == NULL){
			item->flags |= FL_PLAYERSPAWNED;
			player->client->Lmd.spawnedEnts[i] = item;
			return qtrue;
		}
	}
	return qfalse;
}

void PlayerItem_Scan(gentity_t *player){
	//Ufo:
	if (!player || !player->client || player->client->pers.connected != CON_CONNECTED)
		return;
	unsigned int i;
	for(i = 0;i<sizeof(player->client->Lmd.spawnedEnts) / sizeof(player->client->Lmd.spawnedEnts[0]);i++){
		if(player->client->Lmd.spawnedEnts[i] && !(player->client->Lmd.spawnedEnts[i]->inuse &&
			player->client->Lmd.spawnedEnts[i]->flags & FL_PLAYERSPAWNED)){
				player->client->Lmd.spawnedEnts[i] = NULL;
		}
		
	}
}

void hurl (gentity_t *ent, gentity_t *dropped){
	if (!ent || !dropped || !ent->client || !dropped->inuse) {
		return;
	}
	vec3_t dir, origin;
	AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(ent->client->ps.origin, ent->r.maxs[0] + 30, dir, origin);
	G_SetOrigin(dropped, origin); 
	VectorMA(ent->client->ps.velocity, 30, dir, dropped->s.pos.trDelta);

	dropped->s.pos.trDelta[2] += 50;
	dropped->s.apos.trBase[YAW] = ent->client->ps.viewangles[YAW];
	dropped->s.angles[YAW] = ent->client->ps.viewangles[YAW];

	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	dropped->physicsObject = qtrue;
	dropped->bounceCount = 8;
	//RoboPhred: 500 isnt nearly enough time to get out of the way
	dropped->genericValue10 = level.time + 1500;
	//dropped->genericValue10 = level.time + 500;
	dropped->genericValue11 = ent->s.number;
	dropped->flags = FL_DROPPED_ITEM|FL_BOUNCE;
	//dropped->clipmask = MASK_PLAYERSOLID;
	//dropped->r.contents = CONTENTS_SOLID;
	//dropped->r.ownerNum = ent->s.number;
	trap_LinkEntity(dropped);
}

gentity_t* HurlItem (gentity_t *ent, const char *name){
	gitem_t		*it;
	gentity_t		*dropped;
	it = BG_FindItem (name);
	if (!it) {
		return NULL;
	}


	dropped = G_Spawn();

	if(!dropped)
		return NULL;

	G_ModelIndex(it->world_model[0]); 
	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = it - bg_itemlist;//don't get that // store item number in modelindex

	//Com_Printf("info: Spawned model %s\n",it->world_model[0]);

	if (dropped->s.modelindex < 0)
	{
		dropped->s.modelindex = 0;
	}
	//Is this right?
	dropped->s.modelindex2 = 1; // This was non-zero as it was a dropped item

	dropped->classname = it->classname;
	dropped->item = it;

	dropped->r.contents = CONTENTS_TRIGGER|CONTENTS_SOLID;

	dropped->touch = Touch_Item;

	VectorSet(dropped->r.mins, -8, -8, -0);
	VectorSet(dropped->r.maxs, 8, 8, 16);

	// auto-remove after 300 seconds
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 300000;
	dropped->flags |= FL_DROPPED_ITEM;
	if (it->giType == IT_WEAPON || it->giType == IT_POWERUP)
	{
		dropped->s.eFlags |= EF_DROPPEDWEAPON;
	}

	if (it->giType == IT_HOLDABLE && it->giTag == HI_EWEB){
		dropped->r.mins[2] = -24;
	}

	if (it->giType == IT_WEAPON) {
		dropped->r.mins[2] = -16;

		if (it->giTag != WP_BOWCASTER
			&& it->giTag != WP_DET_PACK
			&& it->giTag != WP_THERMAL
			&& it->giTag != WP_TRIP_MINE
			)
		{
			dropped->s.angles[ROLL] = -90;
		}
	}

	dropped->physicsBounce = 0.50;		// items are bouncy
	dropped->flags |= FL_BOUNCE_HALF;

	//RoboPhred: not sure why this was disabled, but it prevents the item from being picked up imidiately upon drop
	dropped->genericValue10 = level.time + 500;
	dropped->genericValue11 = ent->s.number;

	//RegisterItem(it);
	dropped->clipmask = MASK_SOLID;
	hurl(ent, dropped);
	return dropped;
}

void Cmd_Resize_f (gentity_t *ent, int iArg){
	if (g_gametype.integer != GT_FFA) {
		return;
	}
	if (gameMode(GM_ANY)) {
		return;
	}

	if (duelInProgress(&ent->client->ps)){
		Disp(ent, "^3You cannot do this while in a duel.");
		return;
	}


	//RoboPhred
	if(ent->client->sess.spectatorState != SPECTATOR_NOT){
		Disp(ent, "^3You cannot do this while spectating.");
		return;
	}

	int was = ent->client->ps.iModelScale;
	if(ent->client->ps.iModelScale && ((ent->client->ps.iModelScale < 100) ^ (iArg  < 100))){
		if (!Auths_PlayerHasAdmin(ent)){
			return;
		}
		scaleEntity(ent, 0);
	}
	else
		scaleEntity(ent, iArg);
	if(was == ent->client->ps.iModelScale){
		return;
	}

	vec3_t dAng;
	VectorSet(dAng, -90,0,0);

	G_PlayEffectID(G_EffectIndex("scepter/invincibility"), ent->r.currentOrigin, dAng);
	G_Sound(ent,CHAN_AUTO,G_SoundIndex("sound/weapons/scepter/slam_warmup"));
	ent->client->ps.forceDodgeAnim = BOTH_FORCE_PROTECT;
	ent->client->ps.forceHandExtendTime = level.time + 3000;

	G_SetAnim(ent, SETANIM_BOTH, BOTH_FORCE_PROTECT, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
}


//void Cmd_Fnord_f (gentity_t *ent, int iArg) 
//{
//        char msg[MAX_STRING_CHARS];
//        if (fnord(msg)) {
//                G_Say (ent,NULL, SAY_ALL, msg);
//        }       
//}

/*
void Cmd_BothAnim_f (gentity_t *ent, int iArg)
{
if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered){
Cmd_ToggleSaber_f(ent);
}
ent->client->ps.saberMove = LS_NONE;
ent->client->ps.saberBlocked = 0;
ent->client->ps.saberBlocking = 0;


ent->client->ps.forceDodgeAnim = iArg;
ent->client->ps.forceHandExtendTime = level.time + BG_AnimLength(ent->localAnimIndex, (animNumber_t)iArg);
G_SetAnim(ent, NULL, SETANIM_BOTH, iArg, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS|SETANIM_FLAG_OVERRIDE, 100);
ent->client->ps.legsTimer += 300;
ent->client->ps.torsoTimer += 300;
}

void Cmd_TorsoAnim_f (gentity_t *ent, int iArg)
{
if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered){
Cmd_ToggleSaber_f(ent);
}
ent->client->ps.saberMove = LS_NONE;
ent->client->ps.saberBlocked = 0;
ent->client->ps.saberBlocking = 0;

G_SetAnim(ent, NULL, SETANIM_TORSO, iArg, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
}
*/

void
listEmotes (gentity_t *ent) 
{
	int i = 0;
	char dstr[MAX_STRING_CHARS] = "";

	while (emotes[i].name) {
		//trap_SendServerCommand(ent-g_entities,va("print \"%-15s\"",
		//                       emotes[i++].name));
		Q_strcat(dstr, sizeof(dstr), va("%-15s",emotes[i++].name));
		if (i%4 == 0) {
			Q_strcat(dstr, sizeof(dstr),"\n");
			//trap_SendServerCommand(ent-g_entities,"print \"\n\"");
		}
	}
	//trap_SendServerCommand(ent-g_entities,"print \"\n\"");
	Disp(ent, dstr);
}

emote_t* emoteAnim (int anim);
qboolean endAnim (int anim);

/*
emote_t noEmote = { NULL, -1, -1, -1};

emote_t*
holdAnim (int anim) 
{
int i;
for (i = 0; emotes[i].name && emotes[i].animStart != anim; i++);
if (emotes[i].name 
&& (emotes[i].setanim & HOLD)) {
return &emotes[i];
} else {
return &noEmote;
}
}
*/
void Cmd_emote_f (gentity_t *ent, int iArg) {
	if (ent->s.m_iVehicleNum){
		return;
	}

	if (trap_Argc() < 2) {
		listEmotes(ent);
		return;
	}

	if (ent->client->ps.weaponTime > 0 || ent->client->ps.forceHandExtend != HANDEXTEND_NONE ||
		ent->client->ps.groundEntityNum == ENTITYNUM_NONE || ent->health < 1) {
			return;
	}

	char arg [MAX_TOKEN_CHARS];

	trap_Argv(1, arg, sizeof(arg));

	int i = 0;
	while (emotes[i].name && Q_stricmp(emotes[i].name, arg)) {
		i++;
	}
	if (!emotes[i].name) {
		listEmotes(ent);
		return;
	}

	if (emotes[i].animStart == TORSO_SURRENDER_START) {
		//Drop saber if surrender
		if (ent->client->ps.weapon == WP_SABER &&
			ent->client->ps.saberEntityNum &&
			!ent->client->ps.saberInFlight)
		{
			saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
		}

	}

	if (ent->client->ps.weapon == WP_SABER && 
		!((ent->client->ps.saberHolstered != 0) 
		^ (emotes[i].animStart == BOTH_TAVION_SWORDPOWER))){
			Cmd_ToggleSaber_f(ent);
	}

	ent->client->ps.saberMove = LS_NONE;

	ent->client->ps.saberBlocked = 0;
	ent->client->ps.saberBlocking = 0;

	G_SetAnim(ent, emotes[i].setanim, emotes[i].animStart, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);

	if ((SETANIM_TORSO & emotes[i].setanim) 
		&& ent->client->ps.torsoTimer < 500) {
			ent->client->ps.torsoTimer = 500;
	}

	if ((SETANIM_LEGS & emotes[i].setanim) 
		&& ent->client->ps.legsTimer < 500) {
			ent->client->ps.legsTimer = 500;
	}

}

void Cmd_Ionlyduel_f (gentity_t *ent, int iArg) 
{
	if (ent->s.m_iVehicleNum){
		return;
	}

	if (ent->client->ps.weaponTime > 0 
		|| ent->client->ps.weapon != WP_SABER
		|| ent->client->ps.m_iVehicleNum
		|| ent->client->pers.Lmd.persistantFlags & SPF_IONLYDUEL
		|| duelInProgress(&ent->client->ps)
		|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
		|| BG_HasYsalamiri(g_gametype.integer, &ent->client->ps)){
			return;
	}
	if(ent->client->Lmd.moneyStash){
		return;
	}

	if(ent->client->ps.saberHolstered != 2) {
		Cmd_ToggleSaber_f(ent);                                
	}
	//No weapons
	//ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE);
	//ent->client->ps.weapon = WP_MELEE;
	ent->client->pers.Lmd.persistantFlags |= SPF_IONLYDUEL;
	Disp (ent, "IONLYDUEL activated (it can be deactivated by engaging in a duel)\n");
}

qboolean ThereIsAKing (void);
gentity_t* GetKing (void);
void Cmd_King_f (gentity_t *ent, int iArg)
{
	if (g_gametype.integer == GT_JEDIMASTER || !(g_privateDuel.integer & PD_KING)) {
		Disp( ent, "King mode is off.");
		return;
	}
	gentity_t *kingent = GetKing();
	if (!ThereIsAKing() || !kingent || !kingent->client) {
		Disp( ent, "There is no King.");
	} else {
		Disp( ent, va("%s is the King, and has won %i duels.", kingent->client->pers.netname, kingent->client->pers.Lmd.kingScore));
	}
	if (g_bestKing && g_bestKing->client) {
			Disp( ent, va("%s is the best King with %i duels won.", g_bestKing->client->pers.netname, g_bestKingScore));
	}
}

void Cmd_Say2_f (gentity_t *ent, int iArg)
{
	Cmd_Say_f (ent, iArg, qfalse);
}

void Cmd_DropStuff_f (gentity_t *ent, int iArg) {
	if(gameMode(GM_INSTGIB) || gameMode(GM_INSTDIS) || gameMode(GM_ALLWEAPONS)){
		return;
	}


	//RoboPhred
	if(ent->client->sess.spectatorState != SPECTATOR_NOT){
		Disp(ent, "^3You cannot do this while spectating.");
		return;
	}

	gitem_t *item;
	holdable_t hi;
	vec3_t dir;
	switch (iArg) {
		case 1:
			hi = bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
			if (!hi || hi == HI_JETPACK){
				return;
			}
			if (hi == HI_CLOAK) {
				Jedi_Decloak(ent);
			}

			item = BG_FindItemForHoldable( hi );
			if(!item) {
				return;
			}
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS]
			&= ~(1 << hi);
			HurlItem(ent, item->classname);
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			BG_CycleInven(&ent->client->ps, 1);
			break;
		case 2:
			AngleVectors(ent->client->ps.viewangles, dir, 0, 0);
			dir[2] = 1;
			VectorNormalize(dir);
			TossClientWeapon(ent, dir, 300);
			break;
		case 3:
			if (gameMode(GMF_WITH_JP)) {
				return;
			}
			if (PlayerAcc_Prof_GetProfession(ent) == PROF_BOT) {
				return;
			}

			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] 
			& (1 << HI_JETPACK)) {
				//Turn jetpack off first.
				ent->client->jetPackOn = qfalse;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_JETPACK);
				HurlItem(ent, "item_jetpack");
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				BG_CycleInven(&ent->client->ps, 1);
			}
			break;
		default:
			break;
	}
}

void Cmd_SiegeClass_f (gentity_t *ent, int iArg);

void Cmd_RageQuit_f(gentity_t *ent, int iArg) {
	trap_SendServerCommand( -1, va("chat \"%s^7\x19: ^1RAAAGE!!!\"", ent->client->pers.netname));	
	trap_DropClient(ent->s.number, "is Pissed Off! RAGEQUIT!");
}

char *getChatModeName(int mode);
qboolean canUseChatMode(gentity_t *ent, int mode);
qboolean PrivateChat_PlayerCanHear(gentity_t *ent, int player);
//qboolean isBuddy(gentity_t *ent, gentity_t *other);
void Cmd_ChatMode_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int set;
	int argc = trap_Argc();
	if(argc < 2){
		char *str;
		Disp(ent, "^3Usage: ^2/chatmode <say | team> [chat mode]\n"
			"^3Chat mode can be:");
		set = 0;
		while((str = getChatModeName(set))) {
			if(!canUseChatMode(ent, set++))
				continue;
			Disp(ent, va("^5%s", str));
		}
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	int len = strlen(arg);
	if(Q_stricmpn(arg, "say", len) == 0)
		set = 0;
	else if(Q_stricmpn(arg, "team", len) == 0)
		set = 1;
	else{
		Disp(ent, "^3The first arg must be \'^2team^3\' or \'^2say^3\'.");
		return;
	}
	if(argc >= 3){
		unsigned int i;
		trap_Argv(2, arg, sizeof(arg));
		len = strlen(arg);
		char *name;
		for(i = 0; (name = getChatModeName(i)) != NULL; i++){
			if(i == SAY_ADMINS && !Auths_PlayerHasAuthFlag(ent, AUTH_ADMINCHAT))
				continue;
			if(Q_stricmpn(arg, name, len) == 0){
				ent->client->pers.Lmd.chatMode[set] = i;
				Disp(ent, va("^3Chat mode set to: ^2%s", name));
				return;
			}
		}
		Disp(ent, "^3Invalid chat mode.");
	}
	else{
		ent->client->pers.Lmd.chatMode[set]++;
		while(!canUseChatMode(ent, ent->client->pers.Lmd.chatMode[set])) {
			ent->client->pers.Lmd.chatMode[set]++;
			if(!getChatModeName(ent->client->pers.Lmd.chatMode[set])) {
				ent->client->pers.Lmd.chatMode[set] = 0;
				break;
			}
		}
		Disp(ent, va("^3Chat mode for ^2%s^3 is ^2%s", set ? "say" : "team", getChatModeName(ent->client->pers.Lmd.chatMode[set])));
	}
}

extern char* enterMotd;
void Cmd_dispMotd_f(gentity_t *ent, int iArg){
	Disp(ent, enterMotd);
}

int StashHolder(void);
void Cmd_Stash_f(gentity_t *ent, int iArg){
	int check = StashHolder();
	if(check == -3)
		Disp(ent, "^3The stash is doing highly improbable things...");
	else if(check == -2)
		Disp(ent, "^3There is no money stash spawned");
	else if(check == -1)
		Disp(ent, "^3There is currently a money stash spawned");
	else{
		Disp(ent, va("%s^7 ^3is holding onto the money stash", g_entities[check].client->pers.netname));
	}
}

void dropMoneyStash(gentity_t *ent);
void Cmd_DropStash_f(gentity_t *ent, int iArg){
	if(!ent->client->Lmd.moneyStash)
		Disp(ent, "^3You do not have a stash.");
	else{
		Disp(ent, "^3Stash dropped.");
		dropMoneyStash(ent);
	}
}

void Cmd_Examine_f (gentity_t *ent, int iArg)
{
	gentity_t *targ = AimAnyTarget(ent, 64);
	char msg[MAX_STRING_CHARS] = "";
	if(ent->client->sess.spectatorState != SPECTATOR_NOT)
	{
		Disp(ent, "^3You cannot use this command while spectating.");
		return;
	}
	if(!targ || !targ->inuse)
	{
		Disp(ent, "^3Nothing to examine.");
		return;
	}
	if(targ->client && targ->s.number < MAX_CLIENTS)
	{
		Q_strcat(msg, sizeof(msg), va("^3Player\n^3Name: ^7%s", targ->client->pers.netname));
	}
	else
	{
		if(targ->examineStr)
			Q_strcat(msg, sizeof(msg), va("^3%s\n", targ->examineStr));
		if(targ->examine)
		{
			targ->examine(targ, ent);
		}
		if(targ->r.svFlags & SVF_PLAYER_USABLE)
			Q_strcat(msg, sizeof(msg), "^3Entity is: ^2Usable\n");
		if(targ->interact)
			Q_strcat(msg, sizeof(msg), "^3Entity is: ^6Interactable\n");
		if(targ->flags & FL_PAY || targ->pay)
			Q_strcat(msg, sizeof(msg), "^3Entity is: ^5Payable\n");
	}
	if(msg[0])
	{
		msg[strlen(msg) - 1] = 0; //remove ending linefeed.
		Disp(ent, msg);
	}
}

void Cmd_Factions_f(gentity_t *ent, int iArg);

void Cmd_Action_f(gentity_t *ent, int iArg);

void Cmd_Friends_f(gentity_t *ent, int iArg);

void Cmd_None_f (gentity_t *ent, int iArg)
{

}

/*
Cmd disable
1   registerAccounts
2   Special duels
4   drinkme eatme
8   dropstuff
16  fnord
32  emotes
64  ionlyduel
128 Money
256 Professions
//RoboPhred
512: /stash
*/
//static ???

void Cmd_Confirm_f(gentity_t *ent, int iArg);
void Cmd_Interact_f(gentity_t *ent, int iArg);

cmdEntry_t playerCommandEntries[] = {
	//{"testline", "\n", Cmd_TestLine_f, 0, 1, 0, 0, 0},
	{"actions", "List and use your current pending actions.", Cmd_Action_f, 0, qfalse, 0, 0, 0, 0},
	{"admins", "List currently logged in admins and their level.", Cmd_AdminInfo_f, 0, qfalse, 0, 0, 0, 0},
	{"buddy", "Make the player your buddy.", Cmd_BuddyClient_f, 0, qfalse, 0, 0, 0, 0},
	{"challenge", "Challenge someone to a 'special' duel. For example '\\challenge power' will challenge someone to a duel where both players have unlimited force power.", Cmd_Challenge_f, 0, qfalse, 0, 2, ~(1 << GT_FFA), 0},
	{"chatmode", "Switches your team chat mode.  If no mode is set, the next mode in the sequence is selected.", Cmd_ChatMode_f, 0, qfalse, 0, 0, 0, 0},
	{"class", "Pick your Battle Ground class. With no argument it will display how many of each class there are on your team.", Cmd_SiegeClass_f, 0, qfalse, 0, 0, ~(1 << GT_BATTLE_GROUND), 0},
	{"confirm", "Confirm an action, or toggle the confirmation requirement.", Cmd_Confirm_f, 0, 0, 0, 0, 0},
	//{"defender", "\nPlace a defender.", Cmd_Defender_f, 0, 1, 0, 0, 0},
	{"drinkme", "`What a curious feeling!' said Alice; `I must be shutting up like a telescope.'\n^1WARNING!^7 Consumed upon use.", Cmd_Resize_f, 75, qfalse, 0, 4, ~(1 << GT_FFA), 0},
	{"drophi", "Drop the holdable item.", Cmd_DropStuff_f, 1, qfalse, 0, 8, ~(1 << GT_FFA), 0},
	{"dropjp", "Drop your jetpack.", Cmd_DropStuff_f, 3, qfalse, 0, 8, ~(1 << GT_FFA), 0},
	{"dropstash", "If you are holding it, drop the money stash.", Cmd_DropStash_f, 0, qfalse, 0, 128, ~(1 << GT_FFA), 0},
	{"dropwp", "Drop your weapon.", Cmd_DropStuff_f, 2, qfalse, 0, 8, ~(1 << GT_FFA), 0},
	{"eatme", "Let's see how deep the rabbit hole goes.\n^1WARNING!^7 Consumed upon use." , Cmd_Resize_f, 135, qfalse, 0, 4, ~(1 << GT_FFA), 0}, 
	{"emote", "Do an emote. If no argument is provided all available emotes will be listed." , Cmd_emote_f, 0, qfalse, 0, 32, 0, 0}, 
	{"examine", "Examine the object in front of you.", Cmd_Examine_f, 0, qfalse, 0, 0, 0, 0},
	{"factions", "View and interact with player factions." , Cmd_Factions_f, 0, qfalse, 0, 0, 0, 0}, 
	//{"fnord","\nIf you don't have anything to say, but you want to say it anyway, this is the command for you.\nThe Fnorder Program was originally written by Steve Jackson and Creede Lambard.\nIt is used in the Lugormod with permission from Steve Jackson.", Cmd_Fnord_f, 0, 0, 16, 0, 0},
	{"friends", "Players added to your friends list can send you messages and hear your friend chat.", Cmd_Friends_f, 0, qfalse, 0, 0, 0, 0},
	{"hilevel","Display top ten players with the highest level.", HiScore, 3, qfalse, 0, 1, 0, 0},
	{"hikills","Display top ten players with the most kills.", HiScore, 4, qfalse, 0, 1, 0, 0},
	{"hiscore","Display top ten players with the highest score.", HiScore, 0, qfalse, 0, 1, 0, 0},
	{"histashes","Display top ten players with the most stashes.", HiScore, 5, qfalse, 0, 1, 0, 0},
	{"hitime","Display top ten players with most time on the server.", HiScore, 1, qfalse, 0, 1, 0, 0},
	{"ignore", "Ignore messages from the player.  Set player to -1 to ignore/unignore all.", Cmd_IgnoreClient_f, 0, qfalse, 0, 0, 0, 0},
	{"interact", "Use this to interact with certain terminals.", Cmd_Interact_f, 0, qfalse, 0, 0, 0, 0},
	{"ionlyduel","You will be (almost) invulnerable until you engage a duel, but you can't use offensive force powers or hurt anyone.", Cmd_Ionlyduel_f, 0, qfalse, 0, 64, ~(1 << GT_FFA), 0},
	{"king","Get info on the King. You become King by defeating the King in a duel.", Cmd_King_f, 0, qfalse, 0, 0, ~(1 << GT_FFA), 0},
	{"motd", "Display the server motd in the console", Cmd_dispMotd_f, 0, qfalse, 0, 0, 0, 0},
	{"ragequit", "For emergency use only.", Cmd_RageQuit_f, 0, qfalse, 0, 0, 0, 0},
	{"say_buddies", "Send a message to your buddies.", Cmd_Say2_f, SAY_BUDDIES, qfalse, 0, 0, 0, 0},
	{"say_close", "Send a message to those standing close to where you are.", Cmd_Say2_f, SAY_CLOSE, qfalse, 0, 0, 0, 0},
	//{"scanner", "Scans for players, items, and the money stash.", Cmd_TechScanner_f, 0, 0, 256, 0, PROF_TECH},
	{"stash", "Tells you if there is a money stash spawned, and who is holding on to it (if anyone).", Cmd_Stash_f, 0, qfalse, 0, 512 | 128, 0, 0},
	{NULL},
};
