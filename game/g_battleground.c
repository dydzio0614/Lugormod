#include "g_local.h"

#define CP_TIME             500
#define CP_RECOVER          250
#define CP_POINTS           20
#define CP_IMPORTANT_POINTS 30

#define CP_NOCHANGE   1
#define CP_IMPORTANT  2
#define CP_USE        4
#define MAX_CPS 16
#define BG_IMP_ICON "gfx/2d/mp_imp_symbol.tga"
#define BG_REB_ICON "gfx/2d/mp_rebel_symbol.tga"
#define BG_IMP_ICON_W "gfx/2d/mp_imp_symbol_3.tga"
#define BG_REB_ICON_W "gfx/2d/mp_rebel_symbol_3.tga"
#define CP_MAP_SIZE 6
#define CP_MAP_WIDTH 4
#define CP_MAP_CHARS CP_MAP_SIZE * CP_MAP_SIZE * CP_MAP_WIDTH + CP_MAP_SIZE + 1 + (2 * MAX_CPS)

//#define MODEL_NEUTRAL "models/map_objects/mp/"
//#define MODEL_BLUE    "models/map_objects/mp/"
//#define MODEL_RED     "models/map_objects/mp/"

/*
void
control_point_touch (gentity_t *self, gentity_t *other, trace_t *trace)
{
}
*/
static char teamNameList[][20] = {"^7Free", "^1Red", "^4Blue", "^3Spectators", "^2Jailed"};
vec_t cpmapmax = -WORLD_SIZE;
vec_t cpmapmin =  WORLD_SIZE;

//void GenericSpawn (gentity_t *ent, gentity_t *spawn, vec3_t origin);

