#include "g_local.h"
#include "q_shared.h"
#include "botlib.h"
#include "ai_main.h"
//Lugormod
#include "g_active.h"

#include "Lmd_Data.h"

#define MAX_BOT_SPAWN 3
#ifdef BOT_ZMALLOC
#define MAX_BALLOC 8192

void *BAllocList[MAX_BALLOC];
#endif

char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];

void *B_TempAlloc(int size)
{
	return BG_TempAlloc(size);
}

void B_TempFree(int size)
{
	BG_TempFree(size);
}


void *B_Alloc(int size)
{
#ifdef BOT_ZMALLOC
	void *ptr = NULL;
	int i = 0;

#ifdef BOTMEMTRACK
	int free = 0;
	int used = 0;

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			free++;
		}
		else
		{
			used++;
		}

		i++;
	}

	G_Printf("Allocations used: %i\nFree allocation slots: %i\n", used, free);

	i = 0;
#endif

	ptr = trap_BotGetMemoryGame(size);

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			BAllocList[i] = ptr;
			break;
		}
		i++;
	}

	if (i == MAX_BALLOC)
	{
		//If this happens we'll have to rely on this chunk being freed manually with B_Free, which it hopefully will be
#ifdef DEBUG
		G_Printf("WARNING: MAXIMUM B_ALLOC ALLOCATIONS EXCEEDED\n");
#endif
	}

	return ptr;
#else

	return BG_Alloc(size);

#endif
}

void B_Free(void *ptr)
{
#ifdef BOT_ZMALLOC
	int i = 0;

#ifdef BOTMEMTRACK
	int free = 0;
	int used = 0;

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			free++;
		}
		else
		{
			used++;
		}

		i++;
	}

	G_Printf("Allocations used: %i\nFree allocation slots: %i\n", used, free);

	i = 0;
#endif

	while (i < MAX_BALLOC)
	{
		if (BAllocList[i] == ptr)
		{
			BAllocList[i] = NULL;
			break;
		}

		i++;
	}

	if (i == MAX_BALLOC)
	{
		//Likely because the limit was exceeded and we're now freeing the chunk manually as we hoped would happen
#ifdef DEBUG
		G_Printf("WARNING: Freeing allocation which is not in the allocation structure\n");
#endif
	}

	trap_BotFreeMemoryGame(ptr);
#endif
}

void B_InitAlloc(void)
{
#ifdef BOT_ZMALLOC
	memset(BAllocList, 0, sizeof(BAllocList));
#endif

	memset(gWPArray, 0, sizeof(gWPArray));
}

void B_CleanupAlloc(void)
{
#ifdef BOT_ZMALLOC
	int i = 0;

	while (i < MAX_BALLOC)
	{
		if (BAllocList[i])
		{
			trap_BotFreeMemoryGame(BAllocList[i]);
			BAllocList[i] = NULL;
		}

		i++;
	}
#endif
}
//Lugormod
gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle );

int GetValueGroup(char *buf, char *group, char *outbuf)
{
	char *place, *placesecond;
	int iplace;
	int failure;
	int i;
	int startpoint, startletter;
	int subg = 0;

	i = 0;

	iplace = 0;

	place = strstr(buf, group);

	if (!place)
	{
		return 0;
	}

	startpoint = place - buf + strlen(group) + 1;
	startletter = (place - buf) - 1;

	failure = 0;

	while (!(buf[startpoint+1] == '{' && buf[startletter] == '\n'))
	{
		placesecond = strstr(place+1, group);

		if (placesecond)
		{
			startpoint += (placesecond - place);
			startletter += (placesecond - place);
			place = placesecond;
		}
		else
		{
			failure = 1;
			break;
		}
	}

	if (failure)
	{
		return 0;
	}

	//we have found the proper group name if we made it here, so find the opening brace and read into the outbuf
	//until hitting the end brace

	while (buf[startpoint] != '{')
	{
		startpoint++;
		//Lugormod You never know
		if (!buf[startpoint]) {
			return 0;
		}
	}
	startpoint++;
	while (buf[startpoint] != '}' || subg)
	{
		//Lugormod if the botfile is incorrect
		if (!buf[startpoint]) {
			return 0;
		}

		if (buf[startpoint] == '{')
		{
			subg++;
		}
		else if (buf[startpoint] == '}')
		{
			subg--;
		}
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}
	outbuf[i] = '\0';

	return 1;
}

