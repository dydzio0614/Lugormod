

/*
TODO:
"task" item.  Entities can change its description.  Ents reference it by name.
Or make a seperate quest system?
*/

#include "g_local.h"
#include "Lmd_Inventory.h"

typedef struct Item_UpCount_Fields_s{
	char *prop;
	int max;
	int count;
	int noAutoDelete;
}Item_UpCount_Fields_t;

BG_field_t Item_UpCount_Fields[] = {
	{"property", FOFFSET(Item_UpCount_Fields_t, prop), F_GSTRING, COMBINE_MATCH},
	{"max", FOFFSET(Item_UpCount_Fields_t, max), F_INT},
	{"count", FOFFSET(Item_UpCount_Fields_t, count), F_INT}, //COMBINE_ADD
	{"noAutoDelete", FOFFSET(Item_UpCount_Fields_t, noAutoDelete), F_INT}, //COMBINE_ADD
	{NULL, 0, F_IGNORE},
};

void Item_UpCount_Describe(iObject_t *obj, char *buf, unsigned int sze){
	Item_UpCount_Fields_t *data = (Item_UpCount_Fields_t *)obj->data;
	Q_strncpyz(buf, va("^2%i^3 out of ^2%i^3 obtained.", data->count, data->max), sze);
}

qboolean Item_UpCount_Combine(iObject_t *obj, iObject_t *comb) {
	Item_UpCount_Fields_t *data = (Item_UpCount_Fields_t *)obj->data;
	Item_UpCount_Fields_t *combD = (Item_UpCount_Fields_t *)comb->data;
	if(data->count >= data->max)
		return qfalse;
	data->count += combD->count;
	if(data->count > data->max)
		data->count = data->max;
	return qtrue;
}

void Item_UpCount_Spawn(iObject_t *obj){
	Item_UpCount_Fields_t *data = (Item_UpCount_Fields_t *)obj->data;
	obj->describe = Item_UpCount_Describe;
	//Should combine be core def?
	obj->combine = Item_UpCount_Combine;
	if(data->count == 0)
		data->count = 1;
}

iObjectDef_t Item_UpCount = {
	"upcount",
	sizeof(Item_UpCount_Fields_t),
	Item_UpCount_Fields,
	Item_UpCount_Spawn,
	NULL
};

qboolean Inventory_Quest_UpCount_CheckAccess(gentity_t *player, char *prop) {
	int i;
	iObjectList_t *inventory = Inventory_Player_GetInventory(player);
	if(!inventory)
		return qfalse;
	iObject_t *obj;
	Item_UpCount_Fields_t *data;
	for(i = 0; i < inventory->count; i++) {
		obj = inventory->objects[i];
		if(obj->def != &Item_UpCount)
			continue;
		data = (Item_UpCount_Fields_t *)obj->data;
		if(Q_stricmp(data->prop, prop) != 0)
			continue;
		if(data->count >= data->max) {
			if(!data->noAutoDelete) {
				Inventory_DestroyObject(obj);
			}
			return qtrue;
		}
	}
	return qfalse;
}


typedef struct Item_DownCount_Fields_s{
	char *prop;
	int max;
	int count;
	int noAutoDelete;
}Item_DownCount_Fields_t;

BG_field_t Item_Keycard_Fields[] = {
	{"property", FOFFSET(Item_DownCount_Fields_t, prop), F_GSTRING, COMBINE_MATCH},
	{"maxentries", FOFFSET(Item_DownCount_Fields_t, max), F_INT},
	{"entrycount", FOFFSET(Item_DownCount_Fields_t, count), F_INT},
	{NULL, 0, F_IGNORE},
};

BG_field_t Item_DownCount_Fields[] = {
	{"property", FOFFSET(Item_DownCount_Fields_t, prop), F_GSTRING, COMBINE_MATCH},
	{"max", FOFFSET(Item_DownCount_Fields_t, max), F_INT},
	{"count", FOFFSET(Item_DownCount_Fields_t, count), F_INT},
	{"noAutoDelete", FOFFSET(Item_DownCount_Fields_t, noAutoDelete), F_INT},
	{NULL, 0, F_IGNORE},
};

