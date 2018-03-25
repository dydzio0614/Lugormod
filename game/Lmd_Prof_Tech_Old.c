
#ifdef OLD_PROFESSION_SYSTEM

#ifdef LMD_EXPERIMENTAL


#include "g_local.h"

#include "Lmd_ProfCore.h"
#include "Lmd_Inventory.h"

/*
NOTE: FL_SHIELDED flag to make projectiles bounce off things.

TODO: making stuff should take time, and have a possibility of failure.
TODO: need to make some long term delay between choosing to remove points and having them be removed
		TechProf_RemoveItemPoints
		Use some tech stats for it, scan for it in the account update command.  Around 2 points per day, perhapse?
		STAT_TECH_REMOVEPOINTS: how many points left to remove
		STAT_TECH_REMOVEPOINTSTIME: time at last point removal (real world time)
		Skill to control the time it takes to remove a point?

TODO: stun baton into energy tazer, send out that small soundless electrical arc fx.
		Not too much damage, but stun them from firing weapons and the like.

TODO: make tech stash remote deposit larger than the normal time even at lvl 5,
		but allow techs to make items that will speed it up.
		Also allow items to speed up regular deposit?

TODO: tech only items
		modifier to decrease deposit time at a bank?
*/

/*
Each group (consumable, devices, et al), when leveled up, increases the points to level up its objects.

The group should have enough points to max out all subgroups at level 5.
Wait, no they shouldn't.  If I don't let that, then each tech will be differentish based on their skills, so
more techs can survive within the demand.
Ok, doing that.

Should have a way for techs to remove points from items.  Make it take x amount of days to complete, scale based on 
number of points being moved, and enforce a max points per movement (precentage of total).
Max points enforces player participation, cant just pull everything out and come back in a few days if they want to 
reset the item points in a group.

1 2 3 4 5
1 3 6 10 15
Consumable
	(3 6 10 25 35 52)
	   (Maxed Modif  ) (Maxed Item   ) (Even Balance )
	3  (2 0 0        ) (1 1 0        ) (1 1 0        )
	6  (3 0 0        ) (2 2 0        ) (2 2 0        )
	10 (4 0 0 | 0 0 0) (3 2 1 | 0 0 0) (2 1 1 | 2 1 1)
	25 (5 4 0 | 0 0 0) (5 3 2 | 1 0 0) (3 3 1 | 3 3 0) 
	35 (5 5 2 | 1 1 0) (5 4 4 | 0 0 0) (4 3 1 | 4 3 1)
	52 (5 5 5 | 3 0 0) (5 5 5 | 2 2 1) (4 4 3 | 4 4 3)  
	Rations (at consumable level 0) (45 points)
		(There MUST be a debounce time between using this item, dont want players to spam it to get insta-heal)
		Health {1, 2, 4, 6, 8, 10}
		Servings {1, 2, 3, 5, 7, 10}
		Filling (debounce time) {45, 35, 30, 25, 15, 10}
	Medicine  (at consumable level 2) (45 points)
		//(force speed is 425 (1.7), force rage is 375 (1.5))
		//Speed increase {290 (1.16) 5 seconds, 290 (1.16) 8 sec, 290 (1.16) 10 sec, 350 (1.4) 15 sec, 350 (1.4) 20 sec, 350 (1.4) 25 sec}
		//Speed increase {260 (1.04), 275 (1.1), 290 (1.16), 310 (1.24), 330 (1.32), 350 (1.4)}
		health regen (4 7 10 12 16 20) (one per second, until the max)
		Servings {1, 2, 3, 4, 5, 6}
		Filling (debounce time) {45, 35, 30, 25, 15, 10}

	Nanotech (nanites) (slowly repair health when damaged, will repair total of x points, and last forever until those points are used up).
		Efficiency (how long it takes to repair 1 health point.)
		health repair (large) (only deduct from the repair count if we have damage, dont slowly apply like regen).
		
Datachips
	Forged Weapon Cirtification (lets mercs use weapons they don't have a skill for, for a time).
		Expiration (how many spawns until it stops working) {1, 2, 3, 5, 7}
		Level (Weapon level same as merc weapon skill, 6 allows all weapons including conc and rocket) {1, 2, 3, 4, 5, 6} 
	Refined scanner algorythm (increase stash range)
		Durability (number of seconds it lasts for) {10, 30, 60, 90, 120, 150}
		Effectiveness {merc_stash_range[0], merc_stash_range[1], merc_stash_range[2], merc_stash_range[3], merc_stash_range[4], merc_stash_range[5]}

Crystals
	Allow jedi to get more forcepower, or possibly modify other forcepower effectiveness.
	Crystals that jedi can charge with forcepower and use as a sort of grenade.

Mods
	Weapon modification kits, 1 for each weapon.  Increases weapon damage when activated.  Will break after x many shots
	Explosive mods: make the weapon explode on pickup or after x seconds when dropped.

Ammo stores
	Gives certain amounts of certain ammo when used.

Armaments
	Portable AutoTurret
		(1 3 10 19 25 33)
		Durability (health)
		Ammo (10 15 20 40 50)
		Firing speed
		Duration (how long it lasts)
	Seeker (weaker than AutoTurret)
		(1 3 10 19 25 33)
		Durability (health)
		Ammo
		Firing speed
		Duration (how long it lasts)
	Drop-bombs
		If the player is pulled by a jedi, the drop-bomb will get pulled out and explode on contact
		If the player dies, all bombs will detonate.

Devices
	(7 12 24 32 54 85)
	  (Maxed modifier     ) (Maxed item         ) (Balance)
	7  (310|000            ) (221|000            ) (111|111            )
	12 (411|000|000        ) (222|200|000        ) (210|210|210        )
	24 (532|000|000|000    ) (442|100|000|000    ) (220|220|220|220    )
	32 (551|100|000|000|000) (444|110|000|000|000) (221|221|220|220|220)
	54 (555|320|000|000|000) (555|320|000|000|000) (322|322|222|222|222)
	85 (555|554|000|000|000) (555|444|400|000|000) (422|422|422|422|222)

	Stash decoy (if someone gets near it, explode with demp2 alt effect and same damage pattern)
		(level 0)
		Stun (time to be stunned by) (1.2, 1.7, 2.4, 2.8, 3.5, 4.2)
		Range (0, 50, 70, 90, 120, 140)
		Duration (10, 15, 20, 25, 30, 50)
	Force Field
		(level 0)
		Size
		Durability (health)
		Energy (how long it lasts)
	Bodyshield (let the user toggle it on/off)
		(use cloak meter for threshhold)
		(level 1)
		Threshhold (how much damage can be nulled out) (5, 10, 15, 20, 25, 30)
		Recharge (the rate of threshhold recharge, 1 unit per x seconds) (3, 2.5, 2, 1.5, 1, 0.5)
		Energy (total energy reserves) (10, 20, 30, 50, 70, 100)
	GravGenerator
		(level 2)
		(Use jetpack fuel gauge)
		Effect (fraction of gravity to apply to the player)
			(.25 (600), .375 (500), .5 (400), .5625 (350), .625 (300), .75(200))
		Capacitance (how long the effect can last at a time) (4, 10, 15, 20, 25, 30) 
		Duration (total time in use) (8, 20, 30, 40, 50, 60)
	Stasis field (freeze all in area.  After first effect, emit light until it runs out.)
		(level 3)
				 (play the force push aoura around the players that are frozen.)
				 (PM_FREEZE, yes they will get stuck floating in the air.)
				 (once used, have a delay before turning on, catch the tech as well if they are in range.)
				 (Constantly clear the velocity of frozen players, so a player cant repeatedly push them to make them go flying/)
		Range
		Field duration
		Charges (lets it be used multiple times before running out)
*/

