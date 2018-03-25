

#include "g_local.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Factions_Old.h"
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"
#include "Lmd_Time.h"

#define LMD_FACTION_EXT "fac"
#define FACTION_FOLDER "factions"

#define FACTION_SAVE_TIME 60000 //Ufo: was .060000

#define MAX_NAMELEN 20
#define MAX_TAGLEN 6
#define MAX_RANKTAGLEN 4

//for the save/load code
#define MAX_NAMELEN_TEXT "20"
#define MAX_TAGLEN_TEXT "6"
#define MAX_RANKTAGLEN_TEXT "4"

#define MAX_RANKS 15
#define MAX_ANNOUNCEMENTS 7

enum{
	FACTAUTH_SETRANKS,
	FACTAUTH_RECRUIT,
	FACTAUTH_PROMOTE,
	FACTAUTH_KICK,
	FACTAUTH_ANNOUNCE,
	FACTAUTH_MAX
};

const static char* FactAuthNames[] = {
	"Ranks",
	"Recruit",
	"Promote",
	"Kick",
	"Announcements",
};

unsigned int Factions_RankString(unsigned int auths, char *buf, int bufSze){
	unsigned int i;
	unsigned int count = 0;
	buf[0] = 0;
	for(i = 0;i<FACTAUTH_MAX;i++){
		if(auths & (1 << i)){
			Q_strcat(buf, bufSze, va("%s%s", (count>0)?" ":"", FactAuthNames[i]));
			count++;
		}
	}
	return count;
}

typedef struct factionRank_s{
	char name[MAX_NAMELEN + 1];
	char rankTag[MAX_RANKTAGLEN + 1];
	unsigned int rankLevel;
	unsigned int auths;
}factionRank_t;

typedef struct factionPlayer_s{
	int playerId;
	char rankName[MAX_NAMELEN + 1];
}factionPlayer_t;

typedef struct factionMessage_s{
	char title[MAX_NAMELEN + 1];
	char *text;
	unsigned int timeoutDay;
}factionMessage_t;

typedef struct Factions_s{
	char *name;
	char *tag;
	char *Property;

	//int Leader;

	struct factionPlayers_s{
		factionPlayer_t *Player;
		unsigned int count;
	}Players;

	struct factionRanks_s{
		factionRank_t *Rank;
		unsigned int count;
	}Ranks;

	struct factionAnnouncements_s{
		factionMessage_t *Message;
		unsigned int count;
	}Announcements;

	unsigned int modifyTime;

}Faction_t;

#define	FACTOFS(x) ((int)&(((Faction_t *)0)->x))
BG_field_t FactionFields[] = {
	{"tag", FACTOFS(tag), F_QSTRING},
	{"property", FACTOFS(Property), F_QSTRING},
};

struct FactionList_s{
	Faction_t **Factions;
	unsigned int count;
}FactionList;

void Factions_Modify(Faction_t *faction){
	if(faction->modifyTime == 0)
		faction->modifyTime = level.time;
}

int Factions_GetFactionIndex(char *factionName){
	unsigned int i;
	for(i = 0;i<FactionList.count;i++){
		if(Q_stricmp(FactionList.Factions[i]->name, factionName) == 0)
			return i;
	}
	return -1;
}

Faction_t* Factions_GetFaction(char *factionName){
	int i = Factions_GetFactionIndex(factionName);
	if(i < 0)
		return NULL;
	return FactionList.Factions[i];
}

Faction_t *Factions_New(char *name){
	unsigned int i;
	Faction_t *faction;
	if(Factions_GetFaction(name))
		return NULL;
	faction = (Faction_t *)G_Alloc(sizeof(Faction_t));
	memset(faction, 0, sizeof(Faction_t));
	i = Lmd_Arrays_AddArrayElement((void **)&FactionList.Factions, sizeof(Faction_t *), &FactionList.count);
	FactionList.Factions[i] = faction;
	faction->name = G_NewString2(name);
	Factions_Modify(faction);
	return faction;
}


Faction_t *Factions_GetFactionByTag(char *tag){
	unsigned int i;
	for(i = 0;i<FactionList.count;i++){
		if(FactionList.Factions[i]->tag && Q_stricmp(FactionList.Factions[i]->tag, tag) == 0){
			return FactionList.Factions[i];
		}
	}
	return NULL;
}

qboolean Factions_SetFactionTag(Faction_t *faction, char *newTag){
	if(strlen(newTag) > MAX_TAGLEN || Factions_GetFactionByTag(newTag))
		return qfalse;
	//Q_strncpyz(faction->tag, newTag, sizeof(faction->tag));
	G_Free(faction->tag);
	faction->tag = G_NewString2(newTag);
	Factions_Modify(faction);
	return qtrue;
}

qboolean Factions_SetFactionProperty(Faction_t *faction, char *newProperty){
	G_Free(faction->Property);
	faction->Property = G_NewString2(newProperty);
	Factions_Modify(faction);
	return qtrue;
}

factionPlayer_t* Factions_GetPlayerEntry(Faction_t *faction, int playerId){
	unsigned int i;
	for(i = 0;i < faction->Players.count;i++){
		if(faction->Players.Player[i].playerId == playerId){
			return &faction->Players.Player[i];
		}
	}
	return NULL;
}

factionRank_t* Factions_GetRank(Faction_t *faction, char *rankName){
	unsigned int i;
	for(i = 0;i < faction->Ranks.count;i++){
		if(Q_stricmp(faction->Ranks.Rank[i].name, rankName) == 0){
			return &faction->Ranks.Rank[i];
		}
	}
	return NULL;
}