int GetPairedValue(char *buf, char *key, char *outbuf)
{
	char *place, *placesecond;
	int startpoint, startletter;
	int i, found;

	if (!buf || !key || !outbuf)
	{
		return 0;
	}

	i = 0;

	while (buf[i] && buf[i] != '\0')
	{
		if (buf[i] == '/')
		{
			if (buf[i+1] && buf[i+1] != '\0' && buf[i+1] == '/')
			{
				while (buf[i] != '\n')
				{
					buf[i] = '/';
					i++;
				}
			}
		}
		i++;
	}

	place = strstr(buf, key);

	if (!place)
	{
		return 0;
	}
	//tab == 9
	startpoint = place - buf + strlen(key);
	startletter = (place - buf) - 1;

	found = 0;

	while (!found)
	{
		if (startletter == 0 || !buf[startletter] || buf[startletter] == '\0' || buf[startletter] == 9 || buf[startletter] == ' ' || buf[startletter] == '\n')
		{
			if (buf[startpoint] == '\0' || buf[startpoint] == 9 || buf[startpoint] == ' ' || buf[startpoint] == '\n')
			{
				found = 1;
				break;
			}
		}

		placesecond = strstr(place+1, key);

		if (placesecond)
		{
			startpoint += placesecond - place;
			startletter += placesecond - place;
			place = placesecond;
		}
		else
		{
			place = NULL;
			break;
		}

	}

	if (!found || !place || !buf[startpoint] || buf[startpoint] == '\0')
	{
		return 0;
	}

	while (buf[startpoint] == ' ' || buf[startpoint] == 9 || buf[startpoint] == '\n')
	{
		startpoint++;
	}

	i = 0;

	while (buf[startpoint] && buf[startpoint] != '\0' && buf[startpoint] != '\n')
	{
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}

	outbuf[i] = '\0';

	return 1;
}

void BotDoCmd (bot_state_t *bs, gentity_t *ent, char *cmd);

