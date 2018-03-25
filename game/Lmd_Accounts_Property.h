
enum{
	PROPRANK_NONE = 0,
	PROPRANK_OWNER, //owns the property
	PROPRANK_CARETAKER, //can add any of the below ranks
	PROPRANK_GUEST, //can enter but not add any

	PROPRANK_MAX
};

unsigned int Accounts_Property_GetAccess(Account_t *acc, char *key, int *adder, unsigned int *expireDay, qboolean *rented);
#define PlayerAcc_Property_GetAccess(ent, key, adder, expireDay, rented) Accounts_Property_GetAccess(ent->client->pers.Lmd.account, key, adder, expireDay, rented)

void Accounts_Property_SetAccess(Account_t *acc, char *key, unsigned int rank, int adder, unsigned int expireDay, qboolean rented);
#define PlayerAcc_Property_SetAccess(ent, key, rank, adder, expireDay, rented) Accounts_Property_SetAccess(ent->client->pers.Lmd.account, key, rank, adder, expireDay, rented)

void Accounts_Property_ViewAccount(gentity_t *ent, Account_t *acc);
void Accounts_Property_ViewKey(gentity_t *ent, char *key);

int Accounts_Property_GetPlayerPropRank(gentity_t *ent, char *prop);