qboolean PlaceWouldTelefrag( vec3_t origin ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorSet( mins, -200, -200, -200 );
	VectorSet( maxs,  200,  200, 200 );
	VectorAdd(maxs, origin, maxs);
	VectorAdd(mins, origin, mins);

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

void control_point_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	if (level.time < self->genericValue1) {
		return;
	}
	self->genericValue1 = level.time + 1000;
	vec3_t dir;
	int diff;

	VectorSubtract(other->client->ps.origin, self->r.currentOrigin, dir);
	vectoangles(dir,dir);
	if (dir[YAW] > 180) {
		dir[YAW] -= 360; }
	diff = (int)Q_fabs(dir[YAW] - self->s.apos.trBase[YAW]);
	if (diff > 180) {
		diff = 360 - diff; }

	if ( diff > 40) {
		//face the front
		return;
	}

	if (!other->client) {
		return;
	}

	if (other->client->sess.sessionTeam == self->alliedTeam)
	{
		gentity_t *next = self;
		while (1) {
			next = G_Find(next, FOFS(classname), "control_point");
			if (next == self) {
				//showCPMap(next->s.number, other->s.number);
				if (self->message && self->message[0]) {
					trap_SendServerCommand(other->s.number, va("cp \"control point %s.\"",self->message));
				}

				return;
			}

			if (!next 
				|| next->alliedTeam != 
				other->client->sess.sessionTeam 
				|| next->genericValue1 >= level.time
				|| PlaceWouldTelefrag(next->s.origin)) {
					continue;
			}
			vec3_t diff, diffa, origin;
			VectorSubtract(other->client->ps.origin, self->s.origin, diff);
			vectoangles(diff, diffa);

			diffa[YAW] -= self->s.apos.trBase[YAW];
			diffa[YAW] += next->s.apos.trBase[YAW];
			diffa[YAW] = AngleNormalize180(diffa[YAW]);

			AngleVectors(diffa, diffa, 0, 0);
			VectorNormalize(diffa);

			VectorMA(next->s.origin, VectorLength(diff),diffa, origin);

			diff[YAW] = next->s.apos.trBase[YAW] - 
				self->s.apos.trBase[YAW];
			VectorCopy(other->client->ps.viewangles, diffa);
			diffa[YAW] = AngleNormalize180(diffa[YAW] - self->s.apos.trBase[YAW] + next->s.apos.trBase[YAW]);
			origin[2] += 1;
			TeleportPlayer(other, origin, diffa, qfalse);

			VectorSet(other->client->ps.velocity, 0,0,0);
			next->genericValue1 = level.time + 1000;

			//showCPMap(next->s.number, other->s.number);
			if (next->message && next->message[0]) {
				trap_SendServerCommand(other->s.number, va("cp \"control point %s.\"",next->message));
			}
			return;
		}
		return; //shouldn't get here
	}
	if (self->spawnflags & CP_NOCHANGE) {
		return;
	}
	//if (!(self->spawnflags & CP_USE)) {
	//        return;
	//}

	if (other->client->isHacking != self->genericValue10) {
		if (other->client->sess.sessionTeam != self->genericValue14) {
			self->genericValue14 = other->client->sess.sessionTeam;
			self->genericValue15 = self->count * 1000;
		}
		other->client->isHacking = self->genericValue10;
		other->client->ps.hackingTime = level.time + 
			self->genericValue15;
		other->client->ps.hackingBaseTime = self->count * 1000;
		VectorCopy(other->client->ps.viewangles, 
			other->client->hackingAngles);
		self->activator = other;
		if (self->alliedTeam) {
			if (self->message && self->message[0]) {
				trap_SendServerCommand (-1,va("print \"%s team is trying to capture the command post %s ...\n\"",teamNameList[other->client->sess.sessionTeam], self->message));
			}
			else 
			{
				trap_SendServerCommand (-1,va("print \"%s team is trying to capture a command post ...\n\"",teamNameList[other->client->sess.sessionTeam]));
			}
			//showCPMap(self->s.number, -1);
		}

		return;
	} else if (other->client->ps.hackingTime > level.time) {
		//is hacking
		self->genericValue1 = level.time + 50;
		self->genericValue15 = other->client->ps.hackingTime - 
			level.time;
		return;
	}
	self->genericValue15 = self->count * 1000;
	other->client->ps.hackingTime = 0;
	other->client->ps.hackingBaseTime = 0;
	other->client->isHacking = 0;

	if (self->alliedTeam ) {
		if (self->message && self->message[0]) {
			trap_SendServerCommand(-1, va("print \"%s team captured the command post %s.\n\"", teamNameList[other->client->sess.sessionTeam],self->message));
		}
		else 
		{
			trap_SendServerCommand(-1, va("print \"%s team captured a command post.\n\"", teamNameList[other->client->sess.sessionTeam]));
		}
		self->alliedTeam = other->client->sess.sessionTeam;
		//showCPMap(0, -1);
	}
	else {
		self->alliedTeam = other->client->sess.sessionTeam;
		//showCPMap(0, other->s.number);
	}
	if (self->spawnflags & CP_IMPORTANT) {
		other->client->ps.persistant[PERS_SCORE] +=CP_IMPORTANT_POINTS;
	} else {
		other->client->ps.persistant[PERS_SCORE] +=CP_POINTS;
	}
	CalculateRanks();

	//self->s.teamowner = self->alliedTeam;

	self->genericValue1 = level.time + 2000;
	if (self->alliedTeam == TEAM_RED) {
		self->s.constantLight = 100 + (100 << 24);
		//self->s.genericenemyindex = G_IconIndex(BG_IMP_ICON);
		self->s.genericenemyindex = self->genericValue3;
	} else if (self->alliedTeam == TEAM_BLUE) {
		self->s.constantLight = (100 << 16) + (100 << 24);
		//self->s.genericenemyindex = G_IconIndex(BG_REB_ICON);
		self->s.genericenemyindex = self->genericValue4;
	}
	trap_LinkEntity(self);

}

