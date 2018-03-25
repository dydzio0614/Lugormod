// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

//RoboPhred
#include "Lmd_Entities_Public.h"
#include "Lmd_Arrays.h"

extern qboolean disablesenabled;


//#define LMD_SPAWN_HEAPCHECK
#ifdef LMD_SPAWN_HEAPCHECK
#include <crtdbg.h>
#endif

//RoboPhred
gentity_t *errorMessageTarget = NULL;
void EntitySpawnError(const char *msg) {
	if(errorMessageTarget) {
		Disp(errorMessageTarget, va("^1Spawn Error: ^3%s", msg));
	}
	else
		va("Spawn Error: %s", msg);
}

qboolean G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;
	G_Free(*out);

	if ( !level.spawning ) {
		*out = (char *)defaultString;
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {

#ifdef LMD_SPAWN_HEAPCHECK
			assert(_CrtCheckMemory());
#endif
			*out = G_NewString(level.spawnVars[i][1]);

#ifdef LMD_SPAWN_HEAPCHECK
			assert(_CrtCheckMemory());
#endif
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s = NULL;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	G_Free(s);
	return present;
}

qboolean G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s = NULL;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	G_Free(s);
	return present;
}

qboolean G_SpawnVector( const char *key, const char *defaultString, vec3_t out ) {
	char		*s = NULL;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	G_Free(s);
	return present;
}

//RoboPhred: scriptrunners cant be logical, and mostly anything can be a scriptrunner...
#define ENTFIELD_NOLOGIC 1

