

enum{
	PROF_NONE = 0,
	PROF_ADMIN,
	PROF_BOT,
	PROF_JEDI,
	PROF_MERC,
	NUM_PROFESSIONS
};



//#define LMD_TECH_OLDSETUP

//Tech skills.
#ifdef LMD_TECH
enum{
	//SK_TECH_SCANNER = 0,
	SK_TECH_REMOTESTASHDEPO = 0,
	SK_TECH_DEMP2,
	SK_TECH_GENERATOR,

#ifdef LMD_TECH_OLDSETUP
	SK_TECH_ARMAMENTS,
	SK_TECH_CONSUMABLE,
	SK_TECH_CRYSTALS,
	SK_TECH_DATACHIPS,
	SK_TECH_DEVICES,
	SK_TECH_WEAPONMODS,
//#else
	SK_TECH_RATIONS,
	SK_TECH_MEDICINE,
	SK_TECH_STASHDECOY,
#endif

	SK_TECH_NUM_SKILLS,
};

#endif


//General functions.
int Profession_LevelCost(int level, int time);
char* Professions_GetName(int prof);

int Professions_TotalSkillPoints(int prof, int level);

int Accounts_Prof_GetProfession(Account_t *acc);
#define PlayerAcc_Prof_GetProfession(ent) Accounts_Prof_GetProfession(ent->client->pers.Lmd.account)

void Accounts_Prof_SetProfession(Account_t *acc, int value);
#define PlayerAcc_Prof_SetProfession(ent, value) Accounts_Prof_SetProfession(ent->client->pers.Lmd.account, value)

int Accounts_Prof_GetLevel(Account_t *acc);
#define PlayerAcc_Prof_GetLevel(ent) Accounts_Prof_GetLevel(ent->client->pers.Lmd.account)

void Accounts_Prof_SetLevel(Account_t *acc, int value);
#define PlayerAcc_Prof_SetLevel(ent, value) Accounts_Prof_SetLevel(ent->client->pers.Lmd.account, value)

void Accounts_Prof_ClearData(Account_t *acc);

qboolean PlayerAcc_Prof_CanUseProfession(gentity_t *ent);
