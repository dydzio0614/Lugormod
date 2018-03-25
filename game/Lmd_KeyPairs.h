
typedef struct KeyPair_s{
	char *key;
	char *value;
}KeyPair_t;

typedef struct KeyPairSet_s{
	unsigned int count;	
	KeyPair_t *pairs;
}KeyPairSet_t;


#ifdef LMD_MEMORY_DEBUG
void Lmd_Pairs_ParseDatastring_dbg(KeyPairSet_t *set, char *str, char *file, int line);
unsigned int Lmd_Pairs_New_dbg(KeyPairSet_t *set, char *key, char *value, char *file, int line);
void Lmd_Pairs_Remove_dbg(KeyPairSet_t *set, int index, char *file, int line);

#define Lmd_Pairs_ParseDatastring(set, str) Lmd_Pairs_ParseDatastring_dbg(set, str, __FILE__, __LINE__)
#define Lmd_Pairs_New(set, key, value) Lmd_Pairs_New_dbg(set, key, value, __FILE__, __LINE__)
#define Lmd_Pairs_Remove(set, index) Lmd_Pairs_Remove_dbg(set, index, __FILE__, __LINE__)
#else
void Lmd_Pairs_ParseDatastring(KeyPairSet_t *set, char *str);
unsigned int Lmd_Pairs_New(KeyPairSet_t *set, char *key, char *value);
void Lmd_Pairs_Remove(KeyPairSet_t *set, int index);
#endif

void Lmd_Pairs_Clear(KeyPairSet_t *set);

char* Lmd_Pairs_ToDatastring(KeyPairSet_t *set);

int Lmd_Pairs_FindKey(KeyPairSet_t *set, char *key);
char* Lmd_Pairs_GetKey(KeyPairSet_t *set, char *key);
void Lmd_Pairs_SetKey(KeyPairSet_t *set, char *key, char *value);
void Lmd_Pairs_Merge(KeyPairSet_t *base, KeyPairSet_t *add);

