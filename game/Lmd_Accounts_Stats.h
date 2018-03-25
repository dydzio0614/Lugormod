
int Accounts_Stats_GetDuels(Account_t *acc);
#define PlayerAcc_Stats_GetDuels(ent) Accounts_Stats_GetDuels(ent->client->pers.Lmd.account)

void Accounts_Stats_SetDuels(Account_t *acc, int value);
#define PlayerAcc_Stats_SetDuels(ent, value) Accounts_Stats_SetDuels(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetDuelsWon(Account_t *acc);
#define PlayerAcc_Stats_GetDuelsWon(ent) Accounts_Stats_GetDuelsWon(ent->client->pers.Lmd.account)

void Accounts_Stats_SetDuelsWon(Account_t *acc, int value);
#define PlayerAcc_Stats_SetDuelsWon(ent, value) Accounts_Stats_SetDuelsWon(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetKills(Account_t *acc);
#define PlayerAcc_Stats_GetKills(ent) Accounts_Stats_GetKills(ent->client->pers.Lmd.account)

void Accounts_Stats_SetKills(Account_t *acc, int value);
#define PlayerAcc_Stats_SetKills(ent, value) Accounts_Stats_SetKills(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetDeaths(Account_t *acc);
#define PlayerAcc_Stats_GetDeaths(ent) Accounts_Stats_GetDeaths(ent->client->pers.Lmd.account)

void Accounts_Stats_SetDeaths(Account_t *acc, int value);
#define PlayerAcc_Stats_SetDeaths(ent, value) Accounts_Stats_SetDeaths(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetShots(Account_t *acc);
#define PlayerAcc_Stats_GetShots(ent) Accounts_Stats_GetShots(ent->client->pers.Lmd.account)

void Accounts_Stats_SetShots(Account_t *acc, int value);
#define PlayerAcc_Stats_SetShots(ent, value) Accounts_Stats_SetShots(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetHits(Account_t *acc);
#define PlayerAcc_Stats_GetHits(ent) Accounts_Stats_GetHits(ent->client->pers.Lmd.account)

void Accounts_Stats_SetHits(Account_t *acc, int value);
#define PlayerAcc_Stats_SetHits(ent, value) Accounts_Stats_SetHits(ent->client->pers.Lmd.account, value)

int Accounts_Stats_GetStashes(Account_t *acc);
#define PlayerAcc_Stats_GetStashes(ent) Accounts_Stats_GetStashes(ent->client->pers.Lmd.account)

void Accounts_Stats_SetStashes(Account_t *acc, int value);
#define PlayerAcc_Stats_SetStashes(ent, value) Accounts_Stats_SetStashes(ent->client->pers.Lmd.account, value)
