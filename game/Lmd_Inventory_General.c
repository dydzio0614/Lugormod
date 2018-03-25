

#include "g_local.h"
#include "Lmd_Inventory.h"

typedef struct Item_Test_Fields_s{
	char *str;
	int i;
	int add;
	char *replace;
}Item_Test_Fields_t;

BG_field_t Item_Test_Fields[] = {
	{"str", FOFFSET(Item_Test_Fields_t, str), F_GSTRING, COMBINE_MATCH},
	{"int", FOFFSET(Item_Test_Fields_t, i), F_INT},
	{"add", FOFFSET(Item_Test_Fields_t, add), F_INT, COMBINE_ADD},
	{"replace", FOFFSET(Item_Test_Fields_t, replace), F_GSTRING, COMBINE_REPLACE},
	{NULL, 0, F_IGNORE},
};

void Item_Test_Use(iObject_t *obj) {
	Item_Test_Fields_t *data = (Item_Test_Fields_t *)obj->data;
	Disp(obj->holder, va("^3Used ^2%i^3 times.", ++data->i));
}

void Item_Test_Spawn(iObject_t *obj){
	obj->use = Item_Test_Use;	
}

iObjectDef_t Item_Test = {
	"test",
	sizeof(Item_Test_Fields_t),
	Item_Test_Fields,
	Item_Test_Spawn,
	NULL
};

typedef struct Item_Medpack_Fields_s{
	int health;
	int shield;
	int uses;
}Item_Medpack_Fields_t;

//TODO: health and shield per use flags.
BG_field_t Item_Medpack_Fields[] = {
	{"health", FOFFSET(Item_Medpack_Fields_t, health), F_INT},
	{"shield", FOFFSET(Item_Medpack_Fields_t, shield), F_INT},
	{"count", FOFFSET(Item_Medpack_Fields_t, uses), F_INT},
	//mmm, legacy.
	{"uses", FOFFSET(Item_Medpack_Fields_t, uses), F_INT},
	{NULL, 0, F_IGNORE},
};

qboolean Item_Madpack_Combine(iObject_t *obj, iObject_t *comb) {
	Item_Medpack_Fields_t *dobj = (Item_Medpack_Fields_t *)obj->data;
	Item_Medpack_Fields_t *dcomb = (Item_Medpack_Fields_t *)comb->data;
	if(dobj->uses == 0) {
		if(dcomb->uses != 0)
			return qfalse;
		dobj->health += dcomb->health;
		dobj->shield += dcomb->shield;
		return qtrue;
	}
	else {
		if(dcomb->uses == 0)
			return qfalse;
		dobj->uses += dcomb->uses;
		return qtrue;
	}
}

void Item_Medpack_Describe(iObject_t *obj, char *buf, unsigned int sze){
	Item_Medpack_Fields_t *data = (Item_Medpack_Fields_t *)obj->data;
	Q_strncpyz(buf, "^3Contains ", sze);
	if(data->health > 0)
		Q_strcat(buf, sze, va("^2%i^3 health %s", data->health, (data->shield > 0)?"and ":""));
	if(data->shield > 0)
		Q_strcat(buf, sze, va("^2%i^3 shield", data->shield));
	if(data->uses > 0) {
		Q_strcat(buf, sze, va(".  ^2%i^3 uses left.", data->uses));
	}
}

void Item_Medpack_Use(iObject_t *obj) {
	Item_Medpack_Fields_t *data = (Item_Medpack_Fields_t *)obj->data;
	int health = obj->holder->client->ps.stats[STAT_MAX_HEALTH] - obj->holder->client->ps.stats[STAT_HEALTH];
	int shield = obj->holder->client->ps.stats[STAT_MAX_HEALTH] - obj->holder->client->ps.stats[STAT_ARMOR];
	if(health > data->health)
		health = data->health;
	if(shield > data->shield)
		shield = data->shield;
	obj->holder->health = obj->holder->client->ps.stats[STAT_HEALTH] += health;
	obj->holder->client->ps.stats[STAT_ARMOR] += shield;

	char msg[MAX_STRING_CHARS] = "^3Restored ";
	if(health > 0)
		Q_strcat(msg, sizeof(msg), va("^2%i^3 health %s", health, (shield > 0)?"and ":""));
	if(shield > 0)
		Q_strcat(msg, sizeof(msg), va("^2%i^3 shield", shield));
	Q_strcat(msg, sizeof(msg), ".");
	Disp(obj->holder, msg);

	if(data->uses > 0) {
		data->uses--;
		if(data->uses == 0) {
			Inventory_DestroyObject(obj);
			return;
		}
	}
	else {
		data->health -= health;
		data->shield -= shield;
		if(!data->health && !data->shield) {
			Inventory_DestroyObject(obj);
			return;
		}
	}
	Inventory_Player_Modify(obj->holder);
}

