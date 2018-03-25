#ifndef __G_ACTIVE_H__
#define __G_ACTIVE_H__

#include "g_local.h"
#include "bg_saga.h"

//typedef 
enum
{
	TAUNT_TAUNT = 0,
	TAUNT_BOW,
	TAUNT_MEDITATE,
	TAUNT_FLOURISH,
	TAUNT_GLOAT
};

void G_SetTauntAnim( gentity_t *ent, int taunt );


#endif //__G_ACTIVE_H__
