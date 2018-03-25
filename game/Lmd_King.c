
#include "g_local.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Professions.h"

int kingDuelTime;
int kingLastScore;
gentity_t *kingLastKill;
gentity_t *lastKing = NULL;
int g_bestKingScore = 0;
gentity_t *g_bestKing = NULL;

#define MAX_SCORING 5


int scoring(int loser, int winner) {
	float tmpscore;
	int score;
	tmpscore = (float)loser/(float)winner;
	if(tmpscore < 1){
		if(tmpscore > 0.5)
			score = 1;
		else
			score = 0;
	}
	else
		score = (int)tmpscore;

	if (score > MAX_SCORING)
		score = MAX_SCORING;

	if (score < 0)//Hmm just to be safe
		score = 0;

	if (score > loser)
		score = loser;
	return score;
}

char* winlose(int myScore, int hisScore){
	int win,loss;
	static char winlosestr[100];
	win  = scoring(hisScore, myScore);
	loss = scoring(myScore, hisScore);
	Com_sprintf(winlosestr, sizeof(winlosestr), "^2W:%2i L:%2i", win, loss);
	if (win == 0) {
		winlosestr[1] = '1';
	}

	return winlosestr;
}

char* showWinLose(gentity_t *me, gentity_t *him){
	int myScore = PlayerAcc_GetScore(me), hisScore = PlayerAcc_GetScore(him);

	if(!me->client->pers.Lmd.account)
		return "";

	if(!him->client->pers.Lmd.account)
		return "^3W: 0 L: 0";

	return winlose(myScore + me->client->pers.Lmd.chicken, hisScore + him->client->pers.Lmd.chicken);

}

void listWorthy(gentity_t *ent){
	int i;
	char dstr[MAX_TOKEN_CHARS];

	if (!ent->client->pers.Lmd.account) {
		Disp(ent,"You need to be registered to use this.\nType '\\help register' for more information.");
		return;
	}

	for(i = 0; i < MAX_CLIENTS; i++){
		if(i == ent->s.number)
			continue;
		if (!g_entities[i].inuse || !g_entities[i].client || !g_entities[i].client->ps.trueJedi/*Accounts_Prof_GetProfession(&g_entities[i]) > PROF_JEDI*/)
			continue;

		Q_strncpyz(dstr, va("%s", g_entities[i].client->pers.netname), sizeof(dstr));
		Q_CleanStr(dstr);
		Q_strcat (dstr, sizeof(dstr), "                         ");
		dstr[25] = 0;
		Q_strcat(dstr, sizeof(dstr), 
			winlose(PlayerAcc_GetScore(ent) + ent->client->pers.Lmd.chicken, PlayerAcc_GetScore((&g_entities[i])) + g_entities[i].client->pers.Lmd.chicken));
		Disp(ent, dstr);      
	}
}
qboolean IsKing(gentity_t *ent){
	if(!ent || !ent->client || ent->client->pers.connected != CON_CONNECTED || !(ent->client->pers.Lmd.persistantFlags & SPF_ISKING))
		return qfalse;
	return qtrue;
}

void BecomeKing(gentity_t *ent){
	vec3_t dAng;
	if (!ent || !ent->inuse || !ent->client)
		return;

	if (!(g_privateDuel.integer & PD_KING))
		return;

	ent->client->ps.isJediMaster = qtrue;

	if(IsKing(ent))
		return;

	VectorSet(ent->client->ps.velocity, 0, 0, 0);

	VectorSet(dAng, -90, 0, 0);

	G_PlayEffectID(G_EffectIndex("scepter/invincibility"), ent->r.currentOrigin, dAng);

	trap_SendServerCommand(-1, va("cp \"%s\n^3is now the King.\n\"", ent->client->pers.netname));
	trap_SendServerCommand(-1, va("print \"%s ^3is now the King.\n\"", ent->client->pers.netname));

	ent->client->pers.Lmd.persistantFlags |= SPF_ISKING;
	ent->client->pers.Lmd.kingScore = 0;
	kingDuelTime = level.time + g_kingTime.integer * 1000;
}

void BecomeCommoner (gentity_t * ent){
	if(!ent || !ent->client || ent->client->pers.connected != CON_CONNECTED)
		return;

	if(IsKing(ent)){
		if(lastKing && lastKing->client && lastKing->client->pers.connected == CON_CONNECTED && lastKing->client->pers.Lmd.wasKing){
			lastKing->client->pers.Lmd.wasKing = qfalse;
		}
		lastKing = ent;
		ent->client->pers.Lmd.wasKing = qtrue;
	}

	ent->client->ps.isJediMaster = qfalse;
	ent->client->pers.Lmd.persistantFlags &= ~SPF_ISKING;
}