void Item_Medpack_Spawn(iObject_t *obj){
	obj->describe = Item_Medpack_Describe;
	obj->use = Item_Medpack_Use;	
}

iObjectDef_t Item_Medpack = {
	"medpack",
	sizeof(Item_Medpack_Fields_t),
	Item_Medpack_Fields,
	Item_Medpack_Spawn,
	NULL
};

typedef struct Item_Weaponpack_Fields_s{
	int weapons;
}Item_Weaponpack_Fields_t;

//TODO: combineability
/*
2		WP_STUN_BATON,
4		WP_MELEE,
8		WP_SABER,
16		WP_BRYAR_PISTOL,
32		WP_BLASTER,
64		WP_DISRUPTOR,
128		WP_BOWCASTER,
256		WP_REPEATER,
512		WP_DEMP2,
1024	WP_FLECHETTE,
2048	WP_ROCKET_LAUNCHER,
4096		WP_THERMAL,
8192		WP_TRIP_MINE,
13684		WP_DET_PACK,
32768		WP_CONCUSSION,
65536		WP_BRYAR_OLD,
*/
//FIXME; key and values.  key of weapon name, value of ammo (adds ammo of that type when pulled).
BG_field_t Item_Weaponpack_Fields[] = {
	{"weapons", FOFFSET(Item_Weaponpack_Fields_t, weapons), F_INT},
	{NULL, 0, F_IGNORE},
};

void Item_Weaponpack_Describe(iObject_t *obj, char *buf, unsigned int sze){
	Item_Weaponpack_Fields_t *data = (Item_Weaponpack_Fields_t *)obj->data;
	int i, c = 0;
	int s[4], si = 0;
	for(i = 0;i < WP_EMPLACED_GUN;i++) {
		if(data->weapons & (1 << i)) {
			if(si < 4)
				s[si++] = i;
			c++;
		}
	}
	//FIXME: uses wrong name.
	Q_strncpyz(buf, "^3Pack contains ", sze);
	for(i = 0; i < si; i++) {
		Q_strcat(buf, sze, va("^2%s^3, ", weaponNameFromIndex[s[i]]));
	}
	if(c >= si) {
		Q_strcat(buf, sze, va("and ^2%i^3 other weapon%s.", c - si, (c - si != 1)?"s":""));
	}
}

void Item_Weaponpack_Use(iObject_t *obj) {
	Item_Weaponpack_Fields_t *data = (Item_Weaponpack_Fields_t *)obj->data;
	int i, c = 0;
	char arg[MAX_STRING_CHARS];
	qboolean getAll = qfalse;
	for(i = 0;i < WP_EMPLACED_GUN; i++) {
		c++;
	}

	//inventory use <item> ...
	trap_Argv(3, arg, sizeof(arg));
	if(c == 1) {
		if(obj->holder->client->ps.stats[STAT_WEAPONS] & data->weapons) {
			Disp(obj->holder, "^3You already have this weapon.");
			return;
		}
		obj->holder->client->ps.stats[STAT_WEAPONS] |= data->weapons;
		Disp(obj->holder, "^2Weapon aquired.");
		Inventory_Player_Modify(obj->holder);
		Inventory_DestroyObject(obj);
		return;
	}
	if(Q_stricmp(arg, "all") == 0)
		getAll = qtrue;
	if(!getAll && arg[0]) {
		i = atoi(arg);
		if(i < 0 || i >= WP_EMPLACED_GUN) {
			Disp(obj->holder, "^3Invalid weapon index.");
			return;
		}
		i = i << 1;
		if(obj->holder->client->ps.stats[STAT_WEAPONS] & (1 << i)) {
			Disp(obj->holder, "^3You already have this weapon.");
			return;
		}
		obj->holder->client->ps.stats[STAT_WEAPONS] |= (1 << i);
		data->weapons &= ~(1 << i);
		Disp(obj->holder, "^2Weapon aquired");
		Inventory_Player_Modify(obj->holder);
	}
	else {
		arg[0] = 0;
		c = 0;
		for(i = 0;i<WP_EMPLACED_GUN;i++) {
			if(!(data->weapons & (1 << i)))
				continue;
			if(getAll) {
				if(obj->holder->client->ps.stats[STAT_WEAPONS] & (1 << i))
					continue;
				obj->holder->client->ps.stats[STAT_WEAPONS] |= (1 << i);
				data->weapons &= ~(1 << i);
				c++;
			}
			else {
				Q_strcat(arg, sizeof(arg), va("^2%2i ^3%s\n", i, weaponNameFromIndex[i]));
			}
		}
		if(getAll) {
			Disp(obj->holder, va("^3Obtained ^2%i^3 item%s", c, (c != 1)?"s":""));
			Inventory_Player_Modify(obj->holder);
			if(data->weapons == 0)
				Inventory_DestroyObject(obj);
		}
		else {
			//Linefeed-remove-o-matic
			arg[strlen(arg) - 1] = 0;
			Disp(obj->holder, arg);
			Disp(obj->holder, "^3Specify the index of the weapon to get, or \'all\' for all weapons.");
		}
	}
}

