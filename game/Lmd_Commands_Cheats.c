
#include "g_local.h"

#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Console.h"
#include "Lmd_IPs.h"

gentity_t* AimAnyTarget (gentity_t *ent, int length);
gentity_t *ClientFromArg (gentity_t *to, int argNum);


extern stringID_table_t animTable [MAX_ANIMATIONS+1];
void Cmd_Anims_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int argc = trap_Argc();
	if(argc == 1){
		Disp(ent, "^3Usage: anims {^2list ^6[starting index]^3} {^2play ^5<animation name or index>^3} {^2reset^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp("list", arg) == 0){
		//print the first 25 animations from the index
		int start, anim;
		trap_Argv(2, arg, sizeof(arg));
		start = atoi(arg);
		memset(arg, 0, sizeof(arg));
		//Ufo:
		if (start < 0)
			start = 0;
		else if(start + 25 > MAX_ANIMATIONS)
			start = MAX_ANIMATIONS - 25;
		for(anim = start;anim<(start + 25);anim++){
			Q_strcat(arg, sizeof(arg), va("^2%i ^3%s\n", anim, animTable[anim].name));
		}
		//trap_SendServerCommand(ent->s.number, va("print \"%s\n\"", arg2));
		Disp(ent, "^3---------------------\n");
		Disp(ent, arg);
		Disp(ent, "^3---------------------\n");
		return;
	}
	else if(Q_stricmp("play", arg) == 0){
		int anim;
		if (ent->client->ps.weaponTime > 0 || ent->client->ps.forceHandExtend != HANDEXTEND_NONE ||
			ent->client->ps.groundEntityNum == ENTITYNUM_NONE || ent->health < 1) {
				Disp(ent, "^3Cannot do this while busy.");
				return;
		}
		if(argc < 3){
			Disp(ent, "^3You must specify the animation name or index to play.");
			return;
		}
		trap_Argv(2, arg, sizeof(arg));
		anim = atoi(arg);
		if(anim == 0 && arg[0] != '0' && arg[1] != 0) {
			anim = GetIDForString(animTable, arg);
		}
		if(anim < 0 || anim > MAX_ANIMATIONS){
			Disp(ent, "^3Invalid value, you must specify the animation index to play.");
			return;
		}
		ent->client->ps.saberMove = LS_NONE;
		ent->client->ps.saberBlocked = 0;
		ent->client->ps.saberBlocking = 0;
		G_SetAnim(ent, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
		ent->client->ps.torsoTimer = Q3_INFINITE;
		ent->client->ps.legsTimer = Q3_INFINITE;
		Disp(ent, "^2Animation set.");
	}
	else if(Q_stricmp("reset", arg) == 0){
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		Disp(ent, "^3Your animation has been reset.");
	}
	else
		Disp(ent, "^3Unknown argument.  Use the command without args to get its usage.");
}

void BecomeCommoner (gentity_t *ent);
void BecomeKing (gentity_t *ent);
qboolean ThereIsAKing (void);
gentity_t *GetKing (void);
void Cmd_BecomeKing_f (gentity_t *ent, int iArg){
	if (!(g_privateDuel.integer & PD_KING)){
		Disp(ent, "^3The king duel activity is not enabled.  To enable it, use flag 4096 in the cvar g_privateDuel");
		return;
	}
	if(ThereIsAKing())
		BecomeCommoner(GetKing());
	//Ufo:
	gentity_t *targ = trap_Argc() > 1 ? ClientFromArg(ent, 1) : ent;
	BecomeKing(targ);

}

qboolean wpDisp = qfalse;
int wpClient = 0;

void Cmd_BotWPRender_f (gentity_t *ent, int iArg) {
	//NOTE: only works for one client, should make this a mask.
	if(wpDisp){
		Disp(ent, "Rendering OFF");
		wpClient = 0;
	}
	else {
		Disp(ent, "Rendering ON");
		wpClient = ent->s.number;
	}
	wpDisp = !wpDisp;
}


void Cmd_Dragon_f (gentity_t *ent, int iArg) {
	vec3_t mouth,up;
	gentity_t *targEnt;
	//VectorCopy(ent->client->renderInfo.eyePoint, mouth);
	VectorCopy(ent->client->ps.origin, mouth);
	AngleVectors(ent->client->ps.viewangles, NULL,NULL,up);
	//Needed??
	//VectorNormalize(up);

	//mouth[0] -= 4 * up[0];
	//mouth[1] -= 4 * up[1];
	//mouth[2] -= 4 * up[2];
	mouth[2] += ent->client->ps.viewheight;
	if (ent->client->ps.iModelScale) {
		mouth[2] -= 4 * ent->modelScale[2];
	} else {
		mouth[2] -= 4;
	}
	G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("sound/chars/rancor/rancor_roar_02"));
	G_PlayEffectID(G_EffectIndex("env/flame_jet"),mouth, ent->client->ps.viewangles);
	targEnt = AimAnyTarget(ent, 80);
	if (targEnt && targEnt->client) {
		vec3_t dAng;
		VectorSet(dAng, -90,0,0);
		G_PlayEffectID(G_EffectIndex("env/fire_wall"),targEnt->r.currentOrigin, dAng);                       
		G_Damage( targEnt, ent, ent, ent->client->ps.viewangles, NULL, 20, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_LAVA );
	}
}

