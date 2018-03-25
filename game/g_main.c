// Copyright (C) 1999-2000 Id Software, Inc.
//

#define LMD_VER_ENC_LOCK

//Alpha: \xC6\xED\xF5\xF1\xE6
//Beta: \xC7\xE6\xF9\xEA

#ifdef LMD_VER_ENC_LOCK
#define ENC_LUGORMODVERSION_CVAR "\xD1\xF4\xF4\xDB\xF8\xF3\xEC\xF2\xE4\xF7\xF4\xFA\xF7\xE9\xEA\xEE"
#define ENC_GAMENAME_CVAR "\xEC\xEA\xF2\xF2\xE6\xE6\xF3\xEA"
#define ENC_BASECHAR '{'
#define ENC_LUGORMOD "\xD1\xF4\xF4\xEC\xF2\xFA\xF7\xE9"
#define LUGORMODVERSION_CORE "\xDA\xB2\xA8"
#ifdef LMD_EXPERIMENTAL
const int verMajor = 0;
const int verMinor = 0;
const int verRev = 0;
const int verBuild = 91;
#else
const int verMajor = 2;
const int verMinor = 4;
const int verRev = 8;
const int verBuild = 4;
#endif

#ifdef LMD_EXPERIMENTAL
#define LUGORMODVERSION_ATTACH "\xC6\xED\xF5\xF1\xE6"
#elif defined LMD_BETA
#define LUGORMODVERSION_ATTACH "\xC7\xE6\xF9\xEA"
#endif

const char *verMods =
#ifdef LMD_NEW_JETPACK
	"+JP "
#endif
#ifdef LMD_NEW_WEAPONS
	"+Wp "
#endif
#ifdef LMD_NEW_FORCEPOWERS
	"+Fp "
#endif
	"";

#else

#ifdef LMD_EXPERIMENTAL
#define LUGORMODVERSION LUGORMODVERSION_CORE" Alpha"
#else
#define LUGORMODVERSION LUGORMODVERSION_CORE
#endif

#endif

#include "g_local.h"
#include "g_ICARUScb.h"
#include "g_nav.h"
#include "bg_saga.h"

#include "Lmd_Professions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Bans.h"
#include "Lmd_EntityCore.h"

level_locals_t	level;

int eventClearTime = 0;
static int navCalcPathTime = 0;
extern int fatalErrors;

int killPlayerTimer = 0;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
	qboolean teamShader;        // track and if changed, update shader state
	//RoboPhred
	char *description;
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

qboolean gDuelExit = qfalse;

vmCvar_t	g_jediVmerc;

vmCvar_t	g_gametype;
vmCvar_t	g_MaxHolocronCarry;
vmCvar_t	g_ff_objectives;
vmCvar_t	g_autoMapCycle;
vmCvar_t	g_dmflags;
vmCvar_t	g_maxForceRank;
vmCvar_t	g_forceBasedTeams;
vmCvar_t	g_privateDuel;

//Lugormod cvars:
vmCvar_t        g_noVoteTime;
vmCvar_t        g_autoRandomSpots;
vmCvar_t        g_scorePlums;
vmCvar_t        g_uptime;
vmCvar_t        Lugormod_Version;
vmCvar_t        g_maxForceLevel;
//vmCvar_t        g_creditsInBank;
vmCvar_t        g_checkSkin;
vmCvar_t        g_gameMode;
vmCvar_t        g_chickenTime;
vmCvar_t        g_fixShields;
//vmCvar_t        g_regForceRank;
vmCvar_t        g_allowBlackNames;
vmCvar_t        g_profanityFile;
vmCvar_t        g_fixForce;
vmCvar_t        g_nakenPassword;
vmCvar_t        g_nakenAddress;
vmCvar_t        g_nakenRoom;
vmCvar_t        g_grapplingHook;
vmCvar_t        g_disableSpec;
vmCvar_t        g_cmdLvlFile;
vmCvar_t        g_dontLoadNPC;
vmCvar_t        g_tmpBanTime;
vmCvar_t        g_kingTime;
vmCvar_t        g_motdDispTime;
vmCvar_t        g_disableBail;
vmCvar_t        g_voteFix;
vmCvar_t        g_maxVoteCount;
vmCvar_t        g_enterMotd;
vmCvar_t        g_cmdDisable;
vmCvar_t        g_nameForServer;
vmCvar_t        g_duelForcePowerDisable;
vmCvar_t        g_meditateProtect;
vmCvar_t        g_meditateExtraForce;
/*
vmCvar_t        g_levelOnePwd;
vmCvar_t        g_levelTwoPwd;
vmCvar_t        g_levelThreePwd;
vmCvar_t        g_levelFourPwd;
*/
vmCvar_t        g_pickupDisable;
vmCvar_t        g_jmkillhealth;
vmCvar_t        g_jmforcelevel;
vmCvar_t        g_jmstarthealth;
vmCvar_t        g_jmsaberDamageScale;
//vmCvar_t        g_jmmaxdistance;
//vmCvar_t        g_jmsaberdistance;
vmCvar_t        g_jmsaberreplace;
vmCvar_t        g_jmhealthbar;
//End Lugormod cvars.
vmCvar_t	g_allowNPC;

vmCvar_t	g_armBreakage;

vmCvar_t	g_saberLocking;
vmCvar_t	g_saberLockFactor;
vmCvar_t	g_saberTraceSaberFirst;

vmCvar_t	d_saberKickTweak;

vmCvar_t	d_powerDuelPrint;

vmCvar_t	d_saberGhoul2Collision;
vmCvar_t	g_saberBladeFaces;
vmCvar_t	d_saberAlwaysBoxTrace;
vmCvar_t	d_saberBoxTraceSize;

vmCvar_t	d_siegeSeekerNPC;

vmCvar_t	g_debugMelee;
vmCvar_t	g_stepSlideFix;

vmCvar_t	g_noSpecMove;

#ifdef _DEBUG
vmCvar_t	g_disableServerG2;
#endif

vmCvar_t	d_perPlayerGhoul2;

vmCvar_t	d_projectileGhoul2Collision;

vmCvar_t	g_g2TraceLod;

vmCvar_t	g_optvehtrace;

vmCvar_t	g_locationBasedDamage;

vmCvar_t	g_allowHighPingDuelist;

vmCvar_t	g_logClientInfo;

vmCvar_t	g_slowmoDuelEnd;

vmCvar_t	g_saberDamageScale;

vmCvar_t	g_useWhileThrowing;

vmCvar_t	g_RMG;

vmCvar_t	g_svfps;

vmCvar_t	g_forceRegenTime;
vmCvar_t	g_spawnInvulnerability;
vmCvar_t	g_forcePowerDisable;
vmCvar_t	g_weaponDisable;
vmCvar_t	g_duelWeaponDisable;
vmCvar_t	g_allowDuelSuicide;
vmCvar_t	g_fraglimitVoteCorrection;
vmCvar_t	g_fraglimit;
vmCvar_t	g_duel_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	d_saberInterpolate;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_friendlySaber;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_developer;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_siegeRespawn;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
#ifndef FINAL_BUILD
vmCvar_t	g_debugDamage;
#endif
vmCvar_t	g_debugAlloc;
vmCvar_t	g_debugServerSkel;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_adaptRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_log;
vmCvar_t	g_logSync;
vmCvar_t	g_statLog;
vmCvar_t	g_statLogFile;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_debugForward;
vmCvar_t	g_debugRight;
vmCvar_t	g_debugUp;
vmCvar_t	g_smoothClients;

#include "../namespace_begin.h"
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
#include "../namespace_end.h"

vmCvar_t	g_listEntity;
//vmCvar_t	g_redteam;
//vmCvar_t	g_blueteam;
vmCvar_t	g_singlePlayer;
vmCvar_t	g_enableBreath;
vmCvar_t	g_dismember;
vmCvar_t	g_forceDodge;
vmCvar_t	g_timeouttospec;

vmCvar_t	g_saberDmgVelocityScale;
vmCvar_t	g_saberDmgDelay_Idle;
vmCvar_t	g_saberDmgDelay_Wound;

vmCvar_t	g_saberDebugPrint;

vmCvar_t	g_siegeTeamSwitch;

vmCvar_t	bg_fighterAltControl;

#ifdef DEBUG_SABER_BOX
vmCvar_t	g_saberDebugBox;
#endif

//NPC nav debug
vmCvar_t	d_altRoutes;
vmCvar_t	d_patched;

vmCvar_t		g_saberRealisticCombat;
vmCvar_t		g_saberRestrictForce;
vmCvar_t		d_saberSPStyleDamage;
vmCvar_t		g_debugSaberLocks;
vmCvar_t		g_saberLockRandomNess;
// nmckenzie: SABER_DAMAGE_WALLS
vmCvar_t		g_saberWallDamageScale;

vmCvar_t		d_saberStanceDebug;
// ai debug cvars
vmCvar_t		debugNPCAI;			// used to print out debug info about the bot AI
vmCvar_t		debugNPCFreeze;		// set to disable bot ai and temporarily freeze them in place
vmCvar_t		debugNPCAimingBeam;
vmCvar_t		debugBreak;
vmCvar_t		debugNoRoam;
vmCvar_t		d_saberCombat;
vmCvar_t		d_JediAI;
vmCvar_t		d_noGroupAI;
vmCvar_t		d_asynchronousGroupAI;
vmCvar_t		d_slowmodeath;
vmCvar_t		d_noIntermissionWait;

vmCvar_t		g_spskill;

vmCvar_t		g_siegeTeam1;
vmCvar_t		g_siegeTeam2;

vmCvar_t	g_austrian;

vmCvar_t	g_powerDuelStartHealth;
vmCvar_t	g_powerDuelEndHealth;

// nmckenzie: temporary way to show player healths in duels - some iface gfx in game would be better, of course.
// DUEL_HEALTH
vmCvar_t		g_showDuelHealths;

//RoboPhred:
vmCvar_t lmd_DataPath;
vmCvar_t lmd_stashrate;
vmCvar_t lmd_stashdepotime;
vmCvar_t lmd_stashcr;
vmCvar_t lmd_startingcr;
vmCvar_t lmd_enforceentwait;
vmCvar_t lmd_spturrets;
vmCvar_t lmd_maxsameip;
vmCvar_t lmd_autobansameip;
vmCvar_t lmd_admingodlevel;
vmCvar_t lmd_admincheatlevel;
vmCvar_t lmd_saveallplaced;

//these are not used right now.
vmCvar_t lmd_adminloglevels;
vmCvar_t lmd_admindefaultlogfile;

vmCvar_t lmd_chatDisable;
vmCvar_t lmd_chatDisableUseSay;
vmCvar_t lmd_chatPrimary;
vmCvar_t lmd_chatSecondary;
vmCvar_t lmd_banFile;
vmCvar_t lmd_vehcloaking;

vmCvar_t lmd_accBaseDays;
vmCvar_t lmd_accLevelDays;
vmCvar_t lmd_accMaxDays;

vmCvar_t lmd_accLevelDiscount_time;
vmCvar_t lmd_accLevelDiscount_maxTime;

vmCvar_t lmd_logArchive;

vmCvar_t lmd_closeChatRadius;
vmCvar_t lmd_closeChatLOS;

vmCvar_t lmd_loginsecurity;

vmCvar_t bot_enableChat;

vmCvar_t g_startingWeapons;
vmCvar_t g_startingAmmo;

vmCvar_t lmd_penaltyAddTime;
vmCvar_t lmd_penaltyRemoveTime;
vmCvar_t lmd_penaltyJailCount;
vmCvar_t lmd_penaltyJailTime;
vmCvar_t lmd_penaltyTmpbanCount;
vmCvar_t lmd_penaltyTmpbanTime;

vmCvar_t lmd_allowEmptyHostname;

//vmCvar_t lmd_enableUnsafeCvars;

vmCvar_t lmd_enableCorpseDrag;

vmCvar_t lmd_rewardcr_kill;

//RoboPhred: track this and force it to off
vmCvar_t sv_allowdownload;

//Need to keep track of this for checking if someone tries to change it.
vmCvar_t gamename;

