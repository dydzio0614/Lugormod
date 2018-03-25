
#include <stdlib.h>

#include "Lmd_API_Professions.h"
#include "Lmd_Professions.h"

extern profession_t *Professions[];

int LmdApi_Professions_GetNumProfessions() {
	return NUM_PROFESSIONS;
}

profession_t *LmdApi_Professions_GetProfession(int index) {
	if (index < 0 || index >= NUM_PROFESSIONS) {
		return NULL;
	}

	return Professions[index];
}

LmdApi_Professions_v1_t api_professions_v1 = {
	LmdApi_Professions_GetNumProfessions,
	LmdApi_Professions_GetProfession,
};

const void *LmdApi_Get_Professions(unsigned int version) {
	if (version == LMDAPI_PROFESSIONS_VERSION_1) {
		return &api_professions_v1;
	}

	return NULL;
}