extern vmCvar_t bot_enableChat;
int BotDoChat(bot_state_t *bs, char *section, int always)
{
	char *chatgroup;
	int rVal;
	int inc_1;
	int inc_2;
	int inc_n;
	int lines;
	int checkedline;
	int getthisline;
	gentity_t *cobject;
	char *botname;
	botname = level.clients[bs->client].pers.netname;

	if(bot_enableChat.integer <= 0 || trap_Cvar_VariableIntegerValue("se_language"))
		return 0;

	if (!bs->canChat)
		return 0;

	if (bs->doChat)
		return 0;

	if (Q_irand(1, 10) > bs->chatFrequency && !always)
		return 0;

	bs->chatTeam = 0;

	chatgroup = (char *)B_TempAlloc(MAX_CHAT_BUFFER_SIZE);

	rVal = GetValueGroup(gBotChatBuffer[bs->client], section, chatgroup);

	if (!rVal) //the bot has no group defined for the specified chat event
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		Com_Printf("info: %s has no chat group %s\n",botname, section);
		return 0;
	}

	inc_1 = 0;
	inc_2 = 2;

	while (chatgroup[inc_2] && chatgroup[inc_2] != '\0')
	{
		if (chatgroup[inc_2] != 13 && chatgroup[inc_2] != 9)
		{
			chatgroup[inc_1] = chatgroup[inc_2];
			inc_1++;
		}
		inc_2++;
	}
	chatgroup[inc_1] = '\0';

	inc_1 = 0;

	lines = 0;

	while (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
	{
		if (chatgroup[inc_1] == '\n')
		{
			lines++;
		}
		inc_1++;
	}

	if (!lines)
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	getthisline = Q_irand(0, (lines+1));

	if (getthisline < 1)
	{
		getthisline = 1;
	}
	if (getthisline > lines)
	{
		getthisline = lines;
	}

	checkedline = 1;

	inc_1 = 0;

	while (checkedline != getthisline)
	{
		if (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
		{
			if (chatgroup[inc_1] == '\n')
			{
				inc_1++;
				checkedline++;
			}
		}

		if (checkedline == getthisline)
		{
			break;
		}

		inc_1++;
	}

	//we're at the starting position of the desired line here
	inc_2 = 0;

	while (chatgroup[inc_1] != '\n')
	{
		chatgroup[inc_2] = chatgroup[inc_1];
		inc_2++;
		inc_1++;
	}
	chatgroup[inc_2] = '\0';

	//trap_EA_Say(bs->client, chatgroup);
	inc_1 = 0;
	inc_2 = 0;

	if (strlen(chatgroup) > MAX_CHAT_LINE_SIZE)
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	while (chatgroup[inc_1])
	{
		if (chatgroup[inc_1] == '%' && chatgroup[inc_1+1] != '%')
		{
			inc_1++;

			if (chatgroup[inc_1] == 's' && bs->chatObject)
			{
				cobject = bs->chatObject;
			}
			else if (chatgroup[inc_1] == 'a' && bs->chatAltObject)
			{
				cobject = bs->chatAltObject;
			}
			else
			{
				cobject = NULL;
			}

			if (cobject && cobject->client)
			{
				//Lugormod
				char cobjname[MAX_TOKEN_CHARS];//[MAX_NAME_LENGTH];

				inc_n = 0;
				Q_strncpyz(cobjname, cobject->client->pers.netname, sizeof(cobjname));
				Q_StripTags(cobjname);

				//ugly Q_StripTags should see to this. Isn't it???
				/*
				if (strlen(Q_StripTags(cobjname)) < 3) {
				Q_strncpyz(cobjname, cobject->client->pers.netname, sizeof(cobjname));
				Q_CleanStr2(cobjname);

				}
				*/

				while (cobjname[inc_n])
				{
					bs->currentChat[inc_2] = cobjname[inc_n];
					inc_2++;
					inc_n++;
				}
				inc_2--; //to make up for the auto-increment below
			}
		}
		else
		{
			bs->currentChat[inc_2] = chatgroup[inc_1];
		}
		inc_2++;
		inc_1++;
	}
	bs->currentChat[inc_2] = '\0';

	if (bs->currentChat[0] == '@') {
		BotDoCmd (bs, NULL, bs->currentChat + 1);
		B_TempFree(MAX_CHAT_BUFFER_SIZE);
		return 1;
	}

	if (strcmp(section, "GeneralGreetings") == 0)
	{
		bs->doChat = 2;
	}
	else
	{
		bs->doChat = 1;
	}
	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;

	B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup

	return 1;

}

gentity_t* HurlItem (gentity_t *ent, const char *name);

void
Cmd_BotThrow (bot_state_t *bs, char *name)
{
	HurlItem(&g_entities[bs->client],name);
}
gentity_t*
BotDoSpawn (bot_state_t *bs, char *NPC_type) 
{
	int n,i;
	i = 0;
	gentity_t *player;

	for ( n = 1; n < ENTITYNUM_MAX_NORMAL; n++) 
	{
		player = &g_entities[n];
		if (!player || !player->inuse) {
			continue;
		}
		//if (Q_stricmp(player->fullName, "botspawn") == 0){
		if (Q_stricmp(player->targetname, "botspawn") == 0){
			i++;
		}
	}
	if (i >= MAX_BOT_SPAWN) {
		Com_Printf("info: There are too many botspawns.\n");
		return NULL; 
	}
	gentity_t *spawn;
	if (spawn = NPC_SpawnType( &g_entities[bs->client], NPC_type,"botspawn", qtrue)){

		//spawn->fullName = G_NewString("botspawn");
		return spawn;
	}

	return NULL;

}
//Lugormod
int ClientNumberFromString( gentity_t *to, char *s );
extern bot_state_t *botstates[MAX_CLIENTS];

void Cmd_talkingBot_f (void) 
{
	if (trap_Argc() < 3) {
		Com_Printf("ERROR: Wrong number of arguments\n");
		return;
	}

	char otherindex[MAX_TOKEN_CHARS];
	int i;
	char msg[MAX_STRING_CHARS];
	bot_state_t *bs;


	trap_Argv( 1, otherindex, sizeof( otherindex ) );

	i = ClientNumberFromString(NULL, otherindex);

	if(i < 0)
		return;

	Q_strncpyz(msg, ConcatArgs(2),sizeof( msg ));

	bs = botstates[i];
	if (!bs || !bs->inuse) {
		Com_Printf("ERROR: Not a bot.\n");

		return;

	}
	if (msg[0] == '@') {
		gentity_t *target = NULL;

		if (bs->currentEnemy && bs->currentEnemy->client) {
			target = bs->currentEnemy;
		} //else {
		//target = AimTarget(&g_entities[bs->client], 1024);

		//}

		BotDoCmd(bs, target, msg+1);
	} else {
		Q_strncpyz(bs->currentChat, msg, sizeof(bs->currentChat));
		bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
		bs->chatTime = level.time + bs->chatTime_stored;
		bs->doChat = 1;
	}

}
//end Lugormod


//void StandardSetBodyAnim(gentity_t *self, int anim, int flags);
int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore, int ignore2);