// bk001129 - made static to avoid aliasing
static cvarTable_t		gameCvarTable[] = {
	//RoboPhred
	{ &lmd_maxsameip, "lmd_maxSameIp", "3", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Maximum number of users that can have the same ip.  Once this limit is reached for a certain ip, all new "
		"connections from the ip will be ignored.  Usefull for stopping fake player attacks."
	},
	{ &lmd_autobansameip, "lmd_autoBanSameIp", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If lmd_maxsameip is set, then this cvar controls whether to ban ips that go over the given limit."
	},
	{ &lmd_DataPath, "lmd_datapath", "default", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"The data path that lugormod will use for its accounts, entity sets, and other files."
	},
	{ &lmd_startingcr, "lmd_startingCr", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The number of credits a newly registered player starts with",
	},
	{ &lmd_stashrate, "lmd_stashRate", "60000", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"The time in miliseconds to wait between checking to spawn a stash.  This time is offset by a random value, and may not spawn a stash each check."
	},
	{ &lmd_stashdepotime, "lmd_stashDepoTime", "15000", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"The time in miliseconds needed to deposit a stash."
	},
	{ &lmd_stashcr, "lmd_stashCr", "10", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"The default number of credits in a stash.  This is only used in the new lmd_stash* entity set, "
		"and only if no credits value is already given."
	},
	{ &lmd_admingodlevel, "lmd_adminGodLevel", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue, qfalse,
		"The minimum admin level required to be able to use the God profession.  Set this to 0 to disable it fully.  "
		"Only admins this level and lower will be able to view and select this profession.  If an admin has this profession and this value is "
		"lowered below the admin's level, they will NOT loose the profession."
		"This only applies to automatically created level based admin.  Authfiles must specify it manually via an authflag."
	},

	{ &lmd_admincheatlevel, "lmd_adminCheatLevel", "3", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"The minimum admin level required to be able to use cheats.  Set this to 0 to disable it.  "
		"This only applies to automatically created level based admin.  Authfiles must specify it manually via an authflag."
	},

	{ &lmd_saveallplaced, "lmd_saveAllPlaced", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Allow all entities created with the place command to save.  This is for those of you who got too used to "
		"the bug which caused this to happen in the first place.  By default, only entities created by admins with the "
		"\"save placed\" auth flag or the level 1 auto authfile will save"
	},

	{ &lmd_enforceentwait, "lmd_enforceEntWait", "600", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"Minimum time in miliseconds to force certain entities to have a delay.  This works on func_doors and trigger_multiples.  "
		"Use this if the map you are playing on has troublesome doors that constantly open and close when the use button is tapped."
	},
	{ &lmd_spturrets, "lmd_spTurrets", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"This reverses spawnflags 4 and 8 for the misc_turretG2 entity.  "
		"Only turn this on if you are playing on a single player map that has turrets."
	},

	{ &lmd_adminloglevels, "lmd_adminLogLevels", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"Weather or not to log commands for the default admin levels.\n"
		"Set this to 1 to log all levels to seperate files, or 2 to log them all to the "
		"file set by lmd_admindefaultlogfile"
	},

	{ &lmd_admindefaultlogfile, "lmd_adminDefaultLogFile", "all", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"Admins who have no other logfile set will log to this by default."
		"Logs appear under <data folder>/logs/admin/*.log"
	},

	{ &lmd_chatDisable, "lmd_chatDisable", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Bitmask to disable certain chat types.\n"
		"1: Normal\n"
		"2: Team\n"
		"4: Tell\n"
		"8: Admin\n"
		"16: Close\n"
		"32: Buddies\n"
		"64: Friends"
	},

	{ &lmd_chatDisableUseSay, "lmd_chatDisableUseSay", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If a chat mode is disabled with lmd_chatdisable, send is as normal chat rather than blocking it.\n"
		"Does not work for tell."
	},

	{ &lmd_chatPrimary, "lmd_chatPrimary", "1", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Set the default chat mode for say.  If this mode is disabled, the mod will seek the next enabled mode.\n"
		"Values:\n"
		"0: Say\n"
		"1: Team\n"
		"2: Admins\n"
		"3: Close\n"
		"4: Buddies\n"
		"5: Friends"
	},

	{ &lmd_chatSecondary, "lmd_chatSecondary", "4", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Set the default chat mode for team say.  If this mode is disabled, the mod will seek the next enabled mode.\n"
		"Values:\n"
		"0: Say\n"
		"1: Team\n"
		"2: Admins\n"
		"3: Close\n"
		"4: Buddies\n"
		"5: Friends"
	},

	{ &lmd_banFile, "lmd_banFile", "bans.txt", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The file to store bans to."
	},

	{ &lmd_vehcloaking, "lmd_vehcloaking", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Allow the cloak item to be used when in a vehicle.\n"
		"To activate, a player must use the hotkey to directly use the cloak item."
	},

	{ &lmd_accBaseDays, "lmd_accBaseDays", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Number of days to keep an account between user logins."
	},

	{ &lmd_accLevelDays, "lmd_accLevelDays", "7", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Level modifier for days to keep an account between user logins.\n"
		"Days added is account level multiplied by this value."
	},

	{ &lmd_accLevelDiscount_time, "lmd_accLevelDiscount_time", "300", CVAR_ARCHIVE, 0, qfalse, qfalse, //60 * 5
		"Time in seconds of active server play per 1 CR discount to the cost of a new level, starting at their last level up.\n"
		"Set to 0 for no discount",
	},

	{ &lmd_accLevelDiscount_maxTime, "lmd_accLevelDiscount_maxTime", "432000", CVAR_ARCHIVE, 0, qfalse, qfalse, //60 * 60 * 24 * 5
		"Time in seconds from their last level up before a player stops receiving a time based discount to their next level up.\n"
		"Set to 0 for no maximum limit.",
	},

	{ &lmd_accMaxDays, "lmd_accMaxDays", "70", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Maximum number of days to keep an account between user logins."
	},

	{ &lmd_logArchive, "lmd_logArchive", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"Create a new log file every day.  Records them under \"logs/<date>.log\"\n"
		"Only checks for new days on map changes."
	},

	{ &lmd_closeChatRadius, "lmd_closeChatRadius", "512", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Ajust the radius for the close chat mode."
	},

	{ &lmd_closeChatLOS, "lmd_closeChatLOS", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If set to 1, close chat is only hearable by those directly visible to the person chatting."
	},

	{ &lmd_loginsecurity, "lmd_loginSecurity", "2", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If set to 0, users will not be required to use a security code, but can if they choose.\n"
		"If set to 1, users must enter their security code if they are logging in with a new ip.\n"
		"If set to 2, only admins are required to use a security code.\n"
		"It is highly recommended you set this to 1 or 2."
	},

	{ &bot_enableChat, "bot_enableChat", "1", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Enable bot chat."
	},

	{ &g_startingWeapons, "g_startingWeapons", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Players will get these weapons in addition to their normal set."
	},

	{ &g_startingAmmo, "g_startingAmmo", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The percent of each weapon's maximum ammo to load the starting weapons with.  This only affects weapons listed in g_startingWeapons."
	},

	{ &lmd_penaltyAddTime, "lmd_penaltyAddTime", "20", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The minimum time in seconds between penalizing a player."
	},

	{ &lmd_penaltyRemoveTime, "lmd_penaltyRemoveTime", "60", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The time in seconds to wait after a penalty or a previous removal to remove one penalty."
	},

	{ &lmd_penaltyJailCount, "lmd_penaltyJailCount", "3", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"How many penalties needed to jail a player.  Set to 0 to never jail on penalties."
	},

	{ &lmd_penaltyJailTime, "lmd_penaltyJailTime", "5", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"When jailing  a player due to penalties, how many minutes to jail them for."
	},

	{ &lmd_penaltyTmpbanCount, "lmd_penaltyTmpbanCount", "6", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"How many penalties needed to temporarily ban a player.  Set to 0 to never jail on penalties."
	},

	{ &lmd_penaltyTmpbanTime, "lmd_penaltyTmpbanTime", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"When banning a player due to penalties, how many minutes to ban them for.  A value of 0 will use the value in g_tmpBanTime."
	},

	{ &lmd_allowEmptyHostname, "lmd_allowEmptyHostname", "1", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Prevent a player from joining if the attempt to identify their hostname fails."
	},

	/*
	{ &lmd_enableUnsafeCvars, "lmd_enableUnsafeCvars", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Allow the use of sv_allowDownload.  This is NOT recommended, as sv_allowDownload may be used to steal files off the server such as the server config.",
	},
	*/

	{ &lmd_enableCorpseDrag, "lmd_enableCorpseDrag", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Experimental.  Enable dragging corpses with the use key.",
	},

	{ &lmd_rewardcr_kill, "lmd_rewardcr_kill", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Give a player credits for killing other players.  Does not work for killing NPCs.",
	},

	//====================================================================================================
	//====================================================================================================

	{ &sv_allowdownload, "sv_allowDownload", "0", CVAR_ARCHIVE, 0, qtrue  }, //Ufo: we want it

	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	{ &g_debugMelee, "g_debugMelee", "0", CVAR_SERVERINFO, 0, qtrue  },
	{ &g_stepSlideFix, "g_stepSlideFix", "1", CVAR_SERVERINFO, 0, qtrue  },

	{ &g_noSpecMove, "g_noSpecMove", "0", CVAR_SERVERINFO, 0, qtrue },

	// noset vars
	{ &g_uptime, "serveruptime", "0", CVAR_ROM|CVAR_SERVERINFO,0,qfalse},
#ifndef LMD_VER_ENC_LOCK
	{ &Lugormod_Version, "Lugormod_Version", LUGORMODVERSION, CVAR_ROM|CVAR_SERVERINFO,0,qfalse},
	{ &gamename, "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
#endif
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_MaxHolocronCarry, "g_MaxHolocronCarry", "3", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	{ &g_jediVmerc, "g_jediVmerc", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qtrue },

	// change anytime vars

	{ &g_ff_objectives, "g_ff_objectives", "0", /*CVAR_SERVERINFO |*/ CVAR_CHEAT | CVAR_NORESTART, 0, qtrue },

	{ &g_autoMapCycle, "g_autoMapCycle", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_maxForceRank, "g_maxForceRank", "6", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_forceBasedTeams, "g_forceBasedTeams", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_privateDuel, "g_privateDuel", "289", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_allowNPC, "g_allowNPC", "1", CVAR_SERVERINFO | CVAR_CHEAT, 0, qtrue  },

	{ &g_armBreakage, "g_armBreakage", "0", 0, 0, qtrue  },

	{ &g_saberLocking, "g_saberLocking", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberLockFactor, "g_saberLockFactor", "2", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberTraceSaberFirst, "g_saberTraceSaberFirst", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &d_saberKickTweak, "d_saberKickTweak", "1", 0, 0, qtrue  },

	{ &d_powerDuelPrint, "d_powerDuelPrint", "0", 0, qtrue },

	{ &d_saberGhoul2Collision, "d_saberGhoul2Collision", "1", CVAR_CHEAT, 0, qtrue  },
	{ &g_saberBladeFaces, "g_saberBladeFaces", "1", 0, 0, qtrue  },

	{ &d_saberAlwaysBoxTrace, "d_saberAlwaysBoxTrace", "0", CVAR_CHEAT, 0, qtrue  },
	{ &d_saberBoxTraceSize, "d_saberBoxTraceSize", "0", CVAR_CHEAT, 0, qtrue  },

	{ &d_siegeSeekerNPC, "d_siegeSeekerNPC", "0", CVAR_CHEAT, 0, qtrue },

#ifdef _DEBUG
	{ &g_disableServerG2, "g_disableServerG2", "0", 0, 0, qtrue },
#endif

	{ &d_perPlayerGhoul2, "d_perPlayerGhoul2", "0", CVAR_CHEAT, 0, qtrue },

	{ &d_projectileGhoul2Collision, "d_projectileGhoul2Collision", "1", CVAR_CHEAT, 0, qtrue  },

	{ &g_g2TraceLod, "g_g2TraceLod", "3", 0, 0, qtrue  },

	{ &g_optvehtrace, "com_optvehtrace", "0", 0, 0, qtrue  },

	{ &g_locationBasedDamage, "g_locationBasedDamage", "1", 0, 0, qtrue },

	{ &g_allowHighPingDuelist, "g_allowHighPingDuelist", "1", 0, 0, qtrue },

	{ &g_logClientInfo, "g_logClientInfo", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_slowmoDuelEnd, "g_slowmoDuelEnd", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_saberDamageScale, "g_saberDamageScale", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_useWhileThrowing, "g_useWhileThrowing", "1", 0, 0, qtrue  },

	{ &g_RMG, "RMG", "0", 0, 0, qtrue  },

	{ &g_svfps, "sv_fps", "20", 0, 0, qtrue },

	{ &g_forceRegenTime, "g_forceRegenTime", "200", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

	{ &g_spawnInvulnerability, "g_spawnInvulnerability", "3000", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_forcePowerDisable, "g_forcePowerDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_weaponDisable, "g_weaponDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_duelWeaponDisable, "g_duelWeaponDisable", "1", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },

	{ &g_allowDuelSuicide, "g_allowDuelSuicide", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimitVoteCorrection, "g_fraglimitVoteCorrection", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_duel_fraglimit, "duel_fraglimit", "10", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &d_saberInterpolate, "d_saberInterpolate", "0", CVAR_CHEAT, 0, qtrue },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlySaber, "g_friendlySaber", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", 0, 0, qtrue  },
	{ &g_log, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_statLog, "g_statLog", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_statLogFile, "g_statLogFile", "statlog.log", CVAR_ARCHIVE, 0, qfalse },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_developer, "developer", "0", 0, 0, qfalse },

	{ &g_speed, "g_speed", "250", 0, 0, qfalse  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qfalse  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "5", 0, 0, qtrue },
	{ &g_adaptRespawn, "g_adaptrespawn", "1", 0, 0, qtrue  },		// Make weapons respawn faster with a lot of players.
	{ &g_forcerespawn, "g_forcerespawn", "60", 0, 0, qtrue },		// One minute force respawn.  Give a player enough time to reallocate force.
	{ &g_siegeRespawn, "g_siegeRespawn", "20", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue }, //siege respawn wave time
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
#ifndef FINAL_BUILD
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
#endif
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_debugServerSkel, "g_debugServerSkel", "0", CVAR_CHEAT, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

#if 0
	{ &g_debugForward, "g_debugForward", "0", 0, 0, qfalse },
	{ &g_debugRight, "g_debugRight", "0", 0, 0, qfalse },
	{ &g_debugUp, "g_debugUp", "0", 0, 0, qfalse },
#endif

	//	{ &g_redteam, "g_redteam", "Empire", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue },
	//	{ &g_blueteam, "g_blueteam", "Rebellion", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue  },
	{ &g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse, qfalse  },

	{ &g_enableBreath, "g_enableBreath", "0", 0, 0, qtrue, qfalse },
	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

	{ &g_dismember, "g_dismember", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_forceDodge, "g_forceDodge", "1", 0, 0, qtrue  },

	{ &g_timeouttospec, "g_timeouttospec", "70", CVAR_ARCHIVE, 0, qfalse },

	{ &g_saberDmgVelocityScale, "g_saberDmgVelocityScale", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Idle, "g_saberDmgDelay_Idle", "350", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Wound, "g_saberDmgDelay_Wound", "0", CVAR_ARCHIVE, 0, qtrue  },

#ifndef FINAL_BUILD
	{ &g_saberDebugPrint, "g_saberDebugPrint", "0", CVAR_CHEAT, 0, qfalse  },
#endif
	{ &g_debugSaberLocks, "g_debugSaberLocks", "0", CVAR_CHEAT, 0, qfalse },
	{ &g_saberLockRandomNess, "g_saberLockRandomNess", "2", CVAR_CHEAT, 0, qfalse },
	// nmckenzie: SABER_DAMAGE_WALLS
	{ &g_saberWallDamageScale, "g_saberWallDamageScale", "0.4", CVAR_SERVERINFO, 0, qfalse },

	{ &d_saberStanceDebug, "d_saberStanceDebug", "0", 0, 0, qfalse },

	{ &g_siegeTeamSwitch, "g_siegeTeamSwitch", "1", CVAR_SERVERINFO|CVAR_ARCHIVE, qfalse },

	{ &bg_fighterAltControl, "bg_fighterAltControl", "0", CVAR_SERVERINFO, 0, qtrue },

#ifdef DEBUG_SABER_BOX
	{ &g_saberDebugBox, "g_saberDebugBox", "0", CVAR_CHEAT, 0, qfalse },
#endif

	{ &d_altRoutes, "d_altRoutes", "0", CVAR_CHEAT, 0, qfalse },
	{ &d_patched, "d_patched", "0", CVAR_CHEAT, 0, qfalse },

	{ &g_saberRealisticCombat, "g_saberRealisticCombat", "0", CVAR_ARCHIVE },
	{ &g_saberRestrictForce, "g_saberRestrictForce", "0", CVAR_CHEAT },
	{ &d_saberSPStyleDamage, "d_saberSPStyleDamage", "1", CVAR_CHEAT },

	{ &debugNoRoam, "d_noroam", "0", CVAR_CHEAT },
	{ &debugNPCAimingBeam, "d_npcaiming", "0", CVAR_CHEAT },
	{ &debugBreak, "d_break", "0", CVAR_CHEAT },
	{ &debugNPCAI, "d_npcai", "0", CVAR_CHEAT },
	{ &debugNPCFreeze, "d_npcfreeze", "0", CVAR_CHEAT },
	{ &d_JediAI, "d_JediAI", "0", CVAR_CHEAT },
	{ &d_noGroupAI, "d_noGroupAI", "0", CVAR_CHEAT },
	{ &d_asynchronousGroupAI, "d_asynchronousGroupAI", "0", CVAR_CHEAT },

	//0 = never (BORING)
	//1 = kyle only
	//2 = kyle and last enemy jedi
	//3 = kyle and any enemy jedi
	//4 = kyle and last enemy in a group
	//5 = kyle and any enemy
	//6 = also when kyle takes pain or enemy jedi dodges player saber swing or does an acrobatic evasion

	{ &d_slowmodeath, "d_slowmodeath", "0", CVAR_CHEAT },

	{ &d_saberCombat, "d_saberCombat", "0", CVAR_CHEAT },

	{ &g_spskill, "g_npcspskill", "0", CVAR_ARCHIVE | CVAR_INTERNAL },

	//for overriding the level defaults
	{ &g_siegeTeam1, "g_siegeTeam1", "none", CVAR_ARCHIVE|CVAR_SERVERINFO, 0, qfalse  },
	{ &g_siegeTeam2, "g_siegeTeam2", "none", CVAR_ARCHIVE|CVAR_SERVERINFO, 0, qfalse  },

	//mainly for debugging with bots while I'm not around (want the server to
	//cycle through levels naturally)
	{ &d_noIntermissionWait, "d_noIntermissionWait", "0", CVAR_CHEAT, 0, qfalse  },

	{ &g_austrian, "g_austrian", "0", CVAR_ARCHIVE, 0, qfalse  },
	// nmckenzie:
	// DUEL_HEALTH
	{ &g_showDuelHealths, "g_showDuelHealths", "0", CVAR_SERVERINFO },
	{ &g_powerDuelStartHealth, "g_powerDuelStartHealth", "150", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_powerDuelEndHealth, "g_powerDuelEndHealth", "90", CVAR_ARCHIVE, 0, qtrue  },

	// Lugormod cvars:
	{ &g_noVoteTime, "g_noVoteTime", "5", CVAR_ARCHIVE,0, qfalse, qfalse,
		"Number of minutes to wait after a map change before votes are allowed again.  This does not apply to admins level 3 or lower."
	},
	{ &g_autoRandomSpots, "g_autoRandomSpots", "1", CVAR_ARCHIVE|CVAR_LATCH,0, qfalse, qfalse,
		"When certain lugormod specfic game modes are turned on, some items get disabled.  "
		"This will convert those disabled items into random_spots temporarily.\n"
		"Due to recent entity system changes, this currently does not affect the g_weaponDisable cvar."
	},
	{ &g_scorePlums, "g_scorePlums", "0", CVAR_ARCHIVE,0,qfalse, qfalse,
		"When a player gets a score change, everyone will see a small colored number float up from their head with the number of points gained/lossed."
	},
	{ &g_maxForceLevel, "g_maxForceLevel", "5", CVAR_INIT,0,qfalse, qfalse,
		"This is the maximum force level that players can get.  Changing this may hamper the jedi profession."
	},
	{ &g_checkSkin, "g_checkSkin", "1", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Stop players from using skin setups that might result in invisible or untextured players."
	},
	{ &g_gameMode, "g_gameMode", "0", CVAR_ARCHIVE|CVAR_LATCH|CVAR_SERVERINFO, 0, qfalse, qfalse,
		"Settings for the lugormod-specific game modifiers.\n"
		"TODO: describe game modes and values."
	},
	{ &g_chickenTime, "g_chickenTime", "600", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If set, a player will loose points if they do not respond to a duel challenge by another player within this many seconds.  "
		"This does not apply if you are challenged by a bot.  Set to 0 to disable this rule."
	},
	{ &g_fixShields, "g_fixShields", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If no lugormod game mode is set, then the shield item will have 500 health and will regenerate 20 points per second."
	},
	{ &g_allowBlackNames, "g_allowBlackNames", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Allow players to use the color black in their names."
	},
	{ &g_profanityFile, "g_profanityFile", "", CVAR_INIT, 0, qfalse, qfalse,
		"Specifies the config file to load data for the profanity filter."
	},
	{ &g_fixForce, "g_fixForce", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Activates changes/improvements for certain forcepowers.\n"
		"TODO: list values and changes."
	},
	{ &g_nakenAddress, "g_nakenAddress", "", CVAR_INIT, 0, qfalse, qfalse,
		"The ip or hostname of a naken chat server.  If this is set, the game will attempt to join the specified server and "
		"send all player chat to it.\n"
		"WARNING: may cause the server to freeze."
	},
	{ &g_nakenPassword, "g_nakenPassword", "", CVAR_INTERNAL|CVAR_INIT, 0, qfalse, qfalse,
		"If g_nakenAddress is set, this is the password to use when joining the naken server."
	},
	{ &g_nakenRoom, "g_nakenRoom", "ja", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If g_nakenAddress is set, this is the chat room to join."
	},
	{ &g_grapplingHook, "g_grapplingHook", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"If this is set to 1, then the stun baton alt fire becomes a grappling hook.\n"
		"If this is set to 2, then the stun baton will always fire the hook no matter what attack is used."
	},

	{ &g_disableSpec, "g_disableSpec", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"A bitmask containing different options for spectators.\n"
		"1: Spectators will not be able to move.\n"
		"4: Spectators will be unable to follow players.\n"
		"This does not affect admins level 2 or lower."
	},
	{ &g_cmdLvlFile, "g_cmdLvlFile", "", CVAR_INIT, 0, qfalse, qfalse,
		"The lugormod config file to load admin command level information from."
	},
	{ &g_dontLoadNPC, "g_dontLoadNPC", "1", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"This cvar has the following effects when enabled:\n"
		"Disables the fx_rain entity.\n"
		"Disables loading of npc navigation waypoints.\n"
		"Disables point_combat entities.\n"
		"Allows doors with \'toggle\' and \'start locked\' spawnflags to still be used after they are unlocked.\n"
		"Makes the \'misc_security_panel\' entity toggle mover states between locked and unlocked.\n"
		"Makes one-time use \'target_scriptrunner\' entities become infinite use with a 60 second wait between triggerings.\n"
		"Enables spawnflags 1 (player only) and 8 (npc only) for the \'trigger_push\' entity.\n"
		"Forces spawnflag 2048 (multiple) on the \'trigger_push\' entity.\n"
		"Reverses the effect of spawnflag 16 (relative) on the \'trigger_push\' entity.\n"
		"Changes the spawnflag \'push_constant\' from 2 to 32 on the \'trigger_push\' entity.\n"
		"Forces trigger_hurt to always toggle on/off when used.\n"
		"Forces spawnflag 1024 and changes the model of the \'emplaced_gun\' entity."
	},
	{ &g_tmpBanTime, "g_tmpBanTime", "20", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Number of minutes that a temporary ban lasts."
	},
	{ &g_kingTime, "g_kingTime", "600", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Number of seconds the \'duel king\' has to find another opponent to duel before they loose king status."
	},
	{ &g_motdDispTime, "g_motdDispTime", "5", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"Number of seconds to display the motd to a newly joined player."
	},
	{ &g_disableBail, "g_disableBail", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Disable players bailing out of a flying vehicle when it is still airborn."
	},
	{ &g_voteFix, "g_voteFix", "0", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"If the yes votes are greater than the no votes on a vote completion, then consiter the vote passed.\n"
		"This does not affect player kick votes."
	},
	{ &g_maxVoteCount, "g_maxVoteCount", "3", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"The maximum number of votes a player can make.  This does not affect admins."
	},
	{ &g_enterMotd, "g_enterMotd", "", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The message a player will see when they join the game."
	},
	{ &g_cmdDisable, "g_cmdDisable", "0", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"A bitmask for disabling certain commands or feature sets.\n"
		"TODO: document what this does."
	},
	{ &g_nameForServer, "g_nameForServer", "server", CVAR_ARCHIVE, 0, qfalse, qfalse,
		"The name to use when sending chat with \'/rcon say\'."
	},
	{ &g_duelForcePowerDisable, "g_duelForcePowerDisable", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue, qfalse,
		"Bitmask of forcepowers to disable when in the duel or power duel gametype."
	},
	{ &g_pickupDisable, "g_pickupDisable", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse, qfalse,
		"Bitmask of items to disable.\n"
		"TODO: document possible values."
	},
	{ &g_meditateProtect, "g_meditateProtect", "0",CVAR_ARCHIVE, 0, qtrue, qfalse,
		"Number of miliseconds to wait before granting invincibility to someone using the meditation taunt.  Disabled if 0."
	},
	{ &g_meditateExtraForce, "g_meditateExtraForce", "100", CVAR_ARCHIVE, 0, qtrue, qfalse,
		"If a player uses the meditation taunt, then increase their max forcepower by this amount for the duration of the taunt.\n"
		"DANGER: Can severely unbalance the game!  It is highly recommended you leave this as-is to avoid placing "
		"non force using professions at a severe disadvantage."
	},
	{ &g_jmsaberreplace, "g_jmsaberreplace", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_jmhealthbar, "g_jmhealthbar", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_jmkillhealth, "g_jmkillhealth", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_jmforcelevel, "g_jmforcelevel", "3", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_jmstarthealth, "g_jmstarthealth", "100", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_jmsaberDamageScale, "g_jmsaberDamageScale", "2",CVAR_ARCHIVE, 0, qtrue  },

	//end Lugormod cvars.
	//,
};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );

