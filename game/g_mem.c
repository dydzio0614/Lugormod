

#include "g_local.h"


#include "Lmd_Arrays.h"

#define LMD_USE_MALLOC

typedef struct MemData_s{
	void *ptr;
	int size;
}MemData_t;

unsigned int memCount;
MemData_t *Memory = NULL;

//RoboPhred: strange stuff is happening, suspect memory asplodery.
#define LMD_USE_MALLOC

//#define LMD_MEMORY_COUNT

#ifdef LMD_MEMORY_COUNT
unsigned int memcount = 0;
#endif

int Lmd_Memory_GetIndex(void *ptr) {
	int i;
	for(i = 0;i < memCount; i++){
		if(ptr == Memory[i].ptr){
			return i;
		}
	}
	return -1;
}

int Lmd_Memory_AddIndex(int sze) {
	int index = Lmd_Arrays_AddArrayElement((void **)&Memory, sizeof(MemData_t), &memCount);
	Memory[index].size = sze;
	return index;
}

void Lmd_Memory_RemoveIndex(int index) {
	Lmd_Arrays_RemoveArrayElement((void **)&Memory, index, sizeof(MemData_t), &memCount);
}

MemData_t *U1_Memory_Add(int size){
	int index = Lmd_Memory_AddIndex(size);

#ifdef LMD_USE_MALLOC
	Memory[index].ptr = malloc(size);
#else
	trap_TrueMalloc(&Memory[index].ptr, Memory[index].size);
#endif
	if(!Memory[index].ptr)
		G_Error("G_Alloc: Failed to alloc size %i", size);

#ifdef LMD_MEMORY_COUNT
	memcount += size;
	Com_Printf("^1MEM Alloc: %u bytes\n", memcount);
#endif

	memset(Memory[index].ptr, 0, size);
return &Memory[index];
};

void U1_Memory_Remove(void *ptr){
	int index = Lmd_Memory_GetIndex(ptr);
	if(index == -1)
		return;

#ifdef LMD_USE_MALLOC
	free(Memory[index].ptr);
#else
	trap_TrueFree(&Memory[index].ptr);
#endif

#ifdef LMD_MEMORY_COUNT
	memcount -= Memory[index].size;
	Com_Printf("^1MEM Free: %u bytes\n", memcount);
#endif
	Lmd_Memory_RemoveIndex(index);
}

#ifndef __linux__
#ifdef LMD_MEMORY_DEBUG
void* G_AllocDebug(int sze, char *file, int line) {
	int i = Lmd_Memory_AddIndex(sze);
	Memory[i].ptr = _malloc_dbg(sze, _NORMAL_BLOCK, file, line);
	memset(Memory[i].ptr, 0, sze);
	return Memory[i].ptr;
}
#else
void *G_Alloc(int sze) {
	if(sze <= 0)
		return NULL;
	return U1_Memory_Add(sze)->ptr;
}
#endif

#ifdef LMD_MEMORY_DEBUG
void G_Free(void *ptr){
	int i = Lmd_Memory_GetIndex(ptr);
	if(i == -1)
		return;
	_free_dbg(ptr, _NORMAL_BLOCK);
	Lmd_Memory_RemoveIndex(i);
}
#else
void G_Free(void *ptr){
	if(!ptr)
		return;
	U1_Memory_Remove(ptr);
}
#endif

void G_InitMemory( void ) {
	//these are not cleared on map_restart
	/*yes they are, the dll is reloaded
	Lmd_Arrays_RemoveAllElements((void **)&Memory);
	memCount = 0;
	*/
#ifdef LMD_MEMORY_DEBUG
	/*
        int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		//tmpDbgFlag |= _CRTDBG_CHECK_EVERY_16_DF;
        tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
        tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
		tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
		tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
		tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
        _CrtSetDbgFlag(tmpDbgFlag);
	*/
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
#endif
}

void Svcmd_GameMem_f( void ) {
	unsigned int sze = 0;
	unsigned int i;
	for(i = 0;i<memCount;i++){
		sze += Memory[i].size;
	}
	G_Printf("^2%u bytes alloced in %u units\n", sze, memCount);
}
#endif //__linux__