gentity_t* HurlItem (gentity_t *ent, const char *name);
void Cmd_Drop_f (gentity_t *ent, int iArg){
	char		name[MAX_TOKEN_CHARS];

	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR){
		Disp(ent, "^3You cannot drop items while spectating");
		return;
	}
	if (trap_Argc() < 2) {
		Disp(ent, "^3Usage: drop ^2<item name>");
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );
	HurlItem(ent,name);
}

void Cmd_DataFlag_f (gentity_t *ent, int iArg) {

	gentity_t *tEnt;

	int flag;
	char *cmd;

	if (trap_Argc() > 1) {
		tEnt = ClientFromArg(ent, 1);
		if (!tEnt || Auths_Inferior(ent, tEnt)) {
			return;
		}
	} else {
		tEnt = ent;
	}

	if(iArg == 0) {
		flag = FL_GODMODE;
		cmd = "^3God mode";
	}
	else if (iArg == 1) {
		flag = FL_UNDYING;
		cmd = "^6Undying";
	}
	else
		return;

	if (tEnt->flags & flag) {
		Disp(ent, va("%s: ^1OFF", cmd));
	} else {
		Disp(ent, va("%s: ^2ON", cmd));
	}
	tEnt->flags ^= flag;
}

void Cmd_Gravity_f (gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	int grav = 0;
	trap_Argv(2, val, sizeof(val));
	grav = atoi(val);
	trap_Argv(1, arg, sizeof(arg));
	if(arg[0] == 0) {
		Disp(ent, CT_B"Usage: "CT_C"/gravity "CT_AR"<player> "CT_AO"[gravity]\n"CT_B"Use a player index of "CT_V"-1"CT_B" for the map gravity.");
	}
	else if(atoi(arg) == -1) {
		if(val[0] == 0) {
			Disp(ent, va(CT_B"Global gravity: "CT_V"%i", g_gravity.integer));
		}
		else {
			trap_Cvar_Set("g_gravity", va("%i", grav));
			Disp(ent, va(CT_B"Global gravity set to "CT_V"%i^3.", grav));
		}
	}
	else {
		gentity_t *targ = ClientFromArg(ent, 1);
		if(!targ)
			return;
		if(val[0] == 0) {
			Disp(ent, va(CT_B"Player gravity: "CT_V"%i", targ->client->Lmd.customGravity));
		}
		else {
			targ->client->Lmd.customGravity.value = grav;
			if(grav != 0)
				targ->client->Lmd.customGravity.time = level.time + Q3_INFINITE; //Ufo: Q3_INFINITE is insufficient for leveltime dependant stuff
			else
				targ->client->Lmd.customGravity.time = 0;
			Disp(ent, va(CT_B"Player gravity set to "CT_V"%i"CT_B".", grav));
		}
	}
}

void Cmd_Invisible_f (gentity_t *ent, int iArg) {
	gentity_t *tent = ClientFromArg(ent,1);
	if(!tent || !tent->client){
		tent = ent;
	}

	if (tent->client->ps.eFlags & EF_NODRAW || tent->s.eFlags & EF_NODRAW){
		trap_SendServerCommand(ent->s.number, "cp \"^5Invisibility: ^3OFF\"");
		tent->r.svFlags &= ~SVF_NOCLIENT;
		tent->s.eFlags &= ~EF_NODRAW;
		tent->client->ps.eFlags &= ~EF_NODRAW;
	}
	else{
		trap_SendServerCommand(ent->s.number, "cp \"^5Invisibility: ^2ON\"");
		tent->r.svFlags |= SVF_NOCLIENT;
		tent->s.eFlags |= EF_NODRAW;
		tent->client->ps.eFlags |= EF_NODRAW;
	}
}