factionRank_t* Factions_GetRankByTag(Faction_t *faction, char *rankTag){
	unsigned int i;
	for(i = 0;i < faction->Ranks.count;i++){
		if(Q_stricmp(faction->Ranks.Rank[i].rankTag, rankTag) == 0){
			return &faction->Ranks.Rank[i];
		}
	}
	return NULL;
}

qboolean Factions_SetRankTag(Faction_t *faction, factionRank_t *rank, char *tag){
	if(strlen(tag) > MAX_RANKTAGLEN || Factions_GetRankByTag(faction, tag))
		return qfalse;
	Q_strncpyz(rank->rankTag, tag, sizeof(rank->name));
	Factions_Modify(faction);
	return qtrue;
}

qboolean Factions_SetMemberRank(Faction_t *faction, factionPlayer_t *player, factionRank_t *rank){
	if(rank == Factions_GetRank(faction, player->rankName)){
		return qfalse;
	}
	Q_strncpyz(player->rankName, rank->name, sizeof(player->rankName));
	Factions_Modify(faction);
	return qtrue;
}

char* Factions_GetRankTag(Faction_t *faction, char *rankName){
	factionRank_t *rank = Factions_GetRank(faction, rankName);
	if(!rank)
		return "";
	return rank->rankTag;
}

qboolean Factions_PlayerHasAuth(Faction_t *faction, factionPlayer_t *player, unsigned int auth){
	factionRank_t *rank = Factions_GetRank(faction, player->rankName);
	if(!rank)
		return qfalse;
	return (rank->auths & (1 << auth));
}

qboolean Factions_CheckPlayerFactionProperty(int id, char *propName){
	factionPlayer_t *player;
	unsigned int i;
	for(i = 0;i<FactionList.count;i++){
		player = Factions_GetPlayerEntry(FactionList.Factions[i], id);
		if(!player)
			continue;
		if(Q_stricmp(FactionList.Factions[i]->Property, propName) != 0)
			continue;
		return qtrue;
	}
	return qfalse;
}


qboolean Factions_SetPlayerRank(Faction_t *faction, factionPlayer_t *player, factionRank_t *rank){
	Q_strncpyz(player->rankName, rank->name, sizeof(player->rankName));
	Factions_Modify(faction);
	return qtrue;
}

qboolean Factions_SetAuth(Faction_t *faction, factionRank_t *rank, unsigned int auth){
	Factions_Modify(faction);
	auth = (1 << auth);
	if(rank->auths & auth){
		rank->auths &= ~auth;
		return qfalse;
	}
	rank->auths |= auth;
	return qtrue;
}

void Factions_AddPlayer(Faction_t *faction, char *rankName, int player){
	unsigned int i = Lmd_Arrays_AddArrayElement((void **)&faction->Players.Player, sizeof(factionPlayer_t),
		&faction->Players.count);
	Q_strncpyz(faction->Players.Player[i].rankName, rankName, sizeof(faction->Players.Player[i].rankName));
	faction->Players.Player[i].playerId = player;
	Factions_Modify(faction);
}

qboolean Factions_RemovePlayer(Faction_t *faction, factionPlayer_t *player){
	int index = (player - faction->Players.Player);
	if(index < 0 || index > faction->Players.count)
		return qfalse;
	Lmd_Arrays_RemoveArrayElement((void **)&faction->Players, index, sizeof(factionPlayer_t), &faction->Players.count); 
	Factions_Modify(faction);
	return qtrue;
}

void Factions_AddRank(Faction_t *faction, char *rankName, char *rankTag, unsigned int level, unsigned int auths){
	unsigned int i = Lmd_Arrays_AddArrayElement((void **)&faction->Ranks.Rank, sizeof(factionRank_t), &faction->Ranks.count);
	Q_strncpyz(faction->Ranks.Rank[i].name, rankName, sizeof(faction->Ranks.Rank[i].name));
	Q_strncpyz(faction->Ranks.Rank[i].rankTag, rankTag, sizeof(faction->Ranks.Rank[i].rankTag));
	faction->Ranks.Rank[i].rankLevel = level;
	faction->Ranks.Rank[i].auths = auths;
	Factions_Modify(faction);
}

qboolean Factions_RemoveRank(Faction_t *faction, factionRank_t *rank){
	int index = (rank - faction->Ranks.Rank);
	unsigned int i;
	if(index < 0 || index > faction->Ranks.count)
		return qfalse;
	for(i = 0;i<faction->Players.count;i++){
		if(Q_stricmp(faction->Players.Player[i].rankName, rank->name) == 0){
			faction->Players.Player[i].rankName[0] = 0;
		}
	}
	Lmd_Arrays_RemoveArrayElement((void **)&faction->Ranks, index, sizeof(factionRank_t), &faction->Ranks.count); 
	Factions_Modify(faction);
	return qtrue;
}

factionMessage_t* Factions_GetAnnouncement(Faction_t *faction, char *name){
	unsigned int i;
	for(i = 0;i<faction->Announcements.count;i++){
		if(Q_stricmp(faction->Announcements.Message[i].title, name) == 0){
			return &faction->Announcements.Message[i];
		}
	}
	return NULL;
}

qboolean Factions_AddAnouncement(Faction_t *faction, unsigned int timeoutDay, char *title, char *message){
	unsigned int i;
	if((timeoutDay > 0 && timeoutDay - Time_Days(Time_Now()) <= 0) || Factions_GetAnnouncement(faction, title)){
		return qfalse;
	}
	i = Lmd_Arrays_AddArrayElement((void **)&faction->Announcements.Message, sizeof(factionMessage_t),
		&faction->Announcements.count);
	faction->Announcements.Message[i].timeoutDay = timeoutDay;
	Q_strncpyz(faction->Announcements.Message[i].title, title, sizeof(faction->Announcements.Message[i].title));
	faction->Announcements.Message[i].text = G_NewString(message);
	Factions_Modify(faction);
	return qtrue;
}

