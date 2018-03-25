
#ifdef LMD_NEW_FORCEPOWERS

#include "g_local.h"
#include "Lmd_Force_Core.h"
#include "Lmd_Commands_Public.h"

//to revert: define OLD_FORCEPOWERS


//Light
extern forcePower_t Force_Heal;
extern forcePower_t Force_Telepathy;
extern forcePower_t Force_Protect;
extern forcePower_t Force_Absorb;
extern forcePower_t Force_TeamHeal;

//Neutral
extern forcePower_t Force_Levitation;
extern forcePower_t Force_Speed;
extern forcePower_t Force_Push;
extern forcePower_t Force_Pull;
extern forcePower_t Force_See;

//Dark
extern forcePower_t Force_Grip;
extern forcePower_t Force_Lightning;
extern forcePower_t Force_Rage;
extern forcePower_t Force_TeamReplenish;
extern forcePower_t Force_Drain;

forcePower_t *ForcePowers[NUM_FORCE_POWERS] = {
	&Force_Heal, //FP_HEAL = 0,//instant
	&Force_Levitation, //FP_LEVITATION,//hold/duration
	&Force_Speed,//FP_SPEED,//duration
	&Force_Push, //FP_PUSH,//hold/duration
	&Force_Pull, //FP_PULL,//hold/duration
	&Force_Telepathy, //FP_TELEPATHY,//instant
	&Force_Grip, //FP_GRIP,//hold/duration
	&Force_Lightning, //FP_LIGHTNING,//hold/duration
	&Force_Rage, //FP_RAGE,//duration
	&Force_Protect, //FP_PROTECT,
	&Force_Absorb, //FP_ABSORB,
	&Force_TeamHeal,//FP_TEAM_HEAL,
	&Force_TeamReplenish, //FP_TEAM_FORCE,
	&Force_Drain, //FP_DRAIN,
	&Force_See,//FP_SEE,
	//FP_SABER_OFFENSE,
	//FP_SABER_DEFENSE,
	//FP_SABERTHROW,
};

void* Force_GetPlayerForceData(gentity_t *ent, int power) {
	//Temp
	if(!ForcePowers[power])
		return qfalse;
	//Modify this when its time to add player specific modifications.
	int size = ForcePowers[power]->data.size;
	return (byte *)(ForcePowers[power]->data.levels) + (size * (ent->client->ps.fd.forcePowerLevel[power] - 1));
}

void Force_DrainForceEnergy(gentity_t *ent, int power, int energy) {
	//my variant wont drain if 0.  BG_ForcePowerDrain drains default in that case.
	if(energy == 0)
		return;
	BG_ForcePowerDrain( &ent->client->ps, power, energy );
}

qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower );
qboolean Force_CanUsePower(gentity_t *ent, int power) {
	
	if(power >= NUM_FORCE_POWERS)
		return qfalse;

	if ( ent->client->holdingObjectiveItem >= MAX_CLIENTS  
		&& ent->client->holdingObjectiveItem < ENTITYNUM_WORLD )
	{//holding Siege item
		if ( g_entities[ent->client->holdingObjectiveItem].genericValue15 )
		{//disables force powers
			return qfalse;
		}
	}
	
	//Temp
	if(!ForcePowers[power])
		return qfalse;

	void *data = Force_GetPlayerForceData(ent, power);

	if(ForcePowers[power]->available && !ForcePowers[power]->available(ent, data))
		return qfalse;

	// HACK: Force grip uses pull for damage debounce, exclude it.
	//Ufo: changed to rage
	if(power != FP_RAGE && ent->client->ps.fd.forcePowerDebounce[power] > level.time)
		return qfalse;
	
	return WP_ForcePowerUsable(ent, (forcePowers_t)power);
}