/*
===================================================================
Tech skills
===================================================================
*/

//Currently have 10 skills, merc has 9... Keep it balanced?
//Can combine generator and battery into the same...

//demp2 recharges at 1 ammo per 4 energy (code in WP_ForcePowerRegenerate)

const skillEntry_t techSkills[] = {
	//{"Scanner", SK_TECH_SCANNER, 0},
	{"Remote stash deposit", SK_TECH_REMOTESTASHDEPO},

	{"Demp2", SK_TECH_DEMP2},

	{"Portable Generator", SK_TECH_GENERATOR}, //generate "energy" (forcepower)

#ifdef LMD_TECH_OLDSETUP
	{"Craft Consumables", SK_TECH_CONSUMABLE},
	{"Unused", SK_TECH_DATACHIPS},
	//{"Craft Datachips", SK_TECH_DATACHIPS},
	{"Unused", SK_TECH_CRYSTALS},
	//{"Craft Crystals", SK_TECH_CRYSTALS},
	{"Unused", SK_TECH_WEAPONMODS},
	//{"Craft Weapon Mods", SK_TECH_WEAPONMODS},
	{"Unused", SK_TECH_ARMAMENTS},
	//{"Craft Armaments", SK_TECH_ARMAMENTS},
	{"Craft Devices", SK_TECH_DEVICES},
#endif
};