void RevertKing(gentity_t *ent){
	if(!ent || !ent->client || !IsKing(ent)){
		return;
	}

	if (lastKing && lastKing->client && lastKing->client->pers.connected == CON_CONNECTED && lastKing->client->pers.Lmd.wasKing &&
		!(lastKing->client->pers.Lmd.jailTime > level.time)){
			BecomeKing(lastKing);
	}
	else{
		trap_SendServerCommand(-1, "cp \"^3There is no King.\"");
		trap_SendServerCommand(-1, "print \"^3There is no King.\n\"");

	}      
	BecomeCommoner(ent);
	lastKing = NULL;        
}

gentity_t *GetKing(void){
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (IsKing(ent))
		{
			return ent;
		}

		i++;
	}

	return NULL;
}
qboolean ThereIsAKing(void){
	int i;
	for(i = 0;i<MAX_CLIENTS;i++){
		if (IsKing(&g_entities[i]))
			return qtrue;
	}
	return qfalse;
}

void UndoScore(gentity_t *ent){
	if (!IsKing(ent) || kingLastScore == 0)
		return;

	if(!PlayerAcc_GetName(ent))
		return;

	PlayerAcc_SetScore(ent, PlayerAcc_GetScore(ent) - kingLastScore);
	Disp(ent, va("You lost %i points.", kingLastScore));
	if(kingLastKill){
		PlayerAcc_SetScore(kingLastKill, PlayerAcc_GetScore(kingLastKill) + kingLastScore);
		Disp(kingLastKill, va("You gained %i points.", kingLastScore));
		kingLastKill = NULL;
	}
}

void checkKingTimer(gentity_t *ent){
	if (g_privateDuel.integer & PD_KING && IsKing(ent) && !duelInProgress(&ent->client->ps)){
		if (g_kingTime.integer && kingDuelTime < level.time){
			RevertKing(ent);
			Com_Printf("info: The King ran out of time\n");
		}
		else if(ent->client->ps.weapon != WP_SABER && ent->health > 0){
			UndoScore(ent);
			RevertKing(ent);
			Com_Printf("info: The King changed weapons\n");
		}
		else if (kingDuelTime > level.time + 30000 && kingDuelTime < level.time + 29000){
			trap_SendServerCommand(ent-g_entities, "cp \"^3You have ^230 ^3seconds\n^3left as the King.\"");
		}
	}
}

void checkLevelUp(gentity_t *ent);