void Force_StopPower(gentity_t *ent, int power) {
	if(!ForcePowers[power])
		return;
	if(!(ent->client->ps.fd.forcePowersActive & ( 1 << power)))
		return;
	if(ForcePowers[power]->stop) {
		void *data = Force_GetPlayerForceData(ent, power);
		ForcePowers[power]->stop(ent, data);
	}
	ent->client->ps.fd.forcePowersActive &= ~( 1 << power );
}

qboolean Force_UsePower(gentity_t *ent, int power) {
	
	//Ufo: check for invalid force powers and levels
	if (power < 0 || ent->client->ps.fd.forcePowerLevel[power] < FORCE_LEVEL_0 || ent->client->ps.fd.forcePowerLevel[power] > FORCE_LEVEL_5) {
		return qfalse;
	}

	//Temp
	if(!ForcePowers[power])
		return qfalse;

	void *data = Force_GetPlayerForceData(ent, power);

	if(!ForcePowers[power]->hold) {
		if (ent->client->ps.fd.forcePowersActive & (1 << power)) {
			if (ent->client->ps.forceAllowDeactivateTime >= level.time)
				return qfalse;
			Force_StopPower(ent, power);
			return qtrue;
		}

		if (ent->client->ps.fd.forceButtonNeedRelease) {
			return qfalse;
		}
	}

	if(!Force_CanUsePower(ent, power))
		return qfalse;

	//if null start func, assume we are always active.  If we arent usable then available would have returned false.
	if(!ForcePowers[power]->start || ForcePowers[power]->start(ent, data)) {
		ent->client->ps.forceAllowDeactivateTime = level.time + 1500;
		ent->client->ps.fd.forcePowersActive |= (1 << power);
	}

	if(ForcePowers[power]->hearDistance) {
		ent->client->ps.otherSoundLen = ForcePowers[power]->hearDistance;
		ent->client->ps.otherSoundTime = level.time + 100;
	}

	if (ForcePowers[power]->hold) {
		ent->client->ps.fd.forceButtonNeedRelease = 1;
	}

	return qtrue;
}

//TODO: remake this
qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt);
void Force_Update(gentity_t *ent) {
	qboolean stopAll = qfalse;
	if (ent->health < 1 || ent->client->ps.stats[STAT_HEALTH] < 1){
		stopAll = qtrue;
	}
	int i;
	for(i = 0; i < NUM_FORCE_POWERS; i++) {
		if(!(ent->client->ps.fd.forcePowersActive & (1 << i)))
			continue;
		//Temp
		if(!ForcePowers[i])
			continue;
		//FIXME: check if forcepower is still available.
		if(stopAll || !WP_ForcePowerAvailable(ent, i, 0))
			Force_StopPower(ent, i);
		else if(ForcePowers[i]->run && ForcePowers[i]->run(ent, Force_GetPlayerForceData(ent, i)) == qfalse)
			Force_StopPower(ent, i);
	}
}


#ifdef LMD_EXPERIMENTAL
#include "Lmd_Commands_Data.h"

extern forceSpeed_t Force_Speed_Levels[5];
void Cmd_Speed(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_Speed_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Speed_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Speed_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Speed_Levels[level].forcepower));
	}
	else
		Disp(ent, "duration | forcepower");
}

extern forceThrow_t Force_Throw_Levels[5];
void Cmd_Throw(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "power") == 0){
		if(val[0])
			Force_Throw_Levels[level].power = atoi(val);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].power));
	}
	else if(Q_stricmp(arg, "radius") == 0) {
		if(val[0])
			Force_Throw_Levels[level].radius = atoi(val);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].radius));
	}
	else if(Q_stricmp(arg, "arc") == 0){
		if(val[0])
			Force_Throw_Levels[level].arc = atoi(val);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].arc));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Throw_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].forcepower));
	}
	else if(Q_stricmp(arg, "knockdown") == 0) {
		if(val[0])
			Force_Throw_Levels[level].knockdown = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].knockdown));
	}
	else if(Q_stricmp(arg, "saberlockhits") == 0) {
		if(val[0])
			Force_Throw_Levels[level].saberlockhits = atoi(val);
		else
			Disp(ent, va("%i", Force_Throw_Levels[level].saberlockhits));
	}
	else
		Disp(ent, "power | radius | arc | forcepower | knockdown | saberlockhits");
}

