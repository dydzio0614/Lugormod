
#include "Lmd_Data.h"

#include "Lmd_Accounts_Public.h"

void Lmd_Accounts_Modify(Account_t *acc);
#define PlayerAcc_Modify(ent) Lmd_Accounts_Modify(ent->client->pers.Lmd.account)

int Lmd_Accounts_AddDataCategory(accDataModule_t *category);
void* Lmd_Accounts_GetAccountCategoryData(Account_t *acc, int categoryIndex);
