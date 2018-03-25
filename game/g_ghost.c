#include "g_local.h"

#include "Lmd_EntityCore.h"

qboolean isGhost (gentity_t *ent){
	if (ent->client && (ent->client->ps.stats[STAT_WEAPONS] & (1 << WP_MELEE))) {
			return qtrue;
	}
	return qfalse;
}

void WP_RemoveTrickedEnt(forcedata_t *fd, int client){
	if (!fd)
		return;

	if (client > 47)
		fd->forceMindtrickTargetIndex4 &= ~(1 << (client-48));
	else if (client > 31)
		fd->forceMindtrickTargetIndex3 &= ~(1 << (client-32));
	else if (client > 15)
		fd->forceMindtrickTargetIndex2 &= ~(1 << (client-16));
	else
		fd->forceMindtrickTargetIndex &= ~(1 << client);
}
void WP_AddAsMindtricked(forcedata_t *fd, int entNum);

void updateGhost (gentity_t *self){

	int i = 0;

	if (isGhost(self))
		self->client->ps.powerups[PW_CLOAKED] = INT_MAX;
	else{
		self->client->ps.powerups[PW_CLOAKED] = 0;
		self->client->ps.trueNonJedi = qtrue;
	}


	while(i < MAX_CLIENTS){
		gentity_t *ent = &g_entities[i];
		if(isGhost(self) && !(ent->client && (ent->client->ps.stats[STAT_WEAPONS] & ((1 << WP_CONCUSSION) | (1 << WP_MELEE)))))
			WP_AddAsMindtricked(&self->client->ps.fd, i);
		else
			WP_RemoveTrickedEnt(&self->client->ps.fd, i);
		i++;
	}

}

char *G_NewString2( const char *string );
//void GenericSpawn (gentity_t *ent, gentity_t *spawn, vec3_t origin);
extern qboolean disablesenabled;
void LogExit( const char *string );


void ghost_exit_touch(gentity_t *self, gentity_t *other, trace_t *trace){
	if (level.time < self->genericValue10) {
		return;
	}
	self->genericValue10 = level.time + 500;
	if (!other->client
		|| other->client->sess.sessionTeam != self->s.teamowner
		|| isGhost(other)
		|| other->client->ps.stats[STAT_HEALTH] <= 0
		|| other->health <= 0) {
			return;
	}
	trap_SendServerCommand(-1, va("print \"%s has escaped.\n\"", 
		other->client->pers.netname));
	team_t team = other->client->sess.sessionTeam;
	level.teamScores[team] += 1;
	CalculateRanks();
	other->client->ps.stats[STAT_WEAPONS] =
		(1 << WP_MELEE);
	other->client->ps.ammo[other->client->ps.weapon] = 0;
	other->client->ps.weapon = WP_MELEE;

	//player_die(other, other, other, 999, MOD_UNKNOWN);

	//Player exits
	/*
	other->client->tempSpectate = level.time + INT_MAX;
	other->health = other->client->ps.stats[STAT_HEALTH] = -999;
	other->client->ps.weapon = WP_NONE;
	other->client->ps.stats[STAT_WEAPONS] = 0;
	other->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
	other->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	other->takedamage = qfalse;
	other->client->ps.pm_type = PM_DEAD;
	other->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	other->client->respawnTime = INT_MAX;
	trap_LinkEntity(other);

	int i;
	int score = 0;
	qboolean win = qtrue;
	team_t team = other->client->sess.sessionTeam;

	for (i = 0; i < MAX_CLIENTS; i++){
	if (level.clients[i].pers.connected != CON_CONNECTED ||
	level.clients[i].sess.sessionTeam != team) {
	continue;
	}
	if (level.clients[i].respawnTime < INT_MAX) {
	win = qfalse;
	} else {
	score++;
	}

	}
	level.teamScores[team] = score;
	CalculateRanks();
	if (win) {
	if ( team == TEAM_BLUE) {
	trap_SendServerCommand( -1, "print \"Blue team won.\n\"");
	LogExit( "Blue team won." );
	return;
	} else {
	trap_SendServerCommand( -1, "print \"Red team won.\n\"");
	LogExit( "Red team won." );
	return;
	}
	}
	*/
}

void SP_ghost_exit(gentity_t *ent){
	vec3_t origin,rgt;
	gentity_t *gent, *hent;
	qboolean saveddis = disablesenabled;
	disablesenabled = qfalse;

	VectorCopy(ent->s.origin, origin);
	AngleVectors(ent->s.angles, NULL, rgt, NULL);

	VectorMA(origin, 60, rgt, origin);

	if(!(gent = trySpawn(va("classname,weapon_concussion_rifle,angle,%.0f,origin,%s", ent->s.angles, vtos2(origin)))))
		return;


	//GenericSpawn(NULL, gent, origin);


	VectorMA(origin, -120, rgt, origin);
	if(!(hent = trySpawn(va("classname,weapon_disruptor,angle,%.0f,origin,%s", ent->s.angles, vtos2(origin)))))
		return;


	//GenericSpawn(NULL, hent, origin);

	if(Q_stricmp(ent->classname, "ghost_exit_red")){
		hent->s.teamowner = TEAM_RED;
		gent->s.teamowner = TEAM_RED;
		ent->s.teamowner  = TEAM_BLUE;
	}
	else{
		hent->s.teamowner = TEAM_BLUE;
		gent->s.teamowner = TEAM_BLUE;
		ent->s.teamowner  = TEAM_RED;
	}
	disablesenabled = saveddis;

	// Set up the exit

	VectorSet(ent->r.mins, -20, -20, 0);
	VectorSet(ent->r.maxs,  20,  20, 40);
	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = ghost_exit_touch;
	trap_LinkEntity(ent);
}

int OtherTeam(int team);
extern int g_siegeRespawnCheck;

/*
void
returnGhost(team_t team) 
{
int i;
int score = 0;
int clientNum;
team_t otherTeam = OtherTeam(team);

for (i = 0; i < MAX_CLIENTS; i++){
if (level.clients[i].pers.connected != CON_CONNECTED ||
level.clients[i].sess.sessionTeam != otherTeam ||
level.clients[i].respawnTime < INT_MAX) {
continue;
}
score++;
clientNum = i;
}
if (score) {
score--;
gclient_t *client = &level.clients[clientNum];

client->tempSpectate = level.time + 40000;
client->respawnTime = level.time;
client->ps.weapon = WP_NONE;
client->ps.stats[STAT_WEAPONS] = 0;
client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
client->ps.stats[STAT_HOLDABLE_ITEM] = 0;

// Respawn time.
gentity_t *te = G_TempEntity( client->ps.origin, EV_SIEGESPEC );
te->s.time = g_siegeRespawnCheck;
te->s.owner = clientNum;
}
level.teamScores[otherTeam] = score;
CalculateRanks();
}
*/
