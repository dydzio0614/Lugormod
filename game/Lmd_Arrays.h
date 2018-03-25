

#ifdef LMD_MEMORY_DEBUG
unsigned int Lmd_Arrays_AddArrayElement_dbg(void **arry, size_t sze, unsigned int *count, char *file, int line);
qboolean Lmd_Arrays_AddArrayElement_Location_dbg(int index, void **arry, size_t sze, unsigned int *count, char *file, int line);
void Lmd_Arrays_RemoveArrayElement_dbg(void **arry, int element, size_t sze, unsigned int *count, char *file, int line);

#define Lmd_Arrays_AddArrayElement(arry, sze, count) Lmd_Arrays_AddArrayElement_dbg(arry, sze, count, __FILE__, __LINE__)
#define Lmd_Arrays_AddArrayElement_Location(index, arry, sze, count) Lmd_Arrays_AddArrayElement_Location_dbg(index, arry, sze, count, __FILE__, __LINE__)
#define Lmd_Arrays_RemoveArrayElement(arry, element, size, count) Lmd_Arrays_RemoveArrayElement_dbg(arry, element, size, count, __FILE__, __LINE__)
#else
unsigned int Lmd_Arrays_AddArrayElement(void **arry, size_t sze, unsigned int *count);
qboolean Lmd_Arrays_AddArrayElement_Location(int index, void **arry, size_t sze, unsigned int *count);
void Lmd_Arrays_RemoveAllElements(void **arry);
void Lmd_Arrays_RemoveArrayElement(void **arry, int element, size_t sze, unsigned int *count);
#endif
void Lmd_Arrays_RemoveAllElements(void **arry);


void Lmd_Arrays_Shift(void *arry, size_t sze, int count, int start, int offset);