extern forceSee_t Force_See_Levels[5];
void Cmd_See(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
		if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_See_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_See_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_See_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_See_Levels[level].forcepower));
	}
	else
		Disp(ent, "duration | forcepower");
}

extern forceHeal_t Force_Heal_Levels[5];
void Cmd_Heal(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
		if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "health") == 0){
		if(val[0])
			Force_Heal_Levels[level].health = atoi(val);
		else
			Disp(ent, va("%i", Force_Heal_Levels[level].health));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Heal_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Heal_Levels[level].forcepower));
	}
	else
		Disp(ent, "health | forcepower");
}

extern forceTelepathy_t Force_Telepathy_Levels[5];
void Cmd_Telepathy(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
		if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_Telepathy_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "npcduration") == 0) {
		if(val[0])
			Force_Telepathy_Levels[level].npcduration = atoi(val);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].npcduration));
	}
	if(Q_stricmp(arg, "range") == 0){
		if(val[0])
			Force_Telepathy_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].range));
	}
	else if(Q_stricmp(arg, "arc") == 0) {
		if(val[0])
			Force_Telepathy_Levels[level].arc = atoi(val);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].arc));
	}
	else if(Q_stricmp(arg, "divertnpcs") == 0) {
		if(val[0])
			Force_Telepathy_Levels[level].divertnpcs = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].divertnpcs));
	}
	else if(Q_stricmp(arg, "affectnpcjedi") == 0) {
		if(val[0])
			Force_Telepathy_Levels[level].affectnpcjedi = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].affectnpcjedi));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Telepathy_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Telepathy_Levels[level].forcepower));
	}
	else
		Disp(ent, "duration | npcduration | range | arc | divertnpcs | affectnpcjedi | forcepower");
}

extern forceProtect_t Force_Protect_Levels[5];
void Cmd_Protect(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "forcetake") == 0){
		if(val[0])
			Force_Protect_Levels[level].duration = atof(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "damagetake") == 0) {
		if(val[0])
			Force_Protect_Levels[level].damagetake = atof(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].damagetake));
	}
	if(Q_stricmp(arg, "maxdamage") == 0){
		if(val[0])
			Force_Protect_Levels[level].maxdamage = atoi(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].maxdamage));
	}
	else if(Q_stricmp(arg, "duration") == 0) {
		if(val[0])
			Force_Protect_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].duration));
	}
	if(Q_stricmp(arg, "draintime") == 0){
		if(val[0])
			Force_Protect_Levels[level].draintime = atoi(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].draintime));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Protect_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Protect_Levels[level].forcepower));
	}
	else
		Disp(ent, "forcetake | damagetake | maxdamage | duration | draintime | forcepower");
}


extern forceAbsorb_t Force_Absorb_Levels[5];
void Cmd_Absorb(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
		if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_Absorb_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Absorb_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "draintime") == 0) {
		if(val[0])
			Force_Absorb_Levels[level].draintime = atoi(val);
		else
			Disp(ent, va("%i", Force_Absorb_Levels[level].draintime));
	}
	else if(Q_stricmp(arg, "allowprotect") == 0) {
		if(val[0])
			Force_Absorb_Levels[level].allowprotect = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Absorb_Levels[level].allowprotect));
	}
	else
		Disp(ent, "duration | draintime | allowprotect");
}

