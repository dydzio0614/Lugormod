#include "g_local.h"

void Lmd_Arrays_Shift(void *arry, size_t sze, int count, int start, int offset) {
	int shiftSize;
	char *buf;
	
	if(count == 0)
		return;

	//This will break with a negitive offset.  Can fix it later.
	assert(offset > 0);


	//Array {a, b, c, d}
	//count 4
	//start 1
	//offset 1
	//expected result {a, b, b, c}

	//shiftSize = ((4 - 1) - 3) * 1 = 2
	shiftSize = ((count - start) - offset) * sze;
	buf = (char *)malloc(shiftSize);

	//Copy the range to shift to the buffer
	//dest: buf
	//src: array + (1 * 1) = index 1, first element 'b'
	//size: shiftSize = 2, range {b, c}
	memcpy(buf, (byte *)(arry) + (start * sze), shiftSize);
	//buf is now {b, c}

	//Copy the buffer to the new position
	//dest: array + (1 + 1) * 1 = index 2, first element 'c'
	//src: buf
	//size: shiftSize = 2, array range {c, d}
	memcpy((byte *)(arry) + ((start + offset) * sze), buf, shiftSize);
	//index 2 of array has now been written with {b, c}, so
	//array is now {a, b, b, c}

	free(buf);
}

#ifdef LMD_MEMORY_DEBUG
unsigned int Lmd_Arrays_AddArrayElement_dbg(void **arry, size_t sze, unsigned int *count, char *file, int line){
#else
unsigned int Lmd_Arrays_AddArrayElement(void **arry, size_t sze, unsigned int *count){
#endif

#ifdef LMD_MEMORY_DEBUG
	*arry = (void *)_realloc_dbg(*arry, sze * ((*count) + 1), _NORMAL_BLOCK, file, line);
#else
	*arry = (void *)realloc(*arry, sze * ((*count) + 1));
#endif
	if(!(*arry)) {
		G_Error(va("%s: Failed to realloc memory array!", __FUNCTION__));
	}
	memset((byte *)(*arry) + (sze * (*count)), 0, sze);
	(*count)++;
	return (*count) - 1; //return the index, for when we start doing the automatic sorting.
}

#ifdef LMD_MEMORY_DEBUG
qboolean Lmd_Arrays_AddArrayElement_Location_dbg(int index, void **arry, size_t sze, unsigned int *count, char *file, int line) {
#else
qboolean Lmd_Arrays_AddArrayElement_Location(int index, void **arry, size_t sze, unsigned int *count) {
#endif

#ifdef LMD_MEMORY_DEBUG
	*arry = (void *)_realloc_dbg(*arry, sze * ((*count) + 1), _NORMAL_BLOCK, file, line);
#else
	*arry = (void *)realloc(*arry, sze * ((*count) + 1));
#endif

	if(!(*arry)) {
		G_Error(va("%s: Failed to realloc memory array!", __FUNCTION__));
	}

	Lmd_Arrays_Shift(*arry, sze, *count, index, 1);

	memset((byte *)(*arry) + (index * sze), 0, sze);

	(*count)++;
	return qtrue;
}

void Lmd_Arrays_RemoveAllElements(void **arry) {
	if((*arry)) {
#ifdef LMD_MEMORY_DEBUG
		_free_dbg(*arry, _NORMAL_BLOCK);
#else
		free(*arry);
#endif
	}
	*arry = NULL;
}

#ifdef LMD_MEMORY_DEBUG
void Lmd_Arrays_RemoveArrayElement_dbg(void **arry, int element, size_t sze, unsigned int *count, char *file, int line) {
#else
void Lmd_Arrays_RemoveArrayElement(void **arry, int element, size_t sze, unsigned int *count) {
#endif

	if(element == 0 && *count == 1){

	}
	else if(element+1 == *count){

	}
	else{
		memcpy((byte *)(*arry) + (element * sze), (byte *)(*arry) + ((element + 1) * sze), (*count - element - 1) * sze);
	}
	(*count)--;

#ifdef LMD_MEMORY_DEBUG
	*arry = (void *)_realloc_dbg(*arry, sze * (*count), _NORMAL_BLOCK, file, line);
#else
	*arry = (void *)realloc(*arry, sze * (*count));
#endif

	if(!(*arry) && (*count) > 0) {
		G_Error(va("%s: Failed to realloc memory array!", __FUNCTION__));
	}
}