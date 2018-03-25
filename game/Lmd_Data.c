
#include "g_local.h"

#include "Lmd_Data.h"

#include "BG_Fields.h"

#define LMD_DATABASE_FILELIST_MAX 2048
#define LMD_DATABASE_FILENAME_SIZE MAX_NETNAME
#define LMD_DATABASE_DATAPATH "data"

qboolean BG_ParseField( BG_field_t *l_fields, const char *key, const char *value, void *target );
qboolean BG_ParseType(fieldtype_t type, const char *value, void *target);

qboolean Lmd_Data_IsCleanPath(char *path) {
	static const char invalidChars[] = {'?', '%', '*', ':', '|', '\"', '<', '>' };
	if (strstr(path, ".."))
		return qfalse;
	int len = strlen(path);
	for(int i = 0; i < len; i++) {
		for(int j = 0; j < sizeof(invalidChars); j++) {
			if (path[i] == invalidChars[j]) {
				return qfalse;
			}
		}
	}

	return qtrue;
}


extern vmCvar_t lmd_DataPath;
char* Lmd_Data_GetDataPath(char *directory, char *output, int outputSze){
	char *datapath = lmd_DataPath.string;
	if(!datapath[0])
		datapath = "default";
	Q_strncpyz(output, va(LMD_DATABASE_DATAPATH"/%s/%s", datapath, directory), outputSze);
	return output;
}

fileHandle_t Lmd_Data_OpenDataFile(char *directory, char *name, fsMode_t mode) {
	fileHandle_t f;
	char path[MAX_STRING_CHARS] = "";
	if(directory) {
		Lmd_Data_GetDataPath(directory, path, sizeof(path));
		Q_strcat(path, sizeof(path), "/");
	}

	Q_strcat(path, sizeof(path), name);
	trap_FS_FOpenFile(path, &f, mode);
	return f;
}

char* Lmd_Data_AllocFileContents(char *filename) {
	fileHandle_t f;
	int len = trap_FS_FOpenFile(filename, &f, FS_READ);
	char *buf;
	if(len < 0)
		return NULL;
	else if(len <= 0) {
		trap_FS_FCloseFile(f);
		return NULL;
	}

	len += 1;
	buf = (char *)G_Alloc(len);
	trap_FS_Read(buf, len, f);
	trap_FS_FCloseFile(f);
	return buf;
}

unsigned int Lmd_Data_ProcessFiles(char *directory, char *ext, qboolean (*Callback)(char *fileName, char *fileBuf), int maxFiles){
	int listbufSze = LMD_DATABASE_FILENAME_SIZE;
	char *listbuf;// = (char *)G_Alloc(listbufSze);
	char path[MAX_STRING_CHARS];
	int numFiles;
	char *namePtr;
	int nameLen;
	char *fileBuf;
	unsigned int totalFiles = 0;
	char *s, *s2;
	if(maxFiles > LMD_DATABASE_FILELIST_MAX)
		maxFiles = LMD_DATABASE_FILELIST_MAX;
	Lmd_Data_GetDataPath(directory, path, sizeof(path));
	listbufSze *= maxFiles;
	namePtr = listbuf = (char *)G_Alloc(listbufSze);

	numFiles = trap_FS_GetFileList(path, ext, listbuf, listbufSze);
	int i;
	for(i = 0; i < numFiles; i++, namePtr += nameLen + 1){
		nameLen = strlen(namePtr);

		fileBuf = Lmd_Data_AllocFileContents(va("%s/%s", path, namePtr));
		if(!fileBuf)
			continue;

		//remove extention
		s = s2 = namePtr;
		while(s[0] && (s2 = strchr(s2, '.'))){
			s = s2;
			s2++;
		}
		s[0] = 0;

		if(Callback(namePtr, fileBuf)){
			totalFiles++;
		}
		G_Free(fileBuf);
		if(totalFiles >= maxFiles)
			break;
	}
	G_Free(listbuf);
	return totalFiles;
}

