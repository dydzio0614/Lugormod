
#include "g_local.h"

void PlayerGuide_Player_Login(gentity_t *ent) {

}

#if 0

#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"

typedef struct guide_s {
	int version;
	char *name;
	int color;
	char *contents;
	(void *)bridge(gentity_t *ent);
}guide_t;

guide_t guides = {
	{1, "Lugormod", 3, "Welcome to Lugormod!", NULL},
};

const unsigned int numGuides = sizeof(guides) / sizeof(guide_t);

accDataModule_t Accounts_PlayerGuide = {
	NULL,
	0,
	sizeof(int) * numGuides,
	parseCustKey,
	saveCustKey,
	NULL,
	freeCust
};

void PlayerGuide_Player_Login(gentity_t *ent) {

}

void Cmd_Guide_f(gentity_t *ent) {

}

#endif