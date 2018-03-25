

typedef struct weaponDamage_s {
	unsigned int damage;
	unsigned int splashdamage;
	unsigned int splashradius;
	unsigned int dflags;
	unsigned int mod;
	unsigned int splashmod;
}weaponDamage_t;

typedef struct weaponLauncher_s {
	unsigned int rate;
	unsigned int energy;
	float spread;
	float spreadrate;
}weaponLauncher_t;

typedef struct weaponProjectile_s {
	unsigned int size;
	unsigned int velocity;
	qboolean gravity;
	unsigned int life;
	int bounce;
}weaponProjectile_t;


typedef struct weaponFire_s weaponFire_t;
struct weaponFire_s {
	int projectiles;
	void (*fire)(gentity_t *ent, gentity_t *missile, weaponFire_t *data); // If null, use standard projectile with s.weapon set to weapon num.
	weaponLauncher_t launcher;
	weaponDamage_t damage;
	weaponProjectile_t projectile;
	int flags;
	void *extra;
};

//Projectiles, Fire, Damage, Size Energy Velocity Rate Gravity Life Bounce Spread SpreadRate Flags Extra
//Projectiles, Fire, {Rate, Energy, Spread, SpreadRate}, {Damage...} {Size, Velocity, Gravity, Life, Bounce}, Flags, Extra