void Item_Weaponpack_Spawn(iObject_t *obj){
	obj->describe = Item_Weaponpack_Describe;
	obj->use = Item_Weaponpack_Use;	
}

iObjectDef_t Item_Weaponpack = {
	"weaponpack",
	sizeof(Item_Weaponpack_Fields_t),
	Item_Weaponpack_Fields,
	Item_Weaponpack_Spawn,
	NULL
};

enum {
	APACK_BLASTER,
	APACK_POWERCELL,
	APACK_METAL_BOLTS,
	APACK_ROCKETS,
	APACK_THERMAL,
	APACK_TRIPMINE,
	APACK_DETPACK,
	APACK_MAX
};

typedef struct{
	char *name;
	int ammoIndex;
}ammoPackData_t;

ammoPackData_t AmmoPackData[APACK_MAX] = {
	{"Blaster pack", AMMO_BLASTER},
	{"Power cell", AMMO_POWERCELL},
	{"Metalic bolts", AMMO_METAL_BOLTS},
	{"Rockets", AMMO_ROCKETS},
	{"Thermals", AMMO_THERMAL},
	{"Trip mines", AMMO_TRIPMINE},
	{"Det packs", AMMO_DETPACK},

};

typedef struct Item_Ammopack_Fields_s{
	int ammo[APACK_MAX];
}Item_Ammopack_Fields_t;

BG_field_t Item_Ammopack_Fields[] = {
	{"blaster", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_BLASTER]), F_INT, COMBINE_ADD},
	{"powercell", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_POWERCELL]), F_INT, COMBINE_ADD},
	{"metalbolt", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_METAL_BOLTS]), F_INT, COMBINE_ADD},
	{"rocket", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_ROCKETS]), F_INT, COMBINE_ADD},
	{"thermal", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_THERMAL]), F_INT, COMBINE_ADD},
	{"tripmine", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_TRIPMINE]), F_INT, COMBINE_ADD},
	{"detpack", FOFFSET(Item_Ammopack_Fields_t, ammo[APACK_DETPACK]), F_INT, COMBINE_ADD},
	{NULL, 0, F_IGNORE},
};
//FIXME: if you dont have the weapon to use it, dont give the ammo?
void Item_Ammopack_Describe(iObject_t *obj, char *buf, unsigned int sze){
	Item_Ammopack_Fields_t *data = (Item_Ammopack_Fields_t *)obj->data;
	int i, c = 0;
	int s[4], si = 0;
	for(i = 0;i<APACK_MAX;i++) {
		if(data->ammo[i] > 0) {
			if(si < 4)
				s[si++] = i;
			c++;
		}
	}
	Q_strncpyz(buf, "^3Pack contains ", sze);
	for(i = 0;i<si;i++) {
		Q_strcat(buf, sze, va("^2%s^3, ", ammoNameFromIndex[AmmoPackData[s[i]].ammoIndex]));
	}
	if(c >= si) {
		Q_strcat(buf, sze, va("and ^2%i^3 other set%s.", c - si, (c - si != 1)?"s":""));
	}
}

