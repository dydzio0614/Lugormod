

void Force_DrainForceEnergy(gentity_t *ent, int power, int energy);
qboolean Force_CanUsePower(gentity_t *ent, int power);
qboolean Force_UsePower(gentity_t *ent, int power);
void Force_StopPower(gentity_t *ent, int power);

//Neutral
typedef struct forceLevitate_s{
	unsigned int strength;
	unsigned int forcepower;
	qboolean airjump;
}forceLevitate_t;

typedef struct forceSpeed_s{
	unsigned int duration;
	unsigned int forcepower;
}forceSpeed_t;

typedef struct forceThrow_s{
	unsigned int power;
	unsigned int radius;
	unsigned int arc;
	unsigned int forcepower;
	qboolean knockdown;
	unsigned int saberlockhits;
}forceThrow_t;

typedef struct forceSee_s{
	unsigned int duration;
	unsigned int forcepower;
}forceSee_t;

//Light
typedef struct forceHeal_s{
	unsigned int health;
	unsigned int forcepower;
}forceHeal_t;

typedef struct forceTelepathy_s{
	unsigned int duration;
	unsigned int npcduration;
	unsigned int range;
	unsigned int arc;
	qboolean divertnpcs;
	qboolean affectnpcjedi;
	unsigned int forcepower;
}forceTelepathy_t;

typedef struct forceProtect_s{
	float forcetake;
	float damagetake;
	unsigned int maxdamage;
	unsigned int duration;
	unsigned int draintime;
	unsigned int forcepower;
}forceProtect_t;

typedef struct forceAbsorb_s{
	unsigned int duration;
	unsigned int draintime;
	qboolean allowprotect;
	unsigned int forcepower;
}forceAbsorb_t;

typedef struct forceTeamHeal_s{
	unsigned int range;
	unsigned int heal[3];
	unsigned int debounce;
	unsigned int forcepower;
}forceTeamHeal_t;

//Dark
typedef struct forceGrip_s{
	unsigned int range;
	unsigned int damage;
	unsigned int choke;
	unsigned int duration;
	unsigned int moveType;
	unsigned int forcepower;
	unsigned int forcedrain;
	qboolean throatcrush;
	qboolean omni;
}forceGrip_t;

typedef struct forceLightning_s{
	unsigned int range;
	unsigned int arc;
	unsigned int duration;
	unsigned int debounce;
	unsigned int forcepower;
	qboolean twohanded;
}forceLightning_t;

typedef struct forceRage_s{
	unsigned int selfdamage;
	unsigned int duration;
	unsigned int draintime;
	unsigned int recoverytime;
	unsigned int forcepower;
}forceRage_t;

typedef struct forceTeamReplenish_s{
	unsigned int range;
	unsigned int power[3];
	unsigned int debounce;
	unsigned int forcepower;
}forceTeamReplenish_t;

typedef struct forceDrain_s{
	unsigned int damage;
	unsigned int regenstun;
	unsigned int range;
	unsigned int arc;
	unsigned int duration;
	unsigned int debounce;
	unsigned int forcepower;
}forceDrain_t;