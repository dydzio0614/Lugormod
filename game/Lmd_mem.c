

#include "g_local.h"


#include "Lmd_Arrays.h"

#define LMD_USE_MALLOC
#define LMD_LINKLIST_MEM

#ifdef LMD_LINKLIST_MEM

// Should track down some memory leaks by remembering previous allocations and their source.

typedef struct MemAlloc_s{
	void *ptr;
	int size;
	struct MemAlloc_s *next;
}MemAlloc_t;

MemAlloc_t *firstMem;

MemAlloc_t* Mem_NewPtr() {
	MemAlloc_t *mem;
#ifdef LMD_MEMORY_DEBUG
	mem = (MemAlloc_t*)_malloc_dbg(sizeof(MemAlloc_t), _NORMAL_BLOCK, __FILE__, __LINE__);
#else
	mem = (MemAlloc_t*)malloc(sizeof(MemAlloc_t));
#endif

	memset(mem, 0, sizeof(MemAlloc_t));

	if (firstMem == NULL) {
		firstMem = mem;
	}
	else {
		mem->next = firstMem;
		firstMem = mem;
	}

	return mem;
}

#ifdef LMD_MEMORY_DEBUG
void* G_AllocDebug(int sze, char *file, int line) {
	if (sze <= 0) {
		return NULL;
	}

	MemAlloc_t *mem = Mem_NewPtr();
	mem->ptr = _malloc_dbg(sze, _NORMAL_BLOCK, file, line);
	mem->size = sze;
	memset(mem->ptr, 0, sze);
	return mem->ptr;
}
#else
void *G_Alloc(int sze) {
	if(sze <= 0)
		return NULL;
	MemAlloc_t *mem = Mem_NewPtr();
	mem->ptr = malloc(sze);
	memset(mem->ptr, 0, sze);
	return mem->ptr;
}
#endif

void G_Free(void *ptr){
	if(!ptr)
		return;
	MemAlloc_t *prev = NULL;
	MemAlloc_t *mem = firstMem;
	while (mem != NULL) {
		if (mem->ptr == ptr) {
#ifdef LMD_MEMORY_DEBUG
			_free_dbg(mem->ptr, _NORMAL_BLOCK);
#else
			free(mem->ptr);
#endif
			if (prev == NULL) {
				firstMem = mem->next;
			}
			else {
				prev->next = mem->next;
			}
#ifdef LMD_MEMORY_DEBUG
			_free_dbg(mem, _NORMAL_BLOCK);
#else
			free(mem);
#endif
			return;
		}

		prev = mem;
		mem = mem->next;
	}
}

void G_InitMemory() {
	//these are not cleared on map_restart
	/*yes they are, the dll is reloaded
	Lmd_Arrays_RemoveAllElements((void **)&Memory);
	memCount = 0;
	*/
	firstMem = NULL;

#if 0
#ifdef LMD_MEMORY_DEBUG
	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_CHECK_EVERY_16_DF;
	tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
#endif
#endif
}

void G_ShutdownMemory() {
	MemAlloc_t *mem = firstMem;
	do {
		MemAlloc_t *next = mem->next;
#ifdef LMD_MEMORY_DEBUG
		_free_dbg(mem->ptr, _NORMAL_BLOCK);
		_free_dbg(mem, _NORMAL_BLOCK);
#else
		free(mem->ptr);
		free(mem);
#endif
		mem = next;
	} while (mem != NULL);
}

void Svcmd_GameMem_f( void ) {
	unsigned int sze = 0;
	unsigned int count = 0;

	MemAlloc_t *mem = firstMem;
	while(mem != NULL) {
		sze += mem->size;
		count += 1;
	}

	G_Printf("^2%u bytes alloced in %u units\n", sze, count);
}

#else

typedef struct MemData_s{
	void *ptr;
	int size;
}MemData_t;

unsigned int memCount;
MemData_t *Memory = NULL;

//RoboPhred: strange stuff is happening, suspect memory asplodery.
#define LMD_USE_MALLOC

//#define LMD_MEMORYCHECK

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

MemData_t *Lmd_Memory_Add(int size){
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

void Lmd_Memory_Remove(void *ptr){
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
	return Lmd_Memory_Add(sze)->ptr;
}
#endif

#ifdef LMD_MEMORY_DEBUG
void G_Free(void *ptr){
#ifdef LMD_MEMORYCHECK
	assert(_CrtCheckMemory());
#endif
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
	Lmd_Memory_Remove(ptr);
}
#endif

void G_InitMemory() {
	//these are not cleared on map_restart
	/*yes they are, the dll is reloaded
	Lmd_Arrays_RemoveAllElements((void **)&Memory);
	memCount = 0;
	*/
#ifdef LMD_MEMORY_DEBUG
	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_CHECK_EVERY_16_DF;
	tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
#endif
}

void G_ShutdownMemory() {
	while(memCount > 0) {
		G_Free(Memory[0].ptr);
	}
}

void Svcmd_GameMem_f( void ) {
	unsigned int sze = 0;
	unsigned int i;
	for(i = 0;i<memCount;i++){
		sze += Memory[i].size;
	}
	G_Printf("^2%u bytes alloced in %u units\n", sze, memCount);
}

#endif