#include "g_local.h"
#include "Lmd_Data.h"
#include "Lmd_Commands_Auths.h"

//RoboPhred
#if 0


#include "Lmd_Arrays.h"
#include "Lmd_Data.h"

typedef struct lcobject_s{
	vec3_t position;
	char *name;
}lcobject_t;

struct lcArray_s{
	unsigned int count;
	lcobject_t *Locations;
}lcArray;

#error Need to add call to location loader func.
#error make a generic func to find the closest location, rather than repeat the same block of code

void CreateNewLC(vec3_t origin, char *name){
	unsigned int i = Lmd_Arrays_AddArrayElement(&(void *)lcArray.Locations, sizeof(lcobject_t), &lcArray.count);
	lcArray.Locations[i].name = G_NewString2(name);
	VectorCopy(origin, lcArray.Locations[i].position);
}

qboolean parseLocationData(int *iter){

}
void queue_save (const char *filename, void *parsefunc);

int SaveLocationData(void){
	
	return 0;
}

int LoadLocationData(const char *filename)
{
	fileHandle_t f;
	char *fileString;
	char currentVar[2048];
	char lcName[MAX_STRING_CHARS];
	vec3_t lcPosition;
	int len;
	int i, i2, i_cv;

	i = 0;
	i_cv = 0;

	while(i < len){
		i_cv = 0;

		//VectorCopy(vec3_origin, lcPosition);
		//lcName = NULL;

		while(fileString[i] != ' ')
			i++;
		i++;

		for(i2 = 0;i2<3;i2++){
			while(fileString[i] != ' '){
				currentVar[i_cv] = fileString[i];
				i_cv++;
				i++;
			}
			currentVar[i_cv] = '\0';

			lcPosition[i2] = atof(currentVar);

			i_cv = 0;
			i++;
		}

		while (fileString[i] != '\"')
			i++;
		i++;

		while (fileString[i] != '\"'){
			lcName[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		lcName[i_cv] = '\0';

		CreateNewLC(lcOrigin, lcName);
		i++;
		while (fileString[i] != '\n')
			i++;
		i++;
	}

	trap_FS_FCloseFile(f);

	return 1;
}

int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore, int ignore2);

void Cmd_botReportLocation(bot_state_t *bs, gentity_t *ent){
	int i;
	int closesti = 0;
	float d, closestd;
	closestd = WORLD_SIZE;
	vec3_t a;
	float diff, angle;

	if(!lcArray.count)
		return;

	for(i = 0; i < lcArray.count;i++){
		d = Distance(bs->origin, lcArray.Locations[i].position);
		if(d < closestd){
			closesti = i;
			closestd = d;
		}

	}
	Q_strncpyz(bs->currentChat, "I am ", sizeof(bs->currentChat));
	if (ent && ent->client) { //ent->client is probably overkill ...

		VectorSubtract( bs->cur_ps.origin, ent->client->ps.origin, a);

		d = abs((int)a[2]);
		vectoangles(a, a);
		angle = AngleMod(ent->client->ps.viewangles[1]);
		a[1] = AngleMod(a[1]);
		diff = angle - a[1];
		if (a[1] < angle){
			if (diff > 180.0)
				diff -= 360.0;
		}
		else{
			if (diff < -180.0)
				diff += 360.0;
		}
	}

	if(!ent || !ent->client || !EntityVisibleBox(bs->origin, 0, 0, ent->client->ps.origin, bs->client, ent->s.number))
		Q_strcat(bs->currentChat, sizeof(bs->currentChat), va("near %s", lcArray.Locations[closesti].name));
	else if(diff <= -135 || diff >= 135)
		Q_strcat(bs->currentChat, sizeof(bs->currentChat),"behind you.");
	else if(diff <= -45)
		Q_strcat(bs->currentChat, sizeof(bs->currentChat),"to your left.");
	else if(diff >= 45)
		Q_strcat(bs->currentChat, sizeof(bs->currentChat),"to your right.");
	else
		Q_strcat(bs->currentChat, sizeof(bs->currentChat),"in front of you.");


	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;
	bs->doChat = 1;

}

void Cmd_AddLocation_f (gentity_t *ent, int iArg){
	if(trap_Argc() < 2)
		return;
	Q_strncpyz(loc.name,ConcatArgs(1), sizeof(loc.name));
	CreateNewLC(ConcatArgs(1), ent->client->ps.origin);
	SaveLocationData();
}

gentity_t *ClientFromArg (gentity_t *to, int argNum);

void Cmd_Position_f (gentity_t *ent, int iArg){

	char tmpstr[MAX_STRING_CHARS];
	gentity_t *tEnt;
	int i, closesti = -1;
	vec_t d, closestd = 8192;

	if(lcArray.count == 0){
		Disp(ent, "^3No locations added.");
		return;
	}

	if(trap_Argc() > 1){
		tEnt = ClientFromArg(ent, 1);

		if(tEnt && tEnt->client){
			if(Auths_Inferior (ent, tEnt))
				return;
			Q_strncpyz(tmpstr, va("%s is ", tEnt->client->pers.netname), sizeof(tmpstr));
		}

	}
	else{
		tEnt = ent;
		Q_strncpyz(tmpstr, "You are ",sizeof(tmpstr));
	}

	if(!tEnt)
		return;

	for (i = 0; i < lcArray.count; i++) {
		d = Distance(tEnt->client->ps.origin, lcArray.Locations[i].position);
		if(d < closestd){
			closesti = i;
			closestd = d;
		}

	}

	if(closesti < 0)
		Q_strcat(tmpstr, sizeof(tmpstr), "in an unknown area.");
	else
		Q_strcat(tmpstr, sizeof(tmpstr), va("near %s", lcArray.Locations[closesti].name));

	Disp (ent, tmpstr);
}

int ClientNumberFromString( gentity_t *to, char *s );
extern bot_state_t *botstates[MAX_CLIENTS];

gentity_t* AimTarget (gentity_t *ent, int target);
qboolean isLame (bot_state_t *bs);

void Cmd_botStatus_f (void) 
{
	if (trap_Argc() < 2) {
		Com_Printf("ERROR: Wrong number of arguments\n");
		return;
	}

	//char otherindex[MAX_TOKEN_CHARS];
	int i;
	bot_state_t *bs;


	//trap_Argv( 1, otherindex, sizeof( otherindex ) );

	i = ClientNumberFromString(NULL, ConcatArgs(1));

	bs = botstates[i];
	if (!bs || !bs->inuse) {
		Com_Printf("ERROR: Not a bot.\n");

		return;

	}
	int closesti;
	float d, closestd;
	closestd = 4096;
	char ans[MAX_STRING_CHARS];
	Q_strncpyz(ans, level.clients[bs->client].pers.netname, sizeof(ans));
	Q_strcat(ans, sizeof(ans), ": ");


	if (gLCNum){


		for (i = 0; i < gLCNum; i++) {
			d = Distance(bs->origin, gLCArray[i]->origin);
			if (d < closestd) {
				closesti = i;
				closestd = d;

			}

		}
		Q_strcat(ans, sizeof(ans), "I am ");
		Q_strcat(ans, sizeof(ans), gLCArray[closesti]->name);
		Q_strcat(ans, sizeof(ans), " ");
	}

	if (bs->currentEnemy && bs->currentEnemy->client) {
		Q_strcat(ans, sizeof(ans), "My current target is ");
		Q_strcat(ans, sizeof(ans), bs->currentEnemy->client->pers.netname);
		if (bs->frame_Enemy_Vis) {
			if (!isLame(bs)) {
				Q_strcat(ans, sizeof(ans), ", who I am fighting");
			}
		}else{
			Q_strcat(ans, sizeof(ans), ", who I can't see");
		}

		Q_strcat(ans, sizeof(ans), ". ");

		if (bs->cur_ps.duelInProgress) {
			Q_strcat(ans, sizeof(ans), "I am in a duel. ");
		}
	}
	if (bs->revengeEnemy && bs->revengeEnemy->client && 
		bs->revengeHateLevel){
			Q_strcat(ans, sizeof(ans), "I hate ");
			Q_strcat(ans, sizeof(ans), bs->revengeEnemy->client->pers.netname);
			Q_strcat(ans, sizeof(ans), va(" (%i/%i)",bs->revengeHateLevel,bs->loved_death_thresh));
			Q_strcat(ans, sizeof(ans), ". ");

	}

	Com_Printf("info: %s\n", ans);
}















#else

#include "Lmd_Commands_Auths.h"

#include "ai_main.h"

typedef struct lcobject_s
{
	//        int    index;
	vec3_t origin;
	char   name[MAX_STRING_CHARS];
} lcobject_t;

lcobject_t *gLCArray[MAX_WPARRAY_SIZE];
int gLCNum = 0;
//void *B_Alloc(int size);

void CreateNewLC_FromObject(lcobject_t *lc)
{

	if (gLCNum >= MAX_WPARRAY_SIZE)
	{
		return;
	}

	if (!gLCArray[gLCNum])
	{
		gLCArray[gLCNum] = (lcobject_t *)B_Alloc(sizeof(lcobject_t));
	}

	if (!gLCArray[gLCNum])
	{
		G_Printf(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	//gLCArray[gLCNum]->index = gLCNum;
	VectorCopy(lc->origin, gLCArray[gLCNum]->origin);
	Q_strncpyz( gLCArray[gLCNum]->name,lc->name,sizeof(gLCArray[gLCNum]->name));


	gLCNum++;
}


//Lugormod
int SaveLocationData(void){
	fileHandle_t f;
	int i;
	char *buf;
	trap_FS_FOpenFile(va("botlocations/%s.wnt", level.rawmapname), &f, FS_WRITE);

	for(i = 0; i < gLCNum; i++) {
		buf = va("{ %s } \"%s\"\n",vtos2(gLCArray[i]->origin), gLCArray[i]->name);
		trap_FS_Write(buf, strlen(buf), f);
	}
	trap_FS_FCloseFile(f);
	return 0;
}

int LoadLocationData() {
	char *buf;
	lcobject_t thislc;
	char currentVar[MAX_STRING_CHARS];
	int i, len, i_cv;

	i = 0;
	i_cv = 0;

	buf = Lmd_Data_AllocFileContents(va("botlocations/%s.wnt", level.rawmapname));
	if (!buf){
		G_Printf(S_COLOR_YELLOW "Bot location data not found for %s\n", level.rawmapname);
		return 2;
	}
	len = strlen(buf);

	while (i < len)
	{
		i_cv = 0;

		//thislc.index     = 0;
		thislc.origin[0] = 0;
		thislc.origin[1] = 0;
		thislc.origin[2] = 0;
		thislc.name[0]   = 0;

		//nei_num = 0;
		while (buf[i] != ' ')
		{
			i++;
		}

		i++;

		while (buf[i] != ' ')
		{
			currentVar[i_cv] = buf[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thislc.origin[0] = atof(currentVar);

		i_cv = 0;
		i++;

		while (buf[i] != ' ')
		{
			currentVar[i_cv] = buf[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thislc.origin[1] = atof(currentVar);

		i_cv = 0;
		i++;

		while (buf[i] != ' ')
		{
			currentVar[i_cv] = buf[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thislc.origin[2] = atof(currentVar);

		i_cv = 0;
		i++;

		while (buf[i] != '\"')
		{
			i++;
		}
		i++;

		while (buf[i] != '\"')
		{
			thislc.name[i_cv] = buf[i];
			i_cv++;
			i++;
		}
		thislc.name[i_cv] = '\0';

		CreateNewLC_FromObject(&thislc);
		i ++;
		while (buf[i] != '\n')
		{
			i++;
		}
		i++;

	}

	G_Free(buf);

	return 1;
}

int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore, int ignore2);

void Cmd_botReportLocation (bot_state_t *bs, gentity_t *ent) 
{
	if (!gLCNum) {
		return;

	}

	int i;
	int closesti = 0;
	float d, closestd;
	closestd = WORLD_SIZE;
	vec3_t a;
	float diff,angle;


	for (i = 0; i < gLCNum; i++) {
		d = Distance(bs->origin, gLCArray[i]->origin);
		if (d < closestd) {
			closesti = i;
			closestd = d;

		}

	}
	Q_strncpyz(bs->currentChat, "I am ", sizeof(bs->currentChat));
	if (ent && ent->client) { //ent->client is probably overkill ...

		VectorSubtract( bs->cur_ps.origin, ent->client->ps.origin, a);

		d = abs((int)a[2]);
		vectoangles(a, a);
		angle = AngleMod(ent->client->ps.viewangles[1]);
		a[1] = AngleMod(a[1]);
		diff = angle - a[1];
		if (a[1] < angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
	}

	if (!ent || !ent->client ||
		!EntityVisibleBox(bs->origin, 0, 0, ent->client->ps.origin, bs->client, ent->s.number)/*d > 40 || DistanceHorizontal(ent->s.origin , bs->cur_ps.origin) > 512*/) {
			Q_strcat(bs->currentChat, sizeof(bs->currentChat), gLCArray[closesti]->name);
	} else {
		if (diff <= -135 || diff >= 135) {
			//Behind
			Q_strcat(bs->currentChat, sizeof(bs->currentChat),"behind you.");
		} else if (diff <= -45) {
			//Left
			Q_strcat(bs->currentChat, sizeof(bs->currentChat),"to your left.");
		}        
		else if (diff >= 45) {
			//Right
			Q_strcat(bs->currentChat, sizeof(bs->currentChat),"to your right.");
		} else {
			//In front
			Q_strcat(bs->currentChat, sizeof(bs->currentChat),"in front of you.");
		}

	}

	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;
	bs->doChat = 1;

}

void Cmd_AddLocation_f (gentity_t *ent, int iArg) 
{
	if(trap_Argc() < 2){
		Disp(ent, "^3/addloc ^2<location name>\n"
			"Add your current location to the location list with the given name.");
		return;
	}
	lcobject_t loc;
	Q_strncpyz(loc.name,ConcatArgs(1), sizeof(loc.name));
	VectorCopy(ent->client->ps.origin, loc.origin);
	CreateNewLC_FromObject(&loc);
	SaveLocationData();
}

gentity_t *ClientFromArg (gentity_t *to, int argNum);

void Cmd_Position_f (gentity_t *ent, int iArg) {

	char tmpstr[MAX_STRING_CHARS];
	if (!gLCNum) {
		return;
	}
	gentity_t *tEnt;

	if (trap_Argc() > 1) {
		tEnt = ClientFromArg(ent, 1);

		if (tEnt && tEnt->client) {
			if (Auths_Inferior (ent, tEnt)) {
				return;
			}
			Q_strncpyz(tmpstr, va("%s is ", 
				tEnt->client->pers.netname),
				sizeof(tmpstr));
		}

	} else {
		tEnt = ent;
		Q_strncpyz(tmpstr, "You are ",sizeof(tmpstr));
	}
	if (!tEnt) {
		return;
	}


	//int clientNum = tEnt->s.number;
	int i,closesti;
	vec_t d, closestd;
	closestd = 8192;
	closesti = -1;

	for (i = 0; i < gLCNum; i++) {
		d = Distance(tEnt->client->ps.origin, gLCArray[i]->origin);
		if (d < closestd) {
			closesti = i;
			closestd = d;

		}

	}
	if (closesti < 0) {
		Q_strcat(tmpstr, sizeof(tmpstr), "in an unknown area.");
	} else {
		Q_strcat(tmpstr, sizeof(tmpstr), gLCArray[closesti]->name);
	}

	Disp (ent, tmpstr);
	return;
}

int ClientNumberFromString( gentity_t *to, char *s );
extern bot_state_t *botstates[MAX_CLIENTS];

gentity_t* AimTarget (gentity_t *ent, int length);
qboolean isLame (bot_state_t *bs);

void Cmd_botStatus_f (void) 
{
	if (trap_Argc() < 2) {
		Com_Printf("ERROR: Wrong number of arguments\n");
		return;
	}

	//char otherindex[MAX_TOKEN_CHARS];
	int i;
	bot_state_t *bs;


	//trap_Argv( 1, otherindex, sizeof( otherindex ) );

	i = ClientNumberFromString(NULL, ConcatArgs(1));

	bs = botstates[i];
	if (!bs || !bs->inuse) {
		Com_Printf("ERROR: Not a bot.\n");

		return;

	}
	int closesti;
	float d, closestd;
	closestd = 4096;
	char ans[MAX_STRING_CHARS];
	Q_strncpyz(ans, level.clients[bs->client].pers.netname, sizeof(ans));
	Q_strcat(ans, sizeof(ans), ": ");


	if (gLCNum){


		for (i = 0; i < gLCNum; i++) {
			d = Distance(bs->origin, gLCArray[i]->origin);
			if (d < closestd) {
				closesti = i;
				closestd = d;

			}

		}
		Q_strcat(ans, sizeof(ans), "I am ");
		Q_strcat(ans, sizeof(ans), gLCArray[closesti]->name);
		Q_strcat(ans, sizeof(ans), " ");
	}

	if (bs->currentEnemy && bs->currentEnemy->client) {
		Q_strcat(ans, sizeof(ans), "My current target is ");
		Q_strcat(ans, sizeof(ans), bs->currentEnemy->client->pers.netname);
		if (bs->frame_Enemy_Vis) {
			if (!isLame(bs)) {
				Q_strcat(ans, sizeof(ans), ", who I am fighting");
			}
		}else{
			Q_strcat(ans, sizeof(ans), ", who I can't see");
		}

		Q_strcat(ans, sizeof(ans), ". ");

		if (bs->cur_ps.duelInProgress) {
			Q_strcat(ans, sizeof(ans), "I am in a duel. ");
		}
	}
	if (bs->revengeEnemy && bs->revengeEnemy->client && 
		bs->revengeHateLevel){
			Q_strcat(ans, sizeof(ans), "I hate ");
			Q_strcat(ans, sizeof(ans), bs->revengeEnemy->client->pers.netname);
			Q_strcat(ans, sizeof(ans), va(" (%i/%i)",bs->revengeHateLevel,bs->loved_death_thresh));
			Q_strcat(ans, sizeof(ans), ". ");

	}

	Com_Printf("info: %s\n", ans);
}

#endif