qboolean Factions_RemoveAnnouncement(Faction_t *faction, factionMessage_t *announcement){
	int index = (announcement - faction->Announcements.Message);
	if(index < 0 || index > faction->Announcements.count)
		return qfalse;
	G_Free(faction->Announcements.Message[index].text);
	Lmd_Arrays_RemoveArrayElement((void **)&faction->Announcements, index, sizeof(factionMessage_t),
		&faction->Announcements.count); 
	Factions_Modify(faction);
	return qtrue;
}

qboolean Factions_AnnouncementIsOutdated(factionMessage_t *message){
	if(message->timeoutDay > 0 && message->timeoutDay - Time_Days(Time_Now()) <= 0)
		return qtrue;
	return qfalse;
}

void Factions_RemoveOutdatedAnnouncements(Faction_t *faction){
	unsigned int i = 0;
	while(i<faction->Announcements.count){
		if(Factions_AnnouncementIsOutdated(&faction->Announcements.Message[i])){
			Factions_RemoveAnnouncement(faction, &faction->Announcements.Message[i]);
		}
		else
			i++;
	}
}

qboolean Factions_Remove(Faction_t *faction){
	unsigned int i, i2;
	for(i = 0;i<FactionList.count;i++){
		if(faction == FactionList.Factions[i]){
			Lmd_Arrays_RemoveAllElements((void **)&faction->Players.Player);
			Lmd_Arrays_RemoveAllElements((void **)&faction->Ranks.Rank);
			for(i2 = 0;i2<faction->Announcements.count;i2++){
				G_Free(faction->Announcements.Message[i2].text);
			}
			Lmd_Arrays_RemoveAllElements((void **)&faction->Announcements.Message);

			G_Free(faction);
			Lmd_Arrays_RemoveArrayElement((void **)&FactionList.Factions, i, sizeof(Faction_t *), &FactionList.count);
			return qtrue;
		}
	}
	return qfalse;
}

