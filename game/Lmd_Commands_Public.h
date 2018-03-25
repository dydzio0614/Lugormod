
#ifndef LMD_COMMANDS_PUBLIC_H
#define LMD_COMMANDS_PUBLIC_H

#include "gentity_t.h"

typedef struct cmdEntry_s cmdEntry_t;


typedef struct cmdCategory_s{
	char *name;
	char *descr;
	int color;
	cmdEntry_t *entries;
} cmdCategory_t;


typedef struct cmdEntry_s{
	char *name;
	char *descr;
	void (*func)(gentity_t *ent, int iArg);
	int iArg;
	qboolean isAdmin;
	int level;     //allowed at player / admin level 
	int disable;   //g_cmdDisable (mask)
	int gametype;  //disabled in gametype (mask)
	int profession; //usable by profession (0 = all)
	qboolean locked;

	//Non predefined, set on load.
	//For increased performance in authfile info functions (would have a loop in a loop in a loop otherwise).
	cmdCategory_t *category;
}cmdEntry_t;

#endif