qboolean   saberKnockOutOfHand (gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);
void Cmd_Surrender_f ( gentity_t *ent, int iArg) 
{
	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered){
		Cmd_ToggleSaber_f(ent);
	}
	if (ent->client->ps.weapon == WP_SABER &&
		ent->client->ps.saberEntityNum &&
		!ent->client->ps.saberInFlight)
	{
		saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
	}
	ent->client->ps.saberMove = LS_NONE;
	ent->client->ps.saberBlocked = 0;
	ent->client->ps.saberBlocking = 0;

	ent->client->ps.forceDodgeAnim = TORSO_SURRENDER_START;

	ent->client->ps.forceHandExtendTime = level.time + BG_AnimLength(ent->localAnimIndex, TORSO_SURRENDER_START);

	G_SetAnim(ent, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
	ent->client->ps.torsoTimer += 600;
}
void CallVote (gentity_t *ent, char *arg1, char *arg2);

void
Cmd_BotCallVote(bot_state_t *bs, char *vote) 
{
	char *p;
	p = strchr(vote, ' ');
	if (p){
		*p = '\0';
		p++;
	} else {
		p=" ";
	}
	CallVote(&g_entities[bs->client], vote, p);
}

void 
Cmd_BotVote(bot_state_t *bs, int vote)
{
	gclient_t *client = &level.clients[bs->client];

	if ( !level.voteTime ) {
		return;
	}
	if ( client->mGameFlags & PSG_VOTED ) {
		return;
	}
	if (g_gametype.integer != GT_DUEL &&
		g_gametype.integer != GT_POWERDUEL)
	{
		if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return;
		}
	}

	client->mGameFlags |= PSG_VOTED;

	if ( vote ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}
}

void botsVoteNotification (char *votestr) 
{
	int i,vote;
	gentity_t *ent;
	bot_state_t *bs;

	if (Q_stricmp("kick", votestr) == 0 || Q_stricmp("clientkick", votestr) == 0){
		vote = 0;

	} else {
		vote = Q_irand(0,1);
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		ent = &g_entities[i];
		if(!ent) {
			continue;
		}
		bs = botstates[i];

		if(!bs || !bs->inuse) {
			continue;
		}
		bs->voteTime = level.time + Q_irand(5000, 15000);
		bs->doVote = 1 + vote;
	}

}

//char* fnord(char *msg);