//FIXME: this is messy, redo it.
int MaxAmmo(gentity_t *ent, int ammo);
void Item_Ammopack_Use(iObject_t *obj) {
	Item_Ammopack_Fields_t *data = (Item_Ammopack_Fields_t *)obj->data;
	int i, c = 0;
	char arg[MAX_STRING_CHARS];
	qboolean getAll = qfalse;
	qboolean isEmpty = qtrue;
	int s;
	int max;
	int ammoIndex;
	for(i = 0;i<AMMO_MAX;i++) {
		if(data->ammo[i] <= 0)
			continue;
		s = i;
		c++;
	}

	//inventory use <item> ...
	trap_Argv(3, arg, sizeof(arg));
	if(c == 1) {
		ammoIndex = AmmoPackData[s].ammoIndex;
		max = MaxAmmo(obj->holder, ammoIndex);
		if(obj->holder->client->ps.ammo[ammoIndex] >= max) {
			Disp(obj->holder, "^3You are already holding the maximum ammo for this type.");
			return;
		}
		i = max - obj->holder->client->ps.ammo[ammoIndex];
		if(i > data->ammo[ammoIndex])
			i = data->ammo[ammoIndex];
		obj->holder->client->ps.ammo[ammoIndex] = i;
		data->ammo[ammoIndex] -= i;
		Disp(obj->holder, va("^2%i^3 units of ammo aquired.", i));
		Inventory_Player_Modify(obj->holder);
		if(data->ammo[ammoIndex] == 0)
			Inventory_DestroyObject(obj);
		return;
	}
	if(Q_stricmp(arg, "all") == 0)
		getAll = qtrue;
	if(!getAll && arg[0]) {
		s = atoi(arg);
		if(s < 0 || s >= AMMO_MAX) {
			Disp(obj->holder, "^3Invalid Ammo index.");
			return;
		}
		ammoIndex = AmmoPackData[s].ammoIndex;
		max = MaxAmmo(obj->holder, ammoIndex);
		if(obj->holder->client->ps.ammo[ammoIndex] >= max) {
			Disp(obj->holder, "^3You are already holding the maximum ammo for this type.");
			return;
		}
		i = max - obj->holder->client->ps.ammo[ammoIndex];
		if(i > data->ammo[ammoIndex])
			i = data->ammo[ammoIndex];
		obj->holder->client->ps.ammo[ammoIndex] = i;
		data->ammo[ammoIndex] -= i;
		Disp(obj->holder, va("^2%i^3 unit%s of ammo aquired.", i, (i != 1)?"s":""));
		Inventory_Player_Modify(obj->holder);
		if(data->ammo[ammoIndex] == 0)
			Inventory_DestroyObject(obj);
		return;
	}
	else {
		arg[0] = 0;
		c = 0;
		for(s = 0;i<AMMO_MAX;s++) {
			ammoIndex = AmmoPackData[s].ammoIndex;
			if(data->ammo[ammoIndex] <= 0)
				return;
			if(getAll) {
				max = MaxAmmo(obj->holder, ammoIndex);
				if(obj->holder->client->ps.ammo[ammoIndex] >= max)
					continue;
				c++;
				i = max - obj->holder->client->ps.ammo[ammoIndex];
				if(i > data->ammo[ammoIndex])
					i = data->ammo[ammoIndex];
				obj->holder->client->ps.ammo[ammoIndex] = i;
				data->ammo[ammoIndex] -= i;
				if(data->ammo[ammoIndex] > 0)
					isEmpty = qfalse;
			}
			else {
				Q_strcat(arg, sizeof(arg), va("^2%2i ^3%s\n", i, ammoNameFromIndex[s + 1]));
			}
		}
		if(getAll) {
			Disp(obj->holder, va("^2%i^3 type%s of ammo aquired.", c, (c != 1)?"s":""));
			Inventory_Player_Modify(obj->holder);
			if(isEmpty)
				Inventory_DestroyObject(obj);
		}
		else {
			Disp(obj->holder, arg);
			Disp(obj->holder, "^3Specify the index of the ammo to get, or \'all\' for all ammo types.");
		}
	}
}

void Item_Ammopack_Spawn(iObject_t *obj){
	obj->describe = Item_Ammopack_Describe;
	obj->use = Item_Ammopack_Use;	
}

iObjectDef_t Item_Ammopack = {
	"ammopack",
	sizeof(Item_Ammopack_Fields_t),
	Item_Ammopack_Fields,
	Item_Ammopack_Spawn,
	NULL
};

iObjectDef_t *Inventory_Items_General[] = {
	&Item_Test,
	&Item_Medpack,
	&Item_Weaponpack,
	&Item_Ammopack,
	NULL
};