extern forceTeamHeal_t Force_TeamHeal_Levels[5];
void Cmd_TeamHeal(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "range") == 0){
		if(val[0])
			Force_TeamHeal_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamHeal_Levels[level].range));
	}
	else if(Q_stricmp(arg, "heal") == 0) {
		int i = atoi(val);
		if(!val[0]  ||  i < 0  ||  i > 2) {
			Disp(ent, "^3Enter index 0 (1p), 1 (2p), 2 (>=3p)");
		}
		else {
			trap_Argv(4, val, sizeof(val));
			if(val[0])
				Force_TeamHeal_Levels[level].heal[i] = atoi(val);
			else
				Disp(ent, va("%i", Force_TeamHeal_Levels[level].heal[i]));
		}
	}
	else if(Q_stricmp(arg, "debounce") == 0) {
		if(val[0])
			Force_TeamHeal_Levels[level].debounce = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamHeal_Levels[level].debounce));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_TeamHeal_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamHeal_Levels[level].forcepower));
	}
	else
		Disp(ent, "range | heal[3] | debounce | forcepower");
}

extern forceGrip_t Force_Grip_Levels[5];
void Cmd_Grip(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "range") == 0){
		if(val[0])
			Force_Grip_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].range));
	}
	else if(Q_stricmp(arg, "damage") == 0) {
		if(val[0])
			Force_Grip_Levels[level].damage = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].damage));
	}
	else if(Q_stricmp(arg, "choke") == 0){
		if(val[0])
			Force_Grip_Levels[level].choke = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].choke));
	}
	else if(Q_stricmp(arg, "duration") == 0) {
		if(val[0])
			Force_Grip_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "movetype") == 0){
		if(val[0]){
			int i = atoi(val);
			if(i < 0)
				i = 0;
			else if(i > 2)
				i = 2;
			Force_Grip_Levels[level].moveType = i;
		}
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].moveType));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Grip_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].forcepower));
	}
	else if(Q_stricmp(arg, "forcedrain") == 0) {
		if(val[0])
			Force_Grip_Levels[level].forcedrain = atoi(val);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].forcedrain));
	}
	else if(Q_stricmp(arg, "throatcrush") == 0) {
		if(val[0])
			Force_Grip_Levels[level].throatcrush = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].throatcrush));
	}
	else if(Q_stricmp(arg, "omni") == 0) {
		if(val[0])
			Force_Grip_Levels[level].omni = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Grip_Levels[level].omni));
	}
	else
		Disp(ent, "range | damage | choke | duration | movetype | forcepower | forcedrain | throatcrush | omni");
}

extern forceLightning_t Force_Lightning_Levels[5];
void Cmd_Lightning(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "range") == 0){
		if(val[0])
			Force_Lightning_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].range));
	}
	else if(Q_stricmp(arg, "arc") == 0) {
		if(val[0])
			Force_Lightning_Levels[level].arc = atoi(val);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].arc));
	}
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_Lightning_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "debounce") == 0) {
		if(val[0])
			Force_Lightning_Levels[level].debounce = atoi(val);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].debounce));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Lightning_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].forcepower));
	}
	else if(Q_stricmp(arg, "twohanded") == 0) {
		if(val[0])
			Force_Lightning_Levels[level].twohanded = ((atoi(val) == 0) ? qfalse : qtrue);
		else
			Disp(ent, va("%i", Force_Lightning_Levels[level].twohanded));
	}
	else
		Disp(ent, "range | arc | duration | debounce | forcepower | twohanded");
}

extern forceRage_t Force_Rage_Levels[5];
void Cmd_Rage(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "selfdamage") == 0){
		if(val[0])
			Force_Rage_Levels[level].selfdamage = atoi(val);
		else
			Disp(ent, va("%i", Force_Rage_Levels[level].selfdamage));
	}
	if(Q_stricmp(arg, "duration") == 0){
		if(val[0])
			Force_Rage_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Rage_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "draintime") == 0) {
		if(val[0])
			Force_Rage_Levels[level].draintime = atoi(val);
		else
			Disp(ent, va("%i", Force_Rage_Levels[level].draintime));
	}
	else if(Q_stricmp(arg, "recoverytime") == 0) {
		if(val[0])
			Force_Rage_Levels[level].recoverytime = atoi(val);
		else
			Disp(ent, va("%i", Force_Rage_Levels[level].recoverytime));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Rage_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Rage_Levels[level].forcepower));
	}
	else
		Disp(ent, "selfdamage | duration | draintime | recoverytime | forcepower");
}