BG_field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"eType", FOFS(s.eType), F_INT},
	{"teamnodmg", FOFS(teamnodmg), F_INT},
	{"teamowner", FOFS(s.teamowner), F_INT},
	{"teamuser", FOFS(alliedTeam), F_INT},
	{"alliedTeam", FOFS(alliedTeam), F_INT},//for misc_turrets
	{"roffname", FOFS(roffname), F_LSTRING},
	{"rofftarget", FOFS(rofftarget), F_LSTRING},
	{"healingclass", FOFS(healingclass), F_LSTRING},
	{"healingsound", FOFS(healingsound), F_LSTRING},
	{"healingrate", FOFS(healingrate), F_INT},
	{"ownername", FOFS(ownername), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"target2", FOFS(target2), F_LSTRING},
	{"target3", FOFS(target3), F_LSTRING},
	{"target4", FOFS(target4), F_LSTRING},
	{"target5", FOFS(target5), F_LSTRING},
	{"target6", FOFS(target6), F_LSTRING},
	{"NPC_targetname", FOFS(NPC_targetname), F_LSTRING},
	{"NPC_target", FOFS(NPC_target), F_LSTRING},
	{"NPC_target2", FOFS(target2), F_LSTRING},//NPC_spawner only
	{"NPC_target4", FOFS(target4), F_LSTRING},//NPC_spawner only

	//RoboPhred: NPC_target5: target to use on npc death, player as activator
	{"NPC_target5", FOFS(target5), F_LSTRING},//NPC_spawner only
	//RoboPhred: NPC_target7: target to use when npc kills a player, npc as activator
	{"NPC_target6", FOFS(target6), F_LSTRING},//NPC_spawner only

	{"NPC_type", FOFS(NPC_type), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_INT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"maxs", FOFS(r.maxs), F_VECTOR},
	{"mins", FOFS(r.mins), F_VECTOR},
	{"modelscale", FOFS(modelScale[0]), F_FLOAT},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetShaderName", FOFS(targetShaderName), F_LSTRING},
	{"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},
	{"linear", FOFS(alt_fire), F_INT},//for movers to use linear movement

	{"closetarget", FOFS(closetarget), F_LSTRING},//for doors
	{"opentarget", FOFS(opentarget), F_LSTRING},//for doors
	{"paintarget", FOFS(paintarget), F_LSTRING},//for doors

	{"goaltarget", FOFS(goaltarget), F_LSTRING},//for siege
	{"idealclass", FOFS(idealclass), F_LSTRING},//for siege spawnpoints

	//rww - icarus stuff:
	{"spawnscript", FOFS(behaviorSet[BSET_SPAWN]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"usescript", FOFS(behaviorSet[BSET_USE]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"awakescript", FOFS(behaviorSet[BSET_AWAKE]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"angerscript", FOFS(behaviorSet[BSET_ANGER]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"attackscript", FOFS(behaviorSet[BSET_ATTACK]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"victoryscript", FOFS(behaviorSet[BSET_VICTORY]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"lostenemyscript", FOFS(behaviorSet[BSET_LOSTENEMY]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"painscript", FOFS(behaviorSet[BSET_PAIN]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"fleescript", FOFS(behaviorSet[BSET_FLEE]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"deathscript", FOFS(behaviorSet[BSET_DEATH]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"delayscript", FOFS(behaviorSet[BSET_DELAYED]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"delayscripttime", FOFS(delayScriptTime), F_INT, ENTFIELD_NOLOGIC},//name of script to run
	{"blockedscript", FOFS(behaviorSet[BSET_BLOCKED]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"ffirescript", FOFS(behaviorSet[BSET_FFIRE]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"ffdeathscript", FOFS(behaviorSet[BSET_FFDEATH]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"mindtrickscript", FOFS(behaviorSet[BSET_MINDTRICK]), F_LSTRING, ENTFIELD_NOLOGIC},//name of script to run
	{"script_targetname", FOFS(script_targetname), F_LSTRING, ENTFIELD_NOLOGIC},//scripts look for this when "affecting"

	{"fullName", FOFS(fullName), F_LSTRING},

	{"soundSet", FOFS(soundSet), F_LSTRING},
	{"radius", FOFS(radius), F_FLOAT},
	{"numchunks", FOFS(radius), F_FLOAT},//for func_breakables
	{"chunksize", FOFS(mass), F_FLOAT},//for func_breakables
	{"splashDamage", FOFS(splashDamage), F_INT},//for func_breakables
	{"splashRadius", FOFS(splashRadius), F_INT},//for func_breakables

	{"examine", FOFS(examineStr), F_LSTRING},


//Script parms - will this handle clamping to 16 or whatever length of parm[0] is?
	{"parm1", 0, F_PARM1},
	{"parm2", 0, F_PARM2},
	{"parm3", 0, F_PARM3},
	{"parm4", 0, F_PARM4},
	{"parm5", 0, F_PARM5},
	{"parm6", 0, F_PARM6},
	{"parm7", 0, F_PARM7},
	{"parm8", 0, F_PARM8},
	{"parm9", 0, F_PARM9},
	{"parm10", 0, F_PARM10},
	{"parm11", 0, F_PARM11},
	{"parm12", 0, F_PARM12},
	{"parm13", 0, F_PARM13},
	{"parm14", 0, F_PARM14},
	{"parm15", 0, F_PARM15},
	{"parm16", 0, F_PARM16},
	{"modelAngles", FOFS(s.angles2), F_VECTOR},
	{"material", FOFS(material), F_INT},
	
	//RoboPhred
	{"group", FOFS(Lmd.group), F_LSTRING},

	{NULL}
};

//Lugormod getField
BG_field_t* getField (const char *fieldName) {
		int i;
		for (i = 0;fields[i].name;i++) {
				if (Q_stricmp(fields[i].name, fieldName) == 0) {
						return &fields[i];
				}
		}
		return NULL;
}


void SP_item_botroam( gentity_t *ent )
{
}

void SP_gametype_item ( gentity_t* ent )
{
	gitem_t *item = NULL;
	char *value = NULL;
	int team = -1;

	G_SpawnString("teamfilter", "", &value);

	G_SetOrigin( ent, ent->s.origin );

	// If a team filter is set then override any team settings for the spawns
	if ( level.mTeamFilter[0] )
	{
		if ( Q_stricmp ( level.mTeamFilter, "red") == 0 )
		{
			team = TEAM_RED;
		}
		else if ( Q_stricmp ( level.mTeamFilter, "blue") == 0 )
		{
			team = TEAM_BLUE;
		}
	}

	if (ent->targetname && ent->targetname[0])
	{
		if (team != -1)
		{
			if (strstr(ent->targetname, "flag"))
			{
				if (team == TEAM_RED)
				{
					item = BG_FindItem("team_CTF_redflag");
				}
				else
				{ //blue
					item = BG_FindItem("team_CTF_blueflag");
				}
			}
		}
		else if (strstr(ent->targetname, "red_flag"))
		{
			item = BG_FindItem("team_CTF_redflag");
		}
		else if (strstr(ent->targetname, "blue_flag"))
		{
			item = BG_FindItem("team_CTF_blueflag");
		}
		else
		{
			item = NULL;
		}

		if (item)
		{
			ent->targetname = NULL;
			ent->classname = item->classname;
			G_SpawnItem( ent, item );
		}
	}
}

void SP_emplaced_gun( gentity_t *ent );
#define SP (~(1 << GT_SINGLE_PLAYER) & ~(1 << GT_SIEGE))

void SP_target_credits( gentity_t *ent ); //Lugormod
void SP_random_spot (gentity_t *ent); //Lugormod
void SP_money_dispenser (gentity_t *ent);//Lugormod
void SP_control_point (gentity_t *ent);//Lugormod
void SP_misc_camera (gentity_t *ent); //Lugormod
void SP_info_player_jail (gentity_t *ent); //Lugormod
extern entityInfo_t info_player_jail_info;

void SP_info_player_start (gentity_t *ent);
extern entityInfo_t info_player_start_info;

void SP_info_player_duel( gentity_t *ent );
extern entityInfo_t info_player_duel_info;

void SP_info_player_duel1( gentity_t *ent );
extern entityInfo_t info_player_duel1_info;

void SP_info_player_duel2( gentity_t *ent );
extern entityInfo_t info_player_duel2_info;

void SP_info_player_deathmatch (gentity_t *ent);
extern entityInfo_t info_player_deathmatch_info;

void SP_info_player_siegeteam1 (gentity_t *ent);
void SP_info_player_siegeteam2 (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
void SP_info_player_intermission_red (gentity_t *ent);
void SP_info_player_intermission_blue (gentity_t *ent);
void SP_info_jedimaster_start (gentity_t *ent);
void SP_info_player_start_red (gentity_t *ent);
void SP_info_player_start_blue (gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);

void SP_info_siege_objective (gentity_t *ent);
void SP_info_siege_radaricon (gentity_t *ent);
void SP_info_siege_decomplete (gentity_t *ent);
void SP_target_siege_end (gentity_t *ent);
void SP_misc_siege_item (gentity_t *ent);
void SP_target_powerup (gentity_t *ent);

void SP_func_plat (gentity_t *ent);
extern entityInfo_t func_plat_info;

void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);
void SP_func_breakable (gentity_t *ent);
void SP_func_glass (gentity_t *ent);
void SP_func_usable( gentity_t *ent);
void SP_func_wall( gentity_t *ent );
void SP_rail_mover (gentity_t *ent); //Lugormod
void SP_rail_track (gentity_t *ent); //Lugormod
void SP_rail_lane (gentity_t *ent); //Lugormod

void SP_trigger_lightningstrike( gentity_t *ent );
extern entityInfo_t trigger_lightningstrike_info;

void SP_trigger_once( gentity_t *ent );

qboolean trigger_multiple_allowlogical();
void SP_trigger_multiple (gentity_t *ent);
extern entityInfo_t trigger_multiple_info;

void SP_trigger_push (gentity_t *ent);
extern entityInfo_t trigger_push_info;

void SP_trigger_space(gentity_t *self);
extern entityInfo_t trigger_space_info;
void SP_trigger_shipboundary(gentity_t *self);
void SP_trigger_hyperspace(gentity_t *self);
void SP_trigger_asteroid_field(gentity_t *self);

void SP_trigger_always (gentity_t *ent);
void SP_trigger_visible (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);

qboolean target_speaker_allowlogical();
void SP_target_speaker (gentity_t *ent);

void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_character (gentity_t *ent);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_counter (gentity_t *self);
void SP_target_random (gentity_t *self);
void SP_target_scriptrunner( gentity_t *self );
void SP_target_interest (gentity_t *self);
void SP_target_activate (gentity_t *self);
void SP_target_deactivate (gentity_t *self);
void SP_target_level_change( gentity_t *self );
void SP_target_play_music( gentity_t *self );
void SP_target_push (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model_breakable(gentity_t *ent); //Lugormod
void SP_misc_exploding_crate(gentity_t *ent); //Lugormod
void SP_misc_security_panel(gentity_t *ent);
void SP_defender(gentity_t *ent);
void SP_misc_model(gentity_t *ent);
void SP_misc_model_static(gentity_t *ent);
void SP_misc_G2model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);
void SP_misc_weather_zone( gentity_t *ent );

void SP_misc_bsp (gentity_t *ent);
void SP_terrain (gentity_t *ent);
void SP_misc_skyportal_orient (gentity_t *ent);
void SP_misc_skyportal (gentity_t *ent);

void SP_misc_ammo_floor_unit(gentity_t *ent);
void SP_misc_shield_floor_unit( gentity_t *ent );
void SP_misc_model_shield_power_converter( gentity_t *ent );
void SP_misc_model_ammo_power_converter( gentity_t *ent );
void SP_misc_model_health_power_converter( gentity_t *ent );

void SP_fx_runner( gentity_t *ent );

void SP_target_screenshake(gentity_t *ent);
void SP_target_escapetrig(gentity_t *ent);

void SP_misc_maglock ( gentity_t *self );

void SP_misc_faller(gentity_t *ent);

void SP_misc_holocron(gentity_t *ent);

void SP_reference_tag ( gentity_t *ent );

void SP_misc_weapon_shooter( gentity_t *self );

void SP_NPC_spawner( gentity_t *self );

void SP_LMD_spawner( gentity_t *NPCspawner );

void SP_NPC_Vehicle( gentity_t *self);

void SP_NPC_Kyle( gentity_t *self );
void SP_NPC_Lando( gentity_t *self );
void SP_NPC_Jan( gentity_t *self );
void SP_NPC_Luke( gentity_t *self );
void SP_NPC_MonMothma( gentity_t *self );
void SP_NPC_Tavion( gentity_t *self );
void SP_NPC_Tavion_New( gentity_t *self );
void SP_NPC_Alora( gentity_t *self );
void SP_NPC_Reelo( gentity_t *self );
void SP_NPC_Galak( gentity_t *self );
void SP_NPC_Desann( gentity_t *self );
void SP_NPC_Bartender( gentity_t *self );
void SP_NPC_MorganKatarn( gentity_t *self );
void SP_NPC_Jedi( gentity_t *self );
void SP_NPC_Prisoner( gentity_t *self );
void SP_NPC_Rebel( gentity_t *self );
void SP_NPC_Stormtrooper( gentity_t *self );
void SP_NPC_StormtrooperOfficer( gentity_t *self );
void SP_NPC_Snowtrooper( gentity_t *self);
void SP_NPC_Tie_Pilot( gentity_t *self );
void SP_NPC_Ugnaught( gentity_t *self );
void SP_NPC_Jawa( gentity_t *self );
void SP_NPC_Gran( gentity_t *self );
void SP_NPC_Rodian( gentity_t *self );
void SP_NPC_Weequay( gentity_t *self );
void SP_NPC_Trandoshan( gentity_t *self );
void SP_NPC_Tusken( gentity_t *self );
void SP_NPC_Noghri( gentity_t *self );
void SP_NPC_SwampTrooper( gentity_t *self );
void SP_NPC_Imperial( gentity_t *self );
void SP_NPC_ImpWorker( gentity_t *self );
void SP_NPC_BespinCop( gentity_t *self );
void SP_NPC_Reborn( gentity_t *self );
void SP_NPC_ShadowTrooper( gentity_t *self );
void SP_NPC_Monster_Murjj( gentity_t *self );
void SP_NPC_Monster_Swamp( gentity_t *self );
void SP_NPC_Monster_Howler( gentity_t *self );
void SP_NPC_Monster_Claw( gentity_t *self );
void SP_NPC_Monster_Glider( gentity_t *self );
void SP_NPC_Monster_Flier2( gentity_t *self );
void SP_NPC_Monster_Lizard( gentity_t *self );
void SP_NPC_Monster_Fish( gentity_t *self );
void SP_NPC_Monster_Wampa( gentity_t *self );
void SP_NPC_Monster_Rancor( gentity_t *self );
void SP_NPC_MineMonster( gentity_t *self );
void SP_NPC_Droid_Interrogator( gentity_t *self );
void SP_NPC_Droid_Probe( gentity_t *self );
void SP_NPC_Droid_Mark1( gentity_t *self );
void SP_NPC_Droid_Mark2( gentity_t *self );
void SP_NPC_Droid_ATST( gentity_t *self );
void SP_NPC_Droid_Seeker( gentity_t *self );
void SP_NPC_Droid_Remote( gentity_t *self );
void SP_NPC_Droid_Sentry( gentity_t *self );
void SP_NPC_Droid_Gonk( gentity_t *self );
void SP_NPC_Droid_Mouse( gentity_t *self );
void SP_NPC_Droid_R2D2( gentity_t *self );
void SP_NPC_Droid_R5D2( gentity_t *self );
void SP_NPC_Droid_Protocol( gentity_t *self );

void SP_NPC_Reborn_New( gentity_t *self);
void SP_NPC_Cultist( gentity_t *self );
void SP_NPC_Cultist_Saber( gentity_t *self );
void SP_NPC_Cultist_Saber_Powers( gentity_t *self );
void SP_NPC_Cultist_Destroyer( gentity_t *self );
void SP_NPC_Cultist_Commando( gentity_t *self );
//Lugormod the missing npcs
void SP_NPC_HazardTrooper( gentity_t *self );
void SP_NPC_Saboteur( gentity_t *self );

void SP_waypoint (gentity_t *ent);
void SP_waypoint_small (gentity_t *ent);
void SP_waypoint_navgoal (gentity_t *ent);
void SP_waypoint_navgoal_8 (gentity_t *ent);
void SP_waypoint_navgoal_4 (gentity_t *ent);
void SP_waypoint_navgoal_2 (gentity_t *ent);
void SP_waypoint_navgoal_1 (gentity_t *ent);

void SP_CreateSpaceDust( gentity_t *ent );
void SP_CreateSnow( gentity_t *ent );
void SP_CreateRain( gentity_t *ent );
void SP_CreateWind( gentity_t *ent ); //Lugormod

void SP_point_combat( gentity_t *self );

void SP_shooter_blaster( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

extern entityInfo_t misc_turret_info;
void SP_misc_turret( gentity_t *ent );

void SP_misc_turretG2( gentity_t *base );
void SP_misc_slotmachine (gentity_t *ent);//Lugormod
void SP_ghost_exit (gentity_t *ent);//Lugormod
void SP_target_fixdoor (gentity_t *ent); //Lugormod

//RoboPhred

void lmd_actor(gentity_t *ent);
extern entityInfo_t lmd_actor_info;

void lmd_body(gentity_t *ent);
extern entityInfo_t lmd_body_info;

void lmd_chance(gentity_t *ent);
extern entityInfo_t lmd_chance_info;

void lmd_door(gentity_t *ent);
extern entityInfo_t lmd_door_info;

void lmd_drop(gentity_t *ent);
extern entityInfo_t lmd_drop_info;

void lmd_flagplayer(gentity_t *ent);
extern entityInfo_t lmd_flagplayer_info;

void lmd_gate(gentity_t *ent);
extern entityInfo_t lmd_gate_info;

void lmd_gravity(gentity_t *ent);
extern entityInfo_t lmd_gravity_info;

void lmd_speed(gentity_t *ent);
extern entityInfo_t lmd_speed_info;

void lmd_iobject(gentity_t *ent);
extern entityInfo_t lmd_iobject_info;

void lmd_light(gentity_t *ent);
extern entityInfo_t lmd_light_info;

void lmd_mover(gentity_t *ent);
extern entityInfo_t lmd_mover_info;

void lmd_playercheck(gentity_t *ent);
extern entityInfo_t lmd_playercheck_info;

void lmd_propertyterminal(gentity_t *ent);
extern entityInfo_t lmd_propertyterminal_info;

void lmd_property(gentity_t *ent);
extern entityInfo_t lmd_property_info;

void lmd_pwterminal(gentity_t *ent);
extern entityInfo_t lmd_pwterminal_info;

void lmd_remap(gentity_t *ent);
extern entityInfo_t lmd_remap_info;

void lmd_rentterminal(gentity_t *ent);
extern entityInfo_t lmd_rentterminal_info;

void lmd_restrict(gentity_t *ent);
extern entityInfo_t lmd_restrict_info;

void lmd_scale(gentity_t *ent);
extern entityInfo_t lmd_scale_info;

void lmd_stashdepo(gentity_t *ent);
void lmd_stashspawnpoint(gentity_t *ent);
void lmd_stashzone(gentity_t *ent);

void lmd_terminal(gentity_t *ent);
extern entityInfo_t lmd_terminal_info;

void lmd_toggle(gentity_t *ent);
extern entityInfo_t lmd_toggle_info;

void lmd_train (gentity_t *self);
extern entityInfo_t lmd_train_info;

void lmd_customskill(gentity_t *ent);
extern entityInfo_t lmd_customskill_info;

void lmd_event(gentity_t *ent);
extern entityInfo_t lmd_event_info;

void lmd_interact(gentity_t *ent);
extern entityInfo_t lmd_interact_info;

void lmd_playereffect(gentity_t *ent);
extern entityInfo_t lmd_playereffect_info;

//Ufo:
void sp_target_modify(gentity_t* self);
extern entityInfo_t target_modify_info;

void lmd_cskill_compare(gentity_t* self);
extern entityInfo_t lmd_cskill_compare_info;

void lmd_countcheck(gentity_t* self);
extern entityInfo_t lmd_countcheck_info;

void lmd_iterateplayers(gentity_t* self);
extern entityInfo_t lmd_iterateplayers_info;

void sp_target_heal(gentity_t* self);
extern entityInfo_t target_heal_info;

void lmd_actor_modify(gentity_t *ent);
extern entityInfo_t lmd_actor_modify_info;

void sp_target_animate(gentity_t* self);
extern entityInfo_t target_animate_info;

struct spawnTable_s {
	spawn_t *spawns;
	unsigned int count;
} spawnTable;

// Copies spawnData into the spawn table.
void Lmd_AddSpawnableEntry(spawn_t spawnData) {
	int i;

	// Check to see if this is overriding an entry.
	for(i = 0; i < spawnTable.count; i++) {
		if (Q_stricmp(spawnData.name, spawnTable.spawns[i].name) == 0) {
			// Copy in the replacement
			memcpy(&spawnTable.spawns[i], &spawnData, sizeof(spawn_t));
			return;
		}
	}

	// Add a new element
	int index = Lmd_Arrays_AddArrayElement((void **)&spawnTable.spawns, sizeof(spawn_t), &spawnTable.count);
	memcpy(&spawnTable.spawns[i], &spawnData, sizeof(spawn_t));
}

spawn_t	spawnInitValues[] = {
	
	{"lmd_playereffect", lmd_playereffect, Logical_True, &lmd_playereffect_info},

	{"lmd_actor", lmd_actor, Logical_True, &lmd_actor_info},

	{"lmd_body", lmd_body, Logical_True, &lmd_body_info},

	{"lmd_chance", lmd_chance, Logical_True, &lmd_chance_info},

	{"lmd_door", lmd_door, Logical_False, &lmd_door_info},
	{"t2_door", lmd_door, Logical_False},	

	{"lmd_drop", lmd_drop, Logical_True, &lmd_drop_info},
	{"lmd_dropcr", lmd_drop, Logical_True, &lmd_drop_info},

	{"lmd_flagplayer", lmd_flagplayer, Logical_True, &lmd_flagplayer_info},

	{"lmd_gate", lmd_gate, Logical_True, &lmd_gate_info},
	{"t2_gate", lmd_gate, Logical_True},

	{"lmd_gravity", lmd_gravity, Logical_True, &lmd_gravity_info},
	{"target_gravity_change", lmd_gravity, Logical_True, &lmd_gravity_info},

	{"lmd_speed", lmd_speed, Logical_True, &lmd_speed_info},

	{"lmd_iobject", lmd_iobject, Logical_True, &lmd_iobject_info},

	{"lmd_light", lmd_light, Logical_False, &lmd_light_info},
	{"t2_light", lmd_light, Logical_False},

	{"lmd_mover", lmd_mover, Logical_False, &lmd_mover_info},

	{"lmd_playercheck", lmd_playercheck, Logical_True, &lmd_playercheck_info},

	{"lmd_propertyterminal", lmd_propertyterminal, Logical_False, &lmd_propertyterminal_info},
	{"t2_propertyterminal", lmd_propertyterminal, Logical_False},

	{"lmd_property", lmd_property, Logical_True, &lmd_property_info},

	{"lmd_pwterminal", lmd_pwterminal, Logical_False, &lmd_pwterminal_info},
	{"t2_pwterminal", lmd_pwterminal, Logical_False},

	{"lmd_remap", lmd_remap, Logical_True, &lmd_remap_info},

	{"lmd_rentterminal", lmd_rentterminal, Logical_False, &lmd_rentterminal_info},
	{"t2_rentterminal", lmd_rentterminal, Logical_False},

	{"lmd_restrict", lmd_restrict, Logical_True, &lmd_restrict_info},

	{"lmd_scale", lmd_scale, Logical_True, &lmd_scale_info},

	{"lmd_stashdepo", lmd_stashdepo, Logical_False},
	{"lmd_stashspawnpoint", lmd_stashspawnpoint, Logical_True},
	{"lmd_stashzone", lmd_stashzone, Logical_False},

	{"lmd_terminal", lmd_terminal, Logical_False, &lmd_terminal_info},
	{"t2_terminal", lmd_terminal, Logical_False},

	{"lmd_toggle", lmd_toggle, Logical_True, &lmd_toggle_info},
	{"t2_toggle", lmd_toggle, Logical_True},

	{"lmd_train", lmd_train, Logical_False, &lmd_train_info},

	{"lmd_customskill", lmd_customskill, Logical_True, &lmd_customskill_info},

	{"lmd_event", lmd_event, Logical_True, &lmd_event_info},

	{"lmd_interact", lmd_interact, Logical_True, &lmd_interact_info},
	
	
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start, Logical_True, &info_player_start_info},
	{"info_player_jail", SP_info_player_jail, Logical_True, &info_player_jail_info},
	{"info_player_duel", SP_info_player_duel, Logical_True, &info_player_duel_info},
	{"info_player_duel1", SP_info_player_duel1, Logical_True, &info_player_duel1_info},
	{"info_player_duel2", SP_info_player_duel2, Logical_True, &info_player_duel2_info},
	{"info_player_deathmatch", SP_info_player_deathmatch, Logical_True, &info_player_deathmatch_info},
	{"info_player_siegeteam1", SP_info_player_siegeteam1, Logical_True},
	{"info_player_siegeteam2", SP_info_player_siegeteam2, Logical_True},
	{"info_player_intermission", SP_info_player_intermission, Logical_True},
	{"info_player_intermission_red", SP_info_player_intermission_red, Logical_True},
	{"info_player_intermission_blue", SP_info_player_intermission_blue, Logical_True},
	{"info_jedimaster_start", SP_info_jedimaster_start, Logical_True},
	{"info_player_start_red", SP_info_player_start_red, Logical_True},
	{"info_player_start_blue", SP_info_player_start_blue, Logical_True},
	{"info_null", SP_info_null, Logical_True},
	{"info_notnull", SP_info_notnull, Logical_True},		// use target_position instead
	{"info_camp", SP_info_camp, Logical_True},

	{"info_siege_objective", SP_info_siege_objective, Logical_False},
	{"info_siege_radaricon", SP_info_siege_radaricon, Logical_False},
	{"info_siege_decomplete", SP_info_siege_decomplete, Logical_True},
	{"target_siege_end", SP_target_siege_end, Logical_True},
	{"misc_siege_item", SP_misc_siege_item, Logical_False},

	{"func_plat", SP_func_plat, Logical_False, &func_plat_info},
	{"func_button", SP_func_button, Logical_False},
	{"func_door", SP_func_door, Logical_False},
	{"func_static", SP_func_static, Logical_False},
	{"func_rotating", SP_func_rotating, Logical_False},
	{"func_bobbing", SP_func_bobbing, Logical_False},
	{"func_pendulum", SP_func_pendulum, Logical_False},
	{"func_train", SP_func_train, Logical_False},
	{"func_group", SP_info_null, Logical_False},
	{"func_timer", SP_func_timer, Logical_False},			// rename trigger_timer?
	{"func_breakable", SP_func_breakable, Logical_False},
	{"func_glass", SP_func_glass, Logical_False},
	{"func_usable", SP_func_usable, Logical_False},
	{"func_wall", SP_func_wall, Logical_False},

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).

	{"trigger_lightningstrike", SP_trigger_lightningstrike, Logical_True, &trigger_lightningstrike_info},
	{"trigger_once", SP_trigger_once, Logical_False},
	{"trigger_multiple", SP_trigger_multiple, {qtrue, trigger_multiple_allowlogical}, &trigger_multiple_info},
	{"trigger_push", SP_trigger_push, Logical_False, &trigger_push_info},

	{"trigger_space", SP_trigger_space, Logical_False, &trigger_space_info},
	{"trigger_shipboundary", SP_trigger_shipboundary, Logical_False},
	{"trigger_hyperspace", SP_trigger_hyperspace, Logical_False},
	{"trigger_asteroid_field", SP_trigger_asteroid_field, Logical_False},

	{"trigger_teleport", SP_trigger_teleport, Logical_False},
	{"trigger_hurt", SP_trigger_hurt, Logical_False},
	{"trigger_always", SP_trigger_always, Logical_True},
	{"trigger_visible", SP_trigger_visible, Logical_True},


	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give, Logical_True},
	{"target_remove_powerups", SP_target_remove_powerups, Logical_True},
	{"target_delay", SP_target_delay, Logical_True},
	{"target_speaker", SP_target_speaker, {qtrue, target_speaker_allowlogical}},
	{"target_print", SP_target_print, Logical_True},
	{"target_laser", SP_target_laser, Logical_True},
	{"target_score", SP_target_score, Logical_True},
	{"target_teleporter", SP_target_teleporter, Logical_True},
	{"target_relay", SP_target_relay, Logical_True},
	{"target_kill", SP_target_kill, Logical_True},
	{"target_position", SP_target_position, Logical_True},
	{"target_location", SP_target_location, Logical_True},
	{"target_counter", SP_target_counter, Logical_True},
	{"target_random", SP_target_random, Logical_True},
	{"target_scriptrunner", SP_target_scriptrunner, qfalse},
	{"target_interest", SP_target_interest, Logical_True},
	{"target_activate", SP_target_activate, Logical_True},
	{"target_deactivate", SP_target_deactivate, Logical_True},
	{"target_level_change", SP_target_level_change, Logical_True},
	{"target_play_music", SP_target_play_music, Logical_True},
	{"target_push", SP_target_push, Logical_True},
	{"target_powerup", SP_target_powerup, Logical_True}, //Lugormod

	{"light", SP_light, Logical_True},
	{"path_corner", SP_path_corner, Logical_True},
		
	{"misc_teleporter_dest", SP_misc_teleporter_dest, Logical_True},
	{"defender", SP_defender, Logical_False},
	{"misc_security_panel", SP_misc_security_panel, Logical_False},
	{"misc_stuff", SP_misc_model_breakable, Logical_False}, //Lugormod
	{"misc_exploding_crate", SP_misc_exploding_crate, Logical_False}, //Lugormod
	{"misc_model", SP_misc_model, Logical_False},
	{"misc_model_static", SP_misc_model_static, Logical_False},
	{"misc_model_breakable", SP_misc_model_breakable, Logical_False},
	//{"misc_model_breakable", SP_misc_model_static},
	{"misc_G2model", SP_misc_G2model, Logical_False},
	{"misc_portal_surface", SP_misc_portal_surface, Logical_False},
	{"misc_portal_camera", SP_misc_portal_camera, Logical_False},
	{"misc_weather_zone", SP_misc_weather_zone, Logical_False},

	{"misc_bsp", SP_misc_bsp, Logical_False},
	{"terrain", SP_terrain, Logical_False},
	{"misc_skyportal_orient", SP_misc_skyportal_orient, Logical_True},
	{"misc_skyportal", SP_misc_skyportal, Logical_True},

	//rwwFIXMEFIXME: only for testing rmg team stuff
	{"gametype_item", SP_gametype_item, Logical_False},

	{"misc_ammo_floor_unit", SP_misc_ammo_floor_unit, Logical_False},
	{"misc_shield_floor_unit", SP_misc_shield_floor_unit, Logical_False},
	{"misc_model_shield_power_converter", SP_misc_model_shield_power_converter, Logical_False},
	{"misc_model_ammo_power_converter", SP_misc_model_ammo_power_converter, Logical_False},
	{"misc_model_health_power_converter", SP_misc_model_health_power_converter, Logical_False},

	{"fx_runner", SP_fx_runner, Logical_False},

	{"target_screenshake", SP_target_screenshake, Logical_True},
	{"target_escapetrig", SP_target_escapetrig, Logical_True},

	{"misc_maglock", SP_misc_maglock, Logical_False},

	{"misc_faller", SP_misc_faller, Logical_True},

	{"ref_tag",	SP_reference_tag, Logical_True},
	{"ref_tag_huge", SP_reference_tag, Logical_True},

	{"misc_weapon_shooter", SP_misc_weapon_shooter, Logical_True},

	{"lmd_spawner", SP_LMD_spawner, Logical_True},

	//new NPC ents
	{"NPC_spawner", SP_NPC_spawner, Logical_True},
	{"NPC_Vehicle", SP_NPC_Vehicle, Logical_True},
	{"NPC_Kyle", SP_NPC_Kyle, Logical_True},
	{"NPC_Lando", SP_NPC_Lando, Logical_True},
	{"NPC_Jan", SP_NPC_Jan, Logical_True},
	{"NPC_Luke", SP_NPC_Luke, Logical_True},
	{"NPC_MonMothma", SP_NPC_MonMothma, Logical_True},
	{"NPC_Tavion", SP_NPC_Tavion, Logical_True},
	
	//new tavion
	{"NPC_Tavion_New", SP_NPC_Tavion_New, Logical_True},

	//new alora
	{"NPC_Alora", SP_NPC_Alora, Logical_True},

	{"NPC_Reelo", SP_NPC_Reelo, Logical_True},
	{"NPC_Galak", SP_NPC_Galak, Logical_True},
	{"NPC_Desann", SP_NPC_Desann, Logical_True},
	{"NPC_Bartender", SP_NPC_Bartender, Logical_True},
	{"NPC_MorganKatarn", SP_NPC_MorganKatarn, Logical_True},
	{"NPC_Jedi", SP_NPC_Jedi, Logical_True },
	{"NPC_Prisoner", SP_NPC_Prisoner, Logical_True },
	{"NPC_Rebel", SP_NPC_Rebel, Logical_True },
	{"NPC_Stormtrooper", SP_NPC_Stormtrooper, Logical_True },
	{"NPC_StormtrooperOfficer", SP_NPC_StormtrooperOfficer, Logical_True },
	{"NPC_Snowtrooper", SP_NPC_Snowtrooper, Logical_True },
	{"NPC_Tie_Pilot", SP_NPC_Tie_Pilot, Logical_True },
	{"NPC_Ugnaught", SP_NPC_Ugnaught, Logical_True },
	{"NPC_Jawa", SP_NPC_Jawa, Logical_True },
	{"NPC_Gran", SP_NPC_Gran, Logical_True },
	{"NPC_Rodian", SP_NPC_Rodian, Logical_True },
	{"NPC_Weequay", SP_NPC_Weequay, Logical_True },
	{"NPC_Trandoshan", SP_NPC_Trandoshan, Logical_True },
	{"NPC_Tusken", SP_NPC_Tusken, Logical_True },
	{"NPC_Noghri", SP_NPC_Noghri, Logical_True },
	{"NPC_SwampTrooper", SP_NPC_SwampTrooper, Logical_True },
	{"NPC_Imperial", SP_NPC_Imperial, Logical_True },
	{"NPC_ImpWorker", SP_NPC_ImpWorker, Logical_True },
	{"NPC_BespinCop", SP_NPC_BespinCop, Logical_True },
	{"NPC_Reborn", SP_NPC_Reborn, Logical_True },
	{"NPC_ShadowTrooper", SP_NPC_ShadowTrooper, Logical_True },
	{"NPC_Monster_Murjj", SP_NPC_Monster_Murjj, Logical_True },
	{"NPC_Monster_Swamp", SP_NPC_Monster_Swamp, Logical_True },
	{"NPC_Monster_Howler", SP_NPC_Monster_Howler, Logical_True },
	{"NPC_MineMonster",	SP_NPC_MineMonster, Logical_True },
	{"NPC_Monster_Claw", SP_NPC_Monster_Claw, Logical_True },
	{"NPC_Monster_Glider", SP_NPC_Monster_Glider, Logical_True },
	{"NPC_Monster_Flier2", SP_NPC_Monster_Flier2, Logical_True },
	{"NPC_Monster_Lizard", SP_NPC_Monster_Lizard, Logical_True },
	{"NPC_Monster_Fish", SP_NPC_Monster_Fish, Logical_True },
	{"NPC_Monster_Wampa", SP_NPC_Monster_Wampa, Logical_True },
	{"NPC_Monster_Rancor", SP_NPC_Monster_Rancor, Logical_True },
	{"NPC_Droid_Interrogator", SP_NPC_Droid_Interrogator, Logical_True },
	{"NPC_Droid_Probe", SP_NPC_Droid_Probe, Logical_True },
	{"NPC_Droid_Mark1", SP_NPC_Droid_Mark1, Logical_True },
	{"NPC_Droid_Mark2", SP_NPC_Droid_Mark2, Logical_True },
	{"NPC_Droid_ATST", SP_NPC_Droid_ATST, Logical_True },
	{"NPC_Droid_Seeker", SP_NPC_Droid_Seeker, Logical_True },
	{"NPC_Droid_Remote", SP_NPC_Droid_Remote, Logical_True },
	{"NPC_Droid_Sentry", SP_NPC_Droid_Sentry, Logical_True },
	{"NPC_Droid_Gonk", SP_NPC_Droid_Gonk, Logical_True },
	{"NPC_Droid_Mouse", SP_NPC_Droid_Mouse, Logical_True },
	{"NPC_Droid_R2D2", SP_NPC_Droid_R2D2, Logical_True },
	{"NPC_Droid_R5D2", SP_NPC_Droid_R5D2, Logical_True },
	{"NPC_Droid_Protocol", SP_NPC_Droid_Protocol, Logical_True },

	//maybe put these guys in some day, for now just spawn reborns in their place.
	{"NPC_Reborn_New", SP_NPC_Reborn_New, Logical_True },
	{"NPC_Cultist", SP_NPC_Cultist, Logical_True },
	{"NPC_Cultist_Saber", SP_NPC_Cultist_Saber, Logical_True },
	{"NPC_Cultist_Saber_Powers", SP_NPC_Cultist_Saber_Powers, Logical_True },
	{"NPC_Cultist_Destroyer", SP_NPC_Cultist_Destroyer, Logical_True },
	{"NPC_Cultist_Commando", SP_NPC_Cultist_Commando, Logical_True },

	//rwwFIXMEFIXME: Faked for testing NPCs (another other things) in RMG with sof2 assets
	{"NPC_Colombian_Soldier", SP_NPC_Reborn, Logical_True },
	{"NPC_Colombian_Rebel", SP_NPC_Reborn, Logical_True },
	{"NPC_Colombian_EmplacedGunner", SP_NPC_ShadowTrooper, Logical_True },
	{"NPC_Manuel_Vergara_RMG", SP_NPC_Desann, Logical_True },
//	{"info_NPCnav", SP_waypoint},
	//Lugormod missing NPCs
	{"NPC_HazardTrooper", SP_NPC_HazardTrooper, Logical_True },
	{"NPC_Saboteur", SP_NPC_Saboteur, Logical_True },

	{"waypoint", SP_waypoint, qtrue},
	{"waypoint_small", SP_waypoint_small, Logical_True},
	{"waypoint_navgoal", SP_waypoint_navgoal, Logical_True},
	{"waypoint_navgoal_8", SP_waypoint_navgoal_8, Logical_True},
	{"waypoint_navgoal_4", SP_waypoint_navgoal_4, Logical_True},
	{"waypoint_navgoal_2", SP_waypoint_navgoal_2, Logical_True},
	{"waypoint_navgoal_1", SP_waypoint_navgoal_1, Logical_True},

	{"fx_spacedust", SP_CreateSpaceDust, Logical_True},
	{"fx_rain", SP_CreateRain, Logical_True},
	{"fx_snow", SP_CreateSnow, Logical_True},
	//{"fx_wind", SP_CreateWind}, //Lugormod

	{"point_combat", SP_point_combat, Logical_True},

	{"misc_holocron", SP_misc_holocron, Logical_False},

	{"shooter_blaster", SP_shooter_blaster, Logical_True},

	{"team_CTF_redplayer", SP_team_CTF_redplayer, Logical_True},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer, Logical_True},

	{"team_CTF_redspawn", SP_team_CTF_redspawn, Logical_True},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn, Logical_True},

	{"item_botroam", SP_item_botroam, Logical_True},

	{"emplaced_gun", SP_emplaced_gun, Logical_False},

	{ "misc_turret", SP_misc_turret, Logical_False, &misc_turret_info },
	{"misc_turretG2", SP_misc_turretG2, Logical_False},
	{"misc_camera", SP_misc_camera, Logical_False},//Lugormod
	{"random_spot", SP_random_spot, Logical_True},//Lugormod
	{"money_dispenser", SP_money_dispenser, Logical_False},//Lugormod
	{"control_point", SP_control_point, Logical_False},//Lugormod
	{"emplaced_eweb", SP_emplaced_gun, Logical_False},//Lugormod
	{"target_fixdoor", SP_target_fixdoor, Logical_False}, //Lugormod
	{"target_credits", SP_target_credits, Logical_True}, //Lugormod
	{"rail_mover", SP_rail_mover, Logical_False}, //Lugormod
	{"rail_track", SP_rail_track, Logical_True}, //Lugormod
	{"rail_lane", SP_rail_lane, Logical_True}, //Lugormod
	{"misc_slotmachine", SP_misc_slotmachine, Logical_False}, //Lugormod
	{"ghost_exit_red", SP_ghost_exit, Logical_True}, //Lugormod
	{"ghost_exit_blue", SP_ghost_exit, Logical_True}, //Lugormod
	
	//Ufo:	
	{"target_modify", sp_target_modify, Logical_True, &target_modify_info},
	{"lmd_cskill_compare", lmd_cskill_compare, Logical_True, &lmd_cskill_compare_info},
	{"lmd_countcheck", lmd_countcheck, Logical_True, &lmd_countcheck_info},
	{"lmd_iterateplayers", lmd_iterateplayers, Logical_True, &lmd_iterateplayers_info},
	{"target_heal", sp_target_heal, Logical_True, &target_heal_info},
	{"lmd_actor_modify", lmd_actor_modify, Logical_True, &lmd_actor_modify_info},
	{"target_animate", sp_target_animate, Logical_True, &target_animate_info},

	{NULL, 0, Logical_False}
};

void InitializeSpawnTable() {
	spawn_t *spawn = spawnInitValues;
	while(spawn->name) {
		Lmd_AddSpawnableEntry(*spawn);
		spawn++;
	}
}

//RoboPhred
qboolean isValidClassname(const char *classname) {
	int i;
	for(i = 0; i < bg_numItems; i++) {
		if(Q_stricmp(classname, bg_itemlist[i].classname) == 0)
			return qtrue;
	}

	for(i = 0; i < spawnTable.count; i++) {
		if(Q_stricmp(spawnTable.spawns[i].name, classname) == 0)
			return qtrue;
	}
	return qfalse;
}

//RoboPhred
gentity_t* AimAnyTarget (gentity_t *ent, int length);
extern entityInfoData_t model_key_description;
extern entityInfoData_t hitbox_key_description;
extern entityInfoData_t usable_key_description[];
void Cmd_Entityinfo_t(gentity_t *ent, int iArg) {
	int c = 0;
	const entityInfoData_t *data;
#ifdef LMD_EXPERIMENTAL
	if(!ent) {
		int i = 0;
		const char *buf;
		fileHandle_t file;
		spawn_t *spawn = spawnInitValues;
#ifdef LMD_EXPORT_XML
		trap_FS_FOpenFile("docs/entities.xml", &file, FS_WRITE);
		buf = "<entities>\r\n";
		trap_FS_Write(buf, strlen(buf), file);
		while(spawn->name) {
			buf = va("\t<entity classname=\"%s\" logical=\"%s\">\r\n", spawn->name, spawn->logical.allow ? "true" : "false");
			trap_FS_Write(buf, strlen(buf), file);
			if(spawn->info){
				if(spawn->info->description != NULL) {
					buf = va("\t\t<description>%s</description>\r\n", spawn->info->description);
					trap_FS_Write(buf, strlen(buf), file);
				}
				if(spawn->info->spawnflags != NULL) {
					i = 0;
					buf = "\t\t<spawnflags>\r\n";
					trap_FS_Write(buf, strlen(buf), file);
					for(data = spawn->info->spawnflags; data->key != NULL; data++) {
						buf = va("\t\t\t<spawnflag index=\"%s\">%s</spawnflag>\r\n", data->key, data->value);
						trap_FS_Write(buf, strlen(buf), file);
					}

					buf = "\t\t</spawnflags>\r\n";
					trap_FS_Write(buf, strlen(buf), file);
				}
				if(spawn->info->keys != NULL) {
					buf = "\t\t<keys>\r\n";
					trap_FS_Write(buf, strlen(buf), file);
					for(data = spawn->info->keys; data->key != NULL; data++) {
						if(Q_stricmp(data->key, "#UKEYS") == 0) {
							const entityInfoData_t *d2;
							for(d2 = usable_key_description; d2->key != NULL; d2++) {
								buf = va("\t\t\t<key name=\"%s\">%s</key>\r\n", d2->key, d2->value);
								trap_FS_Write(buf, strlen(buf), file);
							}
						}
						else if(Q_stricmp(data->key, "#MODEL") == 0) {
							buf = va("\t\t\t<key name=\"%s\">%s</key>\r\n", model_key_description.key, model_key_description.value);
							trap_FS_Write(buf, strlen(buf), file);
						}
						else if(Q_stricmp(data->key, "#HITBOX") == 0) {
							buf = va("\t\t\t<key name=\"%s\">%s</key>\r\n", hitbox_key_description.key, hitbox_key_description.value);
							trap_FS_Write(buf, strlen(buf), file);
						}
						else {
							buf = va("\t\t\t<key name=\"%s\">%s</key>\r\n", data->key, data->value);
							trap_FS_Write(buf, strlen(buf), file);
						}
					}
					buf = "\t\t</keys>\r\n";
					trap_FS_Write(buf, strlen(buf), file);
				}
			}
			buf = "\t</entity>\r\n";
			trap_FS_Write(buf, strlen(buf), file);
			spawn++;
		}
		buf = "</entities>\r\n";
		trap_FS_Write(buf, strlen(buf), file);
#else
		trap_FS_FOpenFile("docs/entities.csv", &file, FS_WRITE);
		buf = "name,logical,descr,spawnflagIndex,spawnflagDescr,keyName,keyDescr,\n";
		trap_FS_Write(buf, strlen(buf), file);
		//name,logical,descr,spawnflagIndex,spawnflagDescr,keyName,keyDescr,
		while(spawn->name) {
			//name,logical,
			buf = va("\"%s\",\"%s\",", spawn->name, spawn->logical.allow ? "logical" : "normal");
			trap_FS_Write(buf, strlen(buf), file);

			//descr,
			if(spawn->info && spawn->info->description != NULL) {
				buf = va("\"%s\",", spawn->info->description);
				trap_FS_Write(buf, strlen(buf), file);
			}
			else
				trap_FS_Write(",", 1, file);

			//spawnflagIndex,spawnflagDescr,
			if(spawn->info && spawn->info->spawnflags != NULL) {
				i = 0;
				for(data = spawn->info->spawnflags; data->key != NULL; data++) {
					buf = va("\"%s\"", data->key);
					trap_FS_Write(buf, strlen(buf), file);
					if((data + 1)->key != NULL)
						trap_FS_Write("|", 1, file);
				}

				trap_FS_Write(",", 1, file);

				for(data = spawn->info->spawnflags; data->key != NULL; data++) {
					buf= va("\"%s\"", data->value);
					trap_FS_Write(buf, strlen(buf), file);
					if((data + 1)->key != NULL)
						trap_FS_Write("|", 1, file);
				}

				trap_FS_Write(",", 1, file);
			}
			else
				trap_FS_Write(",,", 2, file);

			//keyName, keyDescr,
			if(spawn->info && spawn->info->keys != NULL) {
				for(data = spawn->info->keys; data->key != NULL; data++) {
					if(Q_stricmp(data->key, "#UKEYS") == 0) {
						const entityInfoData_t *d2;
						for(d2 = usable_key_description; d2->key != NULL; d2++) {
							buf = va("%s", d2->key);
							trap_FS_Write(buf, strlen(buf), file);
							if((data + 1)->key != NULL)
								trap_FS_Write("|", 1, file);
						}
					}
					else if(Q_stricmp(data->key, "#MODEL") == 0) {
						buf = va("\"%s\"", model_key_description.key);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
					else if(Q_stricmp(data->key, "#HITBOX") == 0) {
						buf = va("\"%s\"", hitbox_key_description.key);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
					else {
						buf = va("\"%s\"", data->key);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
				}
				
				trap_FS_Write(",", 3, file);

				for(data = spawn->info->keys; data->key != NULL; data++) {
					if(Q_stricmp(data->key, "#UKEYS") == 0) {
						const entityInfoData_t *d2;
						for(d2 = usable_key_description; d2->key != NULL; d2++) {
							buf = va("\"%s\"", d2->value);
							trap_FS_Write(buf, strlen(buf), file);
							if((data + 1)->key != NULL)
								trap_FS_Write("|", 1, file);
						}
					}
					else if(Q_stricmp(data->key, "#MODEL") == 0) {
						buf = va("\"%s\"", model_key_description.value);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
					else if(Q_stricmp(data->key, "#HITBOX") == 0) {
						buf = va("\"%s\"", hitbox_key_description.value);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
					else {
						buf = va("\"%s\"", data->value);
						trap_FS_Write(buf, strlen(buf), file);
						if((data + 1)->key != NULL)
							trap_FS_Write("|", 1, file);
					}
				}

				trap_FS_Write(",", 2, file);

			}
			else
				trap_FS_Write(",,", 2, file);
			trap_FS_Write("\n", 1, file);
			spawn++;
		}
#endif
		trap_FS_FCloseFile(file);
	}
#else
	if(0) {

	}
#endif
	else {
		spawn_t *f;
		qboolean color = qfalse, header = qfalse;
		int len;
		int i;
		char arg[MAX_STRING_CHARS];
		char compare[MAX_STRING_CHARS];
		if(trap_Argc() < 2) {
			gentity_t *targ = AimAnyTarget(ent, 8192);
			if(!targ) {
				Disp(ent, "^3Target is not an entity.");
				return;
			}
			Q_strncpyz(arg, targ->classname, sizeof(arg));
		}
		else
			trap_Argv(1, arg, sizeof(arg));
		Q_strlwr(arg);
		len = strlen(arg);
		for(i = 0; i < spawnTable.count; i++) {
			Q_strncpyz(compare, spawnTable.spawns[i].name, sizeof(compare));
			Q_strlwr(compare);
			if(strstr(compare, arg) != NULL) {
				f = &spawnTable.spawns[i];
				c++;
				if (len == strlen(compare)) {
					// Exact match
					break;
				}

				Disp(ent, va("^5%s", f->name));
			}
		}

		//For some odd reason, they want index 0 to be empty...
		for(i = 1; i < bg_numItems; i++) {
			Q_strncpyz(compare, bg_itemlist[i].classname, sizeof(compare));
			Q_strlwr(compare);
			if(bg_itemlist[i].classname && strstr(compare, arg) != NULL) {
				Disp(ent, va("^5%s", bg_itemlist[i].classname));
				c++;
			}
		}

		if(c > 1) {
			Disp(ent, va("^2%i^3 entities match that name.  Narrow the search down to get more information on an entity.", c));
			return;
		}
		else if(c <= 0) {
			Disp(ent, "^3No entities found.");
			return;
		}
		
		if(!f) {
			Disp(ent, "^3This entity is not part of the standard spawn system, and has no information available.");
			return;
		}

		spawn_t *spawn = f;
		if(spawn->logical.allow) {
			Disp(ent, "^3This entity may not use an entity slot.");
		}
		if(spawn->info) {
			if(spawn->info->spawnflags != NULL) {
				color = qfalse;
				header = qfalse;
				for(data = spawn->info->spawnflags; data->key != NULL; data++) {
					if(!data->key == 0)
						break;
					if(!header) {
						Disp(ent, "^2Spawnflags ===============================================");
						header = qtrue;
					}
					Disp(ent, va("%s%s: %s", (color)?"^3":"", data->key, data->value));
					color = !color;
				}
			}
			if(spawn->info->keys != NULL) {
				color = qfalse;
				header = qfalse;
				for(data = spawn->info->keys; data->key != NULL; data++) {
					if(!header) {
						Disp(ent, "^2Keys =====================================================");
						header = qtrue;
					}
					if(Q_stricmp(data->key, "#UKEYS") == 0) {
						const entityInfoData_t *d2;
						for(d2 = usable_key_description; d2->key != NULL; d2++)
							Disp(ent, va("%s%s: %s", (color)?"^3":"", d2->key, d2->value));
					}
					else if(Q_stricmp(data->key, "#MODEL") == 0) {
						Disp(ent, va("%s%s: %s", (color)?"^3":"", model_key_description.key, model_key_description.value));
					}
					else if(Q_stricmp(data->key, "#HITBOX") == 0) {
						Disp(ent, va("%s%s: %s", (color)?"^3":"", hitbox_key_description.key, hitbox_key_description.value));
					}
					else
						Disp(ent, va("%s%s: %s", (color)?"^3":"", data->key, data->value));
					color = !color;
				}
			}
			if(spawn->info->description != NULL) {
				Disp(ent, "^2Description ==============================================");
				Disp(ent, va("^3%s", spawn->info->description));
			}
		}
		else {
			Disp(ent, "^3No information is available for this entity.");
		}
	}
}




/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;
	if (!string) {
				return NULL;
		}
		
	l = strlen(string) + 1;

#ifdef LMD_SPAWN_HEAPCHECK
			assert(_CrtCheckMemory());
#endif
	newb = (char *) G_Alloc( l );
#ifdef LMD_SPAWN_HEAPCHECK
			assert(_CrtCheckMemory());
#endif

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		//RoboPhred
		if(string[i] == '\\' && i < l-1 && string[i+1] == 'n'){
			i++;
			*new_p++ = '\n';
		}
		else
			*new_p++ = string[i];
		/*
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
		*/
	}
	
	return newb;
}





/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
#include "../namespace_begin.h"
//RoboPhred
qboolean BG_ParseField( BG_field_t *l_fields, const char *key, const char *value, void *target );
//void BG_ParseField( BG_field_t *l_fields, const char *key, const char *value, byte *ent );
#include "../namespace_end.h"
//RoboPhred
gentity_t *Lmd_logic_spawn();

gentity_t* G_SpawnGEntityPtrFromSpawnVars(gentity_t *ent, SpawnData_t *spawnData, qboolean inSubBSP) {
//void G_SpawnGEntityFromSpawnVars( qboolean inSubBSP ) {
	int			i;
	char		*s = NULL, *value = NULL, *gametypeName = NULL;
	static char *gametypeNames[] = {"ffa", "holocron", "jedimaster", "duel", "powerduel", "single", "team", "siege", "ctf", "cty", "battleground", "saberrun", "reborn"};

	spawn_t *type = NULL;
#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif
	G_SpawnString("classname", "", &s);
#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif
	//tmp = G_NewString2("info_notnull");
	for(int i = 0; i < spawnTable.count; i++) {
		if(Q_stricmp(spawnTable.spawns[i].name, s) == 0){
			type = &spawnTable.spawns[i];
			break;
		}
	}
	G_Free(s);
	s = NULL;


	if(!ent){
		qboolean restrictLogical = qfalse;
		BG_field_t *field = fields;
		while(field->name) {
			if(field->flags & ENTFIELD_NOLOGIC) {
				for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
					if ( !Q_stricmp( field->name, level.spawnVars[i][0] ) ) {
						restrictLogical = qtrue;
						break;
					}
				}
			}
			field++;
		}
		//Items don't have a type, so type would be the empty entry (type->name == NULL).
		if(type && (type->logical.allow && (type->logical.check == NULL || type->logical.check())) && !restrictLogical){
			ent = Lmd_logic_spawn();
		}
		else
			ent = G_Spawn();
	}
	if(!ent)
		return NULL; //we couldn't get an entity

#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif

	ent->Lmd.spawnData = spawnData;


	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		BG_ParseField( fields, level.spawnVars[i][0], level.spawnVars[i][1], (void *)ent );
	}

	// check for "notsingle" flag
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return ent;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_DUEL, GT_SINGLE_PLAYER)
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return ent;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return ent;
		}
	}

	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return ent;
	}

	if( G_SpawnString( "gametype", NULL, &value ) ) {
		if( g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if( !s ) {
				G_FreeEntity( ent );
				return ent;
			}
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );



#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif

	if (g_gametype.integer == GT_GHOST) {             
		if (!Q_stricmp(ent->classname, "team_CTF_redflag")){
			G_Free(ent->classname);
			ent->classname = "ghost_exit_red";
		}
		else if (!Q_stricmp(ent->classname, "team_CTF_blueflag")){
			G_Free(ent->classname);
			ent->classname = "ghost_exit_blue";
		}
	}

	if(!type){
		//No type, must be an item.
		gitem_t *item;
		for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
			if ( !Q_stricmp(item->classname, ent->classname) ) {
				if (gameMode(GM_ROCKET_ARENA)) {
					if (item->giType == IT_WEAPON) {
						G_SpawnItem( ent, bg_itemlist + BG_GetItemIndexByTag(WP_ROCKET_LAUNCHER, IT_WEAPON));
						return ent;
					}
					else if (item->giType == IT_AMMO) {
						G_SpawnItem( ent, bg_itemlist + BG_GetItemIndexByTag(AMMO_ROCKETS, IT_AMMO));
						return ent;
					}
				}
				else if (gameMode(GM_SNIPER_ARENA)) {
					if (item->giType == IT_WEAPON) {
						G_SpawnItem( ent, bg_itemlist + BG_GetItemIndexByTag(WP_DISRUPTOR, IT_WEAPON));
						return ent;
					}
					else if (item->giType == IT_AMMO) {
						G_SpawnItem( ent, bg_itemlist + BG_GetItemIndexByTag(AMMO_POWERCELL, IT_AMMO));
						return ent;
					}
				} 
				G_SpawnItem( ent, item );
				return ent;
			}
		}

		//Lugormod
		if (ent->NPC_target) {
			//NPC_targetname seems to be a target, not targetname
			G_Printf ("%s doesn't have a spawn function but has NPC_target, converting to trigger_visible\n", ent->classname);
			ent->target = ent->NPC_target;
			ent->classname = "trigger_visible";
			SP_trigger_visible(ent);
			return ent;
		}
		G_Printf ("%s doesn't have a spawn function\n", ent->classname);
		G_FreeEntity(ent);
#ifdef LMD_SPAWN_HEAPCHECK
		assert(_CrtCheckMemory());
#endif
		return ent;
	}
	else{
		//Lugormod check if NPC (assuming name starts with NPC_)
		if (g_gametype.integer != GT_SIEGE && !spawnData && disablesenabled && g_dontLoadNPC.integer &&
			Q_strncmp(ent->classname,"NPC_", 4) == 0){

			if (ent->NPC_target) {
				ent->behaviorSet[BSET_SPAWN] = NULL;
				ent->NPC = NULL;
				ent->target = ent->NPC_target;
				ent->classname = "trigger_always";
				SP_trigger_always(ent);
				return ent;
			}
			G_FreeEntity(ent);
			return ent;
		}

		if (ent->healingsound && ent->healingsound[0]){
			//yeah...this can be used for anything, so.. precache it if it's there
			G_SoundIndex(ent->healingsound);
		}

#ifdef LMD_SPAWN_HEAPCHECK
		assert(_CrtCheckMemory());
#endif

		type->spawn(ent);
	}


#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif

	//Tag on the ICARUS scripting information only to valid recipients             
	if(trap_ICARUS_ValidEnt(ent)){                
		trap_ICARUS_InitEnt( ent );

		if ( ent->classname && ent->classname[0] ){
			if ( Q_strncmp( "NPC_", ent->classname, 4 ) != 0 ){
				//Not an NPC_spawner (rww - probably don't even care for MP, but whatever)
				G_ActivateBehavior( ent, BSET_SPAWN );
			}
		}
	}
#ifdef LMD_SPAWN_HEAPCHECK
	assert(_CrtCheckMemory());
#endif
	return ent;
}

gentity_t* G_SpawnGEntityFromSpawnVars(qboolean inSubBSP) {
	return G_SpawnGEntityPtrFromSpawnVars(NULL, NULL, inSubBSP);
}
void AddSpawnField(char *field, char *value); //Lugormod need it up here

/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	//RoboPhred
	//Entities expect empty strings, not null strings.
	if(!string)
		return "";

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

void AddSpawnField(char *field, char *value)
{
	int	i;

	for(i=0;i<level.numSpawnVars;i++)
	{
		if (Q_stricmp(level.spawnVars[i][0], field) == 0)
		{
			level.spawnVars[ i ][1] = G_AddSpawnVarToken( value );
			return;
		}
	}

	level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( field );
	level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( value );
	level.numSpawnVars++;
}

#define NOVALUE "novalue"

static void HandleEntityAdjustment(void)
{
	char		*value = NULL;
	vec3_t		origin, newOrigin, angles;
	char		temp[MAX_QPATH];
	float		rotation;

	G_SpawnString("origin", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &origin[0], &origin[1], &origin[2] );
	}
	else
	{
		origin[0] = origin[1] = origin[2] = 0.0;
	}

	rotation = DEG2RAD(level.mRotationAdjust);
	newOrigin[0] = origin[0]*cos(rotation) - origin[1]*sin(rotation);
	newOrigin[1] = origin[0]*sin(rotation) + origin[1]*cos(rotation);
	newOrigin[2] = origin[2];
	VectorAdd(newOrigin, level.mOriginAdjust, newOrigin);
	// damn VMs don't handle outputing a float that is compatible with sscanf in all cases
	Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", newOrigin[0], newOrigin[1], newOrigin[2]);
	AddSpawnField("origin", temp);

	G_SpawnString("angles", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &angles[0], &angles[1], &angles[2] );

		angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
		// damn VMs don't handle outputing a float that is compatible with sscanf in all cases
		Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", angles[0], angles[1], angles[2]);
		AddSpawnField("angles", temp);
	}
	else
	{
		G_SpawnString("angle", NOVALUE, &value);
		if (Q_stricmp(value, NOVALUE) != 0)
		{
			sscanf( value, "%f", &angles[1] );
		}
		else
		{
			angles[1] = 0.0;
		}
		angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
		Com_sprintf(temp, MAX_QPATH, "%0.0f", angles[1]);
		AddSpawnField("angle", temp);
	}

	// RJR experimental code for handling "direction" field of breakable brushes
	// though direction is rarely ever used.
	G_SpawnString("direction", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &angles[0], &angles[1], &angles[2] );
	}
	else
	{
		angles[0] = angles[1] = angles[2] = 0.0;
	}
	angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
	Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", angles[0], angles[1], angles[2]);
	AddSpawnField("direction", temp);


	AddSpawnField("BSPInstanceID", level.mTargetAdjust);

	G_SpawnString("targetname", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("targetname", temp);
	}

	G_SpawnString("target", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("target", temp);
	}

	G_SpawnString("killtarget", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("killtarget", temp);
	}

	G_SpawnString("brushparent", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("brushparent", temp);
	}

	G_SpawnString("brushchild", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("brushchild", temp);
	}

	G_SpawnString("enemy", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("enemy", temp);
	}

	G_SpawnString("ICARUSname", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("ICARUSname", temp);
	}
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( qboolean inSubBSP ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	if (inSubBSP){
		HandleEntityAdjustment();
	}

	return qtrue;
}


static	char *defaultStyles[32][3] = 
{
	{	// 0 normal
		"z",
		"z",
		"z"
	},
	{	// 1 FLICKER (first variety)
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo"
	},
	{	// 2 SLOW STRONG PULSE
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb"
	},
	{	// 3 CANDLE (first variety)
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
	},
	{	// 4 FAST STROBE
		"mamamamamama",
		"mamamamamama",
		"mamamamamama"
	},
	{	// 5 GENTLE PULSE 1
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj"
	},
	{	// 6 FLICKER (second variety)
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno"
	},
	{	// 7 CANDLE (second variety)
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm"
	},
	{	// 8 CANDLE (third variety)
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa"
	},
	{	// 9 SLOW STROBE (fourth variety)
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz"
	},
	{	// 10 FLUORESCENT FLICKER
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma"
	},
	{	// 11 SLOW PULSE NOT FADE TO BLACK
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba"
	},
	{	// 12 FAST PULSE FOR JEREMY
		"mkigegik",
		"mkigegik",
		"mkigegik"
	},
	{	// 13 Test Blending
		"abcdefghijklmqrstuvwxyz",
		"zyxwvutsrqmlkjihgfedcba",
		"aammbbzzccllcckkffyyggp"
	},
	{	// 14
		"",
		"",
		""
	},
	{	// 15
		"",
		"",
		""
	},
	{	// 16
		"",
		"",
		""
	},
	{	// 17
		"",
		"",
		""
	},
	{	// 18
		"",
		"",
		""
	},
	{	// 19
		"",
		"",
		""
	},
	{	// 20
		"",
		"",
		""
	},
	{	// 21
		"",
		"",
		""
	},
	{	// 22
		"",
		"",
		""
	},
	{	// 23
		"",
		"",
		""
	},
	{	// 24
		"",
		"",
		""
	},
	{	// 25
		"",
		"",
		""
	},
	{	// 26
		"",
		"",
		""
	},
	{	// 27
		"",
		"",
		""
	},
	{	// 28
		"",
		"",
		""
	},
	{	// 29
		"",
		"",
		""
	},
	{	// 30
		"",
		"",
		""
	},
	{	// 31
		"",
		"",
		""
	}
};

void *precachedKyle = 0;
void scriptrunner_run (gentity_t *self);

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process

BSP Options
"gridsize"     size of lighting grid to "X Y Z". default="64 64 128"
"ambient"      scale of global light (from _color)
"fog"          shader name of the global fog texture - must include the full path, such as "textures/rj/fog1"
"distancecull" value for vis for the maximum viewing distance
"chopsize"     value for bsp on the maximum polygon / portal size
"ls_Xr"	override lightstyle X with this pattern for Red.
"ls_Xg"	green (valid patterns are "a-z")
"ls_Xb"	blue (a is OFF, z is ON)

"fogstart"		override fog start distance and force linear
"radarrange" for Siege/Vehicle radar - default range is 2500
*/
extern void EWebPrecache(void); //g_items.c
float g_cullDistance;
void SP_worldspawn( void ) 
{
	char		*text = NULL, temp[32];
	int			i;
	int			lengthRed, lengthBlue, lengthGreen;

	//I want to "cull" entities out of net sends to clients to reduce
	//net traffic on our larger open maps -rww
	G_SpawnFloat("distanceCull", "6000.0", &g_cullDistance);
	trap_SetServerCull(g_cullDistance);

	G_SpawnString( "classname", "", &text );
	if ( Q_stricmp( text, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	//RoboPhred
	G_SpawnInt("lavadamage", "30", &level.lavaDamage);
	G_SpawnInt("slimedamage", "10", &level.slimeDamage);

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) 
	{
		if ( Q_stricmp( "spawnscript", level.spawnVars[i][0] ) == 0 )
		{//ONly let them set spawnscript, we don't want them setting an angle or something on the world.
			BG_ParseField( fields, level.spawnVars[i][0], level.spawnVars[i][1], (void *)&g_entities[ENTITYNUM_WORLD] );
		}
	}
	//The server will precache the standard model and animations, so that there is no hit
	//when the first client connnects.
	if (!BGPAFtextLoaded)
	{
		BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue);
	}

	if (!precachedKyle)
	{
		int defSkin;

		trap_G2API_InitGhoul2Model(&precachedKyle, "models/players/kyle/model.glm", 0, 0, -20, 0, 0);

		if (precachedKyle)
		{
			defSkin = trap_R_RegisterSkin("models/players/kyle/model_default.skin");
			trap_G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
		}
	}

	if (!g2SaberInstance)
	{
		trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

		if (g2SaberInstance)
		{
			// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
			trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
			// now set up the gun bolt on it
			trap_G2API_AddBolt(g2SaberInstance, 0, "*blade1");
		}
	}

	if (g_gametype.integer == GT_SIEGE 
			|| g_gametype.integer == GT_BATTLE_GROUND)
	{ //a tad bit of a hack, but..
		EWebPrecache();
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &text );
	trap_SetConfigstring( CS_MUSIC, text );

	G_SpawnString( "message", "", &text );
	trap_SetConfigstring( CS_MESSAGE, text );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	G_SpawnString( "gravity", "800", &text );
	trap_Cvar_Set( "g_gravity", text );

	G_SpawnString( "enableBreath", "0", &text );
	trap_Cvar_Set( "g_enableBreath", text );

	G_SpawnString( "soundSet", "default", &text );
	trap_SetConfigstring( CS_GLOBAL_AMBIENT_SET, text );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";
	//RoboPhred
	g_entities[ENTITYNUM_WORLD].inuse = qtrue;

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} 
	/*
	else if ( g_doWarmup.integer && g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL ) { // Turn it on
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( "Warmup:\n" );
	}
	*/

	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+0, defaultStyles[0][0]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+1, defaultStyles[0][1]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+2, defaultStyles[0][2]);
	
	for(i=1;i<LS_NUM_STYLES;i++)
	{
		Com_sprintf(temp, sizeof(temp), "ls_%dr", i);
		G_SpawnString(temp, defaultStyles[i][0], &text);
		lengthRed = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+0, text);

		Com_sprintf(temp, sizeof(temp), "ls_%dg", i);
		G_SpawnString(temp, defaultStyles[i][1], &text);
		lengthGreen = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+1, text);

		Com_sprintf(temp, sizeof(temp), "ls_%db", i);
		G_SpawnString(temp, defaultStyles[i][2], &text);
		lengthBlue = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+2, text);

		if (lengthRed != lengthGreen || lengthGreen != lengthBlue)
		{
			Com_Error(ERR_DROP, "Style %d has inconsistent lengths: R %d, G %d, B %d", 
				i, lengthRed, lengthGreen, lengthBlue);
		}
	}		
}