void Cmd_botReportLocation (bot_state_t *bs, gentity_t *ent);
//Lugormod we can ask bots to do stuff.
void BotDoCmd (bot_state_t *bs, gentity_t *ent, char *cmd) 
{
	//Lugormod I dont think i need any checking here
	//for ent client and such, cuz it's been done.
	//There should maybe be a cvar to disable this.
	if (bs->cur_ps.duelInProgress) {
		return; //not now
	}

	Q_CleanStr(cmd);
	qboolean visible = qfalse;

	if (ent && ent->client) {
		vec3_t e_ang_vec;
		VectorSubtract(ent->client->ps.origin, bs->eye, e_ang_vec);
		vectoangles(e_ang_vec, e_ang_vec);

		if (InFieldOfVision(bs->cur_ps.viewangles, 100, e_ang_vec)
			&& EntityVisibleBox(bs->origin, NULL, NULL, ent->client->ps.origin, bs->client, ent->s.number)) {
				visible = qtrue;
		}
		Com_Printf("info: %s is doing %s on %s.\n",
			level.clients[bs->client].pers.netname,
			cmd,
			ent->client->pers.netname);

	} else {
		Com_Printf("info: %s is doing %s.\n",
			level.clients[bs->client].pers.netname,
			cmd);

	}



	if (Q_stricmp(cmd, "gloat") == 0) {
		if(bs->cur_ps.legsTimer < 1){
			bs->beStill = level.time + 3000;
			level.clients[bs->client].pers.cmd.upmove = 0;
			level.clients[bs->client].pers.cmd.forwardmove = 0;
			level.clients[bs->client].pers.cmd.rightmove = 0;
			G_SetTauntAnim(&g_entities[bs->client], TAUNT_GLOAT);
		}

	} else if (Q_stricmp(cmd, "flourish") == 0) {
		if(bs->cur_ps.legsTimer < 1){
			bs->beStill = level.time + 3000;
			level.clients[bs->client].pers.cmd.upmove = 0;
			level.clients[bs->client].pers.cmd.forwardmove = 0;
			level.clients[bs->client].pers.cmd.rightmove = 0;
			G_SetTauntAnim(&g_entities[bs->client], TAUNT_FLOURISH);
		}
	} else if (Q_stricmp(cmd, "meditate") == 0) {
		if(bs->cur_ps.legsTimer < 1){
			bs->beStill = level.time + 3000;
			level.clients[bs->client].pers.cmd.upmove = 0;
			level.clients[bs->client].pers.cmd.forwardmove = 0;
			level.clients[bs->client].pers.cmd.rightmove = 0;
			G_SetTauntAnim(&g_entities[bs->client], TAUNT_MEDITATE);
		}
	} else if (Q_stricmp(cmd, "taunt") == 0) {
		if(bs->cur_ps.legsTimer < 1){
			bs->beStill = level.time + 3000;
			level.clients[bs->client].pers.cmd.upmove = 0;
			level.clients[bs->client].pers.cmd.forwardmove = 0;
			level.clients[bs->client].pers.cmd.rightmove = 0;
			G_SetTauntAnim(&g_entities[bs->client], TAUNT_TAUNT);
		}
	} else if (Q_stricmp(cmd, "bow") == 0) {
		if(bs->cur_ps.legsTimer < 1){
			bs->beStill = level.time + 3000;
			level.clients[bs->client].pers.cmd.upmove = 0;
			level.clients[bs->client].pers.cmd.forwardmove = 0;
			level.clients[bs->client].pers.cmd.rightmove = 0;
			G_SetTauntAnim(&g_entities[bs->client], TAUNT_BOW);
		}
	} else if (Q_stricmp(cmd, "wait") == 0) {
		bs->beStill = level.time + 10000;
		if (ent) {

			bs->chatObject = ent;
			bs->chatAltObject = NULL;
			BotDoChat(bs, "OrderAccepted", 1);
		}
	} else if (Q_stricmp(cmd, "ignore") == 0) {
		if (!visible) {
			return;
		}
		bs->ignoreClientTime = level.time + 10000;
		bs->ignoreClient = ent->s.number;

	} else if (Q_stricmp(cmd, "hug") == 0) {
		if (!visible) {
			return;
		}

		//Cmd_Hug_f(&g_entities[bs->client]);
		bs->hugClient = ent->s.number;

		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "OrderAccepted", 1);
	} else if (Q_stricmpn("give ", cmd, 5) == 0) {
		if (!visible || g_gametype.integer != GT_FFA) {
			return;
		}
		char *item_type = cmd + 5;
		gitem_t *it = BG_FindItem(va("%s",item_type));
		if (!it){
			return;
		}
		if ((1 << it->giTag) & ent->client->ps.stats[STAT_HOLDABLE_ITEMS]){
			Com_Printf("info: %s already has a %s.\n",ent->client->pers.netname, item_type);

			return;
		}
		Com_Printf("info: %s is giving a %s.\n", level.clients[bs->client].pers.netname, item_type);

		Cmd_BotThrow(bs, va("%s",item_type));

		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "OrderAccepted", 1);        
	}else if ((Q_stricmpn("spawn ", cmd,6) == 0)) {
		if (!visible || g_gametype.integer != GT_FFA) {
			return;
		}
		char *NPC_type = cmd + 6;

		if (!BotDoSpawn(bs, NPC_type)) {
			return;
		}

		Com_Printf("info: %s is spawning a %s.\n", 
			level.clients[bs->client].pers.netname,
			NPC_type);        
		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "OrderAccepted", 1);
	} else if ((Q_stricmp(cmd, "attack") == 0)) {
		if (!visible) {
			return;
		}
		bs->revengeEnemy = ent;
		bs->revengeHateLevel = bs->loved_death_thresh + 1;
		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "OrderAccepted", 1);
	}else if ((Q_stricmp(cmd, "throw") == 0)) {
		if (!visible) {
			return;
		}
		//Cmd_DebugThrow_f(&g_entities[bs->client]);
		bs->throwClient = ent->s.number;
		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "Hatred", 1);
	}else if ((Q_stricmp(cmd, "forgive") == 0)) {
		if (ent && bs->revengeEnemy
			&& ent->s.number == bs->revengeEnemy->s.number) {
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
		}
	}else if ((Q_stricmp(cmd, "goto") == 0)) {
		if (!ent) { 
			return;
		}
		bs->gotoClient = ent->s.number;
		bs->chatObject = ent;
		bs->chatAltObject = NULL;
		BotDoChat(bs, "OrderAccepted", 1);
	}else if ((Q_stricmp(cmd, "surrender") == 0)) {
		if (!visible) {
			return;
		}
		Cmd_Surrender_f (&g_entities[bs->client], 0);

	}else if ((Q_stricmp(cmd, "where") == 0)) {
		Cmd_botReportLocation (bs, ent);

	}else if (Q_stricmp(cmd, "voteno") == 0) {
		Cmd_BotVote(bs, 0);
	}else if (Q_stricmp(cmd, "voteyes") == 0) {
		Cmd_BotVote( bs, 1);
	}else if (Q_strncmp("callvote ",cmd, 9) == 0) {
		char *vote = cmd + 9;               
		Cmd_BotCallVote( bs, vote);
	}
	//RoboPhred
	else if(Q_strncmp("rcon ", cmd, 5)){
		char *rcon = cmd + 5;
		trap_SendConsoleCommand(EXEC_APPEND, rcon);
	}

	//        else if (Q_stricmp(cmd, "fnord") == 0) {
	//                fnord(bs->currentChat);
	//                bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	//                bs->chatTime = level.time + bs->chatTime_stored;
	//                bs->doChat = 1;
	//        }

}

