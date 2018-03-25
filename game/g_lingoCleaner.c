
#include "g_local.h"
#include "Lmd_Data.h"
#include "Lmd_Bans.h"

static char *badwords = NULL;
static char *measures = NULL;

extern  vmCvar_t g_profanityFile;

qboolean ParseField(char **sourcep, char *dest, char start, char stop);
char* skipline (const char *cp);

void loadLingoFilter (void){
	char *buf;
	int i;
	char *sp, *ep;

	if (!g_profanityFile.string || !g_profanityFile.string[0]) {
			return;
	}


	G_Printf("Loading Lugormod profanity filter ...\n");
	buf = Lmd_Data_AllocFileContents(g_profanityFile.string);

	if (!buf){
		G_Printf(S_COLOR_YELLOW "Lugormod profanity filter data not found.\n");
		return;
	}

	sp = strchr(buf, '{');
	if (sp) {
		*sp = '\0';
		sp++;
	}

	badwords = (char*) G_Alloc(strlen(buf) + 1);
	for (i = 0, ep = buf; *ep; ep++) {
		if (*ep != '\n' && *ep != '\r') {
			badwords[i++] = *ep;
		} else if (*ep == '\n') {
			badwords[i++] = ' ';
		}

	}
	badwords[i] = '\0';

	if (sp) {
		sp = skipline(sp);
		ep = strchr(sp, '}');
		if (ep)
		{
			*ep = '\0';
			measures = (char*)G_Alloc(strlen(sp) + 1);
			Q_strncpyz(measures, sp, strlen(sp) + 1);
		}
	}
	G_Free(buf);
}

qboolean Q_wordInLine (const char *word,/*int wordl,*/const char *line);

qboolean
profanityCheck(const char *msg) 
{
	if (!g_profanityFile.string ||
		!g_profanityFile.string[0] ||
		!msg || !msg[0] || !badwords) {
			return qfalse;
	}
	const char *s;
	s = badwords;
	while (*s) {
		while (*s && (*s == ' ' || *s == '\n' || *s == '\t' || *s == '\r')) {
			s++;
		}
		//This should be more advanced
		if (*s && Q_wordInLine(s,/* s-t,*/ msg)) {
			return qtrue;
		}

		while (*s && *s != ' ' && *s != '\n' && *s != '\t' && *s != '\r'){
			s++;
		}

	}
	return qfalse;
}

qboolean IsKing(gentity_t *ent);
void BecomeCommoner (gentity_t *ent);
//RoboPhred
void jailPlayer(gentity_t *targ, int time);

qboolean takeAction(gentity_t *ent, int num){
	if (!ent || !ent->client) {
		return qfalse;
	}

	int lines = 0;
	int i = 0;
	int n;

	char *sp = measures;
	char *tp;

	char msg[MAX_STRING_CHARS];

	while (*sp) {
		if (*sp++ == '\n') {
			lines++;
		}
	}
	sp = measures;
	if (num > lines - 1) {
		n = lines - 1;

	} else {
		n = num;
	}



	while (i++ < n) {
		sp = skipline(sp);
	}
	tp = sp;
	i = 0;
	n = 0;

	while (*tp && *tp != '\n') {
		if (*tp++ == '\"') {
			i++;
		}
		if (i == 1) {
			if (*tp == '\\' && tp[1]) {
				tp++;
				if (*tp == 'n') {
					msg[n++] = '\n';
					continue;
				}
			}
			msg[n++] = *tp;
		}
	}
	msg[n] = '\0';
	if (i > 1) {
		trap_SendServerCommand(ent-g_entities, va("cp \"%s\"",msg));
	}
	if (Q_strncmp("kick",sp,4) == 0) {
		//trap_SendConsoleCommand(EXEC_APPEND, 
		//                        va("clientkick %i\n",
		//                           ent-g_entities));
		trap_DropClient(ent-g_entities,"was using foul language.");
	} else if (Q_strncmp("mute",sp,4) == 0) {
		ent->client->pers.Lmd.persistantFlags |= SPF_SHUTUP;
	} else if (Q_strncmp("jail",sp,4) == 0) {
		if(!(ent->client->pers.Lmd.jailTime > level.time)){
			ent->client->ps.persistant[PERS_SCORE]++;
			if (IsKing(ent)) {
				BecomeCommoner(ent);
			}
			//RoboPhred
			jailPlayer(ent, -1);
			/*
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE );
			trap_SendServerCommand(ent->s.number, "cp \"^3You have been jailed indefinitely.\n^3If you leave the server while in jail\n^3you will be temporarily banned from this server.\"");
			ent->client->pers.Lmd.jailTime = Q3_INFINITE;
			//SetTeam (ent, "jailed");
			*/
		}
	} else if (Q_strncmp("ban",sp,3) == 0)  {
		Bans_AddIP(ent->client->sess.Lmd.ip, -1, "Temporarily banned: voted off server");
		trap_DropClient(ent->s.number, "Temporarily banned, broke language rules.");
	} else if (Q_strncmp("censor",sp,6) == 0) {
	} else {
		return qfalse;
	}
	return qtrue;
}
