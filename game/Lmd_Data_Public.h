
#ifndef LMD_DATA_PUBLIC_H
#define LMD_DATA_PUBLIC_H

#include "q_shared.h"

typedef enum {
	// Could also be flags:
	// 1: Write value
	// 2: call again

	// No data, do not write the key value pair.
	DWR_NODATA = 0,

	// Completed, write the key value pair.  Do not call write(...) again.
	DWR_COMPLETE = 1,

	// More data is pending, but the current key/value should not be saved.
	// Call write(...) again, but ignore the current key/value.
	DWR_SKIP = 2,

	// More data is pending, write the key value pair and call write(...) again.
	DWR_CONTINUE = 3,
} DataWriteResult_t;

typedef struct DataField_s{
	// Key
	/*
	To make a DataField read every key, set key to 'NULL' and isPrefix to 'qtrue'
	*/

	// The key to read or write.
	char *key;

	// If true, the field will be used if the parsed key starts with key.
	// Otherwise, the key must match exactly.
	qboolean isPrefix;


	// Parsing
	/*
	Parsing is provided as both a function and an optional argument to pass on.
	You may either define a function for every key, or share one function across
	multiple keys using the key parameter and parseArgs.

	The benefit of this method is that predefined parsing functions can be used
	where it would be too much trouble to write out a new function for every key.
	*/
	
	/*
	Parse a key/value pair.
	

	key: The key which was read.
	value: The value which was read.
	target: The target to read data into.
	args: The contents of DataField_t::parseArgs for this field.

	return:
	qtrue if the parser should stop parsing this key/value pair.
	qfalse if the parser should allow other fields to try and parse this key/value pair.
	*/
	qboolean(*parse)(char *key, char *value, void *target, void *args);

	// Arguments to pass to the parse function when parsing this DataField_t.
	void *parseArgs;


	// Writing
	/*
	Writing is a bit more complex, as one field can store
	multiple values.  To assist in this, the write function is
	given a pointer where it can store its state information,
	and returns a value indicating whether the data should be written
	and whether the write function should be called again.
	*/
	
	/*
	Obtain the field's key and value to write to the target.
	target: The target whose data should be written.
	key: The key to write.  This is prepopulated with the contents of 'key', and may be changed.
	keySize: The maximum number of characters in key.
	value: The value to write.  The function should change this.
	valueSize: The maximum number of characters in value.
	writeState: A double-pointer to store data in, initially set to NULL.  You may
		allocate data and store it in '*writeState' to track progress across multiple writes,
		but be sure to free it before you return DWR_NODATA or DWR_COMPLETE.
	args: The contents of DataField_t::writeArgs for this field.

	return:
	DWR_NODATA: key/value will not be written.
	DWR_COMPLETE: key/value will be saved and no more calls to write(...) will be performed for this field.
	DWR_CONTINUE: key/value will be saved, and write(...) will be called again for this field.
	DWR_SKIP: key/value will not be saved, but write(...) will be called again for this field.
	*/
	DataWriteResult_t(*write)(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args);
	void *writeArgs;

	// Memory

	// This is optional, and may not be called by every user of the data field system.
	// Can be NULL
	void (*freeData)(void *target, void *args);
	void *freeDataArgs;
} DataField_t;

#endif