//rww - Planning on having something here?
qboolean SP_bsp_worldspawn ( void )
{
	return qtrue;
}

void G_PrecacheSoundsets( void )
{
	gentity_t	*ent = NULL;
	int i;
	int countedSets = 0;

	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->soundSet && ent->soundSet[0])
		{
			if (countedSets >= MAX_AMBIENT_SETS)
			{
				Com_Error(ERR_DROP, "MAX_AMBIENT_SETS was exceeded! (too many soundsets)\n");
			}

			ent->s.soundSetIndex = G_SoundSetIndex(ent->soundSet);
			countedSets++;
		}
	}
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
//RoboPhred: Only used for misc_bsp now
void G_SubBSP_SpawnEntitiesFromString(void) {
//void G_SpawnEntitiesFromString( qboolean inSubBSP ) {
	int i;
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars(qtrue) ) {
		Com_Printf("No misc_bsp entities!\n");
		return;
		//G_Error( "SpawnEntities: no entities" );
	}

	// Skip this guy if its worldspawn fails
	if ( !SP_bsp_worldspawn() )
	{
		return;
	}

	// parse ents
	//RoboPhred
	while( G_ParseSpawnVars(qtrue)){
	//while( G_ParseSpawnVars(inSubBSP) ) {
		//RoboPhred
		for(i = 0;i<level.numSpawnVars;i++){
			if(Q_stricmp(level.spawnVars[i][0], "classname") == 0){
				//double layered for debugging
				if(Q_stricmp(level.spawnVars[i][1], "info_player_deathmatch") == 0){
					//This isnt the best thing to do, but since we were 'info_player_deathmatch', we know its at least 22 long.
					Q_strncpyz(level.spawnVars[i][1], "info_player_start", 22); 
				}
			}
		}
		//RoboPhred
		G_SpawnGEntityFromSpawnVars(qtrue);
		//G_SpawnGEntityFromSpawnVars(inSubBSP);
	}	

	G_PrecacheSoundsets();
}