qboolean Factions_ParseKey(byte *object, qboolean pre, char *key, char *value){
	Faction_t *faction = (Faction_t *)object;
	if(pre){
		/*
		if(Q_stricmp(key, "leader") == 0){
			int leader = 0;
			if(sscanf((const char *)value, "%u", &leader) == 1)
				faction->Leader = leader;
			return qtrue;
		}
		*/
		if(Q_stricmp(key, "player") == 0){
			unsigned int id = 0;
			char rankName[MAX_NAMELEN + 1];
			//>= 1, in case we dont have a rank
			if(sscanf((const char *)value, "%u \"%"MAX_NAMELEN_TEXT"[^\"]\"", &id, rankName) >= 1 && Accounts_GetById(id)){
				Factions_AddPlayer(faction, rankName, id);
			}
			return qtrue;
		}
		else if(Q_stricmp(key, "rank") == 0){
			if(faction->Ranks.count < MAX_RANKS){
				char name[MAX_NAMELEN + 1], rankTag[MAX_TAGLEN + 1];
				unsigned int auths = 0, rankLevel = 0;
				if(sscanf((const char *)value, "\"%"MAX_NAMELEN_TEXT"[^\"]\" \"%"MAX_RANKTAGLEN_TEXT"[^\"]\" %u %u",
					name, rankTag, &rankLevel, &auths) == 4){
					Factions_AddRank(faction, name, rankTag, rankLevel, auths);
				}
			}
			return qtrue; //even if we have too many ranks, this was still intended for us
		}
		else if(Q_stricmp(key, "announcement") == 0){
			if(faction->Announcements.count < MAX_ANNOUNCEMENTS){
				unsigned int timeoutDay = 0;
				char message[MAX_STRING_CHARS];
				char title[MAX_NAMELEN + 1];
				if(sscanf((const char *)value, "\"%"MAX_NAMELEN_TEXT"[^\"]\" %u \"%1023[^\"]\"", title, &timeoutDay, message) == 3){
					Factions_AddAnouncement(faction, timeoutDay, title, message);
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean Factions_LoadFile(char *name, char *buf){
	char *str = buf;
	Faction_t *faction = Factions_New(name);
	if(!faction)
		return qfalse;
	Lmd_Data_ParseFields_Old(&str, qtrue, Factions_ParseKey, FactionFields, (byte *)faction);
	faction->modifyTime = 0; //we just loaded it, no need to save (the New<item> functions will set this to save).
	return qtrue;
}

unsigned int Factions_Load(void){
	return Lmd_Data_ProcessFiles(FACTION_FOLDER, "."LMD_FACTION_EXT, Factions_LoadFile, Q3_INFINITE);
}

DBSaveFileCallbackReturn_t* Factions_SaveMoreKeys(byte *structure, DBSaveFileCallbackReturn_t *arg, char *key, int keySze, char *value, int valueSze){
	Faction_t *faction = (Faction_t *)structure;
	/*
	if(arg->func == 0){
		Q_strncpyz(key, "leader", keySze);
		Q_strncpyz(value, va("%u", faction->Leader), valueSze);
		arg->func++;
		arg->offset = 0; //not needed
		return arg;
	}
	*/
	if(arg->func == 0){
		if(arg->offset >= faction->Players.count){
			arg->func++;
			arg->offset = 0;
			return arg;
		}
		
		Q_strncpyz(key, "player", keySze);
		Q_strncpyz(value, va("%u \"%s\"", faction->Players.Player[arg->offset].playerId,
			faction->Players.Player[arg->offset].rankName), valueSze);
		arg->offset++;
		return arg;
	}
	else if(arg->func == 1){
		if(arg->offset >= faction->Ranks.count){
			arg->func++;
			arg->offset = 0;
			return arg;
		}
		
		Q_strncpyz(key, "rank", keySze);
		Q_strncpyz(value, va("\"%s\" \"%s\" %u %u", faction->Ranks.Rank[arg->offset].name,
			faction->Ranks.Rank[arg->offset].rankTag, faction->Ranks.Rank[arg->offset].rankLevel, 
			faction->Ranks.Rank[arg->offset].auths), valueSze);
		arg->offset++;
		return arg;
	}
	else if(arg->func == 2){
		if(arg->offset >= faction->Announcements.count){
			arg->func++;
			arg->offset = 0;
			return arg;
		}
		
		Q_strncpyz(key, "announcement", keySze);
		Q_strncpyz(value, va("\"%s\" %u \"%s\"", faction->Announcements.Message[arg->offset].title,
			faction->Announcements.Message[arg->offset].timeoutDay, faction->Announcements.Message[arg->offset].text), valueSze);
		arg->offset++;
		return arg;
	}
	return NULL;
}

void Factions_SaveFaction(unsigned int index){
	Lmd_Data_SaveDatafile(FACTION_FOLDER, va("%s."LMD_FACTION_EXT, FactionList.Factions[index]->name), FactionFields,
		(byte *)FactionList.Factions[index], NULL, Factions_SaveMoreKeys);
	FactionList.Factions[index]->modifyTime = 0;
}

void Factions_Save(qboolean full){
	unsigned int i;
	for(i = 0;i<FactionList.count;i++){
		if(FactionList.Factions[i]->modifyTime > 0 && (full || FactionList.Factions[i]->modifyTime + FACTION_SAVE_TIME <= level.time))
			Factions_SaveFaction(i);
	}
}

void Factions_List(gentity_t *ent, int playerId){
	unsigned int i;
	char name[MAX_STRING_CHARS], tag[MAX_STRING_CHARS], *Property;
	Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^3%-"MAX_TAGLEN_TEXT"s %s\n"
		"^3=====================================%s",
		"Faction name", "Tag", (playerId==0)?"^4Property":"", (playerId==0)?"=============================":""));
	for(i = 0;i<FactionList.count;i++){
		if(playerId == 0 || Factions_GetPlayerEntry(FactionList.Factions[i], playerId)){
			Q_strncpyz(name, FactionList.Factions[i]->name, sizeof(name));
			Q_CleanStr(name);
			if(FactionList.Factions[i]->tag){
				Q_strncpyz(tag, FactionList.Factions[i]->tag, sizeof(tag));
				Q_CleanStr(tag);
			}
			else
				tag[0] = 0;
			if(playerId == 0 && FactionList.Factions[i]->Property)
				Property = FactionList.Factions[i]->Property;
			else
				Property = "";
			Disp(ent, va("^7%-"MAX_NAMELEN_TEXT"s ^3%-"MAX_TAGLEN_TEXT"s ^4%s", name, tag, Property));
		}
	}
	Disp(ent, va("^3=====================================%s", (playerId==0)?"=============================":""));
}

void Factions_ListMembers(gentity_t *ent, Faction_t *faction){
	unsigned int i;
	Account_t *acc;
	char tmpName[MAX_STRING_CHARS], tmpRank[MAX_STRING_CHARS];
	Disp(ent, va("^3%-36s ^2%-36s ^5%-"MAX_NAMELEN_TEXT"s ^1%-"MAX_RANKTAGLEN_TEXT"s\n"
		"^3===================================================",
		"Player name", "Username", "Rank", "Rank tag")); //use %-s so we are all aligned properly
	/*
	//show our leader
	if(faction->Leader && (accIndex = getAccountIndexById(faction->Leader)) > 0){
		Disp(ent, va("^2%-36s ^2%-36s ^7%-"MAX_NAMELEN_TEXT"s",
			tmpName, Accounts_GetUsername(accIndex), "Leader"));
	}
	*/
	for(i=0;i<faction->Players.count;i++){
		acc = Accounts_GetById(faction->Players.Player[i].playerId);
		if(!acc)
			continue;
		Q_strncpyz(tmpName, Accounts_GetName(acc), sizeof(tmpName));
		Q_CleanStr(tmpName);
		Q_strncpyz(tmpRank, faction->Players.Player[i].rankName, sizeof(tmpRank));
		Q_CleanStr(tmpRank);
		Disp(ent, va("^3%-36s ^2%-36s ^7%-"MAX_NAMELEN_TEXT"s ^7%-"MAX_RANKTAGLEN_TEXT"s",
			tmpName, Accounts_GetUsername(acc), tmpRank, Factions_GetRankTag(faction, faction->Players.Player[i].rankName)));
	}
	Disp(ent, "^3===================================================");
}


void Factions_ListRanks(gentity_t *ent, Faction_t *faction, qboolean extraInfo){
	unsigned int i;
	if(extraInfo){
		Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^5Rank tag ^3level  ^1Auths\n"
			"^3=========================================",
			"Rank name")); //use %-s so we are all aligned properly
	}
	else{
		Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^5Rank tag\n"
			"^3=========================================",
			"Rank name")); //use %s so we are all aligned properly
	}
	for(i = 0;i<faction->Ranks.count;i++){
		if(extraInfo){
			char auths[MAX_STRING_CHARS];
			Factions_RankString(faction->Ranks.Rank[i].auths, auths, sizeof(auths));
			Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^5%-"MAX_RANKTAGLEN_TEXT"s      ^3%-5u %s", faction->Ranks.Rank[i].name,
				faction->Ranks.Rank[i].rankTag, faction->Ranks.Rank[i].rankLevel, auths));
		}
		else{
			Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^5%-"MAX_RANKTAGLEN_TEXT"s", faction->Ranks.Rank[i].name,
				faction->Ranks.Rank[i].rankTag));
		}
	}
}