qboolean Lmd_Data_DeleteFile(char *directory, char *name){
	vmCvar_t fs_game;
	char filename[MAX_STRING_CHARS];
	Lmd_Data_GetDataPath(directory, filename, sizeof(filename));
	Q_strcat(filename, sizeof(filename), "/");
	Q_strcat(filename, sizeof(filename), name); 
	trap_Cvar_Register( &fs_game, "fs_game", "", CVAR_SERVERINFO | CVAR_ROM);
	if(fs_game.string[0] == 0)
		return (remove(va("base/%s", filename)) == 0);
	else
		return (remove(va("%s/%s", fs_game.string, filename)) == 0);
}

unsigned int Lmd_Data_ParseKeys_Old(char **str, qboolean requireValue, qboolean (*callback)(char *key, char *value, void *state), void *state) {
	char *p;
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	unsigned int count = 0;
	int colPos;
	while(str[0]) {
		p = COM_ParseExt((const char **)str, qtrue);
		if (p[0] == 0) {
			break;
		}

		Q_strncpyz(key, p, sizeof(key));
		colPos = strlen(key) - 1;
		if (colPos <= 0) {
			// Skip this line.  Untested.
			COM_ParseLine((const char **)str);
			break;
		}

		if (key[colPos] == ':') {
			key[colPos] = 0;
			Q_strncpyz(value, COM_ParseLine((const char **)str), sizeof(value));
			if (value[0] == 0 && requireValue) {
				continue;
			}
		}
		else if (requireValue) {
			continue;
		}
		else {
			value[0] = 0;
		}

		if (callback(key, value, state)) {
			count++;
		}
	}

	return count;
}

struct ParseFieldsState_s {
	BG_field_t *fields;
	byte *structure;
	qboolean (*callback)(byte *object, qboolean pre, char *key, char *value);
};

qboolean Lmd_Data_ParseFields_Callback(char *key, char *value, void *pstate) {
	struct ParseFieldsState_s *state = (struct ParseFieldsState_s*)pstate;

	if((state->callback && state->callback(state->structure, qtrue, key, value)) ||
		(state->fields && state->structure && (BG_ParseField(state->fields, key, value, state->structure) ||
			(state->callback && state->callback(state->structure, qfalse, key, value))))){
				return true;
		}
	return false;
}

unsigned int Lmd_Data_ParseFields_Old(char **str, qboolean requireValue, qboolean (*callback)(byte *object, qboolean pre, char *key, char *value), BG_field_t *fields, byte* structure) {
	struct ParseFieldsState_s state = {fields, structure, callback};
	return Lmd_Data_ParseKeys_Old(str, requireValue, Lmd_Data_ParseFields_Callback, (void *)&state);
}

qboolean ParseField(char **sourcep, char *dest, char start, char stop);
unsigned int Lmd_Data_ParseDatastring(char **str, qboolean (*Callback)(byte *object, qboolean pre, char *key, char *value), BG_field_t *fields, byte* structure){
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	char *token;
	unsigned int count = 0;
	while(str[0]){
		token = COM_ParseDatastring((const char **)str);
		if(!token[0])
			break;
		Q_strncpyz(key, token, sizeof(key));

		token = COM_ParseDatastring((const char **)str);
		if(!token[0])
			break;
		Q_strncpyz(value, token, sizeof(value));
		
		if((Callback && Callback(structure, qtrue, key, value))
			|| BG_ParseField(fields, key, value, structure) ||
			(Callback && Callback(structure, qfalse, key, value))){
				count++;
		}
	}
	return count;
}