//RoboPhred
#ifdef LMD_EXPERIMENTAL
void GenerateCvarDocs() {
	fileHandle_t cvars;
	int i;
	char *buf;
	trap_FS_FOpenFile("docs/cvars.txt", &cvars, FS_WRITE);
	for(i = 0;i<gameCvarTableSize;i++) {
		if(gameCvarTable[i].description == NULL)
			continue;
		if(gameCvarTable[i].cvarFlags & CVAR_ROM || gameCvarTable[i].cvarFlags & CVAR_CHEAT || gameCvarTable[i].cvarFlags & CVAR_INTERNAL)
			continue;
		buf = va("[b][u]%s[/u][/b]\n", gameCvarTable[i].cvarName);
		trap_FS_Write(buf, strlen(buf), cvars);
		if(gameCvarTable[i].defaultString){
			buf = va("[b]Default: %s[/b]\n", gameCvarTable[i].defaultString);
			trap_FS_Write(buf, strlen(buf), cvars);
		}
		if(gameCvarTable[i].cvarFlags & CVAR_LATCH) {
			buf = "[b]Restart required[/b]\n";
			trap_FS_Write(buf, strlen(buf), cvars);
		}
		if(gameCvarTable[i].description) {
			buf = va("%s\n", gameCvarTable[i].description);
			trap_FS_Write(buf, strlen(buf), cvars);
		}
		trap_FS_Write("\n", 1, cvars);
	}
	trap_FS_FCloseFile(cvars);
}

#endif

void G_InitGame					( int levelTime, int randomSeed, int restart );
void G_RunFrame					( int levelTime );
void G_ShutdownGame				( int restart );
void CheckExitRules				( void );
void G_ROFF_NotetrackCallback	( gentity_t *cent, const char *notetrack);

int trap_RealTime( qtime_t *qtime );

extern stringID_table_t setTable[];

qboolean G_ParseSpawnVars( qboolean inSubBSP );
//RoboPhred
gentity_t* G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );
//void G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );

qboolean NAV_ClearPathToPoint( gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEntNum );
qboolean NPC_ClearLOS2( gentity_t *ent, const vec3_t end );
int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask);
qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum );
qboolean G_EntIsUnlockedDoor( int entityNum );
qboolean G_EntIsDoor( int entityNum );
qboolean G_EntIsBreakable( int entityNum );
qboolean G_EntIsRemovableUsable( int entNum );
void CP_FindCombatPointWaypoints( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#include "../namespace_begin.h"

//int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  );
//#pragma export list vmMain

//RoboPhred
//Ufo: discarded
#ifdef _PATCHER
void JKG_PatchEngine();
void JKG_UnpatchEngine();
#endif

#ifdef LMD_EXPERIMENTAL
void ActivateCrashHandler();
void DeactivateCrashHandler();
#else
void EnableStackTrace();
void DisableStackTrace();
#endif

#ifdef __cplusplus
//#ifdef __linux__
extern "C" {
#endif

int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
		switch ( command ) {
	case GAME_INIT:
#ifdef LMD_EXPERIMENTAL
		ActivateCrashHandler();
#else
		EnableStackTrace();
#endif
//Ufo: discarded
#ifdef _PATCHER
		JKG_PatchEngine();
#endif
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );

#ifdef LMD_MEMORY_DEBUG
	_CrtDumpMemoryLeaks();
#endif
//Ufo: discarded
#ifdef _PATCHER
		JKG_UnpatchEngine();
#endif
#ifdef LMD_EXPERIMENTAL
		DeactivateCrashHandler();
#else
		DisableStackTrace();
#endif
		return 0;
	case GAME_CLIENT_CONNECT:
		return (int)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0, NULL );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
#ifdef LMD_MEMORY_DEBUG
		assert(_CrtCheckMemory());
#endif
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	case GAME_ROFF_NOTETRACK_CALLBACK:
		G_ROFF_NotetrackCallback( &g_entities[arg0], (const char *)arg1 );
		return 0;
	case GAME_SPAWN_RMG_ENTITY:
		if (G_ParseSpawnVars(qfalse))
		{
			G_SpawnGEntityFromSpawnVars(qfalse);
		}
		return 0;

		//rww - begin icarus callbacks
	case GAME_ICARUS_PLAYSOUND:
		{
			T_G_ICARUS_PLAYSOUND *sharedMem = (T_G_ICARUS_PLAYSOUND *)gSharedBuffer;
			return Q3_PlaySound(sharedMem->taskID, sharedMem->entID, sharedMem->name, sharedMem->channel);
		}
	case GAME_ICARUS_SET:
		{
			T_G_ICARUS_SET *sharedMem = (T_G_ICARUS_SET *)gSharedBuffer;
			return Q3_Set(sharedMem->taskID, sharedMem->entID, sharedMem->type_name, sharedMem->data);
		}
	case GAME_ICARUS_LERP2POS:
		{
			T_G_ICARUS_LERP2POS *sharedMem = (T_G_ICARUS_LERP2POS *)gSharedBuffer;
			if (sharedMem->nullAngles)
			{
				Q3_Lerp2Pos(sharedMem->taskID, sharedMem->entID, sharedMem->origin, NULL, sharedMem->duration);
			}
			else
			{
				Q3_Lerp2Pos(sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->angles, sharedMem->duration);
			}
		}
		return 0;
	case GAME_ICARUS_LERP2ORIGIN:
		{
			T_G_ICARUS_LERP2ORIGIN *sharedMem = (T_G_ICARUS_LERP2ORIGIN *)gSharedBuffer;
			Q3_Lerp2Origin(sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->duration);
		}
		return 0;
	case GAME_ICARUS_LERP2ANGLES:
		{
			T_G_ICARUS_LERP2ANGLES *sharedMem = (T_G_ICARUS_LERP2ANGLES *)gSharedBuffer;
			Q3_Lerp2Angles(sharedMem->taskID, sharedMem->entID, sharedMem->angles, sharedMem->duration);
		}
		return 0;
	case GAME_ICARUS_GETTAG:
		{
			T_G_ICARUS_GETTAG *sharedMem = (T_G_ICARUS_GETTAG *)gSharedBuffer;
			return Q3_GetTag(sharedMem->entID, sharedMem->name, sharedMem->lookup, sharedMem->info);
		}
	case GAME_ICARUS_LERP2START:
		{
			T_G_ICARUS_LERP2START *sharedMem = (T_G_ICARUS_LERP2START *)gSharedBuffer;
			Q3_Lerp2Start(sharedMem->entID, sharedMem->taskID, sharedMem->duration);
		}
		return 0;
	case GAME_ICARUS_LERP2END:
		{
			T_G_ICARUS_LERP2END *sharedMem = (T_G_ICARUS_LERP2END *)gSharedBuffer;
			Q3_Lerp2End(sharedMem->entID, sharedMem->taskID, sharedMem->duration);
		}
		return 0;
	case GAME_ICARUS_USE:
		{
			T_G_ICARUS_USE *sharedMem = (T_G_ICARUS_USE *)gSharedBuffer;
			Q3_Use(sharedMem->entID, sharedMem->target);
		}
		return 0;
	case GAME_ICARUS_KILL:
		{
			T_G_ICARUS_KILL *sharedMem = (T_G_ICARUS_KILL *)gSharedBuffer;
			Q3_Kill(sharedMem->entID, sharedMem->name);
		}
		return 0;
	case GAME_ICARUS_REMOVE:
		{
			T_G_ICARUS_REMOVE *sharedMem = (T_G_ICARUS_REMOVE *)gSharedBuffer;
			Q3_Remove(sharedMem->entID, sharedMem->name);
		}
		return 0;
	case GAME_ICARUS_PLAY:
		{
			T_G_ICARUS_PLAY *sharedMem = (T_G_ICARUS_PLAY *)gSharedBuffer;
			Q3_Play(sharedMem->taskID, sharedMem->entID, sharedMem->type, sharedMem->name);
		}
		return 0;
	case GAME_ICARUS_GETFLOAT:
		{
			T_G_ICARUS_GETFLOAT *sharedMem = (T_G_ICARUS_GETFLOAT *)gSharedBuffer;
			return Q3_GetFloat(sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value);
		}
	case GAME_ICARUS_GETVECTOR:
		{
			T_G_ICARUS_GETVECTOR *sharedMem = (T_G_ICARUS_GETVECTOR *)gSharedBuffer;
			return Q3_GetVector(sharedMem->entID, sharedMem->type, sharedMem->name, sharedMem->value);
		}
	case GAME_ICARUS_GETSTRING:
		{
			T_G_ICARUS_GETSTRING *sharedMem = (T_G_ICARUS_GETSTRING *)gSharedBuffer;
			int r;
			char *crap = NULL; //I am sorry for this -rww
			char **morecrap = &crap; //and this
			r = Q3_GetString(sharedMem->entID, sharedMem->type, sharedMem->name, morecrap);

			if (crap)
			{ //success!
				strcpy(sharedMem->value, crap);
			}

			return r;
		}
	case GAME_ICARUS_SOUNDINDEX:
		{
			T_G_ICARUS_SOUNDINDEX *sharedMem = (T_G_ICARUS_SOUNDINDEX *)gSharedBuffer;
			G_SoundIndex(sharedMem->filename);
		}
		return 0;
	case GAME_ICARUS_GETSETIDFORSTRING:
		{
			T_G_ICARUS_GETSETIDFORSTRING *sharedMem = (T_G_ICARUS_GETSETIDFORSTRING *)gSharedBuffer;
			return GetIDForString(setTable, sharedMem->string);
		}
		//rww - end icarus callbacks

	case GAME_NAV_CLEARPATHTOPOINT:
		return NAV_ClearPathToPoint(&g_entities[arg0], (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5);
	case GAME_NAV_CLEARLOS:
		return NPC_ClearLOS2(&g_entities[arg0], (const float *)arg1);
	case GAME_NAV_CLEARPATHBETWEENPOINTS:
		return NAVNEW_ClearPathBetweenPoints((float *)arg0, (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5);
	case GAME_NAV_CHECKNODEFAILEDFORENT:
		return NAV_CheckNodeFailedForEnt(&g_entities[arg0], arg1);
	case GAME_NAV_ENTISUNLOCKEDDOOR:
		return G_EntIsUnlockedDoor(arg0);
	case GAME_NAV_ENTISDOOR:
		return G_EntIsDoor(arg0);
	case GAME_NAV_ENTISBREAKABLE:
		return G_EntIsBreakable(arg0);
	case GAME_NAV_ENTISREMOVABLEUSABLE:
		return G_EntIsRemovableUsable(arg0);
	case GAME_NAV_FINDCOMBATPOINTWAYPOINTS:
		CP_FindCombatPointWaypoints();
		return 0;
	case GAME_GETITEMINDEXBYTAG:
		return BG_GetItemIndexByTag(arg0, arg1);
		}

		return -1;
	}
#ifdef __cplusplus
	//#ifdef __linux__
}
#endif
#include "../namespace_end.h"