void Cmd_Kill_f (gentity_t *ent );
void DismembermentTest (gentity_t *self);
void Cmd_KillOther_f (gentity_t *ent, int iArg){
	gentity_t *kEnt;

	if (trap_Argc() > 1)
	{
		trace_t tr;
		vec3_t uPos,dAng, sky;
		int length = 8192;
		if (!(kEnt = ClientFromArg(ent,1))) {
			return;
		}

		uPos[0] = kEnt->client->ps.origin[0];
		uPos[1] = kEnt->client->ps.origin[1];
		uPos[2] = kEnt->client->ps.origin[2] + length;
		VectorSet(dAng, 90,0,0);

		trap_Trace(&tr,kEnt->client->ps.origin, NULL, NULL, uPos, kEnt->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_TERRAIN);
		sky[0] = tr.endpos[0];
		sky[1] = tr.endpos[1];
		sky[2] = tr.endpos[2] - 5;

		G_PlayEffectID(G_EffectIndex("env/huge_lightning"), sky, dAng);
		G_PlayEffectID(G_EffectIndex("env/quake_small"), kEnt->client->ps.origin, dAng);
		G_EntitySound( kEnt, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));

	} else {
		if (!ent) {
			return;
		}

		trace_t tr;
		vec3_t fPos,maxs,mins, hand;
		int length = 8192;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);
		VectorSet( mins, -8, -8, -8 );
		VectorSet( maxs, 8, 8, 8 );

		fPos[0] = ent->client->renderInfo.eyePoint[0] + fPos[0]*length;
		fPos[1] = ent->client->renderInfo.eyePoint[1] + fPos[1]*length;
		fPos[2] = ent->client->renderInfo.eyePoint[2] + fPos[2]*length;

		trap_Trace(&tr,ent->client->renderInfo.eyePoint, mins, maxs, fPos, ent->s.number, ent->clipmask);

		kEnt = &g_entities[tr.entityNum];

		VectorCopy(ent->client->ps.origin, hand);

		hand[2] += ent->client->ps.viewheight;

		G_PlayEffectID(G_EffectIndex("env/electricity"), hand, ent->client->ps.viewangles);
		G_PlayEffectID(G_EffectIndex("force/lightning"), hand, ent->client->ps.viewangles);
		G_PlayEffectID(G_EffectIndex("env/small_explode"), tr.endpos, ent->client->ps.viewangles);
		G_PlayEffectID(G_EffectIndex("env/quake_small"), tr.endpos, ent->client->ps.viewangles);
		G_EntitySound( ent, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1") );
		G_SoundAtLoc( tr.endpos, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));
	}

	if (kEnt && kEnt->inuse && kEnt->client)
	{
		if (kEnt->NPC) {
			kEnt->health = 0;
			kEnt->client->ps.stats[STAT_HEALTH] = 0;
			if (kEnt->die) {
				kEnt->die(kEnt, kEnt, kEnt, kEnt->client->pers.maxHealth, MOD_UNKNOWN);
			}        
		}
		else {
			if (Auths_Inferior(ent, kEnt)){
				return;
			}

			Cmd_Kill_f (kEnt);
			if (kEnt->health < 1)
			{
				DismembermentTest(kEnt);
			}
		}
	}
}

void Cmd_Noclip_f( gentity_t *ent, int iArg ) {
	char	*msg;
	gentity_t *tEnt;

	if (trap_Argc() > 1) {
		tEnt = ClientFromArg(ent, 1);
		if (!tEnt || Auths_Inferior(ent, tEnt)) {
			return;
		}
	} else {
		tEnt = ent;
	}

	if ( tEnt->client->noclip ) {
		msg = "^3Noclip: ^1OFF";
	} else {
		msg = "^3Noclip: ^2ON";
	}
	tEnt->client->noclip = !tEnt->client->noclip;

	Disp(ent, msg);
}

void Cmd_PlayFX_f (gentity_t *ent, int iArg) {
	if (trap_Argc() < 2)
		return;
	G_PlayEffectID(G_EffectIndex(ConcatArgs(1)), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
}

void Cmd_PlayMusic_f (gentity_t *ent, int iArg) {
	if (trap_Argc() < 2)
		return;
	trap_SetConfigstring( CS_MUSIC, ConcatArgs(1) );
}

void Cmd_PlaySnd_f (gentity_t *ent, int iArg) {
	if (trap_Argc() < 2)
		return;
	G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(ConcatArgs(1)));
}