char* skipline (const char *cp);
qboolean Q_wordInLine (const char *word,/*int wordl,*/const char *line);

int BotChatBack(const char *message, gentity_t *ent, gentity_t *other)
{
	char *chatgroup;

	if (!ent ||  //You never know
		!ent->client ||
		!botstates[other-g_entities] || 
		!botstates[other-g_entities]->inuse || 
		ent->s.number == other-g_entities) {
			return 0;
	}
	if (g_gametype.integer >= GT_TEAM &&
		ent->client->sess.sessionTeam != other->client->sess.sessionTeam) {
			return 0;
	}

	char *section = "ChatBack";
	int rVal;
	int inc_1;
	int inc_2;
	int inc_n;
	int lines;
	gentity_t *cobject;
	bot_state_t *bs;

	char s[MAX_STRING_CHARS];

	Q_strncpyz(s, message, sizeof(s));
	Q_CleanStr2(s);

	bs = botstates[other-g_entities];
	/*
	if (bs->cur_ps.duelInProgress) {

	return 0;
	}
	*/
	if (!bs->canChat)
	{

		return 0;
	}

	if (bs->doChat)
	{ //already have a chat scheduled
		return 0;
	}

	if (trap_Cvar_VariableIntegerValue("se_language"))
	{ //no chatting unless English.
		return 0;
	}

	if (Q_irand(1, 10) > bs->chatFrequency)
	{
		vec3_t e_ang_vec;
		VectorSubtract(ent->client->ps.origin, bs->eye, e_ang_vec);
		vectoangles(e_ang_vec, e_ang_vec);

		if (!InFieldOfVision(bs->cur_ps.viewangles, 100, e_ang_vec)) {
			if (!Q_wordInLine(level.clients[bs->client].pers.netname, message)) {  
				return 0;
			}
		}  else if (!EntityVisibleBox(bs->origin, NULL, NULL, ent->client->ps.origin, bs->client, ent->s.number)){
			if (!Q_wordInLine(level.clients[bs->client].pers.netname, message)) {  
				return 0;
			}
		}
	}

	bs->chatTeam = 0;

	chatgroup = (char *)B_TempAlloc(MAX_CHAT_BUFFER_SIZE);


	rVal = GetValueGroup(gBotChatBuffer[bs->client], section, chatgroup);

	if (!rVal) //the bot has no group defined for the specified chat event
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	char *cp = chatgroup;
	cp = skipline(cp);

	while (*cp != '\0' && 
		!Q_wordsInLine(cp, s, &cp) && 
		*cp != '\0' /*&&
					cp - chatgroup < MAX_STRING_CHARS*/) {
						while (*cp == '\t' || *cp == ' ') {
							cp++;
						}

						if (*cp == '{') {
							while (*cp && *cp != '}') {
								cp++;
							}
							cp = skipline(cp);
						}
	}
	if (*cp == '\0') {
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup

		return 0;

	}

	while (*cp && *cp != '{') {
		cp++;
	}
	cp = skipline(cp);

	char *tp = cp;
	lines = 0;

	while (*tp && *tp != '}') {
		if (*tp++ == '\n') {
			lines++;
		}
	}
	if (!lines) {
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	lines = Q_irand(1, lines) - 1;
	while (lines--) {
		cp = skipline(cp);
	}

	//we're at the starting position of the desired line here
	inc_1 = 0;
	inc_2 = 0;

	while (cp[inc_1] != '\n')
	{
		cp[inc_2] = cp[inc_1];
		inc_2++;
		inc_1++;
	}
	cp[inc_2] = '\0';

	//trap_EA_Say(bs->client, chatgroup);
	inc_1 = 0;
	inc_2 = 0;

	if (strlen(cp) > MAX_CHAT_LINE_SIZE)
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	while (cp[inc_1])
	{
		if (cp[inc_1] == '%' && cp[inc_1+1] != '%')
		{
			inc_1++;

			if (cp[inc_1] == 's')
			{
				cobject = ent;
			}
			else
			{
				cobject = NULL;
			}

			if (cobject && cobject->client)
			{
				inc_n = 0;

				char cobjname[MAX_TOKEN_CHARS];//[MAX_NAME_LENGTH];
				Q_strncpyz(cobjname, cobject->client->pers.netname, sizeof(cobjname));
				Q_StripTags(cobjname);

				//ugly Q_StripTags should see to this.
				/*
				if (strlen(Q_StripTags(cobjname)) < 3) {
				Q_strncpyz(cobjname, cobject->client->pers.netname, sizeof(cobjname));
				Q_CleanStr2(cobjname);

				}
				*/

				while (cobjname[inc_n])
				{
					bs->currentChat[inc_2] = cobjname[inc_n];
					inc_2++;
					inc_n++;
				}
				inc_2--; //to make up for the auto-increment below
			}
		}
		else
		{
			bs->currentChat[inc_2] = cp[inc_1];
		}
		inc_2++;
		inc_1++;
	}
	bs->currentChat[inc_2] = '\0';

	if (bs->currentChat[0] == '@') {
		BotDoCmd (bs, ent, bs->currentChat + 1);
		//RoboPhred: lugor forgot one.
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 1;
	}

	bs->doChat = 1;

	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;

	B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup

	return 1;
}



void ParseEmotionalAttachments(bot_state_t *bs, char *buf)
{
	int i = 0;
	int i_c = 0;
	char tbuf[16];

	while (buf[i] && buf[i] != '}')
	{
		while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
		{
			i++;
		}

		if (buf[i] && buf[i] != '}')
		{
			i_c = 0;
			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				bs->loved[bs->lovednum].name[i_c] = buf[i];
				i_c++;
				i++;
			}
			bs->loved[bs->lovednum].name[i_c] = '\0';

			while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
			{
				i++;
			}

			i_c = 0;

			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				tbuf[i_c] = buf[i];
				i_c++;
				i++;
			}
			tbuf[i_c] = '\0';

			bs->loved[bs->lovednum].level = atoi(tbuf);

			bs->lovednum++;
		}
		else
		{
			break;
		}

		if (bs->lovednum >= MAX_LOVED_ONES)
		{
			return;
		}

		i++;
	}
}