//Lugormod
//Ufo: discarded
#ifdef _NAKEN
//void naken_send (/*const char *name,*/ const char *msg);
void naken_queue(const char *msg);
#endif
//end Lugormod

void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end (argptr);
	//Lugormod
	//Ufo: discarded
#ifdef _NAKEN
	if (g_nakenAddress.string[0]) {
		if (g_nakenPassword.string[0]
		&& Q_strncmp(text, "info: ", 6) == 0) {
			naken_queue(va("%%%s",text + 6));
		} else if (Q_strncmp(text, "say: ", 5) == 0) {
			naken_queue(va("%%%s", text + 5));
		} else if (g_nakenPassword.string[0]
		&& Q_strncmp(text, "tell: ", 6) == 0) {
			naken_queue(va("%%%s", text + 6));
		} else if (g_nakenPassword.string[0]
		&& Q_strncmp(text, "sayteam: ", 9) == 0) {
			naken_queue(va("%%%s", text + 9));
		} else if (g_nakenPassword.string[0]
		&& Q_strncmp(text, "ERROR: ", 7) == 0) {
			naken_queue(va("%%%s", text + 7));
		}
	}
#endif
	//end Lugormod
	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	//RoboPhred: Trigger crash log so we log the stack trace and restart
	char *empty = NULL;
	empty[0] = 0;

#if 0
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	//RoboPhred: the trap prevents the message from showing up, placed Com_Printf in front of trap (was behind)
	Com_Printf("ERROR: %s\n",text);
	trap_Error( text ); //I think this hangs
	//make it cash
	//gentity_t *ent = NULL;
	//ent->classname = "";
	//assert(0);
#endif
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		if (e->r.contents==CONTENTS_TRIGGER)
			continue;//triggers NEVER link up in teams!
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				//RoboPhred
				if(e2->targetname && !e2->isAutoTargeted){
				//if ( e2->targetname ) {
					//RoboPhred
					G_Free(e->targetname);
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	//	G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders( void ) {
#if 0
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f);
	AddRemap("textures/ctf2/redteam02", string, f);
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f);
	AddRemap("textures/ctf2/blueteam02", string, f);
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
#endif
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	//RoboPhred
#ifdef LMD_VER_ENC_LOCK
	char cvar[MAX_STRING_CHARS];
	char value[MAX_STRING_CHARS];
	//=========================================================
	//Gamename
	int p = 0;
	int len = strlen(ENC_GAMENAME_CVAR);
	for(i = 0;i<len;i += 3)
	{
		cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
	}
	for(i = 2;i<len;i += 3)
	{
		cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
	}
	for(i = 1;i<len;i += 3)
	{
		cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
	}
	cvar[p] = 0;
	//========
	//Gamename value
	p = 0;
	len = strlen(ENC_LUGORMOD);
	for(i = 0;i<len;i += 3)
	{
		value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
	}
	for(i = 2;i<len;i += 3)
	{
		value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
	}
	for(i = 1;i<len;i += 3)
	{
		value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
	}
	value[p] = 0;
	//=========================================================

	trap_Cvar_Register(&gamename, cvar, value, CVAR_ROM|CVAR_SERVERINFO);

	//=========================================================
	//Lugormod_version

	p = 0;
	len = strlen(ENC_LUGORMODVERSION_CVAR);
	for(i = 0;i<len;i += 3)
		cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
	for(i = 2;i<len;i += 3)
		cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
	for(i = 1;i<len;i += 3)
		cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
	cvar[p] = 0;
	//========
	//Lugormod_version value
	p = 0;
	len = strlen(LUGORMODVERSION_CORE);
	for(i = 0;i<len;i += 3)
		value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
	for(i = 2;i<len;i += 3)
		value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
	for(i = 1;i<len;i += 3)
		value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
	value[p] = 0;
	Q_strcat(value, sizeof(value), va("%i.%i", verMajor, verMinor));
	if(verRev || verBuild) {
		Q_strcat(value, sizeof(value), va(".%i", verRev));
		if(verBuild)
			Q_strcat(value, sizeof(value), va(".%i", verBuild));
	}
	len = p = strlen(value);
#ifdef LUGORMODVERSION_ATTACH
	int offset;
	value[p++] = ' ';
	p = 0;
	offset = len + 1;
	len = strlen(LUGORMODVERSION_ATTACH);
	for(i = 0;i<len;i += 3)
		value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
	for(i = 2;i<len;i += 3)
		value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
	for(i = 1;i<len;i += 3)
		value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
	value[p + offset] = 0;
#endif
	if(verMods[0]) {
		Q_strcat(value, sizeof(value), " ");
		Q_strcat(value, sizeof(value), verMods);
	}
	//=========================================================
	trap_Cvar_Register(&Lugormod_Version, cvar, value, CVAR_ROM|CVAR_SERVERINFO);

#endif

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
	}
	/*else if (g_gametype.integer == GT_HOLOCRON)
	{
	G_Printf( "This gametype is not supported.\n" );
	trap_Cvar_Set( "g_gametype", "0" );
	}*/
	/* else if (g_gametype.integer == GT_JEDIMASTER)
	{
	G_Printf( "This gametype is not supported.\n" );
	trap_Cvar_Set( "g_gametype", "0" );
	} */
	else if (g_gametype.integer == GT_SABER_RUN)
	{
		G_Printf( "This gametype is not supported.\n" );
		trap_Cvar_Set( "g_gametype", "0" );
	}
	/*
	else if (g_gametype.integer == GT_CTY)
	{
	G_Printf( "This gametype is not supported.\n" );
	trap_Cvar_Set( "g_gametype", "0" );
	}
	*/

	level.warmupModificationCount = g_warmup.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"",
						cv->cvarName, cv->vmCvar->string ) );
				}

				if (cv->teamShader) {
					remapped = qtrue;
				}
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}
}

char gSharedBuffer[MAX_G_SHARED_BUFFER_SIZE];

#include "../namespace_begin.h"
void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );
#include "../namespace_end.h"

//Lugormod
//RoboPhred: in Lmd_Main now
//void parseCmdLevelsFile ();
//int loadAccounts(void);
//void loadLingoFilter (void);

/*
============
G_InitGame

============
*/
extern void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace);
extern void JMSaberThink(gentity_t *ent);

extern void RemoveAllWP(void);
extern void BG_ClearVehicleParseParms(void);
extern void SP_info_jedimaster_start(gentity_t *ent);
//Lugormod
extern unsigned char model_frames [MAX_MODELS];
char *enterMotd;
void SP_misc_holocron(gentity_t *ent);
gentity_t* pick_random_spot (void); //Lugormod
int count_random_spots (void); //Lugormod
void LinkBGSpawnPoints(void); //Lugormod

//RoboPhred
void Lmd_Startup(void);
qboolean AllForceDisabled(int force);
void InitializeSpawnTable();

void G_SiegeRegisterWeaponsAndHoldables(int team); //Lugormod GT_BATTLE_GROUND
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;
	vmCvar_t	mapname;
	vmCvar_t	ckSum;

#ifdef _XBOX
	if(restart) {
		BG_ClearVehicleParseParms();
		RemoveAllWP();
	}
#endif

	G_InitMemory();

	//RoboPhred:
	if(!lmd_DataPath.string[0] || lmd_DataPath.string[0] == '\\' || lmd_DataPath.string[0] == '/'){
		trap_Cvar_Set("lmd_DataPath", "default");
	}

	InitializeSpawnTable();

	G_Printf("Setting level globals...\n");

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	level.snd_hack = G_SoundIndex("sound/player/hacking.wav");
	level.snd_medHealed = G_SoundIndex("sound/player/supp_healed.wav");
	level.snd_medSupplied = G_SoundIndex("sound/player/supp_supplied.wav");

	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	Q_strncpyz(level.rawmapname, mapname.string, sizeof(level.rawmapname));
	trap_Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

	//RoboPhred: moved here from ClientSpawn (was stupid being there)
	if(g_gametype.integer == GT_HOLOCRON || g_gametype.integer == GT_REBORN || g_gametype.integer == GT_JEDIMASTER
		|| HasSetSaberOnly() || AllForceDisabled( g_forcePowerDisable.integer ))
	{
		trap_Cvar_Set( "g_jediVmerc", "0" );
	}

	//Init RMG to 0, it will be autoset to 1 if there is terrain on the level.
	trap_Cvar_Set("RMG", "0");
	g_RMG.integer = 0;

	//Clean up any client-server ghoul2 instance attachments that may still exist exe-side
	trap_G2API_CleanEntAttachments();

	BG_InitAnimsets(); //clear it out

	B_InitAlloc(); //make sure everything is clean

	trap_SV_RegisterSharedMemory(gSharedBuffer);

	//Load external vehicle data
	BG_VehicleLoadParms();

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);

	srand( randomSeed );

	G_Printf("Registering cvars...\n");

	G_RegisterCvars();

	//trap_SP_RegisterServer("mp_svgame");
#ifndef _XBOX
	//RoboPhred: in Lmd_main.c now
	/*
	//Lugormod parse cmd level defs
	parseCmdLevelsFile();
	//Lugormod load profanity filter
	loadLingoFilter();
	*/

	G_Printf("Preparing logs...\n");

	if ( g_log.string[0] ) {
		//lmd_logArchive
		char *log = NULL;
		if(lmd_logArchive.integer > 0) {
			qtime_t time;
			trap_RealTime(&time);
			char cvar[MAX_STRING_CHARS];
			Q_strncpyz(cvar, g_log.string, sizeof(cvar));
			char *ext = strrchr(cvar, '.');
			if(ext) {
				ext[0] = 0;
				ext++;
			}
			if(!ext || !ext[0])
				ext = "log";
			log = G_NewString2(va("logs/%s_%i-%i-%i.%s", cvar, time.tm_mon + 1, time.tm_mday, time.tm_year + 1900, ext));
		}
		else {
			log = g_log.string;
		}

		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( log, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( log, &level.logFile, FS_APPEND );
		}

		if(lmd_logArchive.integer > 0) {
			G_Free(log);
		}

		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		}
		else {
			char serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}
#endif

	G_Printf("Preparing weapon logs...\n");

	G_LogWeaponInit();

	G_Printf("Preparing session data...\n");

	G_InitWorldSession();

	G_Printf("Preparing entities...\n");

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	G_Printf("Preparing clients...\n");

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	G_Printf("Setting up server entities...\n");

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	G_Printf("Loading saber data...\n");

	//Load sabers.cfg data
	WP_SaberLoadParms();

	G_Printf("Init npcs...\n");

	NPC_InitGame();

	TIMER_Clear();

	G_Printf("Init icarus...\n");

	trap_ICARUS_Init();

	// reserve some spots for dead player bodies
	InitBodyQue();

	G_Printf("Preparing items...\n");

	ClearRegisteredItems();

	G_Printf("Init siege...\n");

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	InitSiegeMode();

	G_Printf("Loading npc nav... ");

	if ((g_gametype.integer == GT_FFA || g_gametype.integer == GT_TEAM) && g_dontLoadNPC.integer) {//Lugormod don't load waypoints
		navCalculatePaths = qfalse;
		G_Printf("not loading.\n");
	} else { //Lugormod ok load 'em
		navCalculatePaths	= ( trap_Nav_Load( mapname.string, ckSum.integer ) == qfalse );
		G_Printf("loading.\n");
	}

	memset(model_frames, 0, MAX_MODELS);

	G_Printf("Running LMD specific startup...\n");

	Lmd_Startup();

	G_Printf("Loading custom entity data...\n");

	switch (g_gametype.integer) {
		case GT_JEDIMASTER:
			LoadEntitiesData("jedimaster", qfalse);
			break;
		case GT_TEAM:
		case GT_CTF:
		case GT_CTY:
		case GT_SABER_RUN:
		case GT_REBORN:
		case GT_GHOST:
			LoadEntitiesData("team", qfalse);
			break;
		case GT_SIEGE:
			LoadEntitiesData("siege", qfalse);
			break; //Ufo: was missing
		case GT_BATTLE_GROUND:
			level.teamScores[TEAM_RED]  = 50;
			level.teamScores[TEAM_BLUE] = 50;
			LoadEntitiesData("battleground", qfalse);

			//Load the player class types

			//RoboPhred: this is done around 20 lines above here...
			//vmCvar_t		mapname;
			char			levelname[512];
			char			goalreq[64];
			char                    teams[2048];
			char			gParseObjectives[2048];
			char                    team1[512],team2[512];
			int				len;
			fileHandle_t	f;
			len = 0;

			//SiegeSetCompleteData(0);
			//RoboPhred: this is done around 20 lines above here...
			//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

			//RoboPhred
			Com_sprintf(levelname, sizeof(levelname), "maps/%s.bgd\0", level.rawmapname);
			//Com_sprintf(levelname, sizeof(levelname), "maps/%s.bgd\0", mapname.string);

			len = trap_FS_FOpenFile(levelname, &f, FS_READ);

			if (f && len < MAX_SIEGE_INFO_SIZE)
			{
				trap_FS_Read(siege_info, len, f);

				trap_FS_FCloseFile(f);

				if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
				{
					if (g_siegeTeam1.string[0] && Q_stricmp(g_siegeTeam1.string, "none"))
					{ //check for override
						strcpy(team1, g_siegeTeam1.string);
					}
					else
					{ //otherwise use level default
						BG_SiegeGetPairedValue(teams, "team1", team1);
					}

					if (g_siegeTeam2.string[0] && Q_stricmp(g_siegeTeam2.string, "none"))
					{ //check for override
						strcpy(team2, g_siegeTeam2.string);
					}
					else
					{ //otherwise use level default
						BG_SiegeGetPairedValue(teams, "team2", team2);
					}
				}
			}

			//Load the player class types
			BG_SiegeLoadClasses(NULL);
			if (!bgNumSiegeClasses)
			{ //We didn't find any?!
				G_Error("Couldn't find any player classes for Battle Ground");
			}
			//Now load the teams since we have class data.

			BG_SiegeLoadTeams();
			if (!bgNumSiegeTeams)
			{ //React same as with classes.
				G_Error("Couldn't find any player teams for Battle Ground");
			}

			//Get and set the team themes for each team. This will control which classes can be
			//used on each team.
			if (BG_SiegeGetValueGroup(siege_info, team1, gParseObjectives))
			{
				if (BG_SiegeGetPairedValue(gParseObjectives, "UseTeam", goalreq))
				{
					BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, goalreq);
				}
			} else {
				BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, "Siege2_Mercs");
			}

			if (BG_SiegeGetValueGroup(siege_info, team2, gParseObjectives))
			{
				if (BG_SiegeGetPairedValue(gParseObjectives, "UseTeam", goalreq))
				{
					BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, goalreq);
				}
			} else {
				BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, "Siege2_Rebels");
			}

			//---------------------------------------------------------
			//BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM1);
			//BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM2);
			//trap_Cvar_Set( "team1_icon", "gfx/2d/mp_imp_symbol_3");
			//trap_Cvar_Set( "team2_icon", "gfx/2d/mp_rebel_symbol_3");

			//G_SiegeRegisterWeaponsAndHoldables(SIEGETEAM_TEAM1);
			//G_SiegeRegisterWeaponsAndHoldables(SIEGETEAM_TEAM2);

			LinkBGSpawnPoints();
			break;
		default:
			LoadEntitiesData("default", qfalse);
			break;
	}

	//RoboPhred: done in LoadEntitiesData()
	/*
	disablesenabled = qtrue;
	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString(qfalse);
	disablesenabled = qfalse;
	*/

	if (gameMode(GMF_LOWGRAV)) {
		g_gravity.value = 200.0f;
	}

	//Lugormod parse entermotd

	enterMotd = G_NewString(g_enterMotd.string);

	G_Printf("Setting team data...\n");

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( g_gametype.integer >= GT_TEAM ) {
		G_CheckTeamItems();
	}
	else if ( g_gametype.integer == GT_JEDIMASTER )
	{
		trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, va("-1") );
	}

	if (g_gametype.integer == GT_POWERDUEL)
	{
		trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1|-1") );
	}
	else
	{
		trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1") );
	}
	// nmckenzie: DUEL_HEALTH: Default.
	trap_SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("-1|-1|!") );
	trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("-1") );

	G_Printf("Registering items...\n");

	SaveRegisteredItems();

	G_Printf("Completing game init...\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}
	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}
	G_RemapTeamShaders();

	if ( g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL )
	{
		G_LogPrintf("Duel Tournament Begun: kill limit %d, win limit: %d\n", g_fraglimit.integer, g_duel_fraglimit.integer );
	}

	if ( navCalculatePaths )
	{//not loaded - need to calc paths
		navCalcPathTime = level.time + START_TIME_NAV_CALC;//make sure all ents are in and linked
	}
	else
	{//loaded
		//FIXME: if this is from a loadgame, it needs to be sure to write this
		//out whenever you do a savegame since the edges and routes are dynamic...
		//OR: always do a navigator.CheckBlockedEdges() on map startup after nav-load/calc-paths
		//navigator.pathsCalculated = qtrue;//just to be safe?  Does this get saved out?  No... assumed
		trap_Nav_SetPathsCalculated(qtrue);
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;

		/*
		if ( g_eSavedGameJustLoaded == eNO )
		{//clear all the failed edges unless we just loaded the game (which would include failed edges)
		trap_Nav_ClearAllFailedEdges();
		}
		*/
		//No loading games in MP.
	}

	if (g_gametype.integer == GT_SIEGE
		//|| g_gametype.integer == GT_BATTLE_GROUND
		) //Lugormod
	{ //just get these configstrings registered now...
		int i = 0;
		while (i < MAX_CUSTOM_SIEGE_SOUNDS)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				break;
			}
			G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
			i++;
		}
	}
	if (g_gametype.integer == GT_JEDIMASTER && g_jmsaberreplace.integer
		&& !G_Find(NULL, FOFS(classname), "info_jedimaster_start")) {
			int antlitems = 0;
			gentity_t *itemlist[MAX_GENTITIES];
			gentity_t *ent;

			for (i = 0; i < level.num_entities; i++) {
				ent = &g_entities[i];
				if (!ent->inuse) {
					continue;
				}

				if ((ent->item &&
					((g_jmsaberreplace.integer * 2)&
					(1<<ent->item->giType))) ||
					Q_stricmp("random_spot", ent->classname) == 0){
						itemlist[antlitems++] = &g_entities[i];
				}
			}
			if (antlitems) {
				Rand_Init(level.time);
				i = Q_irand(0, antlitems - 1);
				ent = itemlist[i];
				SP_info_jedimaster_start(ent);
			}
	} else if (g_gametype.integer == GT_HOLOCRON) {
		Rand_Init(level.time);
		int numholos = count_random_spots();
		if (numholos > NUM_FORCE_POWERS - 2) {
			//FIXME: two are removed
			numholos = NUM_FORCE_POWERS - 2;
		}
		int i, j;
		gentity_t *holo;
		for (i = 0, j = 0; j < numholos;i++) {
			if (i == FP_TEAM_HEAL || i == FP_TEAM_FORCE) {
				continue;
			}
			j ++;

			if (!(holo = pick_random_spot())) {
				continue;
			}
			G_Free(holo->classname);
			holo->classname = "misc_holocron";
			holo->count = i;
			SP_misc_holocron(holo);
		}
	}
}