void Tech_Spawn(gentity_t *ent);
static const profession_t techProfData = {
	"Tech",
	SKILLENTRY(techSkills),
	{0, 0, NULL},
	Tech_Spawn,
};

//TODO: set this up and hook it into the forcepower regen time code.
//const prevents it from being used as an extern in w_force, for no apparent reason.
//Possibly if  I defined it as const static...
/*const*/ int TechProf_GeneratorTime[] = {
	20000,
	18000,
	16000,
	14000,
	12000,
	10000,
};

//should be 27 to gain a full charge, 30 to be nice and rounded
//8 for a single shot.
//use 70, will give 8 shots.
//100 for 12 shots (12.5)
int TechProf_MaxDemp2Ammo[] = {
	100,
	100,
	100,
	100,
	100,
	100
};

float TechProf_RemoteStashDepoTimeScale [] = {
	2.5f,
	2.3f,
	2.1f,
	1.9f,
	1.7f,
	1.5f,
};

float TechProf_Demp2PrimaryFireVelocityFactor[] = {
	0.5f,
	0.7f,
	1.0f,
	1.6f,
	2.3f,
	3.5f,
};

int TechProf_Demp2PrimaryFireTTL[] = {
	900,
	1200,
	1400,
	1600,
	2800,
	3000,
};

int TechProf_PowercellChargeRate[] = {
	3900,
	3400,
	2800,
	1400,
	800,
	250,
};

int TechProf_MaxCraftpoints(gentity_t *ent){
	return 25 + (PlayerAcc_Prof_GetLevel(ent) * 2.5);
}
int TechProf_CraftingCost(int itemLevel, int totalModifiers){
	return 10 + (5 * itemLevel) + (2 * totalModifiers);
}
void Tech_Spawn(gentity_t *ent){
	ent->client->ps.trueNonJedi = qtrue;
	ent->client->ps.trueJedi = qfalse;
	ent->client->ps.weapon = WP_DEMP2;
	ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_DEMP2);
	Add_Ammo(ent, weaponData[WP_DEMP2].ammoIndex, Q3_INFINITE);

	//FIXME: sith can drain this
	ent->client->ps.fd.forcePowerMax = TechProf_MaxCraftpoints(ent);
	//ent->client->ps.fd.forcePower = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, SPEC_TECH_CRAFTPOINTS);
	//give techs stun baton as a weapon to fall back on when out of ammo
	ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
}

extern vmCvar_t lmd_stashdepotime;
int StashHolder(void);
void Cmd_DepoStash_f(gentity_t *ent, int iArg){
	//SK_TECH_REMOTESTASHDEPO
	//Disp(ent, "^3Not done yet.");
	if(ent->client->sess.spectatorState != SPECTATOR_NOT){
		Disp(ent, "^3Cannot do this as a spectator.");
		return;
	}
	if(ent->s.number != StashHolder()){
		Disp(ent, "^3You do not have a stash.");
		return;
	}
	Disp(ent, "^3Starting stash deposit...");
	VectorCopy(ent->client->ps.origin, ent->client->Lmd.stashDeposit.startPosition);
	ent->client->Lmd.stashDeposit.startTime = level.time;
	//ent->client->Lmd.stashDeposit.totalTime = lmd_stashdepotime.integer *
	//	TechProf_RemoteStashDepoTimeScale[PlayerAcc_Prof_GetSkill(ent, PROF_TECH, SK_TECH_REMOTESTASHDEPO)];
	ent->client->Lmd.stashDeposit.startHealth = ent->health;
}