qboolean BG_GetField( BG_field_t *l_field, char *value, unsigned int sze, byte *ent );
qboolean Lmd_Data_WriteDatafileField( BG_field_t *f, char *value, unsigned int sze, byte *b ){
	char buf[MAX_STRING_CHARS];

	//dont attempt if null
	if((f->type == F_LSTRING || f->type == F_GSTRING) && (*(char **)(b+f->ofs) == NULL || (*(char **)(b+f->ofs))[0] == 0))
		return qfalse;
	//dont write if 0
	else if((f->type == F_INT || f->type == F_FLOAT) && *(int *)(b+f->ofs) == 0)
		return qfalse;


	memset(buf, 0, sizeof(buf));
	BG_GetField(f, buf, sizeof(buf), b);
	Q_strncpyz(value, va("%s: %s", f->name, buf), sze);
	return qtrue;
}

qboolean Lmd_Data_WriteDatastringField( BG_field_t *f, char *value, unsigned int sze, byte *b ){
	char buf[MAX_STRING_CHARS];

	//dont attempt if null
	if((f->type == F_LSTRING || f->type == F_GSTRING) && (*(char **)(b+f->ofs) == NULL || (*(char **)(b+f->ofs))[0] == 0))
		return qfalse;
	//dont write if 0
	else if((f->type == F_INT || f->type == F_FLOAT) && *(int *)(b+f->ofs) == 0)
		return qfalse;


	memset(buf, 0, sizeof(buf));
	BG_GetField(f, buf, sizeof(buf), b);
	Q_strncpyz(value, va("%s,%s,", f->name, buf), sze);
	return qtrue;
}

qboolean Lmd_Data_SaveDatafile(char *directory, char *name, BG_field_t *fields, byte* structure,
							   qboolean (*Override)(byte *structure, char *key, char *value, int valueSze),
							   DBSaveFileCallbackReturn_t* (*MoreKeys)(byte *structure, DBSaveFileCallbackReturn_t *arg, char *key, int keySze, char *value, int valueSze)){
	BG_field_t *bgf;
	//int i;
	DBSaveFileCallbackReturn_t arg, *argPtr = &arg;
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	fileHandle_t f;
	memset(&arg, 0, sizeof(arg));

	f = Lmd_Data_OpenDataFile(directory, name, FS_WRITE);
	if(!f)
		return qfalse;
	
	if(fields && structure) {
		for(bgf = fields;bgf->name != NULL;bgf++){
			Q_strncpyz(key, bgf->name, sizeof(key));
			value[0] = 0;
			if(Override && Override(structure, key, value, sizeof(value))) {
				char *line = va("%s: %s\n", key, value);
				trap_FS_Write(line, strlen(line), f);
			}
			else if (Lmd_Data_WriteDatafileField(bgf, value, sizeof(value), structure)) {
				trap_FS_Write(va("%s\n", value), strlen(value) + 1, f);
			}
		}
	}

	if(MoreKeys){
		char *d;
		key[0] = 0;
		value[0] = 0;
		while((argPtr = MoreKeys(structure, argPtr, key, sizeof(key), value, sizeof(value)))){
			if(key[0] && value[0]){
				d = va("%s: %s\n", key, value);
				trap_FS_Write(d, strlen(d), f);
				key[0] = 0;
				value[0] = 0;
			}
		}
	}

	trap_FS_FCloseFile(f);
	return qtrue;
}

int Lmd_Data_MatchField(char *key, const DataField_t *fields, int fieldCount, int start) {
	int i;
	for(i = start; i < fieldCount; i++) {
		if (fields[i].isPrefix) {
			if (fields[i].key == NULL || Q_stricmpn(key, fields[i].key, strlen(fields[i].key)) == 0) {
				return i;
			}
		}
		else {
			if (Q_stricmp(key, fields[i].key) == 0) {
				return i;
			}
		}
	}

	return -1;
}

