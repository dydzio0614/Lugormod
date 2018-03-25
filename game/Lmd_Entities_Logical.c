

#include "g_local.h"
#include "Lmd_Arrays.h"

gentity_t **logicEnts = NULL;
unsigned int logicCount = 0;

inline int getFreeSlot(){
	int i;
	for(i = 0; i < logicCount; i++){
		if(logicEnts[i]->inuse == qfalse){
			return i;
		}
	}
	return -1;
}

gentity_t *Lmd_logic_spawn(){
	gentity_t *ent;
	int index = getFreeSlot();
	if(index < 0) {
		index = (int)Lmd_Arrays_AddArrayElement((void **)&logicEnts, sizeof(gentity_t *), &logicCount);
	}
	ent = logicEnts[index] = (gentity_t *)G_Alloc(sizeof(gentity_t));
	G_InitGentity(ent);
	assert(ent->s.number > 0);
	return ent;
}

void Lmd_logic_think(){
	int i;
	for(i = 0; i < logicCount; i++) {
		if(!logicEnts[i]->inuse)
			continue;
		G_RunThink(logicEnts[i]);
	}
}

int Lmd_logic_count(){
	return logicCount;
}

gentity_t *Lmd_logic_entity(int index) {
	if(index < 0 || index >= logicCount)
		return NULL;
	return logicEnts[index];
}

int Lmd_logic_entityindex(gentity_t *ent) {
	//Find the index from the ent's position in the list.
	int i;
	for(i = 0; i < logicCount; i++) {
		if(logicEnts[i] == ent)
			return i;
	}
	return -1;
}