void Cmd_RechargePower_f(gentity_t *ent, int iArg){
	/*
		unsigned int ammoRechargeType;
		unsigned int ammoRechargeAmount;
		unsigned int ammoRechargeRate;
	*/
	if(ent->client->Lmd.ammoRecharge.amount == 0 || ent->client->Lmd.ammoRecharge.type != AMMO_POWERCELL){
		Disp(ent, "^2Shunting energy to power cells.");
		ent->client->Lmd.ammoRecharge.type = AMMO_POWERCELL;
		ent->client->Lmd.ammoRecharge.techCost = 2;
		//ent->client->Lmd.ammoRecharge.rate = TechProf_PowercellChargeRate[PlayerAcc_Prof_GetSkill(ent, PROF_TECH, SK_TECH_GENERATOR)];
		ent->client->Lmd.ammoRecharge.amount = Q3_INFINITE;
	}
	else{
		ent->client->Lmd.ammoRecharge.amount = 0;
		Disp(ent, "^3Powercell charge halted.");
	}
}

void Cmd_TechCreate_f(gentity_t *ent, int iArg)
{
	Disp(ent, "^1The tech is a lie...");
}


#ifdef LMD_TECH_OLDSETUP

/*
===================================================================
Tech iObject construction
===================================================================
*/

typedef struct techObjectModifier_s{
	char *name;
	char *descr;
	char *keyName;
	qboolean isCustom; //if this is true, copy whatever the user gives us into the value for the key
	float values[6]; //if isCustom is false, values for the key per level 0 to 5.
}techObjectModifier_t;

typedef struct techObject_s{
	char *itemName; //pull descr from inventory system using this
	unsigned int requiredGroupLevel;
	unsigned int maxCraftPoints;
#ifdef LMD_TECH_ITEMSPECS
	unsigned int pointSpec;
#endif
	techObjectModifier_t *modifiers;
}techObject_t;

typedef struct techObjectGroup_s{
	char *groupName;
	char *descr;
	unsigned int objectPoints[6]; //points to be set to objects, level 0 to 5
	unsigned int skill;
	techObject_t *Objects;
}techObjectGroup_t;
/*
===================================================================
Tech Object: Rations
===================================================================
*/
techObjectModifier_t techObject_Rations_Modifiers[] = {
	{"Health", "The player will be healed when they use the item.", "health", qfalse, {1, 2, 4, 6, 8, 10}},
	{"Servings", "How many times this item can be used.", "count", qfalse, {1, 2, 3, 5, 7, 10}},
	{"Filling", "How long until you can use another serving.", "debounce", qfalse, {45, 35, 30, 25, 15, 10}},

	{NULL, NULL, NULL, qfalse, {0, 0, 0, 0, 0, 0}}
};

/*
===================================================================
Tech Object: Medicine
===================================================================
*/
techObjectModifier_t techObject_Medicine_Modifiers[] = {
	{"Quality", "Higher quality medicines will last for a longer time.", "health", qfalse, {4, 7, 10, 12, 16, 20}},
	{"Servings", "How many times this item can be used.", "count", qfalse, {1, 2, 3, 4, 5, 6}},
	{"Filling", "How long until you can use another serving.", "debounce", qfalse, {60, 57, 53, 48, 40, 30}},

	{NULL, NULL, NULL, qfalse, {0, 0, 0, 0, 0, 0}}
};
/*
===================================================================
Tech Object Table: Consumable
===================================================================
*/
techObject_t techObjects_Consumable[] = {
	{"Rations", 0, 45,
#ifdef LMD_TECH_ITEMSPECS
	SPEC_TECH_CRAFT_CONSUMABLE_RATIONS,
#endif
	techObject_Rations_Modifiers},
	
	{"Medicine", 2, 45,
#ifdef LMD_TECH_ITEMSPECS
	SPEC_TECH_CRAFT_CONSUMABLE_MEDICINE,
#endif
	techObject_Medicine_Modifiers},

	{NULL, 0, 0,
#ifdef LMD_TECH_ITEMSPECS
	0,
#endif
	NULL}
};

/*
===================================================================
Tech Object: Decoy
===================================================================
*/
techObjectModifier_t techObject_Decoy_Modifiers[] = {
	{"Stun", "Players caught in the blast will be stunned for a time.", "stuntime", qfalse, {1.2, 1.7, 2.4, 2.8, 3.5, 4.2}},
	{"Range", "How large the explosion radius is.", "range", qfalse, {0, 50, 70, 90, 120, 140}},
	{"Duration", "How long this can last in the field.", "duration", qfalse, {10, 15, 20, 25, 30, 50}},

	{NULL, NULL, NULL, qfalse, {0, 0, 0, 0, 0, 0}}
};

