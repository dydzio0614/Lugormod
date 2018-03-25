
unsigned int Accounts_Friends_Count(Account_t *acc);
#define PlayerAcc_Friends_Count(ent) Accounts_Friends_Count(ent->client->pers.Lmd.account)

qboolean Accounts_Friends_IsFriend(Account_t *acc, int otherId);
#define PlayerAcc_Friends_IsFriend(ent, otherId) Accounts_Friends_IsFriend(ent->client->pers.Lmd.account, otherId)

