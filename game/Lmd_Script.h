
typedef struct scriptFunc_s scriptFunc_t;

typedef struct scriptFuncDef_s{
	char *name;
	BG_field_t *fields;
	int dataSize;
	char* (*validate)(scriptFunc_t *func); //return null if valid, string error message otherwise.
	void (*execute)();
	void (*toString)(char *buf, int sze);
}scriptFuncDef_t;

struct scriptFunc_s{
	scriptFuncDef_t *def;
	void *data;
};

typedef struct inlineFuncDef_s{
	char *name;
	int numArgs;
	void (*execute)(char *args[]);
}inlineFuncDef_t;