/*
===================================================================
Tech Object: Bodyshield
===================================================================
*/

techObjectModifier_t techObject_Bodyshield_Modifiers[] = {
	{"Threshhold", "How much damage can be absorbed before it needs to recharge.", "maxcharge", qfalse, {5, 10, 15, 20, 25, 30}},
	{"Recharge", "How many seconds it takes to move a unit of energy into the threshhold.", "recharge", qfalse, {3, 2.5, 2, 1.5, 1, 0.5}},
	{"Energy", "How much energy the shield has in reserve.", "energy", qfalse, {10, 20, 30, 50, 70, 100}},

	{NULL, NULL, NULL, qfalse, {0, 0, 0, 0, 0, 0}}
};

/*
===================================================================
Tech Object: GravGenerator
===================================================================
*/
/*
Capacitance (how long the effect can last at a time) (4, 10, 15, 20, 25, 30) 
Duration (total time in use) (8, 20, 30, 40, 50, 60)
*/
techObjectModifier_t techObject_GravGenerator_Modifiers[] = {
	{"Effect", "The fraction of gravity that this item will reach.", "gravfrac", qfalse, {.25f, .375f, .5f, .5625f, .625f, .75f}},
	{"Capacitance", "How long the gravity change can last per use", "capacitance", qfalse, {4, 10, 15, 20, 25, 30}},
	{"Duration", "How much energy the shield has in reserve.", "energy", qfalse, {7, 20, 30, 40, 50, 60}},

	{NULL, NULL, NULL, qfalse, {0, 0, 0, 0, 0, 0}}
};

/*
===================================================================
Tech Object Table: Device
===================================================================
*/

techObject_t techObjects_Device[] = {
	{"Decoy", 0, 45,
#ifdef LMD_TECH_ITEMSPECS
	SPEC_TECH_CRAFT_DEVICE_DECOY,
#endif
	techObject_Decoy_Modifiers},

	{"Bodyshield", 1, 45,
#ifdef LMD_TECH_ITEMSPECS
	SPEC_TECH_CRAFT_DEVICE_BODYSHIELD,
#endif
	techObject_Bodyshield_Modifiers},
	
	{"GravGenerator", 2, 45,
#ifdef LMD_TECH_ITEMSPECS
	SPEC_TECH_CRAFT_DEVICE_GRAVGENERATOR,
#endif
	techObject_GravGenerator_Modifiers},

	{NULL, 0, 0,
#ifdef LMD_TECH_ITEMSPECS
	0,
#endif
	NULL}
};


/*
===================================================================
Tech Object Group table
===================================================================
*/
techObjectGroup_t techObjectGroups[] = {
	{"Consumables", "Foods and Drinks", {3, 6, 10, 25, 35, 52}, SK_TECH_CONSUMABLE, techObjects_Consumable},
	{"Devices", "General non-offensive devices.", {7, 12, 24, 32, 54, 85}, SK_TECH_DEVICES, techObjects_Device},
};

const int techObjectGroups_Count = sizeof(techObjectGroups) / sizeof(techObjectGroup_t);


/*
===================================================================
Tech Object helper functions
===================================================================
*/
#ifdef LMD_TECH_ITEMSPECS
unsigned int TechProf_GetFreeCreatePoints(int id, techObjectGroup_t *group){
	int playerSkill = Accounts_Prof_GetSkill(id, PROF_TECH, group->skill);
	unsigned int points, free = group->objectPoints[playerSkill];
	techObject_t *object;
	for(object = group->Objects; object->itemName; object++){
		//do sanity checking
		points = Accounts_Prof_GetSpec(id, PROF_TECH, object->pointSpec);
		if(points < 0){
			points = 0;
			Accounts_Prof_SetSpec(id, PROF_TECH, object->pointSpec, points);
		}
		else if(points > object->maxCraftPoints){
			points = object->maxCraftPoints;
			Accounts_Prof_SetSpec(id, PROF_TECH, object->pointSpec, points);
		}

		if(points > free){
			points -= (points - free);
			Accounts_Prof_SetSpec(id, PROF_TECH, object->pointSpec, points);
		}
		free -= points;	
	}
	return free;
}
#endif