void CalcScore(gentity_t *winner, gentity_t *loser){
	int winnerId = PlayerAcc_GetId(winner), loserId = PlayerAcc_GetId(loser);
	int winnerScore = PlayerAcc_GetScore(winner);
	int loserScore = PlayerAcc_GetScore(loser);
	int winnerProf = PlayerAcc_Prof_GetProfession(winner);
	int score;
	if (!winner->client || (winner->client->Lmd.duel.duelType & DT_TRAINING)) {
		return;
	}

	if (!winnerId || !loserId){
		if(IsKing(loser) || !ThereIsAKing()){
			kingLastKill = NULL;
			kingLastScore = 0;
		}
		return;
	}
	if (winnerScore < 1) {
		winnerScore = 1;
	}
	if (loserScore < 1) {
		loserScore = 1;
	}

	score = scoring(loserScore + loser->client->pers.Lmd.chicken, winnerScore + winner->client->pers.Lmd.chicken);

	if(score > 1){
		Disp (winner, va("^3You were awarded ^2%i^3 points.", score));
		Disp (loser,  va("^3You lost ^2%i^3 points.", score));
	}

	else if(score < 0){
		Disp (winner, "^3You were awarded ^21^3 point.");
		Disp (loser, "^3You lost ^21^3 point.");
	} 
	else if(winner->client->pers.Lmd.chicken){
		Disp (winner, va("^3You have restored your honor and reclaimed your ^2%i^3 points.", winner->client->pers.Lmd.chicken));
		winnerScore += winner->client->pers.Lmd.chicken;
		winner->client->pers.Lmd.chicken = 0;
	}
	else{
		Disp (winner, va("^2%s's^3 score is too low to give you any points.", loser->client->pers.netname));
	}

	if(IsKing(winner) && loser != kingLastKill && !((winner->r.svFlags & SVF_BOT) && (loser->r.svFlags & SVF_BOT)) && score > 0) {
		int bonus = (int)floor((double)winner->client->pers.Lmd.kingScore / 3) + 1;
		if(bonus > 10)
			bonus = 10;
		kingLastScore += score + bonus;
		Disp (winner, va("^3Bonus points for being The King: ^2%i^3.", bonus));
		winnerScore += bonus;
	}

	if(IsKing(loser) || !ThereIsAKing()){
		kingLastScore = score;
		kingLastKill = loser;
	}

	winnerScore += score;
	loserScore -= score;

	if (loserScore < 1) {
		loserScore = 1;
	}

	if (winnerProf == PROF_NONE || winnerProf == PROF_BOT)
		checkLevelUp(winner);

	PlayerAcc_SetScore(winner, winnerScore);
	PlayerAcc_SetScore(loser, loserScore);

	if ((g_privateDuel.integer & PD_KING) && winner->client->Lmd.duel.duelType == 0 && IsKing(winner)){
		kingDuelTime = level.time + g_kingTime.integer * 1000;
		winner->client->pers.Lmd.kingScore++;

		if (winner->client->pers.Lmd.kingScore > g_bestKingScore || !g_bestKing || !g_bestKing->client) {
			g_bestKingScore = winner->client->pers.Lmd.kingScore;
			g_bestKing = winner;
		}
	}
	else if((g_privateDuel.integer & PD_KING) && loser->client->Lmd.duel.duelType == 0 && (IsKing(loser) || !ThereIsAKing())) {        
		if(IsKing(loser)){
			if(!g_bestKing || !g_bestKing->client || g_bestKing->client->pers.connected != CON_CONNECTED){
				g_bestKingScore = winner->client->pers.Lmd.kingScore;
				g_bestKing = winner;
			}
			Disp(loser, va("^3You won ^2%i^3 duels as King. The best king so far is ^7%s^3 who won ^2%i^3 duels.\n\"",
				loser->client->pers.Lmd.kingScore, g_bestKing->client->pers.netname, g_bestKingScore));
			BecomeCommoner(loser);
		}

		BecomeKing(winner);

		winner->client->ps.forceDodgeAnim = BOTH_FORCE_RAGE;
		winner->client->ps.forceHandExtendTime = level.time + 3000;

		G_SetAnim(winner, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	}
}

extern vmCvar_t g_chickenTime;

void Chicken (gentity_t *chicken){
	gentity_t *other = &g_entities[chicken->client->Lmd.duel.challengedBy];
	int otherScore, chickenScore;

	if (g_chickenTime.integer == 0 || ((g_cmdDisable.integer * 2) & (1 << 1)) ||
		!chicken->client->pers.Lmd.account || chicken->client->Lmd.duel.challengedBy < 0 ||
		!chicken->client->Lmd.duel.challengedTime || chicken->client->Lmd.duel.challengedTime > level.time){
			return;
	}

	if (other->r.svFlags & SVF_BOT) {
		return; //The chicken rule don't apply when challenged by a bot
	}

	chicken->client->Lmd.duel.challengedTime = 0;
	chicken->client->Lmd.duel.challengedBy = -1;

	if (!other->inuse || !other->client ||
		!other->client->pers.Lmd.account || other->client->ps.duelIndex != chicken->s.number	||
		other->client->Lmd.duel.duelType) {
			return;
	}

	otherScore = PlayerAcc_GetScore(other);
	chickenScore = PlayerAcc_GetScore(chicken);
	if (scoring(otherScore + other->client->pers.Lmd.chicken, chickenScore + chicken->client->pers.Lmd.chicken) == 0) {
		int ammount = chickenScore / 20;
		if (ammount > 5) {
			ammount = 5;
		}
		if (ammount > chickenScore - 1) {
			ammount = chickenScore - 1;
		}
		chickenScore -= ammount;
		chicken->client->pers.Lmd.chicken += ammount;
		chicken->client->Lmd.duel.challengedTime = level.time + (1000 * g_chickenTime.integer);
		if(ammount){
			if(ammount > 1){
				Disp(chicken, va("You lost %i points for being chicken.", ammount));
			}
			else{
				Disp (chicken, "You lost 1 point for being chicken.");
			}
		} 
		PlayerAcc_SetScore(other, otherScore);
		PlayerAcc_SetScore(chicken, chickenScore);
	}
}
