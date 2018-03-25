
#ifndef LMD_DATA_H
#define LMD_DATA_H

#include "Lmd_Data_Public.h"

qboolean Lmd_Data_IsCleanPath(char *path);

// Opens a data file from the current data directory.
fileHandle_t Lmd_Data_OpenDataFile(char *directory, char *name, fsMode_t mode);

// Copies the file content of the non-data path to a G_Alloc string.
char* Lmd_Data_AllocFileContents(char *filename);



unsigned int Lmd_Data_ProcessFiles(char *directory, char *ext, qboolean (*Callback)(char *fileName, char *fileBuf), int maxFiles);

// Parses a set of keys and values as
//key: long value text terminated by linefeed
//key: long value text terminated by linefeed
unsigned int Lmd_Data_ParseKeys_Old(char **str, qboolean requireValue, qboolean (*callback)(char *key, char *value, void *state), void *state);

// Parses a set of fields as
//key: value
//key: "quoted value"
unsigned int Lmd_Data_ParseFields_Old(char **str, qboolean requireValue, qboolean (*Callback)(byte *object, qboolean pre, char *key, char *value), BG_field_t *fields, byte* structure);

// Parses a set of fields as
//key,value,key,value
unsigned int Lmd_Data_ParseDatastring(char **str, qboolean (*Callback)(byte *object, qboolean pre, char *key, char *value), BG_field_t *fields, byte* structure);

qboolean Lmd_Data_DeleteFile(char *directory, char *name);

typedef struct{
	unsigned int func;
	int offset;
}DBSaveFileCallbackReturn_t;

qboolean Lmd_Data_SaveDatafile(char *directory, char *name, BG_field_t *fields, byte* structure, qboolean (*Override)(byte *structure,
							   char *key, char *value, int valueSze),
							   DBSaveFileCallbackReturn_t* (*MoreKeys)(byte *structure, DBSaveFileCallbackReturn_t *arg, char *key, int keySze, char *value, int valueSze));


qboolean Lmd_Data_WriteDatastringField( BG_field_t *f, char *value, unsigned int sze, byte *b );
qboolean Lmd_Data_WriteDatafileField( BG_field_t *f, char *value, unsigned int sze, byte *b );
char* Lmd_Data_GetDataPath(char *directory, char *output, int outputSze);

qboolean BG_CompareFields(BG_field_t *f, byte *v1, byte *v2);

void BG_CopyField(BG_field_t *f, byte *dst, byte *src);
void BG_FreeFields(BG_field_t *fields, byte* structure);

typedef struct DataAutoFieldArgs_s
{
	int ofs;
	fieldtype_t	type;
} DataAutoFieldArgs_t;


int Lmd_Data_Parse_LineDelimited(
		char **str,
		void *target,
		const DataField_t fields[],
		int fieldCount);

// TODO: Define macros for recursive key value pair delegation.
qboolean Lmd_Data_Parse_KeyValuePair(char *key, char *value, void *target, const DataField_t fields[], int fieldCount);


int Lmd_Data_WriteToFile_LinesDelimited(
	fileHandle_t file,
	const DataField_t fields [],
	int fieldCount,
	void *target);



void Lmd_Data_FreeFields(void *target, const DataField_t fields [], int fieldCount);


// Predefined callbacks

qboolean Lmd_Data_AutoFieldCallback_Parse(char *key, char *value, void *target, void *args);
DataWriteResult_t Lmd_Data_AutoFieldCallback_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args);
void Lmd_Data_AutoFieldCallback_Free(void *target, void *args);


// Field generation

#define AUTOFIELD(name, ofs, type) {name, qfalse, Lmd_Data_AutoFieldCallback_Parse, (void*)&DataAutoFieldArgs_t(ofs, type)}

// Until the compiler supports compound literals, this will have to do.
#define DEFINE_FIELD_PRE_AUTO(name, ofs, type) const DataAutoFieldArgs_t DataFieldArgs_##name = { ofs, type };
#define DEFINE_FIELD_PRE_FUNC(name, parseFunc, writeFunc, args)
#define DEFINE_FIELD_PRE_PREF(prefix, parseFunc, writeFunc, args)
#define DEFINE_FIELD_PRE_DEFL(parseFunc, writeFunc, args)

#define DATAFIELDS_BEGIN(name) const DataField_t name [] = {
#define DEFINE_FIELD_LIST_AUTO(name, ofs, type) {#name, qfalse, Lmd_Data_AutoFieldCallback_Parse, (void *) &DataFieldArgs_##name, Lmd_Data_AutoFieldCallback_Write, (void *) &DataFieldArgs_##name, Lmd_Data_AutoFieldCallback_Free, (void *) &DataFieldArgs_##name},
#define DEFINE_FIELD_LIST_FUNC(name, parseFunc, writeFunc, args) {#name, qfalse, parseFunc, args, writeFunc, args},
#define DEFINE_FIELD_LIST_PREF(prefix, parseFunc, writeFunc, args) {#prefix, qtrue, parseFunc, args, writeFunc, args},
#define DEFINE_FIELD_LIST_DEFL(parseFunc, writeFunc, args)		 {NULL, qtrue, parseFunc, args, writeFunc, args},
#define DATAFIELDS_END };

#define DATAFIELDS_COUNT(name) (sizeof(name) / sizeof(DataField_t))

#endif