/*
=================
G_ShutdownGame
=================
*/

//Lugormod
//RoboPhred
void Lmd_Shutdown(void);
//void saveAccounts(qboolean full);
//Ufo: discarded
#ifdef _NAKEN
void naken_disconnect(void);
#endif
//endLugormod

void G_ShutdownGame( int restart ) {
	int i = 0;
	gentity_t *ent;

	//	G_Printf ("==== ShutdownGame ====\n");

	G_CleanAllFakeClients(); //get rid of dynamically allocated fake client structs.

	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

	//	Com_Printf("... Gameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{ //clean up all the ghoul2 instances
		ent = &g_entities[i];

		if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
		{
			trap_G2API_CleanGhoul2Models(&ent->ghoul2);
			ent->ghoul2 = NULL;
		}
		if (ent->client)
		{
			int j = 0;

			while (j < MAX_SABERS)
			{
				if (ent->client->weaponGhoul2[j] && trap_G2_HaveWeGhoul2Models(ent->client->weaponGhoul2[j]))
				{
					trap_G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[j]);
				}
				j++;
			}
		}
		i++;
	}
	if (g2SaberInstance && trap_G2_HaveWeGhoul2Models(g2SaberInstance))
	{
		trap_G2API_CleanGhoul2Models(&g2SaberInstance);
		g2SaberInstance = NULL;
	}
	if (precachedKyle && trap_G2_HaveWeGhoul2Models(precachedKyle))
	{
		trap_G2API_CleanGhoul2Models(&precachedKyle);
		precachedKyle = NULL;
	}

	//	Com_Printf ("... ICARUS_Shutdown\n");
	trap_ICARUS_Shutdown ();	//Shut ICARUS down

	//	Com_Printf ("... Reference Tags Cleared\n");
	TAG_Init();	//Clear the reference tags

	G_LogWeaponOutput();

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap_ROFF_Clean();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	//RoboPhred
	// This should be done after session data is written.
	Lmd_Shutdown();

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc
	//Lugormod disconnect from naken chat
//Ufo: discarded
#ifdef _NAKEN
	naken_disconnect();
#endif

	G_ShutdownMemory();
}

//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	G_Printf ("%s", text);
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	//	if ( level.intermissiontime ) {
	//		return;
	//	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if (!g_allowHighPingDuelist.integer && client->ps.ping >= 999)
		{ //don't add people who are lagging out if cvar is not set to allow it.
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
				continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

void G_PowerDuelCount(int *loners, int *doubles, qboolean countSpec)
{
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS)
	{
		cl = g_entities[i].client;

		if (g_entities[i].inuse && cl && (countSpec || cl->sess.sessionTeam != TEAM_SPECTATOR))
		{
			if (cl->sess.duelTeam == DUELTEAM_LONE)
			{
				(*loners)++;
			}
			else if (cl->sess.duelTeam == DUELTEAM_DOUBLE)
			{
				(*doubles)++;
			}
		}
		i++;
	}
}

qboolean g_duelAssigning = qfalse;
void AddPowerDuelPlayers( void )
{
	int			i;
	int			loners = 0;
	int			doubles = 0;
	int			nonspecLoners = 0;
	int			nonspecDoubles = 0;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 3 )
	{
		return;
	}

	nextInLine = NULL;

	G_PowerDuelCount(&nonspecLoners, &nonspecDoubles, qfalse);
	if (nonspecLoners >= 1 && nonspecDoubles >= 2)
	{ //we have enough people, stop
		return;
	}

	//Could be written faster, but it's not enough to care I suppose.
	G_PowerDuelCount(&loners, &doubles, qtrue);

	if (loners < 1 || doubles < 2)
	{ //don't bother trying to spawn anyone yet if the balance is not even set up between spectators
		return;
	}

	//Count again, with only in-game clients in mind.
	loners = nonspecLoners;
	doubles = nonspecDoubles;
	//	G_PowerDuelCount(&loners, &doubles, qfalse);

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_FREE)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
		{
			continue;
		}

		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
				continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );

	//Call recursively until everyone is in
	AddPowerDuelPlayers();
}

qboolean g_dontFrickinCheck = qfalse;

void RemovePowerDuelLosers(void)
{
	int remClients[3];
	int remNum = 0;
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS && remNum < 3)
	{
		//cl = &level.clients[level.sortedClients[i]];
		cl = &level.clients[i];

		if (cl->pers.connected == CON_CONNECTED)
		{
			if ((cl->ps.stats[STAT_HEALTH] <= 0 || cl->iAmALoser) &&
				(cl->sess.sessionTeam != TEAM_SPECTATOR || cl->iAmALoser))
			{ //he was dead or he was spectating as a loser
				remClients[remNum] = cl->ps.clientNum;
				remNum++;
			}
		}

		i++;
	}

	if (!remNum)
	{ //Time ran out or something? Oh well, just remove the main guy.
		remClients[remNum] = level.sortedClients[0];
		remNum++;
	}

	i = 0;
	while (i < remNum)
	{ //set them all to spectator
		SetTeam( &g_entities[ remClients[i] ], "s" );
		i++;
	}

	g_dontFrickinCheck = qfalse;

	//recalculate stuff now that we have reset teams.
	CalculateRanks();
}

void RemoveDuelDrawLoser(void)
{
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[ level.sortedClients[0] ].pers.connected != CON_CONNECTED )
	{
		return;
	}
	if ( level.clients[ level.sortedClients[1] ].pers.connected != CON_CONNECTED )
	{
		return;
	}

	clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
	clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		clFailure = 1;
	}
	else if (clSec > clFirst)
	{
		clFailure = 0;
	}
	else
	{
		clFailure = 2;
	}

	if (clFailure != 2)
	{
		SetTeam( &g_entities[ level.sortedClients[clFailure] ], "s" );
	}
	else
	{ //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[ level.sortedClients[1] ], "s" );
	}
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		int clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
		int clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if (clFirst > clSec)
		{
			clFailure = 1;
			clSuccess = 0;
		}
		else if (clSec > clFirst)
		{
			clFailure = 0;
			clSuccess = 1;
		}
		else
		{
			clFailure = 2;
			clSuccess = 2;
		}

		if (clFailure != 2)
		{
			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
		else
		{
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );

			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	if (g_gametype.integer == GT_POWERDUEL)
	{
		//sort single duelists first
		if (ca->sess.duelTeam == DUELTEAM_LONE && ca->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return -1;
		}
		if (cb->sess.duelTeam == DUELTEAM_LONE && cb->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return 1;
		}

		//others will be auto-sorted below but above spectators.
	}

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorTime < cb->sess.spectatorTime ) {
			return -1;
		}
		if ( ca->sess.spectatorTime > cb->sess.spectatorTime ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
	> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
	< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

//A new duel started so respawn everyone and make sure their stats are reset
qboolean G_CanResetDuelists(void)
{
	int i;
	gentity_t *ent;

	i = 0;
	while (i < 3)
	{ //precheck to make sure they are all respawnable
		ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse || !ent->client || ent->health <= 0 ||
			ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
			ent->client->sess.duelTeam <= DUELTEAM_FREE)
		{
			return qfalse;
		}
		i++;
	}

	return qtrue;
}

qboolean g_noPDuelCheck = qfalse;
void G_ResetDuelists(void)
{
	int i;
	gentity_t *ent;
	gentity_t *tent;

	i = 0;
	while (i < 3)
	{
		ent = &g_entities[level.sortedClients[i]];

		g_noPDuelCheck = qtrue;
		player_die(ent, ent, ent, 999, MOD_SUICIDE);
		g_noPDuelCheck = qfalse;
		trap_UnlinkEntity (ent);
		ClientSpawn(ent);

		// add a teleportation effect
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
		i++;
	}
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	int		preNumSpec = 0;
	//int		nonSpecIndex = -1;
	gclient_t	*cl;

	preNumSpec = level.numNonSpectatorClients;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots //Lugormod count bots
	for ( i = 0; i < TEAM_NUM_TEAMS; i++ ) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( (level.clients[i].mGameFlags&PSG_VOTED|PSG_TEAMVOTED) ||//Lugormod count them if they voted
				level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL )
			{
				if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR)
				{
					level.numNonSpectatorClients++;
					//nonSpecIndex = i;
				}

				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED )
				{
					if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.clients[i].iAmALoser)
					{
						level.numPlayingClients++;
					}
					//Lugormod my bots vote
					//if ( !(g_entities[i].r.svFlags & SVF_BOT) )
					//{
					level.numVotingClients++;
					if ( level.clients[i].sess.sessionTeam == TEAM_RED )
						level.numteamVotingClients[0]++;
					else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
						level.numteamVotingClients[1]++;
					//}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	//if (!g_warmup.integer)
	if (1)
	{
		level.warmupTime = 0;
	}

	/*
	if (level.numNonSpectatorClients == 2 && preNumSpec < 2 && nonSpecIndex != -1 && g_gametype.integer == GT_DUEL && !level.warmupTime)
	{
	gentity_t *currentWinner = G_GetDuelWinner(&level.clients[nonSpecIndex]);

	if (currentWinner && currentWinner->client)
	{
	trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
	currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), level.clients[nonSpecIndex].pers.netname));
	}
	}
	*/
	//NOTE: for now not doing this either. May use later if appropriate.

	qsort( level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( g_gametype.integer >= GT_TEAM ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}

		if (g_gametype.integer != GT_DUEL || g_gametype.integer != GT_POWERDUEL)
		{ //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 )
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0] ) );
			}
			else
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}

