
struct Action_s{
	char *name;
	char *descr;
	qboolean (*action)(gentity_t *ent, Action_t *action);
	char *strArgs[4];
	int iArgs[4];
	float fArgs[4];
};

void PlayerActions_Remove(gentity_t *ent, Action_t *action);
Action_t* PlayerActions_Add(gentity_t *ent, char *name, char *descr, qboolean (*action)(gentity_t *ent, Action_t *action), qboolean overrideLast);

