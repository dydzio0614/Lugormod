
#include "Lmd_Data.h"

#ifndef LMD_INVENTORY_NEW_H
#define LMD_INVENTORY_NEW_H

typedef struct iObject_s iObject_t;

enum {
	COMBINE_IGNORE,
	COMBINE_MATCH, //if this field does not match, then it cannot be combined.
	COMBINE_ADD,
	COMBINE_REPLACE,
};

typedef struct iObjectDef_s{
	char *name;
	unsigned int dataSize;
	BG_field_t *fields; //->flag will mark a combinable field with COMBINE_ enum.
	void (*spawnObj)(iObject_t *obj);
	void (*freeObj)(iObject_t *obj);
}iObjectDef_t;

typedef struct iObectFields_s{
	char *name;
	qboolean noCombine;
	qboolean noDelete;
	qboolean noGive;
}iObjectFields_t;

 struct iObject_s{
	iObjectDef_t *def;
	gentity_t *holder; //If dropped, then drop ent.
	iObjectFields_t fields;
	void *data;
	int nextThink;
	void (*think)(iObject_t *obj);
	qboolean (*aquire)(iObject_t *obj, gentity_t *player);
	qboolean (*combine)(iObject_t *obj, iObject_t *comb);
	qboolean (*loose)(iObject_t *obj);
	qboolean (*give)(iObject_t *obj, gentity_t *ent);
	qboolean (*del)(iObject_t *obj);
	void (*use)(iObject_t *obj);
	void (*describe)(iObject_t *obj, char *descr, unsigned int sze);
};

typedef struct iObjectCatagory_s{
	char *name;
	iObjectDef_t **objects;
}iObjectCatagory_t;

typedef struct iObjectList_s{
	unsigned int count;
	iObject_t **objects;
}iObjectList_t;

qboolean Inventory_DestroyObject(iObject_t *obj);

void Inventory_Player_Modify(gentity_t *player);
iObjectList_t *Inventory_Player_GetInventory(gentity_t *player);
#endif

