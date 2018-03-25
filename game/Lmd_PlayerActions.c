
#include "g_local.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_Arrays.h"

void PlayerActions_Remove(gentity_t *ent, Action_t *action){
	unsigned int i;
	unsigned int index;
	qboolean found = qfalse;
	for(i = 0;i<ent->client->pers.Lmd.Actions.count;i++){
		if(&(ent->client->pers.Lmd.Actions.Action[i]) == action){
			index = i;
			found = qtrue;
			break;
		}
	}
	if(!found)
		return;

	for(i = 0;i<(sizeof(ent->client->pers.Lmd.Actions.Action[index].strArgs) / sizeof(ent->client->pers.Lmd.Actions.Action[index].strArgs[0]));i++){
		if(ent->client->pers.Lmd.Actions.Action[index].strArgs[i] != NULL){
			G_Free(ent->client->pers.Lmd.Actions.Action[index].strArgs[i]);
			ent->client->pers.Lmd.Actions.Action[index].strArgs[i] = NULL;
		}
	}
	Lmd_Arrays_RemoveArrayElement((void **)&ent->client->pers.Lmd.Actions.Action, index,
		sizeof(Action_t), &ent->client->pers.Lmd.Actions.count);
}

int PlayerActions_FindAction(gentity_t *ent, char *name){
	unsigned int i;
	for(i = 0;i<ent->client->pers.Lmd.Actions.count;i++){
		if(Q_stricmp(name, ent->client->pers.Lmd.Actions.Action[i].name) == 0){
			return i;
		}
	}
	return -1;
}

Action_t* PlayerActions_Add(gentity_t *ent, char *name, char *descr, qboolean (*action)(gentity_t *ent, Action_t *action), qboolean overrideLast){
	int index;
	index = PlayerActions_FindAction(ent, name);
	if(index > -1){
		if(overrideLast)
			PlayerActions_Remove(ent, &ent->client->pers.Lmd.Actions.Action[index]);
		else
			return NULL;
	}
	index = Lmd_Arrays_AddArrayElement((void **)&ent->client->pers.Lmd.Actions.Action, sizeof(Action_t),
		&ent->client->pers.Lmd.Actions.count);
	ent->client->pers.Lmd.Actions.Action[index].name = G_NewString2(name);
	ent->client->pers.Lmd.Actions.Action[index].descr = G_NewString2(descr);
	ent->client->pers.Lmd.Actions.Action[index].action = action;

	//probably better style if I let the caller of this function print a message if it wants.
	Disp(ent, va("^3New action added: ^2%s^3: ^5%s^3.", name, descr));

	return &ent->client->pers.Lmd.Actions.Action[index];
}

void Cmd_Action_f(gentity_t *ent, int iArg){
	unsigned int i;
	if(trap_Argc() == 1){
		if(!ent->client->pers.Lmd.Actions.count){
			Disp(ent, "^3You have no pending actions.");
			return;
		}
		else{
			Disp(ent, "^3Your pending actions:");
			for(i = 0;i<ent->client->pers.Lmd.Actions.count;i++){
				Disp(ent, va("^2%s: ^5%s", ent->client->pers.Lmd.Actions.Action[i].name, 
					ent->client->pers.Lmd.Actions.Action[i].descr));
			}
			Disp(ent, "^3Use \'^2/actions <action name> [args]^3\' to use an action.");
			return;
		}
	}
	else{
		char arg[MAX_STRING_CHARS];
		int act;
		trap_Argv(1, arg, sizeof(arg));
		act = PlayerActions_FindAction(ent, arg);
		if(act > -1){
			if(ent->client->pers.Lmd.Actions.Action[act].action(ent, &ent->client->pers.Lmd.Actions.Action[act]))
				PlayerActions_Remove(ent, &ent->client->pers.Lmd.Actions.Action[act]);
		}
		else
			Disp(ent, "^3There is no pending action by that name.");
	}
}
