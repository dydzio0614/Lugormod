
#include "Lmd_Professions_Public.h"

#define LMDAPI_PROFESSIONS_VERSION_1 (0)

#define LMDAPI_PROFESSIONS_VERSION_CURRENT LMDAPI_PROFESSIONS_VERSION_1

/*
Manipulate profession information.

Currently, all profession data is defined at compile time.
You may fetch and manipulate the skill trees as you wish, as long as you do it
before the mod dll recieves GAME_INIT.  Doing so afterward will cause unpredictable behavior.

You may replace the function calls in profSkill_t, and the mod will make an effort to use them.

Some areas of the mod might not use them for the active skill effect.
If you find a skill where they do not seem to have an effect,
let me know and I will check and correct this.

Note that the /skills command will always use the callbacks defined here, even if the skill effect
is directly read from the internal profession data.


At the moment, there is no support for adding new skills or professions, this may come in a later release.
*/

typedef struct LmdApi_Professions_v1_s {
	// Gets the number of professions
	int (*getProfessionCount)();

	profession_t* (*getProfession)(int index);
} LmdApi_Professions_v1_t;


// Represents the current memory function list at the time of compile.
#define LmdApi_Professions_t LmdApi_Professions_v1_t

#ifdef LUGORMOD
const void *LmdApi_Get_Professions(unsigned int version);
#else
// Fetch the current function list at the time of compile.
// May return NULL if the version is no longer supported.
#define LmdApi_GetCurrent_Professions() (LmdApi_Professions_t*) LmdApi_Get_Professions(LMDAPI_PROFESSIONS_VERSION_CURRENT)
#endif