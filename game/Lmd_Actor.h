

gentity_t* Actor_Create(char *model, char *skin, vec3_t origin, vec3_t angles);

void Actor_SetOrigin(gentity_t *actor, vec3_t origin);
void Actor_SetAngles(gentity_t *actor, vec3_t angles);

void Actor_SetModel(gentity_t *actor, char *model, char *skin);
char* Actor_GetModel(gentity_t *actor);

void Actor_SetScale(gentity_t *actor, float scale);

void Actor_SetAnimation_Torso(gentity_t *actor, int animID, int length);
void Actor_SetAnimation_Legs(gentity_t *actor, int animID, int length);
void Actor_SetAnimation_Both(gentity_t *actor, int animID, int length);

void Actor_SetSpeed(gentity_t *actor, int speed);
void Actor_SetDestination(gentity_t *actor, vec3_t dest, int radius);