/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.loopIsSoundset = qfalse;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
extern qboolean	gSiegeRoundWinningTeam;
void FindIntermissionPoint( void ) {
	gentity_t	*ent = NULL;
	gentity_t	*target;
	vec3_t		dir;

	// find the intermission spot
	if ( (g_gametype.integer == GT_SIEGE
		|| g_gametype.integer == GT_BATTLE_GROUND)
		&& level.intermissiontime
		&& level.intermissiontime <= level.time
		&& gSiegeRoundEnded )
	{
		if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM1)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_red");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
		else if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM2)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_blue");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	}
	if ( !ent )
	{
		ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		//RoboPhred: Grab one manually, no need to call a spawn function.
		ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	//RoboPhred
	if(!ent) {
		//Still no entity, just use default.
		VectorCopy (vec3_origin, level.intermission_origin);
		VectorCopy (vec3_origin, level.intermission_angle);
	}
	else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}
}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL ) {
		trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

		if (g_gametype.integer != GT_POWERDUEL)
		{
			AdjustTournamentScores();
		}
		if (DuelLimitHit())
		{
			gDuelExit = qtrue;
		}
		else
		{
			gDuelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	/*
	if (g_singlePlayer.integer) {
	trap_Cvar_Set("ui_singlePlayerActive", "0");
	UpdateTournamentInfo();
	}
	*/
	//what the? Well, I don't want this to happen.

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			if (g_gametype.integer != GT_POWERDUEL ||
				!client->client ||
				client->client->sess.sessionTeam != TEAM_SPECTATOR)
			{ //don't respawn spectators in powerduel or it will mess the line order all up
				respawn(client);
			}
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

qboolean DuelLimitHit(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( g_duel_fraglimit.integer && cl->sess.wins >= g_duel_fraglimit.integer )
		{
			return qtrue;
		}
	}

	return qfalse;
}

void DuelResetWinsLosses(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
extern void SiegeDoTeamAssign(void); //g_saga.c
extern siegePers_t g_siegePersistant; //g_saga.c
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL ) {
		if (!DuelLimitHit())
		{
			if ( !level.restarted ) {
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;
		}

		DuelResetWinsLosses();
	}

	if (g_gametype.integer == GT_SIEGE &&
		g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime)
	{ //restart same map...
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
	else
	{
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}
	level.changemap = NULL;
	level.intermissiontime = 0;

	if (g_gametype.integer == GT_SIEGE &&
		g_siegeTeamSwitch.integer)
	{ //switch out now
		SiegeDoTeamAssign();
	}

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
int Q_vsnprintf( char* dest, int size, const char *fmt, va_list argptr );
void Lmd_LogfilePrintf( fileHandle_t f, qboolean timestamp, const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;
	int			l;
	qtime_t time;
	trap_RealTime(&time);

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof(string), "%i-%i-%i %i:%2i (%i:%i%i): ",
		time.tm_mon + 1, time.tm_mday + 1, time.tm_year + 1900, time.tm_hour, time.tm_min,
		min, tens, sec );

	l = strlen(string);

	va_start( argptr, fmt );
	Q_vsnprintf(string + l, sizeof( string ) - l, fmt, argptr );
	va_end( argptr );

	if ( !f ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), f );
}
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];

	va_start( argptr, fmt );
	Q_vsnprintf(string, sizeof( string ), fmt, argptr );
	va_end( argptr );
	if ( g_dedicated.integer ) {
		G_Printf( "%s", string);
	}
	Lmd_LogfilePrintf(level.logFile, qtrue, "%s", string);
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
	//	qboolean		won = qtrue;
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( g_gametype.integer >= GT_TEAM
		&& g_gametype.integer != GT_REBORN) {
			G_LogPrintf( "red:%i  blue:%i\n",
				level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],	cl->pers.netname );
		//		if (g_singlePlayer.integer && (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)) {
		//			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
		//				won = qfalse;
		//			}
		//		}
	}

	//yeah.. how about not.
	/*
	if (g_singlePlayer.integer) {
	if (g_gametype.integer >= GT_CTF) {
	won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
	}
	trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
	*/
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) {
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) )
	{
		gDidDuelStuff = qtrue;

		if ( g_austrian.integer && g_gametype.integer != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Results:\n");
			//G_LogPrintf("Duel Time: %d\n", level.time );
			G_LogPrintf("winner: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
			G_LogPrintf("loser: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses );
		}
		// if we are running a tournement map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (g_gametype.integer == GT_POWERDUEL)
			{
				RemovePowerDuelLosers();
				AddPowerDuelPlayers();
			}
			else
			{
				if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
					level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
					level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
					level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
				{
					RemoveDuelDrawLoser();
				}
				else
				{
					RemoveTournamentLoser();
				}
				AddTournamentPlayer();
			}

			if ( g_austrian.integer )
			{
				if (g_gametype.integer == GT_POWERDUEL)
				{
					G_LogPrintf("Power Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						level.clients[level.sortedClients[2]].pers.netname,
						level.clients[level.sortedClients[2]].sess.wins,
						level.clients[level.sortedClients[2]].sess.losses,
						g_fraglimit.integer );
				}
				else
				{
					G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						g_fraglimit.integer );
				}
			}

			if (g_gametype.integer == GT_POWERDUEL)
			{
				if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
				{
					trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
					trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}
			else
			{
				if (level.numPlayingClients >= 2)
				{
					trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
					trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}

			return;
		}

		if ( g_austrian.integer && g_gametype.integer != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Tournament Winner: %s wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
		}

		if (g_gametype.integer == GT_POWERDUEL)
		{
			RemovePowerDuelLosers();
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
		else
		{
			//this means we hit the duel limit so reset the wins/losses
			//but still push the loser to the back of the line, and retain the order for
			//the map change
			if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	if ((g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) && !gDuelExit)
	{ //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 )
		{ //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for (i=0 ; i< g_maxclients.integer ; i++)
		{ //being in a "ready" state is not necessary here, so clear it for everyone
			//yes, I also thinking holding this in a ps value uniquely for each player
			//is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	if (d_noIntermissionWait.integer)
	{ //don't care who wants to go, just go.
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
qboolean g_endPDuel = qfalse;
void CheckExitRules( void ) {
	int			i;
	gclient_t	*cl;
	char *sKillLimit;
	qboolean printLimit = qtrue;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if (gDoSlowMoDuel)
	{ //don't go to intermission while in slow motion
		return;
	}

	//if (g_gametype.integer == GT_GHOST) { //Lugormod not here in ghost
	//        return;
	//}

	if (gEscaping)
	{
		int i = 0;
		int numLiveClients = 0;

		while (i < MAX_CLIENTS)
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
				{
					numLiveClients++;
				}
			}

			i++;
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		//int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		int time = INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}
	//Lugormod Battle Ground
	if (g_gametype.integer == GT_BATTLE_GROUND) {
		if ( level.teamScores[TEAM_RED] <= 0 ) {
			trap_SendServerCommand( -1, "print \"Blue team won.\n\"");
			LogExit( "Blue team won" );
			return;
		}
		if ( level.teamScores[TEAM_BLUE] <= 0 ) {
			trap_SendServerCommand( -1, "print \"Red team won.\n\"");
			LogExit( "Red team won" );
			return;
		}
		return;
	}

	//end Lugormod
	/*
	if (g_gametype.integer == GT_POWERDUEL)
	{
	if (level.numPlayingClients < 3)
	{
	if (!level.intermissiontime)
	{
	if (d_powerDuelPrint.integer)
	{
	Com_Printf("POWERDUEL WIN CONDITION: Duel forfeit (1)\n");
	}
	LogExit("Duel forfeit.");
	return;
	}
	}
	}
	*/

	// check for sudden death
	if (g_gametype.integer != GT_SIEGE
		&& g_gametype.integer != GT_BATTLE_GROUND) //Lugormod
	{
		if ( ScoreIsTied() ) {
			// always wait for sudden death
			if ((g_gametype.integer != GT_DUEL) || !g_timelimit.integer)
			{
				if (g_gametype.integer != GT_POWERDUEL)
				{
					return;
				}
			}
		}
	}

	if (g_gametype.integer != GT_SIEGE
		&& g_gametype.integer != GT_BATTLE_GROUND) //Lugormod
	{
		if ( g_timelimit.integer && !level.warmupTime ) {
			if ( level.time - level.startTime >= g_timelimit.integer*60000 ) {
				//				trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				trap_SendServerCommand( -1, va("print \"%s.\n\"",G_GetStringEdString("MP_SVGAME", "TIMELIMIT_HIT")));
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Timelimit hit (1)\n");
				}
				LogExit( "Timelimit hit." );
				return;
			}
		}
	}

	if (g_gametype.integer == GT_POWERDUEL && level.numPlayingClients >= 3)
	{
		if (g_endPDuel)
		{
			g_endPDuel = qfalse;
			LogExit("Powerduel ended.");
		}

		//yeah, this stuff was completely insane.
		/*
		int duelists[3];
		duelists[0] = level.sortedClients[0];
		duelists[1] = level.sortedClients[1];
		duelists[2] = level.sortedClients[2];

		if (duelists[0] != -1 &&
		duelists[1] != -1 &&
		duelists[2] != -1)
		{
		if (!g_entities[duelists[0]].inuse ||
		!g_entities[duelists[0]].client ||
		g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] <= 0 ||
		g_entities[duelists[0]].client->sess.sessionTeam != TEAM_FREE)
		{ //The lone duelist lost, give the other two wins (if applicable) and him a loss
		if (g_entities[duelists[0]].inuse &&
		g_entities[duelists[0]].client)
		{
		g_entities[duelists[0]].client->sess.losses++;
		ClientUserinfoChanged(duelists[0]);
		}
		if (g_entities[duelists[1]].inuse &&
		g_entities[duelists[1]].client)
		{
		if (g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] > 0 &&
		g_entities[duelists[1]].client->sess.sessionTeam == TEAM_FREE)
		{
		g_entities[duelists[1]].client->sess.wins++;
		}
		else
		{
		g_entities[duelists[1]].client->sess.losses++;
		}
		ClientUserinfoChanged(duelists[1]);
		}
		if (g_entities[duelists[2]].inuse &&
		g_entities[duelists[2]].client)
		{
		if (g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] > 0 &&
		g_entities[duelists[2]].client->sess.sessionTeam == TEAM_FREE)
		{
		g_entities[duelists[2]].client->sess.wins++;
		}
		else
		{
		g_entities[duelists[2]].client->sess.losses++;
		}
		ClientUserinfoChanged(duelists[2]);
		}

		//Will want to parse indecies for two out at some point probably
		trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[1] ) );

		if (d_powerDuelPrint.integer)
		{
		Com_Printf("POWERDUEL WIN CONDITION: Coupled duelists won (1)\n");
		}
		LogExit( "Coupled duelists won." );
		gDuelExit = qfalse;
		}
		else if ((!g_entities[duelists[1]].inuse ||
		!g_entities[duelists[1]].client ||
		g_entities[duelists[1]].client->sess.sessionTeam != TEAM_FREE ||
		g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] <= 0) &&
		(!g_entities[duelists[2]].inuse ||
		!g_entities[duelists[2]].client ||
		g_entities[duelists[2]].client->sess.sessionTeam != TEAM_FREE ||
		g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] <= 0))
		{ //the coupled duelists lost, give the lone duelist a win (if applicable) and the couple both losses
		if (g_entities[duelists[1]].inuse &&
		g_entities[duelists[1]].client)
		{
		g_entities[duelists[1]].client->sess.losses++;
		ClientUserinfoChanged(duelists[1]);
		}
		if (g_entities[duelists[2]].inuse &&
		g_entities[duelists[2]].client)
		{
		g_entities[duelists[2]].client->sess.losses++;
		ClientUserinfoChanged(duelists[2]);
		}

		if (g_entities[duelists[0]].inuse &&
		g_entities[duelists[0]].client &&
		g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] > 0 &&
		g_entities[duelists[0]].client->sess.sessionTeam == TEAM_FREE)
		{
		g_entities[duelists[0]].client->sess.wins++;
		ClientUserinfoChanged(duelists[0]);
		}

		trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[0] ) );

		if (d_powerDuelPrint.integer)
		{
		Com_Printf("POWERDUEL WIN CONDITION: Lone duelist won (1)\n");
		}
		LogExit( "Lone duelist won." );
		gDuelExit = qfalse;
		}
		}
		*/
		return;
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if (g_gametype.integer == GT_DUEL ||
		g_gametype.integer == GT_POWERDUEL)
	{
		if (g_fraglimit.integer > 1)
		{
			sKillLimit = "Kill limit hit.";
		}
		else
		{
			sKillLimit = "";
			printLimit = qfalse;
		}
	}
	else
	{
		sKillLimit = "Kill limit hit.";
	}
	if ( g_gametype.integer < GT_SIEGE && g_fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, va("print \"Red %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (1)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, va("print \"Blue %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (2)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) && g_duel_fraglimit.integer && cl->sess.wins >= g_duel_fraglimit.integer )
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel limit hit (1)\n");
				}
				LogExit( "Duel limit hit." );
				gDuelExit = qtrue;
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname ) );
				return;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Kill limit (3)\n");
				}
				LogExit( sKillLimit );
				gDuelExit = qfalse;
				if (printLimit)
				{
					trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
						cl->pers.netname,
						G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")
						)
						);
				}
				return;
			}
		}
	}

	if (g_gametype.integer == GT_REBORN && g_fraglimit.integer) {
		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( sKillLimit );
				gDuelExit = qfalse;
				if (printLimit)
				{
					trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
						cl->pers.netname,
						G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")
						)
						);
				}
				return;
			}
		}
	} else if ( g_gametype.integer >= GT_CTF &&
		g_gametype.integer != GT_REBORN &&
		g_capturelimit.integer ) {
			if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer )
			{
				trap_SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")));
				trap_SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
				LogExit( "Capturelimit hit." );
				return;
			}

			if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
				trap_SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")));
				trap_SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
				LogExit( "Capturelimit hit." );
				return;
			}
	}
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

void G_RemoveDuelist(int team)
{
	int i = 0;
	gentity_t *ent;
	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->sess.duelTeam == team)
		{
			SetTeam(ent, "s");
		}
		i++;
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
int g_duelPrintTimer = 0;
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	//	if ( level.numPlayingClients == 0 && (g_gametype.integer != GT_POWERDUEL) ) {
	//		return;
	//	}

	if (g_gametype.integer == GT_POWERDUEL)
	{
		if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
		{
			trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
		}
	}
	else
	{
		if (level.numPlayingClients >= 2)
		{
			trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
		}
	}

	if ( g_gametype.integer == GT_DUEL )
	{
		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 && !level.intermissiontime && !level.intermissionQueued ) {
			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			}
		}

		if (level.numPlayingClients >= 2)
		{
			// nmckenzie: DUEL_HEALTH
			if ( g_showDuelHealths.integer >= 1 )
			{
				playerState_t *ps1, *ps2;
				ps1 = &level.clients[level.sortedClients[0]].ps;
				ps2 = &level.clients[level.sortedClients[1]].ps;
				trap_SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("%i|%i|!",
					ps1->stats[STAT_HEALTH], ps2->stats[STAT_HEALTH]));
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{ //don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
#endif
	}
	else if (g_gametype.integer == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 2)
		{ //hmm, ok, pull more in.
			g_dontFrickinCheck = qfalse;
		}

		if (level.numPlayingClients > 3)
		{ //umm..yes..lets take care of that then.
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone > 1)
			{
				G_RemoveDuelist(DUELTEAM_LONE);
			}
			else if (dbl > 2)
			{
				G_RemoveDuelist(DUELTEAM_DOUBLE);
			}
		}
		else if (level.numPlayingClients < 3)
		{ //hmm, someone disconnected or something and we need em
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
			else if (dbl < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
		}

		// pull in a spectator if needed
		if (level.numPlayingClients < 3 && !g_dontFrickinCheck)
		{
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 &&
				G_CanResetDuelists())
			{
				gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
				te->r.svFlags |= SVF_BROADCAST;
				//this is really pretty nasty, but..
				te->s.otherEntityNum = level.sortedClients[0];
				te->s.otherEntityNum2 = level.sortedClients[1];
				te->s.groundEntityNum = level.sortedClients[2];

				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				G_ResetDuelists();

				g_dontFrickinCheck = qtrue;
			}
			else if (level.numPlayingClients > 0 ||
				level.numConnectedClients > 0)
			{
				if (g_duelPrintTimer < level.time)
				{ //print once every 10 seconds
					int lone = 0, dbl = 0;

					G_PowerDuelCount(&lone, &dbl, qtrue);
					if (lone < 1)
					{
						trap_SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMORESINGLE")) );
					}
					else
					{
						trap_SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMOREPAIRED")) );
					}
					g_duelPrintTimer = level.time + 10000;
				}
			}

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{ //pulled in a needed person
				if (G_CanResetDuelists())
				{
					gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
					te->r.svFlags |= SVF_BROADCAST;
					//this is really pretty nasty, but..
					te->s.otherEntityNum = level.sortedClients[0];
					te->s.otherEntityNum2 = level.sortedClients[1];
					te->s.groundEntityNum = level.sortedClients[2];

					trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );

					if ( g_austrian.integer )
					{
						G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
							level.clients[level.sortedClients[0]].pers.netname,
							level.clients[level.sortedClients[0]].sess.wins,
							level.clients[level.sortedClients[0]].sess.losses,
							level.clients[level.sortedClients[1]].pers.netname,
							level.clients[level.sortedClients[1]].sess.wins,
							level.clients[level.sortedClients[1]].sess.losses,
							level.clients[level.sortedClients[2]].pers.netname,
							level.clients[level.sortedClients[2]].sess.wins,
							level.clients[level.sortedClients[2]].sess.losses,
							g_fraglimit.integer );
					}
					//trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
					//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
				}
			}
		}
		else
		{ //if you have proper num of players then don't try to add again
			g_dontFrickinCheck = qtrue;
		}

		level.warmupTime = 0;
		return;
	}
	else if ( level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( g_gametype.integer > GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		/*
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
		level.warmupModificationCount = g_warmup.modificationCount;
		level.warmupTime = -1;
		}
		*/

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

void G_KickAllBots(void)
{
	int i;
	char netname[36];
	gclient_t	*cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ )
	{
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) )
		{
			continue;
		}
		strcpy(netname, cl->pers.netname);
		Q_CleanStr(netname);
		trap_SendConsoleCommand( EXEC_INSERT, va("kick \"%s\"\n", netname) );
	}
}