void Factions_ListAnnouncements(gentity_t *ent, Faction_t *faction){
	unsigned int i;
	int now = Time_Days(Time_Now());
	char daysLeft[MAX_STRING_CHARS];
	Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s ^3Days left\n"
		"^3=========================================",
		"Announcements"));
	for(i = 0;i<faction->Announcements.count;i++){
		if(faction->Announcements.Message[i].timeoutDay > 0)
			Q_strncpyz(daysLeft, va("%u", faction->Announcements.Message[i].timeoutDay - now), sizeof(daysLeft));
		else
			Q_strncpyz(daysLeft, "Infinite", sizeof(daysLeft));
			
		Disp(ent, va("^2%-"MAX_NAMELEN_TEXT"s %s", faction->Announcements.Message[i].title, daysLeft));
	}
	Disp(ent, "^3=========================================");
}

char *ConcatArgs(int start);
void Cmd_Factions_f(gentity_t *ent, int iArg){
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	Faction_t *faction;
	factionPlayer_t *player;
	/*
	faction -- lists factions you are in
	faction <faction name>
		info -- gives information about the faction (leader, number of players, et al
		members
			list -- lists all members and their ranks
			recruit <username> <message> -- sends a recruition offer via in-game mail
			promote <member> <rank> -- Set the member to the specified rank.  You cannot set someones rank beond your own rank.
			kick <member> -- kick the member out of the faction.
		ranks
			list -- lists all ranks
			new <name> <rank level> -- create a new rank.  Players with the promotion auth will not be able to promote players
											beond their own promotion level of their rank (so a member cannot promote themself to
											a leader).  A lower level is a higher rank, '1' being the highest.
			setauth <rank> <auth flag> -- set or remove an auth flag from the given rank
			settag <name> <tag> -- set the tag for a rank
			setmember <member> <rank> -- set a member to the specified rank
			delete <name> -- remove a rank
		announcements
			list -- list all announcements
			read <title> -- read an announcement
			new <days to display> <text> -- creates a new message
			delete <title> -- deletes a message

	factionadmin list
	factionadmin new <faction name>
	factionadmin edit <faction name>
		{generally same as /faction, but no locks}
		setproperty <property name>
	*/

	//check for the id, since an id of 0 is a special case for Factions_List
	if(ent->client->pers.Lmd.account == 0 || ent->client->sess.Lmd.id == 0){
		Disp(ent, "^3You must be logged in to use this command.");
		return;
	}
	if(iArg){
		if(argc < 2){
			Disp(ent, "^3Usage: factionadmin {^2list^3} {^2new ^4<faction name>^3} {^2edit ^4<faction name> <commands>^3} {^2delete ^4<faction name>^3}\n"
				"^3This command lets you perform general administrative actions on the factions.");
			return;
		}
		trap_Argv(1, arg, sizeof(arg));
		if(Q_stricmp(arg, "list") == 0){
			Factions_List(ent, 0);
			return;
		}
		else if(Q_stricmp(arg, "new") == 0){
			if(argc < 3){
				Disp(ent, "^3Usage: factionadmin new <faction name>\n"
					"^3This command creates a new faction.");
				return;
			}
			trap_Argv(2, arg, sizeof(arg));
			if(!Factions_New(arg)){
				Disp(ent, "^3A faction already exists by this name.");
				return;
			}
			Disp(ent, "^2Faction created.");
			return;
		}
		else if(Q_stricmp(arg, "delete") == 0){
			if(argc < 3){
				Disp(ent, "^3Usage: factionadmin delete <faction name>\n"
					"^3This command deletes a faction.");
				return;
			}
			trap_Argv(2, arg, sizeof(arg));
			faction = Factions_GetFaction(arg);
			if(!faction){
				Disp(ent, "^3There is no faction by this name.");
				return;
			}
			if(!Factions_Remove(faction)){
				Disp(ent, "^1Error removing faction.");
				return;
			}
			Disp(ent, "^1Faction deleted.");
			return;
		}
		else if(Q_stricmp(arg, "edit") != 0){
			Disp(ent, "^3Unknown argument.");
			return;
		}
		else if(argc < 3){
			Disp(ent, "^3Usage: factionadmin edit <faction name>");
			return;
		}
	}
	else{
		if(argc < 2){
			Disp(ent, "^3Your factions:");
			Factions_List(ent, ent->client->sess.Lmd.id);
			return;
		}
	}
	trap_Argv(1 + (iArg == 1), arg, sizeof(arg));
	faction = Factions_GetFaction(arg);
	if(!faction || (!iArg && !(player = Factions_GetPlayerEntry(faction, ent->client->sess.Lmd.id)))){
		Disp(ent, "^3Unknown faction.");
		return;
	}
	if(argc < 3 + (iArg == 1)){
		Disp(ent, "^3Faction commands:\n"
			"^2info\n"
			"^2members\n"
			"^2announcements");
		if(iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_SETRANKS) ||
			Factions_PlayerHasAuth(faction, player, FACTAUTH_PROMOTE)){
				Disp(ent, "^1ranks");
		}
		if(iArg){
			Disp(ent, "^6settag\n"
					  "^6setproperty\n"
					  "^6addplayer");
		}
		return;
	}
	trap_Argv(2 + (iArg == 1), arg, sizeof(arg));
	if(Q_stricmp(arg, "settag") == 0 && iArg){
		if(argc < 5){
			Disp(ent, "^3Usage: factionadmin edit <faction name> settag <tag>\n"
				"^3This command sets the faction tag.  Currently this is unused.");
			return;
		}
		trap_Argv(4, arg, sizeof(arg));
		if(strlen(arg) > MAX_TAGLEN){
			Disp(ent, va("^3The tag must be less than %u characters.", MAX_TAGLEN + 1));
			return;
		}
		if(!Factions_SetFactionTag(faction, arg)){
			Disp(ent, "^3That tag is already in use.");
			return;
		}
		Disp(ent, "^2Tag changed.");
	}
	else if(Q_stricmp(arg, "setproperty") == 0){
		if(argc < 5){
			Disp(ent, "^3Usage: factionadmin edit <faction name> setproperty <property name>\n"
				"^3This command sets the faction property.  Players in this faction will be able to use property terminals by this name.");
			return;
		}
		trap_Argv(4, arg, sizeof(arg));
		Factions_SetFactionProperty(faction, arg);
		Disp(ent, "^3New property set.");
	}
	else if(Q_stricmp(arg, "addplayer") == 0){
		Account_t *acc;
		if(argc < 5){
			Disp(ent, "^3Usage: factionadmin edit <faction name> addplayer <player name>\n"
				"^3This command lets you add a player into the faction.");
			return;
		}
		trap_Argv(4, arg, sizeof(arg));
		acc = Accounts_GetById(atoi(arg));
		if(!acc)
			acc = Accounts_GetByUsername(arg);
		if(!acc){
			Disp(ent, "^3Unable to find account.");
			return;
		}
		int index = Accounts_GetId(acc);
		if(index > 0){
			Factions_AddPlayer(faction, "", index);
			Disp(ent, "^2Player added.");
		}
		else
			Disp(ent, "^1Unable to get player ID.");
	}
	else if(Q_stricmp(arg, "info") == 0){
		//int leaderIndex = getAccountIndexById(faction->Leader);
		Disp(ent,		va("^3Name:   ^7%s\n"
						   "^3Tag:    ^7%s", faction->name, faction->tag));
		//if(leaderIndex > 0)
		//	Disp(ent,	va("^3Leader: ^7%s", Accounts_GetName(leaderIndex)));
		//Disp(ent,		va("^3Number of members: ^2%i", faction->Players.count + (leaderIndex > 0)));
		Disp(ent,		va("^3Number of members: ^2%i", faction->Players.count));
		//if(faction->Property)
		//	Disp(ent,	va("^3Property: ^4%s", faction->Property));
	}
	else if(Q_stricmp(arg, "members") == 0){
		if(argc < 4 + (iArg == 1)){
			Disp(ent, "^3Faction member commands:\n"
				"^2list");
			if(iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_RECRUIT))
				Disp(ent, "^1recruit");
			if(!iArg && Factions_PlayerHasAuth(faction, player, FACTAUTH_PROMOTE))
				Disp(ent, "^1promote");
			return;
		}
		trap_Argv(3 + (iArg == 1), arg, sizeof(arg));
		if(Q_stricmp(arg, "list") == 0){
			Factions_ListMembers(ent, faction);
			return;
		}
		else if(Q_stricmp(arg, "recruit") == 0 && (iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_RECRUIT))){
			if(argc < 5 + (iArg == 1)){
				Disp(ent, va("^3Usage: /%s <faction name> members recruit <username> <message>\n"
					"^3This command will send a recruitment letter to the specified player, if they have turned on reciving of recruitment messages.",
					iArg?"factionadmin edit":"factions"));
				return;
			}
			Disp(ent, "^6Not programmed in yet...");
		}
		else if(Q_stricmp(arg, "promote") == 0 && !iArg && Factions_PlayerHasAuth(faction, player, FACTAUTH_PROMOTE)){
			Account_t *acc;
			factionPlayer_t *otherPlayer;
			factionRank_t *myRank, *otherRank, *newRank;
			if(argc < 6 + (iArg == 1)){
				Disp(ent, va("^3Usage: /%s <faction name> members promote <member username> <rank>\n"
					"^3This command lets you set the rank of a member.  You cannot set a rank with a greater rank than your own.",
					iArg?"factionadmin edit":"factions"));
				return;
			}
			trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
			acc = Accounts_GetByUsername(arg);
			if(!acc || !(otherPlayer = Factions_GetPlayerEntry(faction, Accounts_GetId(acc)))){
				Disp(ent, "^3That player does not exist or is not in this faction.");
				return;
			}
			if(otherPlayer == player){
				Disp(ent, "^3You cannot promote yourself.");
				return;
			}
			trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
			newRank = Factions_GetRank(faction, arg);
			if(!newRank){
				Disp(ent, "^3That rank does not exist.");
				return;
			}
			if(!iArg){
				myRank = Factions_GetRank(faction, player->rankName);
				otherRank = Factions_GetRank(faction, otherPlayer->rankName);
				if(otherRank && otherRank->rankLevel <= myRank->rankLevel){
					Disp(ent, "^3You are an inferior rank to that player, you may not affect their rank.");
					return;
				}
				if(newRank->rankLevel <= myRank->rankLevel){
					Disp(ent, "^3You cannot promote someone to a rank equal to or beond your own.");
					return;
				}
			}
			Factions_SetPlayerRank(faction, otherPlayer, newRank);
			Disp(ent, "^2Player rank has been changed.");
		}
		else if(Q_stricmp(arg, "kick") == 0 && (iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_KICK))){
			Account_t *acc;
			factionPlayer_t *otherPlayer;
			factionRank_t *myRank, *otherRank;
			if(argc < 6 + (iArg == 1)){
				Disp(ent, va("^3Usage: /%s <faction name> members kick <member username>%s",
					iArg?"factionadmin edit":"factions", iArg?"":"\n^3You cannot kick a member with a greater rank than your own."));
				return;
			}
			trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
			acc = Accounts_GetByUsername(arg);
			if(!acc || !(otherPlayer = Factions_GetPlayerEntry(faction, Accounts_GetId(acc)))){
				Disp(ent, "^3A player by this username does not exist or is not in this faction.");
				return;
			}
			if(otherPlayer == player){
				Disp(ent, "^3You cannot kick yourself.");
				return;
			}
			if(!iArg){
				myRank = Factions_GetRank(faction, player->rankName);
				otherRank = Factions_GetRank(faction, otherPlayer->rankName);
				if(otherRank && otherRank->rankLevel <= myRank->rankLevel){
					Disp(ent, "^3You are an inferior rank to that player, you may not kick them.");
					return;
				}
			}
			if(!Factions_RemovePlayer(faction, otherPlayer))
				Disp(ent, "^1Error removing player.");
			else
				Disp(ent, "^2Player kicked from the faction/");
		}
		else
			Disp(ent, "^3Unknown arg for ^2members^3.");
	}
	else if(Q_stricmp(arg, "ranks") == 0 && (iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_SETRANKS) ||
		Factions_PlayerHasAuth(faction, player, FACTAUTH_PROMOTE))){
			if(argc < 4 + (iArg == 1)){
			/*
			delete <name> -- remove a rank
			*/
				Disp(ent, "^3Faction rank commands:\n"
					"^2list");
				if(iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_SETRANKS)){
					Disp(ent, "^1new\n"
						"^1setauth\n"
						"^1settag\n"
						"^1setmember\n"
						"^1delete");
					return;
				}
			}
			trap_Argv(3 + (iArg == 1), arg, sizeof(arg));
			if(Q_stricmp(arg, "list") == 0){
				Factions_ListRanks(ent, faction, qtrue);
				return;
			}
			if(!iArg && !Factions_PlayerHasAuth(faction, player, FACTAUTH_SETRANKS)){
				Disp(ent, "^3Unknown arg for ^2ranks");
				return;
			}
			else if(Q_stricmp(arg, "new") == 0){
				int rankLevel;
				if(argc < 6 + (iArg == 1)){
					Disp(ent, va("^3Usage: /%s <faction name> ranks new <name> <level>\n"
						"^3This command lets you make a new rank.  The ranks with a high level value cannot affect ranks with a lower level value.",
						iArg?"factionadmin edit":"factions"));
					return;
				}
				if(faction->Ranks.count >= MAX_RANKS){
					Disp(ent, va("^3You have reached the limit for number of ranks (%u).", MAX_RANKS));
					return;
				}
				trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
				rankLevel = atoi(arg);
				if(rankLevel < 1){
					Disp(ent, "^3Rank level must be greater than 0.  1 is the highest level.");
					return;
				}
				trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
				if(strlen(arg) > MAX_NAMELEN){
					Disp(ent, va("^3The name must be less than %u letters long.", MAX_NAMELEN + 1));
					return;
				}
				if(Factions_GetRank(faction, arg)){
					Disp(ent, "^3A rank already exists by this name.");
					return;
				}
				Factions_AddRank(faction, arg, "", rankLevel, 0);
				Disp(ent, "^2Rank added.");
			}
			else if(Q_stricmp(arg, "setauth") == 0){
				unsigned int auth;
				factionRank_t *rank;
				if(argc < 6 + (iArg == 1)){
					Disp(ent, va("^3Usage: /%s <faction name> ranks setauth <rank name> <auth>\n"
						"^3This command sets or removes an auth from the rank.\n"
						"^1Possible auths:\n"
						"^2Ranks: ^3Add/delete/modify ranks\n"
						"^2Recruit: ^3Send recruitment offers to players.\n"
						"^2Promote: ^3Control player ranks of players with lower ranks than them.\n"
						"^2Kick: ^3Kick a player out of the faction.\n"
						"^2Announcements: ^3Add or remove faction-wide announcements.",
						iArg?"factionadmin edit":"factions"));
					return;
				}
				trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
				if(Q_stricmp(arg, "ranks") == 0)
					auth = FACTAUTH_SETRANKS;
				else if(Q_stricmp(arg, "recruit") == 0)
					auth = FACTAUTH_RECRUIT;
				else if(Q_stricmp(arg, "promote") == 0)
					auth = FACTAUTH_PROMOTE;
				else if(Q_stricmp(arg, "kick") == 0)
					auth = FACTAUTH_KICK;
				else if(Q_stricmp(arg, "Announcements") == 0)
					auth = FACTAUTH_ANNOUNCE;
				else{
					Disp(ent, "^3Invalid auth.");
					return;
				}
				trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
				rank = Factions_GetRank(faction, arg);
				if(!rank){
					Disp(ent, "^3There is no rank by that name.");
					return;
				}
				if(Factions_SetAuth(faction, rank, auth))
					Disp(ent, "^2Auth added.");
				else
					Disp(ent, "^2Auth removed.");
			}
			else if(Q_stricmp(arg, "settag") == 0){
				factionRank_t *rank;
				if(argc < 6 + (iArg == 1)){
					Disp(ent, va("^3Usage: /%s <faction name> ranks settag <rank name> <tag>\n"
						"^3This command lets you set the tag for the rank.  Currently this is unused.",
						iArg?"factionadmin edit":"factions"));
					return;
				}
				trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
				rank = Factions_GetRank(faction, arg);
				if(!rank){
					Disp(ent, "^3There is no rank by that name.");
					return;
				}
				trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
				if(strlen(arg) > MAX_RANKTAGLEN){
					Disp(ent, va("^3The tag must be less than %u characters long.", MAX_RANKTAGLEN + 1));
					return;
				}
				if(!Factions_SetRankTag(faction, rank, arg)){
					Disp(ent, "^3That tag is already in use.");
					return;
				}
				Disp(ent, "^2Tag set.");
			}
			else if(Q_stricmp(arg, "setmember") == 0){
				Account_t *acc;
				factionPlayer_t *otherPlayer;
				factionRank_t *newRank;
				if(argc < 6 + (iArg == 1)){
					Disp(ent, va("^3Usage: /%s <faction name> ranks setmember <member username> <rank>\n"
						"^3This command lets you set the rank of a member.  You cannot set a rank with a greater rank than your own.",
						iArg?"factionadmin edit":"factions"));
					return;
				}
				trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
				acc = Accounts_GetByUsername(arg);
				if(!acc || !(otherPlayer = Factions_GetPlayerEntry(faction, Accounts_GetId(acc)))){
					Disp(ent, "^3That player does not exist or is not in this faction.");
					return;
				}
				trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
				newRank = Factions_GetRank(faction, arg);
				if(!newRank){
					Disp(ent, "^3That rank does not exist.");
					return;
				}
				if(!Factions_SetMemberRank(faction, otherPlayer, newRank)){
					Disp(ent, "^3Rank was not changed.  The player already has this rank.");
					return;
				}
				Disp(ent, "^2Member rank has been changed.");
			}
			else if(Q_stricmp(arg, "delete") == 0){
				factionRank_t *rank;
				if(argc < 5 + (iArg == 1)){
					Disp(ent, va("^3Usage: /%s <faction name> ranks delete <rank name>\n"
						"^3This command lets delete a rank.  All players with this rank will loose the rank.",
						iArg?"factionadmin edit":"factions"));
					return;
				}
				trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
				rank = Factions_GetRank(faction, arg);
				if(!rank){
					Disp(ent, "^3That rank does not exist.");
					return;
				}
				Factions_RemoveRank(faction, rank);
				Disp(ent, "^1Rank deleted.");
			}
			else
				Disp(ent, "^3Unknown arg for ^2ranks^3.");
	}
	else if(Q_stricmp(arg, "announcements") == 0){
		if(argc < 4 + (iArg == 1)){
			Disp(ent, "^3Faction announcement commands:\n"
				"^2list\n"
				"^2read");
			if(iArg || Factions_PlayerHasAuth(faction, player, FACTAUTH_ANNOUNCE))
				Disp(ent, "^1new\n"
						  "^1delete");
			return;
		}

		Factions_RemoveOutdatedAnnouncements(faction);

		trap_Argv(3 + (iArg == 1), arg, sizeof(arg));

		if(Q_stricmp(arg, "list") == 0){
			Factions_ListAnnouncements(ent, faction);
			return;
		}
		else if(Q_stricmp(arg, "read") == 0){
			factionMessage_t *msg;
			if(argc < 5 + (iArg == 1)){
				Disp(ent, va("^3Usage: %s <faction name> announcements read <announcement name>\n"
					"^3This command lets you read faction-wide announcements.", iArg?"factionadmin edit":"factions"));
				return;
			}
			//trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
			if(!(msg = Factions_GetAnnouncement(faction, ConcatArgs(4 + (iArg == 1))))){
				Disp(ent, "^3There is no announcement by that name.");
				return;
			}
			Disp(ent, va("^3Announcement: ^2%s\n"
					  "^3====================================================================", msg->title));
			Disp(ent, msg->text);
			Disp(ent, "^3====================================================================");
			return;
		}

		if(!iArg && !Factions_PlayerHasAuth(faction, player, FACTAUTH_ANNOUNCE)){
			Disp(ent, "^3Unknown arg for ^2announcements");
			return;
		}
		else if(Q_stricmp(arg, "new") == 0){
			int days;
			if(argc < 7 + (iArg == 1)){
				Disp(ent, va("^3Usage: %s <faction name> announcements new <name> <days to display> <message>\n"
					"^3This command lets you create announcements.  To have no time limit, set <days to display> to 0",
					iArg?"factionadmin edit":"factions"));
				return;
			}
			if(faction->Announcements.count >= MAX_ANNOUNCEMENTS){
				Disp(ent, va("^3You have reached the limit for number of announcements (%u).", MAX_ANNOUNCEMENTS));
				return;
			}
			trap_Argv(5 + (iArg == 1), arg, sizeof(arg));
			days = atoi(arg);
			if(days < 0)
				days = 0;
			trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
			if(!Factions_AddAnouncement(faction, Time_Now() + Time_DaysToTime(days), arg, ConcatArgs(6 + (iArg == 1)))){
				Disp(ent, "^3There is already an announcement by that name.");
				return;
			}
			Disp(ent, "^2Announcement posted.");
		}
		else if(Q_stricmp(arg, "delete") == 0){
			factionMessage_t *msg;
			if(argc < 5 + (iArg == 1)){
				Disp(ent, va("^3Usage: %s <faction name> announcements delete <announcement name>\n"
					"^3This command will delete an announcement.", iArg?"factionadmin edit":"factions"));
				return;
			}
			trap_Argv(4 + (iArg == 1), arg, sizeof(arg));
			if(!(msg = Factions_GetAnnouncement(faction, arg))){
				Disp(ent, "^3There is no announcement by that name.");
				return;
			}
			Factions_RemoveAnnouncement(faction, msg);
			Disp(ent, "^1Announcement deleted.");
		}
		else
			Disp(ent, "^3Unknown arg for ^2announcements^3.");
	}
	else
		Disp(ent, "^3Unknown argument.");
}

