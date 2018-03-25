
#ifdef LMD_NEW_INVENTORY_SYSTEM

#include "g_local.h"
#ifdef LMD_NEW_ACCOUNT_SYSTEM
#include "Lmd_Accounts_Core.h"
#else
#include "Lmd_AccountUtils.h"
#endif
#include "Lmd_Arrays.h"
#include "Lmd_Inventory_New.h"
#include "Lmd_PlayerActions.h"

extern iObjectDef_t Inventory_Items_General[];
extern iObjectDef_t Inventory_Items_Quest[];
iObjectCatagory_t catagories[] = {
	{"General", Inventory_Items_General},
	{"Quest", Inventory_Items_Quest},
	{NULL, NULL}
};

struct{
	unsigned int count;
	iObject_t **items;
}iObjects;

iObjectCatagory_t *Inventory_GetCatagory(char *name){
	iObjectCatagory_t *category = catagories;
	while(category->name){
		if(Q_stricmp(category->name, name) == 0)
			return category;
		category++;
	}
	return NULL;
}

iObjectDef_t *Inventory_GetObjectDef(char *name, iObjectCatagory_t **catagory){
	iObjectCatagory_t *search = catagories;
	iObjectDef_t *def;
	while(search->name){
		def = search->objects;
		while(def->name){
			if(Q_stricmp(def->name, name) == 0) {
				if(catagory)
					(*catagory) = search;
				return def;
			}
			def++;
		}
		search++;
	}
	return NULL;
}

iObject_t *Inventory_NewItem(iObjectDef_t *objDef) {
	iObject_t *obj = (iObject_t *)G_Alloc(sizeof(iObject_t));

	int i = Lmd_Arrays_AddArrayElement((void **)&iObjects.items,
		sizeof(iObject_t *), &iObjects.count);
	iObjects.items[i] = obj;

	obj->def = objDef;
	obj->data = G_Alloc(objDef->dataSize);
	//Init local name
	obj->name = objDef->name;
	//Init descr
	obj->descr = "";
	return obj;
}

void Inventory_SpawnItem(iObject_t *obj) {
	if(obj->def->spawn)
		obj->def->spawn(obj);
}

void Inventory_Think(){
	iObject_t *obj;
	int i;
	for(i = 0; i < iObjects.count; i++){
		obj = iObjects.items[i];
		if(obj->nextThink > 0 && obj->nextThink <= level.time){
			//Is this the right way to do this?  How do entities do it?
			obj->nextThink = 0;
			obj->think(obj);
		}	
	}
}

qboolean Inventory_LooseItem(iObject_t *obj, qboolean force);
qboolean Inventory_DestroyItem(iObject_t *obj){
	int index = -1, i;
	Inventory_LooseItem(obj, qtrue);
	for(i = 0; i < iObjects.count; i++){
		if(obj == iObjects.items[i]){
			index = i;
			break;
		}
	}
	assert(index != -1);
	if(index == -1)
		return qfalse; //should never come up

	Lmd_Arrays_RemoveArrayElement((void **)&iObjects.items, index, sizeof(iObject_t *), &iObjects.count);

	obj->def->free(obj);
	G_Free(obj->data);
	G_Free(obj);
	return qtrue;
}

qboolean Inventory_TryCombine(iObject_t *obj, iObject_t *targ){
	BG_field_t *field = obj->def->fields;
	//Check merge requirements
	qboolean canMerge = qfalse;
	while(field->name){
		if(field->flags != COMBINE_IGNORE)
			canMerge = qtrue;
		if(field->flags == COMBINE_MATCH){
			if(!Lmd_Database_CompareFields(field, (byte *)obj, (byte *)targ))
				return qfalse;
		}
		field++;
	}
	//We can combine, check aquire on target
	if(targ->combine && !targ->combine(targ, obj))
		return qfalse;
	field = obj->def->fields;
	byte *targD = (byte *)targ->data, *objD = (byte *)obj->data;
	while(field->name){
		if(field->flags == COMBINE_ADD){
			switch(field->type){
			case F_INT:
				*(int *)(targD+field->ofs) = *(int *)(targD+field->ofs) + *(int *)(objD+field->ofs);
				break;
			case F_FLOAT:
				*(float *)(targD+field->ofs) = *(float *)(targD+field->ofs) + *(float *)(objD+field->ofs);
				break;
			default:
				assert(!"Unhandled F_ type in COMBINE_ADD");
			}
		}
		else if(field->flags == COMBINE_REPLACE){
			Lmd_Database_CopyField(field, targD, objD);
		}
		field++;
	}
	//Can only merge if we have at least one non-ignore flag.
	return canMerge;
}

