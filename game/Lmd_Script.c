
#ifdef LMD_EXPERIMENTAL

#ifdef LMD_SCRIPTING

#include "g_local.h"
#include "Lmd_Arrays.h"
#include "Lmd_Data.h"
#include "Lmd_Script.h"

/*
Generic logical block programming system
Concepts:
	Stack: script, group of blocks
	Block: Single statement

Invoking (calling) a script:
	classname,lmd_stack,invoke,"Stack1:Var1,5,Var2,6"

Example stack:
	/script create Stack1
	/script add Stack1 logic_if "condition1,$VAR(Var1),condition2,5,trueinvoke,#EXIT,falseinvoke,#STACK(Stack2)"

Variables
	All variables are saved on a per-mapentity-file basis.
	Scripts have local variables, with global variables being defined via a function.

Special case stack names:
	Start: runs on entity load.
	End: runs on server shutdown (NOT on entity unload/clearmap).
	
	Player_Spawn: runs on player spawn, var PlayerNum set to the player number.

In-line commands
	#EXIT: stop the script.
	#RCON(cmd): run an rcon cmd.
	#GETVAR(var): replace this with the value of the var.
	#SETVAR(var, val): 
	#SETACCKEY(player number, key, value): set an account key (from the list of allowed keys)
	#GETACCKEY(player number, key): Replace this with the value of the key.

Issues
	When a map is cleared, what about variables that are left behind?

Notes
	There will be no time delay functions.  Scripts are only for deeper logical control that entities cannot give.  Use a target_delay to get delays.
	Recursion can be achieved with func_timer and state variables.
*/

scriptFuncDef_t Funcs[] = {
	NULL,
};
const int numFuncs = sizeof(Funcs) / sizeof(scriptFuncDef_t);

scriptFuncDef_t* Scripts_GetFunc(char *name){
	int i;
	for(i = 0; i < numFuncs; i++){
		if(Q_stricmp(name, Funcs[i].name) == 0)
			return &Funcs[i];
	}
	return NULL;
}

scriptFunc_t* Scripts_ParseFunc(scriptFuncDef_t *def, char *data){
	scriptFunc_t *func = (scriptFunc_t *)G_Alloc(sizeof(scriptFunc_t));
	func->data = G_Alloc(def->dataSize);
	Lmd_Data_ParseDatastring(&data, NULL, def->fields, (byte *)func->data);
	return func;
}

typedef struct script_s{
	char *name;
	KeyPairSet_t locals;
	struct {
		unsigned int count;
		scriptFunc_t **funcs;
	}stack;
}script_t;

struct {
	unsigned int count;
	script_t **scripts;
}Scripts;

void Scripts_AddScriptFunc(script_t *script, scriptFunc_t *func, int index){
	if(index < 0 || index > script->stack.count)
		index = script->stack.count;
	assert(Lmd_Arrays_AddArrayElement_Location(index, (void **)&script->stack.funcs, sizeof(scriptFunc_t **), &script->stack.count));
	script->stack.funcs[index] = func;
}

script_t* Scripts_New(char *name){
	int i = Lmd_Arrays_AddArrayElement((void **)&Scripts.scripts, sizeof(script_t *), &Scripts.count);
	Scripts.scripts[i] = (script_t *)G_Alloc(sizeof(script_t));
	Scripts.scripts[i]->name = G_NewString2(name);
	return Scripts.scripts[i];
}

qboolean Scripts_ParseFuncPair(byte *obj, qboolean pre, char *key, char *value){
	if(pre){
		script_t *script = (script_t *)obj;
		scriptFuncDef_t *f = Scripts_GetFunc(key);
		if(!f)
			return qfalse;
		scriptFunc_t *func = Scripts_ParseFunc(f, value);
		Scripts_AddScriptFunc(script, func, -1);
		return qtrue;
	}
	return qfalse;
}

qboolean Scripts_Parse(char *name, char *buf){
	script_t *script = Scripts_New(name);
	Lmd_Data_ParseFields(&buf, Scripts_ParseFuncPair, NULL, (byte *)script);
	return qtrue;
}

int Scripts_Load(){
	return Lmd_Data_ProcessFiles(va("mapdata/%s/scripts", level.rawmapname), ".script", Scripts_Parse, Q3_INFINITE);
}

void Scripts_Save(){
	
}

#endif

#endif