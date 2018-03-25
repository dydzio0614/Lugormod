
#include <stdlib.h>

#include "lmd_mem.h"
#include "Lmd_API_Memory.h"

LmdApi_Memory_v1_t api_memory_v1 = {
	G_Alloc,
	G_Free
};

const void *LmdApi_Get_Memory(unsigned int version) {
	if (version == LMDAPI_MEMORY_VERSION_1) {
		return &api_memory_v1;
	}

	return NULL;
}