void
control_point_think (gentity_t *ent)
{
	ent->genericValue13++;
	ent->nextthink = level.time + CP_TIME;

	if (ent->s.genericenemyindex != ent->genericValue3
		&& ent->s.genericenemyindex != ent->genericValue4) {
			if (ent->alliedTeam == TEAM_RED) {
				ent->s.genericenemyindex = ent->genericValue3;
			} else if (ent->alliedTeam == TEAM_BLUE) {
				ent->s.genericenemyindex = ent->genericValue4;
			}
	} else if (ent->activator && ent->activator->client
		&& ent->activator->client->isHacking == ent->genericValue10
		&& ent->activator->client->ps.hackingTime > level.time) {
			if (ent->alliedTeam == TEAM_RED) {
				ent->s.genericenemyindex = ent->genericValue5;
			} else if (ent->alliedTeam == TEAM_BLUE) {
				ent->s.genericenemyindex = ent->genericValue6;
			}
	}

	if (ent->genericValue15 < ent->count * 1000 &&
		!(ent->genericValue13%2)) {
			ent->genericValue15 += CP_RECOVER;
	}
	if (ent->genericValue15 > ent->count * 1000) {
		ent->genericValue15 = ent->count * 1000;
	}

	int time;

	if (ent->spawnflags & CP_IMPORTANT ) {
		time = CP_POINTS;
	} else {
		time = CP_IMPORTANT_POINTS;
	}
	if (ent->genericValue13%time == 0) {
		if (ent->alliedTeam != TEAM_RED) {
			level.teamScores[TEAM_RED]--;
		}
		if (ent->alliedTeam != TEAM_BLUE) {
			level.teamScores[TEAM_BLUE]--;
		}
		CalculateRanks();
	}

}

void zone_think (gentity_t *ent);
int trap_RealTime( qtime_t *qtime );
//char* fixpath (char *str);
//extern unsigned char model_frames [MAX_MODELS];
//extern vec3_t model_mins [MAX_MODELS];
//extern vec3_t model_maxs [MAX_MODELS];
//void animate_model (gentity_t *self);
void SP_misc_model_breakable(gentity_t *ent);


