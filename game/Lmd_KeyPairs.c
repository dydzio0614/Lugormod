
#include "g_local.h"

#include "Lmd_KeyPairs.h"
#include "Lmd_Arrays.h"

#ifdef LMD_MEMORY_DEBUG
void Lmd_Pairs_ParseDatastring_dbg(KeyPairSet_t *set, char *str, char *file, int line) {
#else
void Lmd_Pairs_ParseDatastring(KeyPairSet_t *set, char *str) {
#endif
	qboolean inQuotes = qfalse;
	char *end;
	unsigned int index;
	char buf[MAX_STRING_CHARS];
	int bufPos;
	qboolean inKey = qtrue;
	while(str[0]) {
		if(inKey) {
#ifdef LMD_MEMORY_DEBUG
			index = Lmd_Arrays_AddArrayElement_dbg((void **)&set->pairs, sizeof(KeyPair_t), &set->count, file, line);
#else
			index = Lmd_Arrays_AddArrayElement((void **)&set->pairs, sizeof(KeyPair_t), &set->count);
#endif
		}
		end = str;
		bufPos = 0;
		while(end[0] && (inQuotes || end[0] != ',')){
			if(end[0] == '\"')
				inQuotes = !inQuotes;
			buf[bufPos++] = end[0];
			end++;
			if(bufPos == MAX_STRING_CHARS - 1) {
				//too long, skip the rest.
				end = strchr(end, ',');
				break;
			}
		}
		buf[bufPos] = 0;
		if(inKey)
			set->pairs[index].key = G_NewString2(buf);
		else
			set->pairs[index].value = G_NewString2(buf);
		inKey = !inKey;
		if(!end[0])
			break;
		str = end + 1;
	}
	//Check to see if we have an unset value left over
	//I feel this should be left out, since if we dont have a value we should be NULL,
	//The entity parser needs a value though, so if this isnt here, a check needs to be added
	//at AddSpawnField or G_AddSpawnVarToken
	//Also, the entity save code relies on having a value set here, else
	//it will convert to "(null)" on saving.
	//To be consistant, should I add a similar check to Lmd_Pairs_New?
	if(!inKey)
		set->pairs[index].value = G_NewString2("");
}

char* Lmd_Pairs_ToDatastring(KeyPairSet_t *set){
	int i;
	int sze = 0;
	char *str;
	qboolean needQuotes = qfalse;

	//Calc the length
	for(i = 0; i < set->count; i++) {
		if(strchr(set->pairs[i].key, ','))
			sze += 2;
		sze += strlen(set->pairs[i].key) + 1; //+1 for comma

		if(strchr(set->pairs[i].value, ','))
			sze += 2;
		sze += strlen(set->pairs[i].value);
	}
	sze += set->count - 1; //for ending commas.
	sze += 1; //null char
	str = (char *)G_Alloc(sze);

	for(i = 0;i<set->count;i++) {
		//Key
		if(strchr(set->pairs[i].key, ',')){
			needQuotes = qtrue;
			Q_strcat(str, sze, "\"");
		}
		Q_strcat(str, sze, set->pairs[i].key);
		Q_strcat(str, sze, ",");
		if(needQuotes)
			Q_strcat(str, sze, "\"");
		needQuotes = qfalse;

		//Value
		if(strchr(set->pairs[i].value, ',')){
			needQuotes = qtrue;
			Q_strcat(str, sze, "\"");
		}
		Q_strcat(str, sze, set->pairs[i].value);
		if(needQuotes)
			Q_strcat(str, sze, "\"");
		needQuotes = qfalse;

		if(i < set->count - 1)
			Q_strcat(str, sze, ",");
	}
	return str;
}

#ifdef LMD_MEMORY_DEBUG
unsigned int Lmd_Pairs_New_dbg(KeyPairSet_t *set, char *key, char *value, char *file, int line) {
#else
unsigned int Lmd_Pairs_New(KeyPairSet_t *set, char *key, char *value) {
#endif

#ifdef LMD_MEMORY_DEBUG
	unsigned int index = Lmd_Arrays_AddArrayElement_dbg((void **)&set->pairs, sizeof(KeyPair_t), &set->count, file, line);
#else
	unsigned int index = Lmd_Arrays_AddArrayElement((void **)&set->pairs, sizeof(KeyPair_t), &set->count);
#endif
	set->pairs[index].key = G_NewString2(key);
	set->pairs[index].value = G_NewString2(value);
	return index;
}

#ifdef LMD_MEMORY_DEBUG
void Lmd_Pairs_Remove_dbg(KeyPairSet_t *set, int index, char *file, int line) {
#else
void Lmd_Pairs_Remove(KeyPairSet_t *set, int index) {
#endif
	if(set->pairs[index].key)
		G_Free(set->pairs[index].key);
	if(set->pairs[index].value)
		G_Free(set->pairs[index].value);

#ifdef LMD_MEMORY_DEBUG
	Lmd_Arrays_RemoveArrayElement_dbg((void **)&set->pairs, index, sizeof(KeyPair_t), &set->count, file, line);
#else
	Lmd_Arrays_RemoveArrayElement((void **)&set->pairs, index, sizeof(KeyPair_t), &set->count);
#endif
}

void Lmd_Pairs_Clear(KeyPairSet_t *set){
	int i;

	for(i = 0; i < set->count; i++){
		if(set->pairs[i].key)
			G_Free(set->pairs[i].key);
		if(set->pairs[i].value)
			G_Free(set->pairs[i].value);
	}
	Lmd_Arrays_RemoveAllElements((void **)&set->pairs);
	set->count = 0;
}

int Lmd_Pairs_FindKey(KeyPairSet_t *set, char *key){
	int i;
	for(i = 0;i<set->count;i++) {
		if(Q_stricmp(set->pairs[i].key, key) == 0)
			return i;
	}
	return -1;
}

char* Lmd_Pairs_GetKey(KeyPairSet_t *set, char *key) {
	int i = Lmd_Pairs_FindKey(set, key);
	if(i >= 0)
		return set->pairs[i].value;
	return NULL;
}

void Lmd_Pairs_SetKey(KeyPairSet_t *set, char *key, char *value) {
	int index = Lmd_Pairs_FindKey(set, key);
	if(index >= 0) {
		G_Free(set->pairs[index].value);
		set->pairs[index].value = G_NewString2(value);
	}
	else {
		Lmd_Pairs_New(set, key, value);
	}
}

void Lmd_Pairs_Merge(KeyPairSet_t *base, KeyPairSet_t *add) {
	unsigned int i;
	for(i = 0; i < add->count; i++) {
		Lmd_Pairs_SetKey(base, add->pairs[i].key, add->pairs[i].value);
	}
}

