

#define LMDAPI_MEMORY_VERSION_1 (0)

#define LMDAPI_MEMORY_VERSION_CURRENT LMDAPI_MEMORY_VERSION_1

/*
Provides access to the memory management functionality of Lugormod.

As various entity fields can be set with hard coded strings or user defined values,
memory needs to be tracked to know which values can be safely freed.  This is done by
keeping track of what memory was allocated, and only freeing pointers which we know about.

These functions should always be used when creating or freeing memory that the mod will
make use of, otherwise memory leaks and crashes can occur.
*/
typedef struct LmdApi_Memory_v1_s {
	// Allocates tracked memory.
	void* (*allocMemory)(int size);

	/*
	Safely frees memory at the given pointer.
	The memory will only be freed if it was allocated with allocMemory.
	*/
	void (*freeMemory)(void *ptr);
} LmdApi_Memory_v1_t;


// Represents the current memory function list at the time of compile.
#define LmdApi_Memory_t LmdApi_Memory_v1_t

#ifdef LUGORMOD
const void *LmdApi_Get_Memory(unsigned int version);
#else
// Fetch the current function list at the time of compile.
// May return NULL if the version is no longer supported.
#define LmdApi_GetCurrent_Memory() (LmdApi_Memory_t*) LmdApi_Get_Memory(LMDAPI_MEMORY_VERSION_CURRENT)
#endif