void SP_control_point (gentity_t *ent)
{
	if (ent->alliedTeam == 0 
		&& ent->spawnflags & CP_NOCHANGE) {
			ent->spawnflags &= ~CP_NOCHANGE;
	}
	if (g_gametype.integer != GT_BATTLE_GROUND) {
		ent->s.eFlags = EF_NODRAW;
		ent->r.contents = 0;
		ent->clipmask = 0;
		return;

	}
	ent->s.eFlags |= EF_RADAROBJECT;
	ent->genericValue3 = G_IconIndex(BG_IMP_ICON);
	ent->genericValue4 = G_IconIndex(BG_REB_ICON);
	ent->genericValue5 = G_IconIndex(BG_IMP_ICON_W);
	ent->genericValue6 = G_IconIndex(BG_REB_ICON_W);

	//if (!(ent->spawnflags & CP_NOCHANGE)) {
	//}

	if (ent->spawnflags & CP_IMPORTANT) {
		ent->count = CP_IMPORTANT_POINTS;
	} else {
		ent->count = CP_POINTS;
	}
	ent->genericValue15 = ent->count * 1000;
	ent->genericValue14 = 0;

	//ent->s.teamowner = ent->alliedTeam;
	level.teamScores[TEAM_RED]  += 2 * ent->count;
	level.teamScores[TEAM_BLUE] += 2 * ent->count;
	if (ent->alliedTeam == TEAM_RED) {
		ent->s.constantLight =  100 + (100 << 24);
		//ent->s.genericenemyindex = G_IconIndex(BG_IMP_ICON);
		ent->s.genericenemyindex = ent->genericValue3;
	} else if (ent->alliedTeam == TEAM_BLUE) {
		ent->s.constantLight = (100 << 16) + (100 << 24);
		//ent->s.genericenemyindex = G_IconIndex(BG_REB_ICON);
		ent->s.genericenemyindex = ent->genericValue4;
	}

	/*
	vec3_t origin;
	gentity_t *fx;
	if (ent->alliedTeam && (fx = G_Spawn())) {
	if (ent->alliedTeam == TEAM_RED) {
	fx->spawnString = "classname,fx_runner,fxfile,effects/mp/redcrystalrespawn,delay,400,angles,90 0 0,random,200";
	} else {
	fx->spawnString = "classname,fx_runner,fxfile,effects/mp/bluecrystalrespawn,delay,400,angles,90 0 0,random,200";
	}
	VectorCopy(ent->s.origin, origin);
	origin[2] += 200;
	GenericSpawn(NULL,fx,origin);
	ent->parent = fx;
	}
	*/
	G_Free(ent->model);
	G_SpawnString("model", "map_objects/imperial/controlpanel", &ent->model);
	int spawnflags = ent->spawnflags;
	ent->spawnflags = 3;
	SP_misc_model_breakable(ent);
	ent->use = control_point_use;
	/*
	char *model = G_NewString(ent->model);
	if (strstr(model, ".glm")) {
	ent->s.modelindex = G_ModelIndex(va("models/%s", fixpath(model)));
	//trap_G2API_InitGhoul2Model(&ent->ghoul2, va("models/%s", fixpath(model)), 0, 0, 0, 0, 0);
	ent->s.modelGhoul2 = 1;
	G_SpawnVector( "maxs", "8 8 16", ent->r.maxs);
	G_SpawnVector( "mins", "-8 -8 0", ent->r.mins);
	//ent->s.g2radius = 64;
	} else {
	ent->s.modelindex = G_ModelIndex(va("models/%s.md3", fixpath(model)));
	G_SpawnVector( "maxs", 
	vtos2(model_maxs[ent->s.modelindex]), 
	ent->r.maxs);
	G_SpawnVector( "mins",
	vtos2(model_mins[ent->s.modelindex]), 
	ent->r.mins);
	if (//(ent->spawnflags & 2) &&
	(model_frames[ent->s.modelindex] > 1)) {
	ent->genericValue5 = 
	model_frames[ent->s.modelindex];
	ent->think = animate_model;
	ent->nextthink = level.time;// + 100;
	//ent->s.eFlags |= EF_CLIENTSMOOTH;
	}

	}
	G_Free(model);
	//ent->s.modelindex = G_ModelIndex("models/map_objects/imperial/controlpanel.md3");
	//ent->s.modelindex = G_ModelIndex(ent->model);
	G_SetOrigin( ent, ent->s.origin );
	//VectorCopy(ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	*/
	ent->spawnflags = spawnflags;

	ent->r.svFlags = SVF_PLAYER_USABLE|SVF_BROADCAST;
	//G_SpawnVector("mins", "-30 -30 0",ent->r.mins);
	//G_SpawnVector("maxs", "30 30 50",ent->r.maxs);
	//VectorSet(ent->r.mins, -30, -30, 0);
	//VectorSet(ent->r.maxs, 30, 30, 50);
	ent->genericValue1 = 0;
	ent->think = control_point_think;
	ent->nextthink = level.time + CP_TIME * Q_irand(1,CP_IMPORTANT_POINTS);
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_PLAYERSOLID;
	trap_LinkEntity(ent);
	//I need better rand_init
	qtime_t now;
	trap_RealTime(&now);
	Rand_Init( (60 * (60 * (24 *  now.tm_yday) 
		+ now.tm_hour) + now.tm_min) + now.tm_sec );
	gentity_t *zone = G_Spawn();
	if (!zone) {
		G_FreeEntity(ent);
		return;
	}
	if (ent->s.origin[0] > cpmapmax) {
		cpmapmax = ent->s.origin[0];
	}
	if (ent->s.origin[1] > cpmapmax) {
		cpmapmax = ent->s.origin[1];
	}
	if (ent->s.origin[0] < cpmapmin) {
		cpmapmin = ent->s.origin[0];
	}
	if (ent->s.origin[1] < cpmapmin) {
		cpmapmin = ent->s.origin[1];
	}

	zone->classname = "control_point_zone";
	zone->parent = ent;
	zone->think = zone_think;
	zone->nextthink = level.time + 1000;
	zone->s.eFlags = EF_NODRAW;
	zone->r.contents = 0;
	zone->clipmask = 0;
	zone->r.maxs[0] = ent->r.maxs[0] * 2;
	zone->r.maxs[1] = ent->r.maxs[1] * 2;
	zone->r.maxs[2] = ent->r.maxs[2];
	zone->r.mins[0] = ent->r.mins[0] * 2;
	zone->r.mins[1] = ent->r.mins[1] * 2;
	zone->r.mins[2] = ent->r.mins[2];
	G_SetOrigin(zone, ent->r.currentOrigin);
	ent->genericValue10 = zone->s.number;

	trap_LinkEntity(zone);

}
int
BattleGroundControlPoints (gentity_t **list, team_t team) 
{
	int cpc = 0;
	gentity_t *ent = NULL;
	//Put all control points in a list
	while ((ent = G_Find (ent, FOFS(classname), 
		"control_point")) != NULL) {
			if (team && team != ent->alliedTeam) {
				continue;
			}
			if (cpc == MAX_CPS) {
				G_FreeEntity(ent);
				continue;
			}
			if (list) {
				list[cpc++] = ent;
			} else {
				cpc++;
			}
	}
	return cpc;
}