qboolean Inventory_CheckCombine(Account_t *acc, iObject_t *obj){
	int i;
	invData_t *data = INVDATA(player->client->Lmd.ammoRecharge
	for(i = 0; i < player->client->sess.Lmd.inventory.count; i++){
		if(player->client->sess.Lmd.inventory.items[i]->def != obj->def)
			continue;
		if(Inventory_TryCombine(obj, player->client->sess.Lmd.inventory.items[i]))
			return qtrue;
	}
	return qfalse;
}

qboolean Inventory_AquireItem(invData_t *data, iObject_t *obj) {
	if(Inventory_CheckCombine(acc, obj)){
		//We were combined, remove the item.
		Inventory_DestroyItem(obj);
		return qtrue;
	}

	int index = Lmd_Arrays_AddArrayElement((void **)&data->items, sizeof(iObject_t *), &data->count);
	data->items[index] = obj;
	obj->holder = acc;
	return qtrue;
}

qboolean Inventory_CanAquireItem(Account_t* acc, invData_t *data, iObject_t *obj){
	if(!Inventory_TryModify(acc))
		return qfalse;
	//What if we have a holder already?  Run LooseItem?  Assume it is taken care of?
	assert(!obj->holder);

	//Check for aquire before combining
	if(obj->aquire && obj->aquire(obj, acc) == qfalse)
		return qfalse;
	return qtrue;
}

/*==============================================================*\
Account
\*==============================================================*/
typedef struct invData_s{
	unsigned int count;
	iObject_t **items;
}invData_t;

qboolean Lmd_WriteDatastringField( BG_field_t *f, char *value, unsigned int sze, byte *b );
qboolean saveInventoryKey(void *data, int ofs, char *key, int keySze, char *value, int valSze){
	invData_t *d = (invData_t *)data;
	char data[MAX_STRING_CHARS];
	char buf[MAX_STRING_CHARS];
	
	if(ofs >= d->count)
		return qfalse;

	Q_strncpyz(key, "inventory", sizeof(key));
	iObject_t *obj = d->items[ofs];
	BG_field_t *field = obj->def->fields;
	while(field->name){
		if(Lmd_WriteDatastringField(field, buf, sizeof(buf), (byte *)obj->data))
			Q_strcat(data, sizeof(data), buf);
		field++;
	}

	return qtrue;
}

qboolean parseInventoryKey(char *key, char *value, void *data){
	if(Q_stricmp(key, "inventory") != 0)
		return qfalse;

	char item[MAX_STRING_CHARS];
	char data[MAX_STRING_CHARS];
	if(sscanf((const char *)value, "\"%1023[^\"]\" \"%1023[^\"]\"", item, data) != 2) {
		int creator = 0;
		if(sscanf((const char *)value, "\"%1023[^\"]\" %i {%1023[^}]}", item, &creator, data) < 3)
			return qtrue;
	}

	iObjectDef_t *def = Inventory_GetObjectDef(item, NULL);
	if(!def)
		return qtrue;
	//I could theoretically parse the data into the object's data field to
	//test for combineability and only create the iObject if needed, but
	//since that will happen on trade and all other cases, I will leave it.
	//Code to merge will be handled in Inventory_AquireItem.
	iObject_t *obj = Inventory_NewItem(def);
	char *str = data;
	Lmd_ParseDatastring(&str, NULL, def->fields, (byte *)obj->data);
	Inventory_SpawnItem(obj);
	if(Inventory_CanAquireItem(acc, obj)) {
		Inventory_AquireItem((invData_t *)data, obj);
	}
	else{
		Inventory_DestroyItem(obj);
	}
	return qtrue;
}

void freeInventoryData(void *data){
	invData_t *invData = (invData_t *)data;
	int i;
	for(i = 0; i < invData->count; i++) {
		Inventory_DestroyItem(invData->items[i]);
	}
	Lmd_Arrays_RemoveAllElements(invData->items);
}

accDataCatagory_t Accounts_Inventory = {
	NULL,
	0,
	sizeof(invData_t),
	parseInventoryKey,
	saveInventoryKey,
	freeInventoryData
};

#define INVDATA(acc) (invData_t *)Accounts_GetCatagoryData(acc, &Accounts_Inventory)

void Accounts_Modify(Account_t *acc);
void Inventory_Modify(Account_t *acc) {
	Accounts_Modify(acct);
}

qboolean Inventory_DeleteItem(iObject_t *obj){
	if(!Inventory_LooseItem(obj))
		return qfalse;
	if(obj->del && !obj->del(obj))
		return qfalse;
	return Inventory_DestroyItem(obj);
}

qboolean Inventory_LooseItem(iObject_t *obj, qboolean force){
	if(obj->holder) {
		invData_t *data = INVDATA(obj->holder);
		int index = -1, i;
		Inventory_Modify(obj->holder);
		if(force)
			obj->loose(obj, qtrue)
		else if(obj->loose && !obj->loose(obj, qfalse))
			return qfalse;
		for(i = 0; i < data->count; i++){
			if(data->items[i] == obj){
				index = i;
				break;
			}
		}
		assert(index != -1);
		if(index == -1)
			return qfalse; //should never come up
		Lmd_Arrays_RemoveArrayElement((void **)&data->items, index, sizeof(iObject_t *), &data->count);
		obj->holder = NULL;
	}
	return qtrue;
}

void Cmd_Inventory_ListCatagory(gentity_t *ent, iObjectCatagory_t *category, iObjectDef_t *type, int *offset, int *count){
	int i;
	iObject_t *obj;
	for(i = 0; i < ent->client->sess.Lmd.inventory.count; i++){
		obj = ent->client->sess.Lmd.inventory.items[i];
		if(type && obj->def != type)
			continue;
		if((*offset) > 0){
			(*offset)--;
			continue;
		}
		Disp(ent, va("^5%-5i    ^2%-25s ^3%s", i, obj->name, obj->descr));
		(*count)++;
	}
}

void Cmd_Inventory_List(gentity_t *ent, int argnum){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	int count = 25;
	int offset = 0;
	iObjectCatagory_t *category = NULL;
	iObjectDef_t *type = NULL;
	if(arg[0]){
		offset = atoi(arg);
		if(offset < 0)
			offset = 0;
		if(offset == 0){
			category = Inventory_GetCatagory(arg);
			if(category == NULL)
				type = Inventory_GetObjectDef(arg, &category);
			trap_Argv(argnum + 1, arg, sizeof(arg));
			offset = atoi(arg);
		}
	}
	if(offset < 0)
		offset = 0;
	if(category) {
		Disp(ent, "^5Index    ^2Name                     ^3Description");
		Cmd_Inventory_ListCatagory(ent, category, type, &offset, &count);
	}
	else{
		category = catagories;
		while(category->name){
			Disp(ent, va("^5%s", category->name));
			Disp(ent, "^5Index    ^2Name                     ^3Description");
			Cmd_Inventory_ListCatagory(ent, category, type, &offset, &count);
			category++;
		}
	}		
}

void Cmd_Inventory_Use(gentity_t *ent, int argnum){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	int num = atoi(arg);
	iObject_t *obj;
	if(num != 0 || (arg[0] == '0' && arg[1] == 0)){
		if(num < 0 || num >= ent->client->sess.Lmd.inventory.count){
			Disp(ent, "^Invalid index.");
			return;
		}
		obj =  ent->client->sess.Lmd.inventory.items[num];
	}
	else{
		iObjectDef_t *type = Inventory_GetObjectDef(arg, NULL);
		if(!type){
			Disp(ent, "^3Unknown type.");
			return;
		}
		int i;
		for(i = 0; i < ent->client->sess.Lmd.inventory.count; i++){
			if(ent->client->sess.Lmd.inventory.items[i]->def == type){
				obj = ent->client->sess.Lmd.inventory.items[i];
				break;
			}
		}
		if(!obj){
			Disp(ent, "^3Unknown type.");
			return;
		}
	}
	if(!obj->use){
		Disp(ent, "^3This item is currently not usable.");
	}
	else{
		obj->use(obj);
		Disp(ent, "^2Item used.");
	}
}

qboolean Cmd_Inventory_Delete_Item_Confirm(gentity_t *ent, Action_t *action){
	iObject_t *obj = ent->client->sess.Lmd.inventory.items[action->iArgs[0]];
	if(obj != (iObject_t *)action->iArgs[1]){
		Disp(ent, "^3The specified item is no longer at that index, perhapse it was traded or deleted?");
		return qtrue;
	}
	if(Inventory_DeleteItem(obj))
		Disp(ent, "^2Item deleted.");
	else
		Disp(ent, "^3Item could not be deleted.");
	return qtrue;
}

qboolean Cmd_Inventory_Delete_Type_Confirm(gentity_t *ent, Action_t *action){
	int i, c = 0;
	for(i = 0; i < ent->client->sess.Lmd.inventory.count; i++){
		if(Q_stricmp(ent->client->sess.Lmd.inventory.items[i]->name, action->strArgs[0]) == 0){
			if(Inventory_DeleteItem(ent->client->sess.Lmd.inventory.items[i])) {
				c++;
				i--;
			}
		}
	}
	G_Free(action->strArgs[0]);
	Disp(ent, va("^3%i ^2item%s deleted.", c, (c != 1)?"s":""));
	return qtrue;
}


void Cmd_Inventory_Delete(gentity_t *ent, int argnum){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	int num = atoi(arg);
	if(num != 0 || (arg[0] == '0' && arg[1] == 0)){
		if(num < 0 || num >= ent->client->sess.Lmd.inventory.count){
			Disp(ent, "^3Invalid index.");
			return;
		}
		Action_t *action = PlayerActions_Add(ent, "itemdelete", 
			va("^3Delete inventory index ^2%i^3 (^2%s^3)", num, 
			ent->client->sess.Lmd.inventory.items[num]->name), 
			Cmd_Inventory_Delete_Item_Confirm, qtrue);
		action->iArgs[0] = num;
		action->iArgs[1] = (int)ent->client->sess.Lmd.inventory.items[num];
	}
	else{
		iObjectDef_t *type = Inventory_GetObjectDef(arg, NULL);
		if(!type){
			Disp(ent, "^3Unknown type.");
			return;
		}
		int i, c = 0;
		for(i = 0; i < ent->client->sess.Lmd.inventory.count; i++){
			if(ent->client->sess.Lmd.inventory.items[i]->def == type){
				c++;
			}
		}
		if(c == 0){
			Disp(ent, "^3Unknown type.");
			return;
		}
		Action_t *action = PlayerActions_Add(ent, "itemdelete", 
			va("^3Delete all items of type ^2%s^3 (^2%i^3 item%s)", 
			ent->client->sess.Lmd.inventory.items[num]->name, c, (c != 1)?"s":""), 
			Cmd_Inventory_Delete_Type_Confirm, qtrue);
		action->strArgs[0] = G_NewString(ent->client->sess.Lmd.inventory.items[num]->name);
	}
}

void Cmd_Inventory_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(argc < 2){
		//destroy <type> destroys all of type, with confirmation.
		Disp(ent, "^3Usage: Inventory {list ^5[catagory/type] [offset]^3} {use ^2<Index/type>^3} {destroy ^2<Index/type>^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "list") == 0){
		Cmd_Inventory_List(ent, 2);
		return;
	}
	else if(Q_stricmp(arg, "use") == 0){
		Cmd_Inventory_Use(ent, 2);
		return;
	}
	else if(Q_stricmp(arg, "destroy") == 0){
		if(argc < 2){
			Disp(ent, "^3Usage: /inventory destroy ^2<index or type>");
			return;
		}
		Cmd_Inventory_Delete(ent, 2);
		return;
	}
	else
		Disp(ent, "^3Unknown argument");
}

void GenerateInvDocs() {
	fileHandle_t f;
	trap_FS_FOpenFile("docs/inven.txt", &f, FS_WRITE);
	iObjectCatagory_t *catagory = catagories;
	iObjectDef_t *def;
	BG_field_t *field;
	char *str;
	while(catagory->name) {
		str = va("Catagory: %s\n\n\n", catagory->name);
		trap_FS_Write(str, strlen(str), f);
		def = catagory->objects;
		while(def->name) {
			str = va("Name: %s\nFields:\n", def->name);
			trap_FS_Write(str, strlen(str), f);
			field = def->fields;
			while(field->name) {
				str = va("%s\n", field->name);
				trap_FS_Write(str, strlen(str), f);
				field++;
			}
			str = "\n";
			trap_FS_Write(str, strlen(str), f);
			def++;
		}
		str = "\n\n\n";
		trap_FS_Write(str, strlen(str), f);
		catagory++;
	}
	trap_FS_FCloseFile(f);
}


#ifdef LMD_NEW_ACCOUNT_SYSTEM

#error TODO: Account inventory

#endif

#endif