/*
==================
CheckVote
==================
*/
int ClientNumberFromString( gentity_t *to, char *s );
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		//Lugormod temp ban'em
		if (Q_strncmp(level.voteString, "kick", 4) == 0) {
			int num = ClientNumberFromString(NULL, level.voteString + 5);
			Bans_AddIP(g_entities[num].client->sess.Lmd.ip, -1, "Temporarily banned: voted off server");
			trap_DropClient(num, "Temporarily banned, voted off server.");
		}
		if (Q_strncmp(level.voteString, "clientkick", 10) == 0) {
			int num = atoi(level.voteString + 11);
			Bans_AddIP(g_entities[num].client->sess.Lmd.ip, -1, "Temporarily banned: voted off server");
			trap_DropClient(num, "Temporarily banned, voted off server.");
		}

		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
		if (Q_strncmp(level.voteString, "g_gamemode ", 11) == 0) {
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart\n");
		}

		if (level.votingGametype)
		{
			if (trap_Cvar_VariableIntegerValue("g_gametype") != level.votingGametypeTo)
			{ //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap(level.votingGametypeTo, qtrue);

				if (level.votingGametypeTo == GT_SIEGE
					|| level.votingGametypeTo == GT_BATTLE_GROUND)
				{ //ok, kick all the bots, cause they aren't supported yet!
					G_KickAllBots();
					//just in case, set this to 0 too... I guess...maybe?
					//trap_Cvar_Set("bot_minplayers", "0");
				}

				if (nextMap && nextMap[0])
				{
					trap_SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
				}
			}
			else
			{ //otherwise, just leave the map until a restart
				G_RefreshNextMap(level.votingGametypeTo, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{ //This means to auto-correct fraglimit when voting to and from duel.
				const int currentGT = trap_Cvar_VariableIntegerValue("g_gametype");
				const int currentFL = trap_Cvar_VariableIntegerValue("fraglimit");
				const int currentTL = trap_Cvar_VariableIntegerValue("timelimit");

				if ((level.votingGametypeTo == GT_DUEL || level.votingGametypeTo == GT_POWERDUEL) && currentGT != GT_DUEL && currentGT != GT_POWERDUEL)
				{
					//Separate FL:s would be better
					if (currentFL > 3 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
					if (currentTL)
					{ //if voting to duel, and timelimit is set, make it unlimited
						trap_SendConsoleCommand(EXEC_APPEND, "timelimit 0\n");
					}
				}
				else if ((level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL) &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL))
				{
					if (currentFL && currentFL < 20)
					{ //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= VOTE_TIME ) {
		if (g_voteFix.integer && level.voteYes > level.voteNo && Q_strncmp(level.voteString, "kick", 4)
			&& Q_strncmp(level.voteString, "clientkick", 10)) {
				trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED")) );
				Com_Printf("info: %s vote passed\n",
					level.voteDisplayString);
				level.voteExecuteTime = level.time + 3000;
		} else {
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED")) );
			Com_Printf("info: %s vote failed\n",
				level.voteDisplayString);
			level.votingGametype = qfalse;
		}
	} else {
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED")) );
			Com_Printf("info: %s vote passed\n",
				level.voteDisplayString);
			level.voteExecuteTime = level.time + 3000;
		} else if ( level.voteNo >= level.numVotingClients/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED")) );
			Com_Printf("info: %s vote failed\n",
				level.voteDisplayString);
			level.votingGametype = qfalse;
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
}

/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s %s\n\"", level.clients[client].pers.netname, G_GetStringEdString("MP_SVGAME", "NEWTEAMLEADER")) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED")) );
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEPASSED")) );
			//
			if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				//set the team leader
				//SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED")) );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );
}

/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;
	//RoboPhred
	//static int lastDlMod = -1;
	static int lmdVer = -1;
	static int gamenameVer = -1;
	/*
	if(lmd_enableUnsafeCvars.integer == 0 && sv_allowdownload.modificationCount != lastDlMod) {
		trap_Cvar_Set("sv_allowDownload", "0");
		Com_Printf("^3sv_allowDownload forced to 0.  ^1Do NOT turn this on, it is commonly exploited.\n");
		lastDlMod = sv_allowdownload.modificationCount;
	}
	*/
	//LUGORMODVERSION_VAL_1
	if(Lugormod_Version.modificationCount != lmdVer || gamename.modificationCount != gamenameVer) {
#ifdef LMD_VER_ENC_LOCK
		char cvar[MAX_STRING_CHARS];
		char value[MAX_STRING_CHARS];
		int i;

		//=========================================================
		//Gamename
		int p = 0;
		int len = strlen(ENC_GAMENAME_CVAR);
		for(i = 0;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
		for(i = 2;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
		for(i = 1;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_GAMENAME_CVAR[p++];
		cvar[p] = 0;
		//========
		//Gamename value
		p = 0;
		len = strlen(ENC_LUGORMOD);
		for(i = 0;i<len;i += 3)
			value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
		for(i = 2;i<len;i += 3)
			value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
		for(i = 1;i<len;i += 3)
			value[i] = ENC_BASECHAR + ENC_LUGORMOD[p++];
		value[p] = 0;
		//=========================================================

		trap_Cvar_Set(cvar, value);

		//=========================================================
		//Lugormod_version
		p = 0;
		len = strlen(ENC_LUGORMODVERSION_CVAR);
		for(i = 0;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
		for(i = 2;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
		for(i = 1;i<len;i += 3)
			cvar[i] = ENC_BASECHAR + ENC_LUGORMODVERSION_CVAR[p++];
		cvar[p] = 0;
		//========
		//Lugormod_version value
		p = 0;
		len = strlen(LUGORMODVERSION_CORE);
		for(i = 0;i<len;i += 3)
			value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
		for(i = 2;i<len;i += 3)
			value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
		for(i = 1;i<len;i += 3)
			value[i] = ENC_BASECHAR + LUGORMODVERSION_CORE[p++];
		value[p] = 0;
		Q_strcat(value, sizeof(value), va("%i.%i", verMajor, verMinor));
		if(verRev || verBuild) {
			Q_strcat(value, sizeof(value), va(".%i", verRev));
			if(verBuild)
				Q_strcat(value, sizeof(value), va(".%i", verBuild));
		}
		len = p = strlen(value);
#ifdef LUGORMODVERSION_ATTACH
		int offset;
		value[p++] = ' ';
		p = 0;
		offset = len + 1;
		len = strlen(LUGORMODVERSION_ATTACH);
		for(i = 0;i<len;i += 3)
			value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
		for(i = 2;i<len;i += 3)
			value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
		for(i = 1;i<len;i += 3)
			value[i + offset] = ENC_BASECHAR + LUGORMODVERSION_ATTACH[p++];
		value[p + offset] = 0;
#endif
		if(verMods[0]) {
			Q_strcat(value, sizeof(value), " ");
			Q_strcat(value, sizeof(value), verMods);
		}
		//=========================================================
		trap_Cvar_Set(cvar, value);
#else
		trap_Cvar_Set("Lugormod_Version", LUGORMODVERSION);
		trap_Cvar_Set("gamename", GAMEVERSION);
#endif

		lmdVer = Lugormod_Version.modificationCount;
		gamenameVer = gamename.modificationCount;
	}

	if ( g_password.modificationCount != lastMod ) {
		char password[MAX_INFO_STRING];
		char *c = password;
		lastMod = g_password.modificationCount;

		strcpy( password, g_password.string );
		while( *c )
		{
			if ( *c == '%' )
				*c = '.';
			c++;
		}
		trap_Cvar_Set("g_password", password );

		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;
	//Lugormod it is int everywhere else. No it's not.
	//int thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		goto runicarus;
	}
	if (thinktime > level.time) {
		goto runicarus;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		//G_Error ( "NULL ent->think");
		goto runicarus;
	}
	ent->think (ent);

runicarus:
	if ( ent->inuse )
	{
		trap_ICARUS_MaintainTaskManager(ent->s.number);
	}
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;
//#define _G_FRAME_PERFANAL

void NAV_CheckCalcPaths( void ){
	if ( navCalcPathTime && navCalcPathTime < level.time )
	{//first time we've ever loaded this map...
		//vmCvar_t	mapname;
		vmCvar_t	ckSum;

		//RoboPhred
		//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap_Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

		//clear all the failed edges
		trap_Nav_ClearAllFailedEdges();

		//Calculate all paths
		//RoboPhred
		NAV_CalculatePaths(level.rawmapname, ckSum.integer );
		//NAV_CalculatePaths( mapname.string, ckSum.integer );

		trap_Nav_CalculatePaths(qfalse);

#ifndef FINAL_BUILD
		if ( fatalErrors )
		{
			Com_Printf( S_COLOR_RED"Not saving .nav file due to fatal nav errors\n" );
		}
		else
#endif
#ifndef _XBOX
			//RoboPhred
			if ( trap_Nav_Save( level.rawmapname, ckSum.integer ) == qfalse )
			//if ( trap_Nav_Save( mapname.string, ckSum.integer ) == qfalse )
			{
				Com_Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", level.rawmapname, ckSum.integer );
			}
#endif
			navCalcPathTime = 0;
	}
}

//so shared code can get the local time depending on the side it's executed on
#include "../namespace_begin.h"
int BG_GetTime(void)
{
	return level.time;
}
#include "../namespace_end.h"

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void ClearNPCGlobals( void );
void AI_UpdateGroups( void );
void ClearPlayerAlertEvents( void );
void SiegeCheckTimers(void);
void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd );
extern void Jedi_Decloak( gentity_t *self );
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
#define MAX_BG_SPAWN_POINT 128

int g_siegeRespawnCheck = 0;
//Lugormod
//void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);
int g_siegeLeaveCheck = 0;
int g_uptimeupdate = 0;
int g_nextReborn = 0;
qboolean last_man_standing = qfalse;
int survival_count = 0;
#define SURVIVAL_TIME 180 //seconds
int G_CountHumanPlayers (int team);
int BattleGroundControlPoints (gentity_t **list, team_t team);
gentity_t *SelectBGSpawnPoint ( int siegeClass, team_t team, int teamstate, vec3_t origin, vec3_t angles );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);
//end Lugormod

//RoboPhred
/*meh, mabey later
inline unsigned int chenc(unsigned int sum, char c){
	BYTE cipher = (value ^ (r >> 8));
	r = (cipher + r) * c1 + c2;
	sum += cipher;
	return sum;
}

inline unsigned qboolean encCompare(char *str1, char *str2){
}
*/

//RoboPhred
void Lmd_RunFrame(void);
void Lmd_logic_think();
int Merc_JetpackDefuel(gentity_t *ent);
int Merc_JetpackRefuel(gentity_t *ent);
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	       *ent;
	int			msec;
#ifdef _G_FRAME_PERFANAL
	int			iTimer_ItemRun = 0;
	int			iTimer_ROFF = 0;
	int			iTimer_ClientEndframe = 0;
	int			iTimer_GameChecks = 0;
	int			iTimer_Queues = 0;
	void		*timer_ItemRun;
	void		*timer_ROFF;
	void		*timer_ClientEndframe;
	void		*timer_GameChecks;
	void		*timer_Queues;
#endif
	//char gn[MAX_STRING_CHARS], lm[MAX_STRING_CHARS], str[MAX_STRING_CHARS]; //RoboPhred
	//int a, b, c;
	//Lugormod update uptime
	if (g_uptimeupdate < level.time) {
		g_uptimeupdate = level.time + 300000;
		if (g_uptime.integer < level.time / 3600000) {
			trap_Cvar_Set("serveruptime",va("%i",
				level.time / 3600000));
		}
	}

	/*eh, mabey later
	//RoboPhred: override boss trying to change the mod name
	//need to avoid function calls if at all possible
	//create the 'gamename' string and set it to gn
	//create the 'Lugormod' string and set it to lm
	trap_Cvar_VariableStringBuffer(gn, str, sizeof(str));
	//compare manually
	a = strlen(str);
	if(a != strlen(lm)){
		trap_Cvar_Set(gn, lm);
		goto ContinueThink;
	}
	for(i = 0;i<a;i++){
		if(str[i] != lm[i]){
			trap_Cvar_Set(gn, lm);
			goto ContinueThink;
		}
	}

ContinueThink:
	*/
	//RoboPhred: check account saves
	Lmd_RunFrame();

	if (g_gametype.integer == GT_SIEGE &&
		g_siegeLeaveCheck < level.time) {
			//Lugormod go back to ffa if all players leave

			if (G_CountHumanPlayers( -1) == 0
				&& g_allowVote.integer&(1|16)){
					if (g_siegeLeaveCheck && g_siegeLeaveCheck + 60000 < level.time) {
						trap_SendConsoleCommand( EXEC_APPEND, "g_gametype 0\n");
						const char *nextMap = G_RefreshNextMap(0, qtrue);
						if (nextMap && nextMap[0])
						{
							trap_SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
						}
					}
			} else {
				//Ten seconds until next check
				g_siegeLeaveCheck = level.time + 10000;
			}

			//end Lugormod
	} else if (g_gametype.integer == GT_REBORN &&
		g_siegeLeaveCheck < level.time &&
		level.startTime + 30000 < level.time &&
		!level.intermissiontime) {
			g_siegeLeaveCheck = level.time + 1000;
			int i,j;

			level.teamScores[TEAM_RED] = 0;
			level.teamScores[TEAM_BLUE] = 0;
			if (++survival_count > SURVIVAL_TIME) {
				survival_count = 0;
			}

			for (i = 0; i < level.maxclients; i++) {
				gclient_t *client;
				client = &level.clients[i];
				if (client->pers.connected == CON_CONNECTED &&
					client->sess.sessionTeam == TEAM_RED) {
						level.teamScores[TEAM_RED]++;
				} else if (client->pers.connected == CON_CONNECTED &&
					client->sess.sessionTeam == TEAM_BLUE) {
						level.teamScores[TEAM_BLUE]++;
						if (survival_count == SURVIVAL_TIME) {
							trap_SendServerCommand(i, "cp \"^4Survival bonus\"");
							AddScore(&g_entities[i], g_entities[i].r.currentOrigin, 1);
						}

						j = i;
				}
			}

			if (level.teamScores[TEAM_BLUE] == 0) {
				//New Round
				gentity_t *Tent;

				for (i = 0; i < level.maxclients; i++) {
					Tent = &g_entities[i];
					if (Tent->client->pers.connected != CON_CONNECTED) {
						continue;
					}

					SetTeamQuick(Tent, TEAM_BLUE, qfalse);
					ClientSpawn(Tent);
				}
			} else if (!last_man_standing &&
				level.teamScores[TEAM_BLUE] == 1) {
					if (level.teamScores[TEAM_RED] > 1) {
						AddScore(&g_entities[j],
							g_entities[j].r.currentOrigin,3);
					}
					last_man_standing = qtrue;
					trap_SendServerCommand(j, "cp \"^4Last man standing\"");
			}
			if (level.teamScores[TEAM_RED] == 0) {
				survival_count = 0;
				last_man_standing = qfalse;
				while (level.clients[++g_nextReborn].pers.connected !=
					CON_CONNECTED
					&& g_nextReborn < MAX_CLIENTS) {
				}
				if (g_nextReborn == MAX_CLIENTS) {
					//New game
					if (level.teamScores[TEAM_BLUE] > 1) {
						LogExit( "End of Game." );
						g_siegeLeaveCheck = level.time + 20000;
						g_nextReborn = 0;
						return;
					} else {
						g_nextReborn = 0;
					}
				} else {
					SetTeamQuick(&g_entities[g_nextReborn], TEAM_RED, qfalse);
					ClientSpawn(&g_entities[g_nextReborn]);
				}
			}
			CalculateRanks();
	}

	if (g_gametype.integer == GT_SIEGE &&
		g_siegeRespawn.integer &&
		g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
		int i = 0;
		gentity_t *clEnt;
		while (i < MAX_CLIENTS)
		{
			clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate > level.time &&
				//clEnt->client->tempSpectate < level.time && //Ufo: what the fuck
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				respawn(clEnt);
				clEnt->client->tempSpectate = 0;
			}
			i++;
		}

		g_siegeRespawnCheck = level.time + g_siegeRespawn.integer * 1000;
	} else if (g_gametype.integer == GT_BATTLE_GROUND //Lugormod
		&& g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
		int i = 0;
		gentity_t *clEnt;
		int canspawn[TEAM_NUM_TEAMS] = {0,0,0};
		int inteam[TEAM_NUM_TEAMS] = {0,0,0};
		int cantspawn[TEAM_NUM_TEAMS] = {0,0,0};
		int team;

		//Goes through all entities twice = stupid
		canspawn[TEAM_RED]  = BattleGroundControlPoints(NULL,TEAM_RED);
		canspawn[TEAM_BLUE] = BattleGroundControlPoints(NULL,TEAM_BLUE);

		while (i < MAX_CLIENTS)
		{
			clEnt = &g_entities[i];
			if (!clEnt->inuse || !clEnt->client) {
				i++;
				continue;
			}
			team = clEnt->client->sess.sessionTeam;

			inteam[team]++;

			if (clEnt->client->tempSpectate > level.time &&
				team != TEAM_SPECTATOR)
			{
				if (canspawn[team] == 0 && canspawn[TEAM_BLUE]+canspawn[TEAM_RED]){
					cantspawn[team]++;
					clEnt->client->tempSpectate = level.time+ 25000;
				} else {
					respawn(clEnt);
					clEnt->client->tempSpectate = 0;
				}
			}
			i++;
		}
		if (canspawn[TEAM_RED] == 0 &&
			cantspawn[TEAM_RED] == inteam[TEAM_RED] &&
			canspawn[TEAM_BLUE]) {
				level.teamScores[TEAM_RED] = 0;
		}
		if (canspawn[TEAM_BLUE] == 0 &&
			cantspawn[TEAM_BLUE] == inteam[TEAM_BLUE] &&
			canspawn[TEAM_RED]) {
				level.teamScores[TEAM_BLUE] = 0;
		}

		g_siegeRespawnCheck = level.time + 20000;
	} else if (g_gametype.integer == GT_GHOST &&
		g_siegeRespawnCheck < level.time) {
			int i = 0;
			gentity_t *clEnt;
			while (i < MAX_CLIENTS)
			{
				clEnt = &g_entities[i];

				if (clEnt->inuse && clEnt->client &&
					clEnt->client->tempSpectate > level.time &&
					//clEnt->client->tempSpectate <= level.time+40000 &&
					clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
				{
					respawn(clEnt);
					clEnt->client->tempSpectate = 0;
				}
				i++;
			}

			g_siegeRespawnCheck = level.time + 20000;
	}
	/*
	else if (g_gametype.integer == GT_REBORN//Lugormod
	&& g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
	int i = 0;
	gentity_t *clEnt;
	while (i < MAX_CLIENTS)
	{
	clEnt = &g_entities[i];

	if (clEnt->inuse && clEnt->client &&
	clEnt->client->tempSpectate > level.time &&
	clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
	respawn(clEnt);
	clEnt->client->tempSpectate = 0;
	}
	i++;
	}

	g_siegeRespawnCheck = level.time + 20000;
	}
	*/
	else if (gameMode(GMF_RESPAWN_TIMER)//Lugormod
		&& g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
		int i = 0;
		gentity_t *clEnt;
		while (i < MAX_CLIENTS)
		{
			clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate > level.time &&
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				respawn(clEnt);
				clEnt->client->tempSpectate = 0;
			}
			i++;
		}

		g_siegeRespawnCheck = level.time + 20000;
	}

	if (gDoSlowMoDuel)
	{
		if (level.restarted)
		{
			char buf[128];
			float tFVal = 0;

			trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

			tFVal = atof(buf);

			trap_Cvar_Set("timescale", "1");
			if (tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
		else
		{
			float timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
			float useDif = 0; //the difference to use when actually setting the timescale

			if (timeDif < 150)
			{
				trap_Cvar_Set("timescale", "0.1f");
			}
			else if (timeDif < 1150)
			{
				useDif = (timeDif/1000); //scale from 0.1 up to 1
				if (useDif < 0.1)
				{
					useDif = 0.1;
				}
				if (useDif > 1.0)
				{
					useDif = 1.0;
				}
				trap_Cvar_Set("timescale", va("%f", useDif));
			}
			else
			{
				char buf[128];
				float tFVal = 0;

				trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

				tFVal = atof(buf);

				trap_Cvar_Set("timescale", "1");
				if (timeDif > 1500 && tFVal == 1.0f)
				{
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	msec = level.time - level.previousTime;

	if (g_allowNPC.integer)
	{
		NAV_CheckCalcPaths();
	}

	AI_UpdateGroups();

	if (g_allowNPC.integer)
	{
		if ( d_altRoutes.integer )
		{
			trap_Nav_CheckAllFailedEdges();
		}
		trap_Nav_ClearCheckedNodes();

		//remember last waypoint, clear current one
		for ( i = 0; i < level.num_entities ; i++)
		{
			ent = &g_entities[i];

			if ( !ent->inuse )
				continue;

			if ( ent->waypoint != WAYPOINT_NONE
				&& ent->noWaypointTime < level.time )
			{
				ent->lastWaypoint = ent->waypoint;
				ent->waypoint = WAYPOINT_NONE;
			}
			if ( d_altRoutes.integer )
			{
				trap_Nav_CheckFailedNodes( ent );
			}
		}

		//Look to clear out old events
		ClearPlayerAlertEvents();
	}

	g_TimeSinceLastFrame = (level.time - g_LastFrameTime);

	// get any cvar changes
	G_UpdateCvars();

#ifdef _G_FRAME_PERFANAL
	trap_PrecisionTimer_Start(&timer_ItemRun);
#endif
	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		//RoboPhred: I have no idea what this is for, but it is messing up a specific entity in
		//Atlantica_RPG by Szico that uses a model2 key.  Since seperate models can have different mins and maxs
		//outside the origin, then we should not do this.
		//What IS this for!!!
		//screw it, turning it off for now.  At the very least, it should not be checked every frame...
		/*
		if (g_spFixes ent->s.eType == ET_MOVER && !(ent->spawnflags & 16384) && ent->spawnflags & 3
			&& Q_stricmp("func_static", ent->classname) == 0
			&& !G_PointInBounds(ent->s.origin, ent->r.absmin, ent->r.absmax)) {
				int it;
				for (it = 0;it < 3;it++) {
					ent->s.origin[it] = (ent->r.absmin[it] + ent->r.absmax[it])/2;
					ent->r.maxs[it] = (ent->r.absmax[it] - ent->r.absmin[it])/2;
					ent->r.mins[it] = -ent->r.maxs[it];
				}

				G_SetOrigin(ent, ent->s.origin);
				trap_LinkEntity(ent);
				ent->spawnflags |= 16384;
		}
		*/

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{ //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					if (!Lmd_Entities_IsSaveable(ent)) {
						G_FreeEntity( ent );
						continue;
					} else {
						//Lugormod don't remove
						continue;
					}
				}
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
#if 0 //use if body dragging enabled?
			if (ent->s.eType == ET_BODY)
			{ //special case for bodies
				float grav = 3.0f;
				float mass = 0.14f;
				float bounce = 1.15f;

				G_RunExPhys(ent, grav, mass, bounce, qfalse, NULL, 0);
			}
			else
			{
				G_RunItem( ent );
			}
#else
			G_RunItem( ent );
#endif
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ){
			//RoboPhred
			int prof = PlayerAcc_Prof_GetProfession(ent);
			G_CheckClientTimeouts ( ent );

			if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
			{ //we're in space, check for suffocating and for exiting
				gentity_t *spacetrigger = &g_entities[ent->client->inSpaceIndex];

				//Ufo:
				if (!spacetrigger || !spacetrigger->inuse ||
					!G_PointInBounds(ent->client->ps.origin, spacetrigger->r.absmin, spacetrigger->r.absmax) ||
					spacetrigger->flags & FL_INACTIVE//RoboPhred
					)
				{ //no longer in space then I suppose
					ent->client->inSpaceIndex = 0;
				}
				else
				{ //check for suffocation
					if (ent->client->inSpaceSuffocation < level.time)
					{ //suffocate!
						if (ent->health > 0 && ent->takedamage)
						{ //if they're still alive..
							//play the choking sound
							G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )));

							//make them grasp their throat
							ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
							ent->client->ps.forceHandExtendTime = level.time + 2000;

							//Ufo:
							if (spacetrigger->damage > 0)
								G_Damage(ent, spacetrigger, spacetrigger, NULL, ent->client->ps.origin, spacetrigger->damage, DAMAGE_NO_ARMOR, MOD_SUICIDE);
						}

						ent->client->inSpaceSuffocation = level.time + Q_irand(100, 200);
					}
				}
			}

			if (ent->client->isHacking)
			{ //hacking checks
				gentity_t *hacked = GetEnt(ent->client->isHacking);
				vec3_t angDif;

				VectorSubtract(ent->client->ps.viewangles, ent->client->hackingAngles, angDif);

				//keep him in the "use" anim
				if (ent->client->ps.torsoAnim != BOTH_CONSOLE1)
				{
					G_SetAnim( ent, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				}
				else
				{
					ent->client->ps.torsoTimer = 500;
				}
				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if (!(ent->client->pers.cmd.buttons & BUTTON_USE))
				{ //have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!hacked || !hacked->inuse)
				{ //shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!G_PointInBounds( ent->client->ps.origin, hacked->r.absmin, hacked->r.absmax ))
				{ //they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (VectorLength(angDif) > 10.0f)
				{ //must remain facing generally the same angle as when we start
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}

#define JETPACK_DEFUEL_RATE		200 //approx. 20 seconds of idle use from a fully charged fuel amt
#define JETPACK_REFUEL_RATE		150 //seems fair
#ifdef LMD_NEW_JETPACK
			if (ent->client->jetPackTime > level.time){ //using jetpack, drain fuel
#else
			if (ent->client->jetPackOn){ //using jetpack, drain fuel
#endif
				if (ent->client->jetPackDebReduce < level.time && (g_gametype.integer != GT_FFA || prof != PROF_ADMIN || gameMode(GM_ANY))){
					//RoboPhred: take into account the time since last check
					int debTime;
					float debPoints = 1;

					//Ufo: Same fuel capacity for merc and none
					/*if (prof == PROF_MERC)
						debTime = Merc_JetpackDefuel(ent);
					else*/ if (g_gametype.integer == GT_BATTLE_GROUND)
						//Lugormod jp too powerful
						debTime = (JETPACK_DEFUEL_RATE/4);
					else if (gameMode(GM_ALLWEAPONS))
						debTime = (JETPACK_DEFUEL_RATE/6);
					else
						debTime = JETPACK_DEFUEL_RATE;

					//Scale debounce points based on time since last regen.
					debPoints = (float)debPoints * ((level.time - ent->client->jetPackDebReduce) /
						(float)debTime);

					if(debPoints >= 1) {
						//take more if they're thrusting
#ifndef LMD_NEW_JETPACK
						if (ent->client->pers.cmd.upmove > 0)
							ent->client->ps.jetpackFuel -= 2 * debPoints;
						else
#endif
							ent->client->ps.jetpackFuel -= debPoints;

						//RoboPhred: Client game draws the red bar in negitive space at 0, keep it at 1.
#ifdef LMD_NEW_JETPACK
						if(ent->client->ps.jetpackFuel <= 1)
							ent->client->ps.jetpackFuel = 1;
#else
						if (ent->client->ps.jetpackFuel <= 1){ //turn it off
							ent->client->ps.jetpackFuel = 1;
							Jetpack_Off(ent);
						}
#endif

						ent->client->jetPackDebReduce = level.time + debTime;
					}
				}

				//RoboPhred: Keep the debounce time current so we dont take a massive chunk next frame.
				ent->client->jetPackDebRecharge = level.time;
			}
			else {
				//RoboPhred: Keep the debounce time current so we dont take a massive chunk next frame.
				ent->client->jetPackDebReduce = level.time;

				if(gameMode(GM_ALLWEAPONS) && (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG]
					|| ent->client->ps.powerups[PW_NEUTRALFLAG]))
				{
					ent->client->jetPackDebRecharge = level.time;
				}
				else if (ent->client->jetPackToggleTime < level.time && ent->client->ps.jetpackFuel < 100)
				{ //recharge jetpack
					if (ent->client->jetPackDebRecharge < level.time) {
						//RoboPhred: take into account the time since last check
						int refTime;
						float refPoints = 1;

						if (prof == PROF_MERC)
							refTime = Merc_JetpackRefuel(ent);
						else
							refTime = JETPACK_REFUEL_RATE;

						//Scale debounce points based on time since last regen.
						refPoints = refPoints * ((level.time - ent->client->jetPackDebRecharge) /
							(float)refTime);

						if(refPoints >= 1) {
							ent->client->ps.jetpackFuel += refPoints;
							ent->client->jetPackDebRecharge = level.time + refTime;
						}
					}
				}
				else {
					ent->client->jetPackDebRecharge = level.time;
				}
			}

#define CLOAK_DEFUEL_RATE		200 //approx. 20 seconds of idle use from a fully charged fuel amt
#define CLOAK_REFUEL_RATE		150 //seems fair
			if (ent->client->ps.powerups[PW_CLOAKED] &&
				//RoboPhred
				ent->client->Lmd.bodyshieldPower == 0 &&
				g_gametype.integer != GT_GHOST && !gameMode(GMF_CLOAKING)){ //using cloak, drain battery
				if (ent->client->cloakDebReduce < level.time && (prof != PROF_ADMIN ||
					g_gametype.integer != GT_FFA)){
						ent->client->ps.cloakFuel--;
						//RoboPhred: veh cloaking
						if(lmd_vehcloaking.integer && ent->client->ps.m_iVehicleNum &&
							g_entities[ent->client->ps.m_iVehicleNum].client->ps.powerups[PW_CLOAKED]) {
								ent->client->ps.cloakFuel--;
						}

						if (ent->client->ps.cloakFuel <= 0)
						{ //turn it off
							ent->client->ps.cloakFuel = 0;
							Jedi_Decloak(ent);
						}
						ent->client->cloakDebReduce = level.time + CLOAK_DEFUEL_RATE;
				}
			}
			//RoboPhred
			else if(ent->client->Lmd.bodyshieldPower == 0 && ent->client->ps.cloakFuel < 100)
			//else if (ent->client->ps.cloakFuel < 100)
			{ //recharge cloak
				if (ent->client->cloakDebRecharge < level.time)
				{
					ent->client->ps.cloakFuel++;
					ent->client->cloakDebRecharge = level.time + CLOAK_REFUEL_RATE;
				}
			}

			if (g_gametype.integer == GT_SIEGE && ent->client->siegeClass != -1 &&
				(bgSiegeClasses[ent->client->siegeClass].classflags & (1<<CFL_STATVIEWER)))
			{ //see if it's time to send this guy an update of extended info
				if (ent->client->siegeEDataSend < level.time)
				{
					G_SiegeClientExData(ent);
					ent->client->siegeEDataSend = level.time + 1000; //once every sec seems ok
				}
			}

			if((!level.intermissiontime)&&!(ent->client->ps.pm_flags&PMF_FOLLOW) && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				//if (ent->client->ps.isJediMaster) {
				//Let's try to display the master's health
				//Ok, this works but it's centered !
				//trap_SendServerCommand( -1, va("cp \"Jedi Master Health: %i\n\"", ent->client->ps.stats[STAT_HEALTH]));
				//}

				WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
				WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
				WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
			}

			if (g_allowNPC.integer)
			{
				//This was originally intended to only be done for client 0.
				//Make sure it doesn't slow things down too much with lots of clients in game.
				NAV_FindPlayerWaypoint(i);
			}

			trap_ICARUS_MaintainTaskManager(ent->s.number);

			G_RunClient( ent );
			continue;
		}
		else if (ent->s.eType == ET_NPC)
		{
			int j;
			// turn off any expired powerups
			for ( j = 0 ; j < MAX_POWERUPS ; j++ ) {
				if ( ent->client->ps.powerups[ j ] < level.time ) {
					ent->client->ps.powerups[ j ] = 0;
				}
			}

			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
		}

		G_RunThink( ent );

		if (g_allowNPC.integer)
		{
			ClearNPCGlobals();
		}
	}

	//RoboPhred
	Lmd_logic_think();

#ifdef _G_FRAME_PERFANAL
	iTimer_ItemRun = trap_PrecisionTimer_End(timer_ItemRun);
#endif

	SiegeCheckTimers();

#ifdef _G_FRAME_PERFANAL
	trap_PrecisionTimer_Start(&timer_ROFF);
#endif
	trap_ROFF_UpdateEntities();
#ifdef _G_FRAME_PERFANAL
	iTimer_ROFF = trap_PrecisionTimer_End(timer_ROFF);
#endif

#ifdef _G_FRAME_PERFANAL
	trap_PrecisionTimer_Start(&timer_ClientEndframe);
#endif
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ClientEndframe = trap_PrecisionTimer_End(timer_ClientEndframe);
#endif

#ifdef _G_FRAME_PERFANAL
	trap_PrecisionTimer_Start(&timer_GameChecks);
#endif
	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_GameChecks = trap_PrecisionTimer_End(timer_GameChecks);
#endif

#ifdef _G_FRAME_PERFANAL
	trap_PrecisionTimer_Start(&timer_Queues);
#endif
	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();

	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = 0;
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_Queues = trap_PrecisionTimer_End(timer_Queues);
#endif

#ifdef _G_FRAME_PERFANAL
	Com_Printf("---------------\nItemRun: %i\nROFF: %i\nClientEndframe: %i\nGameChecks: %i\nQueues: %i\n---------------\n",
		iTimer_ItemRun,
		iTimer_ROFF,
		iTimer_ClientEndframe,
		iTimer_GameChecks,
		iTimer_Queues);
#endif

	g_LastFrameTime = level.time;
}

const char *G_GetStringEdString(char *refSection, char *refName)
{
	/*
	static char text[1024]={0};
	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024]={0};
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}