void Cmd_Powerup_f (gentity_t *ent, int iArg) {
	if (trap_Argc() < 2)
		return;
	char arg[MAX_TOKEN_CHARS];
	int time;
	trap_Argv(2, arg, sizeof(arg));
	time = atoi(arg);
	if(time <= 0)
		time = 30;
	trap_Argv(1, arg, sizeof(arg));
	int  pwr = atoi(arg);

	if (pwr < 1 || pwr >= PW_NUM_POWERUPS)
		return;

	ent->client->ps.powerups[pwr] = level.time + (time * 1000);
}

void scaleEntity(gentity_t *scaleEnt, int scale);
void Cmd_Scale_f (gentity_t *ent, int iArg){
	gentity_t *scaleEnt;
	int n = 0;
	char arg [MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		if (ent) {
			trap_SendServerCommand(ent->s.number, va("print \"%4i\n\"", ent->client->ps.iModelScale));
		}
		return;
	}
	if (trap_Argc() < 3 && ent) {
		scaleEnt = ent;
	} else {
		scaleEnt = ClientFromArg(ent, 1);
		n = 1;
		if(!scaleEnt)
			return;
		if (trap_Argc() < 3) {
			Disp(ent, va("Scale: %4i maxs: %s mins: %s standheight: %i viewheight: %i\n", 
				scaleEnt->client->ps.iModelScale,
				vtos(scaleEnt->r.maxs),
				vtos(scaleEnt->r.mins),
				scaleEnt->client->ps.standheight,
				scaleEnt->client->ps.viewheight));
			return;
		}

	}

	if (!scaleEnt->inuse || Auths_Inferior(ent, scaleEnt)) {
		return;
	}
	trap_Argv(1 + n, arg, sizeof(arg));
	scaleEntity(scaleEnt, atoi(arg));
}

void Cmd_Shield_f (gentity_t *ent, int iArg) {

	gentity_t *tEnt;

	if (trap_Argc() > 1) {
		tEnt = ClientFromArg(ent, 1);
		if (!tEnt || Auths_Inferior(ent, tEnt)) {
			return;
		}
	} else {
		tEnt = ent;
	}
	if (tEnt->flags & FL_SHIELDED) {
		tEnt->client->ps.powerups[PW_SHIELDHIT] = 0;
		Disp(ent, "^4Shield: ^1OFF");
	} else {
		tEnt->client->ps.powerups[PW_SHIELDHIT] = level.time + Q3_INFINITE; //Ufo: Q3_INFINITE is insufficient for leveltime dependant stuff
		Disp(ent, "^4Shield: ^2ON");
	}

	tEnt->flags ^= FL_SHIELDED;
}

void Cmd_ShowHealth_f (gentity_t *ent, int iArg) {
	gentity_t *hEnt = NULL;
	if(trap_Argc() < 2)
		hEnt = AimAnyTarget(ent, 4096);
	else {
		hEnt = ClientFromArg(ent, 1);
		if(!hEnt)
			return;
	}
	if (!hEnt || !hEnt->client || hEnt->client->sess.sessionTeam == TEAM_SPECTATOR)
		return;
	trap_SendServerCommand(ent->s.number,va("cp \"^1%i^7/^2%i\"", hEnt->client->ps.stats[STAT_HEALTH], hEnt->client->ps.stats[STAT_ARMOR]));
}

void Cmd_Speed_f (gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	int speed = 0;
	trap_Argv(2, val, sizeof(val));
	speed = atoi(val);
	if(speed < 0)
		speed = 0;
	trap_Argv(1, arg, sizeof(arg));
	if(arg[0] == 0) {
		Disp(ent, CT_B"Usage: "CT_C"/speed "CT_AR"<player> "CT_AO"[speed]\n"CT_B"Use a player index of "CT_V"-1"CT_B" for the map speed.");
	}
	else if(atoi(arg) == -1) {
		if(val[0] == 0) {
			Disp(ent, va(CT_B"Global speed: "CT_V"%i", g_speed.integer));
		}
		else {
			trap_Cvar_Set("g_speed", va("%i", speed));
			Disp(ent, va(CT_B"Global speed set to "CT_V"%i^3.", speed));
		}
	}
	else {
		gentity_t *targ = ClientFromArg(ent, 1);
		if(!targ)
			return;
		if(val[0] == 0) {
			Disp(ent, va(CT_B"Player speed: "CT_V"%i", targ->client->Lmd.customSpeed));
		}
		else {
			targ->client->Lmd.customSpeed.value = speed;
			if(speed > 0)
				targ->client->Lmd.customSpeed.time = level.time + Q3_INFINITE; //Ufo: Q3_INFINITE is insufficient for leveltime dependant stuff
			else
				targ->client->Lmd.customSpeed.time = 0;
			Disp(ent, va(CT_B"Player speed set to "CT_V"%i"CT_B".", speed));
		}
	}
}