void Item_DownCount_Describe(iObject_t *obj, char *buf, unsigned int sze) {
	Item_DownCount_Fields_t *data = (Item_DownCount_Fields_t *)obj->data;
	if(data->count == 0) {
		Q_strncpyz(buf, "^3No more uses.", sze);
	}
	else if(data->max == 1) {
		Q_strncpyz(buf, "^3Single use.", sze);
	}
	else if(data->count == -1) {
		Q_strncpyz(buf, "^5Infinite uses.", sze);
	}
	else
		Q_strncpyz(buf, va("^2%i^3 use%s left.", data->count, (data->count != 1) ? "s" : ""), sze);
}

qboolean Item_DownCount_Combine(iObject_t *obj, iObject_t *comb) {
	Item_DownCount_Fields_t *data = (Item_DownCount_Fields_t *)obj->data;
	Item_DownCount_Fields_t *combD = (Item_DownCount_Fields_t *)comb->data;
	if(data->count == -1)
		return qtrue;
	if(combD->count == -1) {
		data->count = -1;
		return qtrue;
	}

	if (Q_stricmp(data->prop, combD->prop) != 0) {
		return qfalse;
	}

	if(data->max > 0 && data->count + combD->count > data->max)
		return qfalse;
	data->count += combD->count;
	return qtrue;
}

void Item_DownCount_Spawn(iObject_t *obj);

iObjectDef_t Item_Keycard = {
	"keycard",
	sizeof(Item_DownCount_Fields_t),
	Item_Keycard_Fields,
	Item_DownCount_Spawn,
	NULL
};

iObjectDef_t Item_DownCount = {
	"downcount",
	sizeof(Item_DownCount_Fields_t),
	Item_DownCount_Fields,
	Item_DownCount_Spawn,
	NULL
};

void Item_DownCount_Spawn(iObject_t *obj){
	Item_DownCount_Fields_t *data = (Item_DownCount_Fields_t *)obj->data;
	obj->describe = Item_DownCount_Describe;
	//Should combine be core def?
	obj->combine = Item_DownCount_Combine;

	if(obj->def == &Item_Keycard) {
		if(data->max)
			data->count = data->max - data->count;
		else
			data->count = -1;
		data->max = 0;
		obj->def = &Item_DownCount;
	}
	else if(data->count == 0)
		data->count = data->max;
	else if(data->count < -1)
		data->count = -1;
}

qboolean Inventory_Quest_DownCount_CheckAccess(gentity_t *player, char *prop) {
	int i;
	iObjectList_t *inventory = Inventory_Player_GetInventory(player);
	if(!inventory)
		return qfalse;
	iObject_t *obj;
	Item_DownCount_Fields_t *data;
	for(i = 0; i < inventory->count; i++) {
		obj = inventory->objects[i];
		if(obj->def != &Item_DownCount)
			continue;
		data = (Item_DownCount_Fields_t *)obj->data;
		if(Q_stricmp(data->prop, prop) != 0)
			continue;
		if(data->count > 0) {
			data->count--;
			if(data->count == 0) {
				if(!data->noAutoDelete) {
					Inventory_DestroyObject(obj);
				}
			}
			Inventory_Player_Modify(player);
			return qtrue;
		}
		else if(data->count == -1)
			return qtrue;
	}
	return qfalse;
}

iObjectDef_t *Inventory_Items_Quest[] = {
	&Item_UpCount,
	&Item_Keycard,
	&Item_DownCount,
	NULL
};

qboolean Inventory_Quest_CheckAccess(gentity_t *player, char *prop) {
	return Inventory_Quest_DownCount_CheckAccess(player, prop) || 
		Inventory_Quest_UpCount_CheckAccess(player, prop);
}