techObjectGroup_t *TechProf_GetTechObjectGroupByName(char *name){
	unsigned int i;
	for(i = 0;i<techObjectGroups_Count;i++){
		if(Q_stricmp(techObjectGroups[i].groupName, name) == 0)
			return &techObjectGroups[i];
	}
	return NULL;
}

techObject_t *TechProf_GetTechObjectByName(char *name, techObjectGroup_t *group){
	techObject_t *object;
	for(object = group->Objects;object->itemName;object++){
		if(Q_stricmp(object->itemName, name) == 0){
			return object;
		}
	}
	return NULL;
}

techObjectModifier_t *TechProf_GetTechObjectModifierByName(techObject_t *object, char *name){
	techObjectModifier_t *modifier;
	for(modifier = object->modifiers;modifier->name;modifier++){
		if(Q_stricmp(modifier->name, name) == 0)
			return modifier;
	}
	return NULL;
}

qboolean TechProf_CanMakeObject(gentity_t *ent, techObject_t *object, techObjectGroup_t *group){
	if(object->requiredGroupLevel > PlayerAcc_Prof_GetSkill(ent, PROF_TECH, group->skill))
		return qfalse;

	return qtrue;
}

/*
===================================================================
Tech commands
===================================================================
*/

void TechProf_ListGroups(gentity_t *ent){
	unsigned int i;
	int playerSkill;
#ifdef LMD_TECH_ITEMSPECS
	unsigned int free;
#endif
	for(i = 0;i<techObjectGroups_Count;i++){
		playerSkill = PlayerAcc_Prof_GetSkill(ent, PROF_TECH, techObjectGroups[i].skill);
#ifdef LMD_TECH_ITEMSPECS
		free = TechProf_GetFreeCreatePoints(ent->client->pers.Lmd.account, &techObjectGroups[i]);
		Disp(ent, va("^3%s ^6(^2%i^6/^2%i^6 points, level ^2%i^6)", techObjectGroups[i].groupName, free, 
			techObjectGroups[i].objectPoints[playerSkill], playerSkill));
#else
		Disp(ent, va("^3%s ^6(level ^2%i^6)", techObjectGroups[i].groupName, playerSkill));
#endif
	}
}

void TechProf_ListCreatableItems(gentity_t *ent, char *groupName){
	int playerSkill;
	techObjectGroup_t *group = TechProf_GetTechObjectGroupByName(groupName);
	techObject_t *object;
	techObjectModifier_t *modifier;
#ifdef LMD_TECH_ITEMSPECS
	unsigned int free;
#endif
	if(!group){
		Disp(ent, "^3Unknown group.");
		return;
	}
	playerSkill = PlayerAcc_Prof_GetSkill(ent, PROF_TECH, group->skill);
#ifdef LMD_TECH_ITEMSPECS
	free = TechProf_GetFreeCreatePoints(ent->client->pers.Lmd.account, group);
	Disp(ent, va("^3%s ^6(^2%i^6/^2%i^6 points, level ^2%i^6)", group->groupName, free, 
		group->objectPoints[playerSkill], playerSkill));
#else
	Disp(ent, va("^3%s ^6(level ^2%i^6)", group->groupName, playerSkill));
#endif
	for(object = group->Objects; object->itemName; object++){
		if(!TechProf_CanMakeObject(ent, object, group))
			continue;
		Disp(ent, va("   ^5%s ^6(^2%i^6 points)", object->itemName,
			PlayerAcc_Prof_GetSpec(ent, PROF_TECH, object->pointSpec)));
		for(modifier = object->modifiers; modifier->name; modifier++){
			Disp(ent, va("      ^2%s^3: %s", modifier->name, modifier->descr));
			if(modifier->isCustom)
				Disp(ent, "         ^3Modifier takes custom text value.");
			else{
				Disp(ent, va("         ^3Modifier takes level 0 to 5.  Values: {^2%-2f %-2f %-2f %-2f %-2f %-2f^3}",
					modifier->values[0], modifier->values[1], modifier->values[2], modifier->values[3],
					modifier->values[4], modifier->values[5]));
			}
		}
	}
}