//Ufo:
void Cmd_HidePing_f (gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	trap_Argv(1, arg, sizeof(arg));
	int newPing = atoi(arg);
	if (trap_Argc() == 1 || newPing < 1 || newPing == ent->client->pers.Lmd.fakePing)
	{
		ent->client->pers.Lmd.fakePing = 0;
		Disp(ent, CT_B"Hideping: ^1OFF\n"CT_B"Specify a positive value as an argument.");
	}
	else
	{
		ent->client->pers.Lmd.fakePing = newPing;
		Disp(ent, va(CT_B"Hideping: ^2ON\n"CT_B"Your ping will from now appear to be roughly "CT_V"%i"CT_B".", newPing));
	}
}

//Ufo:
void Lmd_IPs_SetPlayerIP(gclient_t *client, IP_t ip);
void Cmd_FakeIP_f (gentity_t *ent, int iArg) {
	gentity_t *targ = ClientFromArg(ent, 1);
	if(!targ)
		return;
	char *ipstr = ConcatArgs(2);
	IP_t ip;
	if(Lmd_IPs_ParseIP(ipstr, ip))
		Lmd_IPs_SetPlayerIP(targ->client, ip);
}

cmdEntry_t cheatCommandEntries[] = {
	{"anims", "Play a custom animation.", Cmd_Anims_f, 0, qtrue, 3, 0, 0},
	{"becomeking","You become the King.", Cmd_BecomeKing_f, 0, qtrue, 1, 0, 0},
	{"botwprender","Renders bot waypoints (only visible to you).", Cmd_BotWPRender_f, 0, qtrue, 1, 0, 0},
	{"dragon", "Katla.", Cmd_Dragon_f, 0, qtrue, 3, 0, 0},
	{"drop", "Spawns and hurls the given item in front of you.\nEg. \\drop item_jetpack", Cmd_Drop_f, 0, qtrue, 3, 0, ~(1 << GT_FFA)},
	{"fakeIP", "Yes, we are alike that way, blinded one.", Cmd_FakeIP_f, 0, qtrue, 1, 0, 0  },
	{"god", "Bestows invulnurability to the specified player, or yourself if no argument is provided.", Cmd_DataFlag_f, 0, qtrue, 1, 0, 0},
	{"gravity", "Set custom gravity for the map or a specific player.", Cmd_Gravity_f, 0, qtrue, 1, 0, 0},
	{"hideping", "And so you wait, as a shadow.", Cmd_HidePing_f, 0, qtrue, 1, 0, 0  },
	{"invisible","The specified player will become completly invisible.\nIf no argument is provided, you will become invisible.", Cmd_Invisible_f, 0, qtrue, 2, 0, 0},
	{"killother","Kills the specified player. If no argument is provided, the target in sight will be killed.", Cmd_KillOther_f, 0, qtrue, 3, 0, (1 << GT_SIEGE)|(1 << GT_BATTLE_GROUND)},
	{"noclip", "Bestows noclip to the specified player, or yourself if no argument is provided.", Cmd_Noclip_f, 0, qtrue, 2, 0, 0},
	{"playfx", "Play the specified effect (relative to /effects).", Cmd_PlayFX_f, 0, qtrue, 1, 0, 0},
	{"playmusic", "Play the specified music.", Cmd_PlayMusic_f, 0, qtrue, 1, 0, 0},
	{"playsnd", "Play the specified sound.", Cmd_PlaySnd_f, 0, qtrue, 1, 0, 0},
	{"powerup", "Enable a power for 30 s.", Cmd_Powerup_f, 0, qtrue, 1, 0, 0},
	{"scale", "Scale the given player to the size specified. If the player is not provided, you will be scaled.", Cmd_Scale_f, 0, qtrue, 3, 0, 0},
	{"shield", "Bestows a shield to the specified player, or yourself if no argument is provided.  The shield will protect the player from damage and reflect projectiles.", Cmd_Shield_f, 0, qtrue, 1, 0, 0},
	{"showhealth", "Display the health and armor of the target in sight.", Cmd_ShowHealth_f, 0, qtrue, 2, 0, (1 << GT_SIEGE)|(1 << GT_BATTLE_GROUND)},
	{"speed", "Set custom speed for the map or a specific player.", Cmd_Speed_f, 0, qtrue, 1, 0, 0},
	{"undying", "Bestows invincibility to the specified player, or yourself if no argument is provided.  The player will still take damage, but never die.", Cmd_DataFlag_f, 1, qtrue, 1, 0, 0},
	{NULL},
};