extern forceTeamReplenish_t Force_TeamReplenish_Levels[5];
void Cmd_TeamReplenish(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "range") == 0){
		if(val[0])
			Force_TeamReplenish_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamReplenish_Levels[level].range));
	}
	else if(Q_stricmp(arg, "power") == 0) {
		int i = atoi(val);
		if(!val[0]  ||  i < 0  ||  i > 2) {
			Disp(ent, "^3Enter index 0 (1p), 1 (2p), 2 (>=3p)");
		}
		else {
			trap_Argv(4, val, sizeof(val));
			if(val[0])
				Force_TeamReplenish_Levels[level].power[i] = atoi(val);
			else
				Disp(ent, va("%i", Force_TeamReplenish_Levels[level].power[i]));
		}
	}
	else if(Q_stricmp(arg, "debounce") == 0) {
		if(val[0])
			Force_TeamReplenish_Levels[level].debounce = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamReplenish_Levels[level].debounce));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_TeamReplenish_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_TeamReplenish_Levels[level].forcepower));
	}
	else
		Disp(ent, "range | power[3] | debounce | forcepower");
}

extern forceDrain_t Force_Drain_Levels[5];
void Cmd_Drain(gentity_t *ent, int iArg) {
	char arg[MAX_STRING_CHARS];
	char val[MAX_STRING_CHARS];
	trap_Argv(1, val, sizeof(val));
	trap_Argv(2, arg, sizeof(arg));
	int level = atoi(val);
	if(!val[0]) {
		Disp(ent, "cmd <lvl> <data> <val>");
		return;
	}
	if(level >= 5) {
		Disp(ent, "lvl < 5");
		return;
	}
	trap_Argv(3, val, sizeof(val));
	if(Q_stricmp(arg, "damage") == 0){
		if(val[0])
			Force_Drain_Levels[level].damage = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].damage));
	}
	else if(Q_stricmp(arg, "regenstun") == 0) {
		if(val[0])
			Force_Drain_Levels[level].regenstun = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].regenstun));
	}
	else if(Q_stricmp(arg, "range") == 0) {
		if(val[0])
			Force_Drain_Levels[level].range = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].range));
	}
	else if(Q_stricmp(arg, "arc") == 0) {
		if(val[0])
			Force_Drain_Levels[level].arc = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].arc));
	}
	else if(Q_stricmp(arg, "duration") == 0) {
		if(val[0])
			Force_Drain_Levels[level].duration = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].duration));
	}
	else if(Q_stricmp(arg, "debounce") == 0) {
		if(val[0])
			Force_Drain_Levels[level].debounce = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].debounce));
	}
	else if(Q_stricmp(arg, "forcepower") == 0) {
		if(val[0])
			Force_Drain_Levels[level].forcepower = atoi(val);
		else
			Disp(ent, va("%i", Force_Drain_Levels[level].forcepower));
	}
	else
		Disp(ent, "damage | regenstun | range | arc | duration | debounce | forcepower");
}

cmdEntry_t forceDebugCommands[] = {
	{"dbg_speed", "", Cmd_Speed, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_throw", "", Cmd_Throw, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_see", "", Cmd_See, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_heal", "", Cmd_Heal, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_telepathy", "", Cmd_Telepathy, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_protect", "", Cmd_Protect, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_absorb", "", Cmd_Absorb, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_teamheal", "", Cmd_TeamHeal, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_grip", "", Cmd_Grip, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_lightning", "", Cmd_Lightning, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_rage", "", Cmd_Rage, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_replenish", "", Cmd_TeamReplenish, 0, qtrue, 1, 0, 0, 0, qfalse},
	{"dbg_drain", "", Cmd_Drain, 0, qtrue, 1, 0, 0, 0, qfalse},

	{NULL}
};

#endif
#endif