int ReadChatGroups(bot_state_t *bs, char *buf)
{
	char *cgroupbegin;
	int cgbplace;
	int i;

	cgroupbegin = strstr(buf, "BEGIN_CHAT_GROUPS");

	if (!cgroupbegin)
	{
		return 0;
	}

	if (strlen(cgroupbegin) >= MAX_CHAT_BUFFER_SIZE)
	{
		G_Printf(S_COLOR_RED "Error: Personality chat section exceeds max size\n");
		return 0;
	}

	cgbplace = cgroupbegin - buf+1;

	while (buf[cgbplace] != '\n')
	{
		cgbplace++;
	}

	i = 0;

	while (buf[cgbplace] && buf[cgbplace] != '\0')
	{
		gBotChatBuffer[bs->client][i] = buf[cgbplace];
		i++;
		cgbplace++;
	}

	gBotChatBuffer[bs->client][i] = '\0';

	return 1;
}

void BotUtilizePersonality(bot_state_t *bs)
{
	int failed;
	int i;
	char *buf;
	char *readbuf, *group;

	buf = Lmd_Data_AllocFileContents(bs->settings.personalityfile);


	if (!buf) {
		G_Printf(S_COLOR_RED "Error: Specified personality not found\n");
		return;
	}

	failed = 0;

	readbuf = (char *)B_TempAlloc(1024);
	group = (char *)B_TempAlloc(65536);

	if (!GetValueGroup(buf, "GeneralBotInfo", group))
	{
		G_Printf(S_COLOR_RED "Personality file contains no GeneralBotInfo group\n");
		failed = 1; //set failed so we know to set everything to default values
	}

	if (!failed && GetPairedValue(group, "reflex", readbuf))
	{
		bs->skills.reflex = atoi(readbuf);
	}
	else
	{
		bs->skills.reflex = 100; //default
	}

	if (!failed && GetPairedValue(group, "accuracy", readbuf))
	{
		bs->skills.accuracy = atof(readbuf);
	}
	else
	{
		bs->skills.accuracy = 10; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed", readbuf))
	{
		bs->skills.turnspeed = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed = 0.01f; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed_combat", readbuf))
	{
		bs->skills.turnspeed_combat = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed_combat = 0.05f; //default
	}

	if (!failed && GetPairedValue(group, "maxturn", readbuf))
	{
		bs->skills.maxturn = atof(readbuf);
	}
	else
	{
		bs->skills.maxturn = 360; //default
	}

	if (!failed && GetPairedValue(group, "perfectaim", readbuf))
	{
		bs->skills.perfectaim = atoi(readbuf);
	}
	else
	{
		bs->skills.perfectaim = 0; //default
	}

	if (!failed && GetPairedValue(group, "chatability", readbuf))
	{
		bs->canChat = atoi(readbuf);
	}
	else
	{
		bs->canChat = 0; //default
	}

	if (!failed && GetPairedValue(group, "chatfrequency", readbuf))
	{
		bs->chatFrequency = atoi(readbuf);
	}
	else
	{
		bs->chatFrequency = 5; //default
	}

	if (!failed && GetPairedValue(group, "hatelevel", readbuf))
	{
		bs->loved_death_thresh = atoi(readbuf);
	}
	else
	{
		bs->loved_death_thresh = 3; //default
	}

	if (!failed && GetPairedValue(group, "camper", readbuf))
	{
		bs->isCamper = atoi(readbuf);
	}
	else
	{
		bs->isCamper = 0; //default
	}

	if (!failed && GetPairedValue(group, "saberspecialist", readbuf))
	{
		bs->saberSpecialist = atoi(readbuf);
	}
	else
	{
		bs->saberSpecialist = 0; //default
	}

	if (!failed && GetPairedValue(group, "forceinfo", readbuf))
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", readbuf);
	}
	else
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", DEFAULT_FORCEPOWERS);
	}

	i = 0;

	while (i < MAX_CHAT_BUFFER_SIZE)
	{ //clear out the chat buffer for this bot
		gBotChatBuffer[bs->client][i] = '\0';
		i++;
	}

	if (bs->canChat)
	{
		if (!ReadChatGroups(bs, buf))
		{
			bs->canChat = 0;
		}
	}

	if (GetValueGroup(buf, "BotWeaponWeights", group))
	{
		if (GetPairedValue(group, "WP_STUN_BATON", readbuf))
		{
			bs->botWeaponWeights[WP_STUN_BATON] = atoi(readbuf);
			bs->botWeaponWeights[WP_MELEE] = bs->botWeaponWeights[WP_STUN_BATON];
		}

		if (GetPairedValue(group, "WP_SABER", readbuf))
		{
			bs->botWeaponWeights[WP_SABER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BRYAR_PISTOL", readbuf))
		{
			bs->botWeaponWeights[WP_BRYAR_PISTOL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BLASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BLASTER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DISRUPTOR", readbuf))
		{
			bs->botWeaponWeights[WP_DISRUPTOR] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BOWCASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BOWCASTER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_REPEATER", readbuf))
		{
			bs->botWeaponWeights[WP_REPEATER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DEMP2", readbuf))
		{
			bs->botWeaponWeights[WP_DEMP2] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_FLECHETTE", readbuf))
		{
			bs->botWeaponWeights[WP_FLECHETTE] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_ROCKET_LAUNCHER", readbuf))
		{
			bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_THERMAL", readbuf))
		{
			bs->botWeaponWeights[WP_THERMAL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_TRIP_MINE", readbuf))
		{
			bs->botWeaponWeights[WP_TRIP_MINE] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DET_PACK", readbuf))
		{
			bs->botWeaponWeights[WP_DET_PACK] = atoi(readbuf);
		}
	}

	bs->lovednum = 0;

	if (GetValueGroup(buf, "EmotionalAttachments", group))
	{
		ParseEmotionalAttachments(bs, group);
	}

	B_TempFree(1024); //readbuf
	B_TempFree(65536); //group
	G_Free(buf);
}
