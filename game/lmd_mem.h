
#ifdef LMD_MEMORY_DEBUG
void* G_AllocDebug(int sze, char *file, int line);
#define G_Alloc(size) G_AllocDebug(size, __FILE__, __LINE__)
#else
void *G_Alloc( int size );
#endif

void G_Free(void *ap);
void G_InitMemory( void );
void G_ShutdownMemory();
void Svcmd_GameMem_f( void );