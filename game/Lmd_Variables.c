
#ifdef LMD_EXPERIMENTAL

#include "g_local.h"
#include "Lmd_Data.h"
#include "Lmd_KeyPairs.h"

/*
Temp vars are not saved, cleared on mapfile change
Local vars are per-mapfile
Global vars are per-server (data folder)

All vars are key/value text pairs
*/

#define VARIABLE_SAVE_DELAY 1000

typedef struct variables_s {
	char *name;
	KeyPairSet_t pairs;
	int firstModifyTime;
}variables_t;

variables_t temps;
variables_t locals; //mapdata/maps/<map>/variables/<file>.lmv
variables_t globals; //mapdata/globals/variables/<file>.lmv

#define VARIABLE_EXT "lmv"


DBSaveFileCallbackReturn_t* Variables_Write(byte* data, DBSaveFileCallbackReturn_t *arg, char *key, int keySze, char *value, int valSze) {
	KeyPairSet_t *set = (KeyPairSet_t *)data;
	if(arg->offset >= set->count)
		return NULL;
	Q_strncpyz(key, set->pairs[arg->offset].key, keySze);
	Q_strncpyz(value, set->pairs[arg->offset].value, valSze);
	return arg;
}

void Variables_TrySave(qboolean force){
	if(locals.firstModifyTime > 0 && (force || locals.firstModifyTime > level.time + VARIABLE_SAVE_DELAY))
		Lmd_Data_SaveDatafile(va("mapdata/maps/%s/variables", level.rawmapname), va("%s."VARIABLE_EXT, locals.name), NULL, (byte *)&locals.pairs, NULL, Variables_Write);
	if(globals.firstModifyTime > 0 && (force || globals.firstModifyTime > level.time + VARIABLE_SAVE_DELAY))
		Lmd_Data_SaveDatafile("mapdata/globals/variables", va("%s."VARIABLE_EXT, globals.name), NULL, (byte *)&globals.pairs, NULL, Variables_Write);
}

qboolean Variables_Parse(byte *obj, qboolean pre, char *key, char *value){
	KeyPairSet_t *set = (KeyPairSet_t *)obj;
	Lmd_Pairs_New(set, key, value);
	return qtrue;
}

void Variables_Load(){
	char *data, *buf;
	
	//Save the existing variables, if needed.
	Variables_TrySave(qtrue);

	//For future extention
	locals.name = "default";
	globals.name = "default";

	data = buf = Lmd_Data_AllocFileContents(va("mapdata/maps/%s/variables/%s."VARIABLE_EXT, level.rawmapname, locals.name));
	Lmd_Data_ParseFields_Old(&buf, qtrue, Variables_Parse, NULL, (byte *)&locals.pairs);
	G_Free(data);

	data = buf = Lmd_Data_AllocFileContents(va("mapdata/globals/variables/%s."VARIABLE_EXT, globals.name));
	Lmd_Data_ParseFields_Old(&buf, qtrue, Variables_Parse, NULL, (byte *)&globals.pairs);
	G_Free(data);
}

#endif