qboolean TechProf_MakeItem(gentity_t *ent, char *groupName, char *itemName, char *modifiers){
	techObjectGroup_t *group = TechProf_GetTechObjectGroupByName(groupName);
	techObjectModifier_t *modifier;
	int modifierCost = 0, points, totalModifiers = 0;
	iObject_t *iObject;
	qboolean found;
	char *str = modifiers, *token, itemInfo[MAX_STRING_CHARS] = "";
	techObject_t *object;
	if(!group){
		Disp(ent, "^3Unknown group.");
		return qfalse;
	}
	object = TechProf_GetTechObjectByName(itemName, group);
	if(!object || !TechProf_CanMakeObject(ent, object, group)){
		Disp(ent, "^3Unknown object.");
		return qfalse;
	}
	while(str[0]){
		token = COM_ParseDatastring((const char **)&str);
		if(!token[0])
			break;
		modifier = TechProf_GetTechObjectModifierByName(object, token);
		if(!modifier){
			Disp(ent, va("^3Unknown modifier ^2%s^3 for item ^2%s^3.", token, object->itemName));
			return qfalse;
		}
		Q_strcat(itemInfo, sizeof(itemInfo), va("\"%s\",", modifier->keyName));

		token = COM_ParseDatastring((const char **)&str);
		if(!token[0]){
			Disp(ent, va("^3Modifier ^2%s^3 has no value!", modifier->keyName));
			return qfalse;
		}
		if(modifier->isCustom){
			Q_strcat(itemInfo, sizeof(itemInfo), va("\"%s\",", token));
		}
		else{
			int level = atoi(token);
			if(level < 0 || level > 5 || (level == 0 && !(token[0] == '0' && token[1] == 0))){
				Disp(ent, va("^3Invalid value for modifier ^2%s^3, expected a level 0 to 5", modifier->name));
				return qfalse;
			}
			modifierCost += (level * (level + 1)) / 2;
			totalModifiers++;
			Q_strcat(itemInfo, sizeof(itemInfo), va("\"%f\",", modifier->values[level]));
		}
	}
#ifdef LMD_TECH_ITEMSPECS
	points = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, object->pointSpec) - modifierCost;
	if(points < 0){
		Disp(ent, va("^3You have overshot your modifier points by ^2%i^3.", -points));
		return qfalse;
	}
#endif

	points = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, SPEC_TECH_CRAFTPOINTS) - 
		TechProf_CraftingCost(object->requiredGroupLevel, totalModifiers);
	if(points < 0){
		Disp(ent, va("^3You have overshot your energy requirements by ^2%i^3.", -points));
		return qfalse;
	}
	PlayerAcc_Prof_SetSpec(ent, PROF_TECH, SPEC_TECH_CRAFTPOINTS, points);
	ent->client->ps.fd.forcePower = points;

	//find the modifiers we did not set, and set them to the zero value.
	for(modifier = object->modifiers;modifier->name;modifier++){
		if(modifier->isCustom)
			continue;
		found = qfalse;
		str = itemInfo;
		while(str[0]){
			token = COM_ParseDatastring((const char **)&str);
			if(!token[0])
				break;
			if(Q_stricmp(token, modifier->keyName) == 0){
				//pull the value off
				token = COM_ParseDatastring((const char **)&str);
				//if(!token[0])
				//	break;
				found = qtrue;
				break;
			}
			token = COM_ParseDatastring((const char **)&str);
			if(!token[0])
				break;
		}
		if(!found){
			Q_strcat(itemInfo, sizeof(itemInfo), va("\"%s\",\"%f\",", modifier->keyName, modifier->values[0]));
		}
	}

	if((iObject = Inventory_AddItem(getPlayerInventory(ent), object->itemName, PlayerAcc_GetId(ent), itemInfo))){
		Disp(ent, "^2Item successfully created.");
		Inventory_InitObject(ent, iObject);
	}
	else
		Disp(ent, "^1Error: Failed to make item.");
	return qtrue;
}

qboolean TechProf_AddItemPoints(gentity_t *ent, char *groupName, char *itemName, int points){
	techObjectGroup_t *group = TechProf_GetTechObjectGroupByName(groupName);
	techObject_t *object;
	int curPoints;
	if(!group){
		Disp(ent, "^3Unknown group.");
		return qfalse;
	}
	object = TechProf_GetTechObjectByName(itemName, group);
	if(!object){
		Disp(ent, "^3Unknown object.");
		return qfalse;
	}
	curPoints = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, object->pointSpec);
	if(points < 0){
		Disp(ent, "^3Invalid amount of points.");
		return qfalse;
	}
	else if(points > TechProf_GetFreeCreatePoints(ent->client->pers.Lmd.account, group)){
		Disp(ent, "^3You do not have enough free points.  Remove points from another item to get more.");
		return qfalse;
	}
	if(curPoints + points > object->maxCraftPoints){
		Disp(ent, va("^3You can only fit ^2%i^3 more points into this item.", object->maxCraftPoints - curPoints));
		return qfalse;
	}
	curPoints += points;
	PlayerAcc_Prof_SetSpec(ent, PROF_TECH, object->pointSpec, curPoints);
	Disp(ent, "^2Points set.");
	return qtrue;
}