/*
void
showCPMap (int steal, int client)
{
gentity_t *cps[MAX_CPS];
int cpc = BattleGroundControlPoints(cps, 0);
int i;
char m;
int x,y,c;
char cpmap[CP_MAP_CHARS];
char str[CP_MAP_CHARS];

if (cpc < 2) {
return;
}
memset ((void*)cpmap, ' ', CP_MAP_CHARS);
cpmap[(CP_MAP_SIZE * CP_MAP_WIDTH * CP_MAP_SIZE)] = '\0';

for (i = 0; i < cpc;i++){
if (steal > MAX_CLIENTS && cps[i]->s.number == steal) {
m = '!';
} else if (cps[i]->alliedTeam == TEAM_RED) {
m = 'R';
} else if (cps[i]->alliedTeam == TEAM_BLUE) {
m = 'B';
} else {
m = 'X';
}
x = (int)((cps[i]->s.origin[0] - cpmapmin) * ((float)(CP_MAP_SIZE * CP_MAP_WIDTH - 1) / (cpmapmax - cpmapmin)));
y = (int)((cps[i]->s.origin[1] - cpmapmin) * ((float)(CP_MAP_SIZE - 1)/ (cpmapmax - cpmapmin)));
c = x + (y * CP_MAP_SIZE * CP_MAP_WIDTH);
cpmap[c] = m;

}
for (i = 0, c = 0; cpmap[i]; i++) {
if (cpmap[i] == '!') {
str[c++] = '^';
str[c++] = '7';
str[c++] = 'X';
} else if (cpmap[i] == 'R') {
str[c++] = '^';
str[c++] = '1';
str[c++] = '*';
} else if (cpmap[i] == 'B') {
str[c++] = '^';
str[c++] = '4';
str[c++] = '*';
} else if (cpmap[i] == 'X') {
str[c++] = '^';
str[c++] = '7';
str[c++] = '*';
} else {
str[c++] = ' ';
}
if ((i + 1)%(CP_MAP_SIZE * CP_MAP_WIDTH) == 0) {
str[c++] = '\n';

}
}
str[c] = '\0';

trap_SendServerCommand(client, va("cp \"%s\"", str));
}
*/

gentity_t*
closest (vec3_t origin, gentity_t **cps, int cpc)
{
	int i;
	vec_t d,closest;
	gentity_t *ent = NULL;

	if (!cpc) {
		return NULL;
	}

	closest = WORLD_SIZE;
	for (i = 0; i < cpc; i++) {
		d = Distance(origin, cps[i]->s.origin);
		if (d < closest) {
			closest = d;
			ent = cps[i];
		}
	}
	return ent;
}