/*
TODO: Take arbitrary seperators:
	char *keyValueSeperator,
	char *fieldSeperator,
*/
int Lmd_Data_Parse_LineDelimited(
		char **str,
		void *target,
		const DataField_t fields[],
		int fieldCount)
{
	char *p;
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	unsigned int count = 0;
	int colPos;

	char *valuePtr;

	while(str[0]) {
		p = COM_ParseExt((const char **)str, qtrue);
		if (p[0] == 0) {
			break;
		}

		Q_strncpyz(key, p, sizeof(key));
		colPos = strlen(key) - 1;
		if (colPos <= 0) {
			// Skip this line.  Untested.
			COM_ParseLine((const char **)str);
			break;
		}

		if (key[colPos] == ':') {
			key[colPos] = 0;
			Q_strncpyz(value, COM_ParseLine((const char **)str), sizeof(value));
			valuePtr = value; // We got a real value (may be empty).
		}
		else {
			valuePtr = NULL; // No value.
		}

		if (Lmd_Data_Parse_KeyValuePair(key, valuePtr, target, fields, fieldCount)) {
			count++;
		}
	}

	return count;
}

qboolean Lmd_Data_Parse_KeyValuePair(char *key, char *value, void *target, const DataField_t fields[], int fieldCount) {
	int start = 0;
	while (start < fieldCount) {
		int index = Lmd_Data_MatchField(key, fields, fieldCount, start);
		const DataField_t *field;
		if (index < 0) break;
		field = &fields[index];
		if (field->parse(key, value, target, field->parseArgs)) {
			return qtrue;
		}

		start = index + 1;
	}

	return qfalse;
}

int Lmd_Data_WriteToFile_LinesDelimited(
	fileHandle_t file,
	const DataField_t fields [],
	int fieldCount,
	void *target){

	int i;
	char key[MAX_STRING_CHARS];
	char value[MAX_STRING_CHARS];
	void *writeState = NULL;

	int writes = 0;

	for (i = 0; i < fieldCount; i++) {
		DataWriteResult_t dwr;

		if (!fields[i].write) continue;
		writeState = NULL;
		do {
			// May be NULL for 'any key' parsers/writers
			if (fields[i].key != NULL) {
				Q_strncpyz(key, fields[i].key, sizeof(key));
			}
			else {
				key[0] = 0;
			}
			dwr = fields[i].write(target, key, sizeof(key), value, sizeof(value), &writeState, fields[i].writeArgs);
			if (dwr != DWR_NODATA && key[0]) {
				char *line;
				if (value[0]) {
					line = va("%s: %s\n", key, value);
				}
				else {
					line = key;
				}
				trap_FS_Write(line, strlen(line), file);
				writes++;
			}
		} while (dwr == DWR_CONTINUE || dwr == DWR_SKIP);
	}
		
	return writes;
}

void Lmd_Data_FreeFields(void *target, const DataField_t fields [], int fieldCount) {
	int i;
	for(i = 0; i < fieldCount; i++) {
		if (fields[i].freeData) {
			fields[i].freeData(target, fields[i].freeDataArgs);
		}
	}
}


qboolean Lmd_Data_AutoFieldCallback_Parse(char *key, char *value, void *target, void *args) {
	if (value != NULL && value[0]) {
		DataAutoFieldArgs_t *fieldArgs = (DataAutoFieldArgs_t *) args;
		BG_ParseType(fieldArgs->type, value, (byte *) target + fieldArgs->ofs);
		return qtrue;
	}

	return qfalse;
}

DataWriteResult_t Lmd_Data_AutoFieldCallback_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	DataAutoFieldArgs_t *fieldArgs = (DataAutoFieldArgs_t *) args;
	BG_field_t f = { key, fieldArgs->ofs, fieldArgs->type };
	BG_GetField(&f, value, valueSize, (byte *)target);

	assert(key[0]);

	return DWR_COMPLETE;
}

void Lmd_Data_AutoFieldCallback_Free(void *state, void *args) {
	DataAutoFieldArgs_t *fieldArgs = (DataAutoFieldArgs_t *) args;
	BG_FreeField(fieldArgs->type, (byte *) state + fieldArgs->ofs);
}