qboolean TechProf_RemoveItemPoints(gentity_t *ent, char *groupName, char *itemName, int points){
	techObjectGroup_t *group = TechProf_GetTechObjectGroupByName(groupName);
	techObject_t *object;
	int curPoints;
	if(!group){
		Disp(ent, "^3Unknown group.");
		return qfalse;
	}
	object = TechProf_GetTechObjectByName(itemName, group);
	if(!object){
		Disp(ent, "^3Unknown object.");
		return qfalse;
	}
	curPoints = PlayerAcc_Prof_GetSpec(ent, PROF_TECH, object->pointSpec);
	if(points < 0){
		Disp(ent, "^3Invalid amount of points.");
		return qfalse;
	}
	else if(points > curPoints){
		Disp(ent, "^3You cannot remove more points than the item has.");
		return qfalse;
	}

	curPoints -= points;
	PlayerAcc_Prof_SetSpec(ent, PROF_TECH, object->pointSpec, curPoints);
	Disp(ent, "^2Points set.");
	return qtrue;
}

char *ConcatArgs(int start);
void Cmd_TechCreate_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(ent->client->sess.spectatorState != SPECTATOR_NOT){
		Disp(ent, "^3Cannot do this as a spectator.");
		return;
	}
	if(argc == 1){
		Disp(ent, "^3/create {list [group name]} {make <group> <item> [modifier,level,...]} {addpoints <group> <item> <amount>} {removepoints <group> <item> <amount>}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "list") == 0){
		trap_Argv(2, arg, sizeof(arg));
		if(!arg[0]){
			TechProf_ListGroups(ent);
			Disp(ent, "^3Type \'^2/create list <group name>^3\' for a list of items in the groups.");
		}
		else
			TechProf_ListCreatableItems(ent, arg);
		return;
	}
	else if(Q_stricmp(arg, "make") == 0){
		char group[MAX_STRING_CHARS];
		if(argc < 4){
			Disp(ent, "^3Usage: /create make <group> <item> [modifier,value,...]");
			return;
		}
		if(ent->client->sess.spectatorState != SPECTATOR_NOT){
			Disp(ent, "^3You cannot do this from spectation.");
			return;
		}
		trap_Argv(2, group, sizeof(group));
		trap_Argv(3, arg, sizeof(arg));
		TechProf_MakeItem(ent, group, arg, ConcatArgs(4));
		return;
	}
	else if(Q_stricmp(arg, "addpoints") == 0){
		char group[MAX_STRING_CHARS];
		int amount;
		if(argc < 5){
			Disp(ent, "^3Usage: /create addpoints <group> <item> <amount>");
			return;
		}
		trap_Argv(2, group, sizeof(group));
		trap_Argv(4, arg, sizeof(arg));
		amount = atoi(arg);
		if(amount <= 0){
			Disp(ent, "^3Invalid amount.");
			return;
		}
		trap_Argv(3, arg, sizeof(arg));
		TechProf_AddItemPoints(ent, group, arg, (unsigned int)amount);
		return;
	}
	else if(Q_stricmp(arg, "removepoints") == 0){
		char group[MAX_STRING_CHARS];
		int amount;
		if(argc < 4){
			Disp(ent, "^3Usage: /create removepoints <group> <item> <amount>");
			return;
		}
		trap_Argv(2, group, sizeof(group));
		trap_Argv(4, arg, sizeof(arg));
		amount = atoi(arg);
		if(amount <= 0){
			Disp(ent, "^3Invalid amount.");
			return;
		}
		trap_Argv(3, arg, sizeof(arg));
		TechProf_RemoveItemPoints(ent, group, arg, amount);
		return;
	}
	else
		Disp(ent, "^3Unknown arg.");
}


#endif

#endif

#endif