/*
===========
SelectBGSpawnPoint

============
*/

#define	MAX_BG_SPAWN_POINTS	128
gentity_t *SelectRandomBGSpawnPoint( int teamstate, team_t team, int siegeClass, vec3_t origin ) {
	gentity_t	*spot;
	int			count;
	gentity_t	*spots[MAX_BG_SPAWN_POINTS];
	char		*classname = "info_player_deathmatch";
	count = 0;
	spot = NULL;
	//vec_t dist, dists[MAX_BG_SPAWN_POINTS];

	while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {

		if (!spot->parent || spot->parent->alliedTeam != team)
		{ //not our team, can't use it
			continue;
		}

		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}

		//dists[ count ] = Distance(origin, spot->s.origin);
		spots[ count ] = spot;

		//sort em
		/*
		for (i = 0; i < count && i < 10; i++) {
		if (dists[count] > dists[i]) {
		dist = dists[i];
		dists[i] = dists[count];
		dists[count] = dist;
		spot = spots[i];
		spots[i] = spots[count];
		spots[count] = spot;
		}
		}
		*/
		if (++count == MAX_BG_SPAWN_POINTS)
			break;
	}
	if ( !count ) {	// no spots that won't telefrag
		spot = NULL;

		while ((spot = G_Find (spot, FOFS(classname), 
			classname)) != NULL) {
				if (!spot->parent || spot->parent->alliedTeam != team)
				{ //not our team, can't use it
					continue;
				}

				//dists[ count ] = Distance(origin, spot->s.origin);
				spots[ count ] = spot;

				//sort em
				/*
				for (i = 0; i < count && i < 10; i++) {
				if (dists[count] > dists[i]) {
				dist = dists[i];
				dists[i] = dists[count];
				dists[count] = dist;
				spot = spots[i];
				spots[i] = spots[count];
				spots[count] = spot;
				}
				}
				*/
				if (++count == MAX_BG_SPAWN_POINTS)
					break;
		}
	}
	if (!count) {
		return NULL;
	}
	/*
	if (count > 10) {
	i = 9;
	} else {
	i = count - 1;
	}
	spot = spots[Q_irand(0, i)];
	*/
	spot = spots[Q_irand(0, count - 1)];
	return spot;
}

gentity_t *SelectBGSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = SelectRandomBGSpawnPoint ( ent->client->pers.teamState.state, ent->client->sess.sessionTeam, ent->client->siegeClass, origin );

	if (!spot) {
		return SelectSpawnPoint( ent, origin, angles);
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/
void LinkBGSpawnPoints (void) 
{
	gentity_t *ent;
	gentity_t *entlist[MAX_CPS];
	int cpc = BattleGroundControlPoints(entlist, 0);

	//Use G_Find for logical ent support.
	ent = NULL;
	while(ent = G_Find(ent, FOFS(classname), "info_player_deathmatch"))
		ent->parent = closest(ent->s.origin, entlist, cpc);
	ent = NULL;
	while(ent = G_Find(ent, FOFS(classname), "misc_turretG2"))
		ent->parent = closest(ent->s.origin, entlist, cpc);
	ent = NULL;
	while(ent = G_Find(ent, FOFS(classname), "lmd_spawner"))
		ent->parent = closest(ent->s.origin, entlist, cpc);
	
}

#undef MAX_CPS
#undef CP_NOCHANGE
#undef CP_POINTS
#undef CP_IMPORTANT_POINTS
#undef CP_IMPORTANT
#undef MODEL_NEUTRAL
#undef MODEL_BLUE
#undef MODEL_RED
#undef BG_REB_ICON
#undef BG_IMP_ICON
#undef CP_MAP_SIZE
#undef CP_MAP_WIDHT
