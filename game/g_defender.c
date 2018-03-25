#include "g_local.h"

#define DEFENDER_RANGE 8192
#define DEFENDER_TIME  300
#define DEFENDER_ANGLE 160
int InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles);
void Cmd_Kill_f( gentity_t *ent );
void DismembermentTest(gentity_t *self);
void ShieldRemove(gentity_t *self);

void DefenderThink(gentity_t *self)
{
        self->nextthink = level.time + DEFENDER_TIME;
        int i;
        gentity_t *check, *kEnt;
        vec3_t diff, dir, pos, add;
        trace_t tr;
        //int count;
	//gentity_t	*entity_list[MAX_GENTITIES];
        
        /*
        //Check all clients
        for (i = 0; i < MAX_CLIENTS; i++){
                check = &g_entities[i];
                if (!check->inuse
                    || !check->client
                    || !check->client->ps.m_iVehicleNum
                    || g_entities[check->client->ps.m_iVehicleNum].m_pVehicle->m_pPilot->s.number != i) {
                        continue;
                }
                if (authenticated(check, 2)) {
                        continue;
                }
                
                check = &g_entities[check->client->ps.m_iVehicleNum];
        */
        //Check all vehicles
        //count = G_RadiusList(self->r.currentOrigin, DEFENDER_RANGE, self, qtrue, entity_list );
        for (i = MAX_CLIENTS; i < level.num_entities; i++){
                check = &g_entities[i];
                if (!check->inuse || !check->m_pVehicle ||
					check->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
                {
                        continue;
                }
                
                VectorCopy(self->r.currentOrigin, pos);
                pos[2] += 200;
                AngleVectors(self->s.angles, add,0,0);
                pos[0] += add[0]*120;
                pos[1] += add[1]*120;
                
                VectorSubtract(check->r.currentOrigin, pos, diff);
                if (VectorLength(diff) > DEFENDER_RANGE){
                        continue;
                }
                VectorNormalize(diff);
                
                VectorCopy(check->s.pos.trDelta, dir);
                dir[2] = 0;
                VectorNormalize(dir);
                VectorAdd(dir, diff, dir);
                if (VectorLength(dir) >= 0.999) {
                        continue;
                }
                vectoangles(diff, dir);
                
                if (!InFieldOfVision(self->s.angles, DEFENDER_ANGLE, dir)){
                        continue;
                }
                //VectorSet(mins, -8, -8, -8);
                //VectorSet(maxs, 8, 8, 8);
                
                trap_Trace(&tr,pos,0,0, check->r.currentOrigin, 
                           self->s.number, MASK_PLAYERSOLID);
                kEnt = &g_entities[tr.entityNum];
                
                if (kEnt->inuse && kEnt->health/*tr.entityNum == check->s.number*/)
                {
                        
                        
                        G_PlayEffectID(G_EffectIndex("env/hevil_bolt"), 
                                       pos,dir);
                        G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/chars/ion_cannon/ion_cannon2.mp3"));
                        if (kEnt->s.eType == ET_SPECIAL
                            && kEnt->s.modelindex == HI_SHIELD) {
                                ShieldRemove(kEnt);
                        } else if (kEnt->NPC) {
                                kEnt->health = 0;
                                kEnt->client->ps.stats[STAT_HEALTH] = 0;
                                if (kEnt->die) {
                                        kEnt->die(check, check, check, 
                                                  kEnt->client->pers.maxHealth,
                                                  MOD_UNKNOWN);
                                }        
                        } else {
                                Cmd_Kill_f (kEnt);
                                if (kEnt->health < 1)
                                {
                                        DismembermentTest(kEnt);
                                }
                                
                        }
                }
                
        }
}

void
SP_defender (gentity_t *ent) 
{
        ent->clipmask = MASK_SOLID;
        ent->r.contents = CONTENTS_SOLID;
        
	ent->s.modelindex = G_ModelIndex("models/map_objects/imperial/dish.md3");
        
	G_SetOrigin( ent, ent->s.origin );
	VectorSet(ent->r.mins, -150, -170, 0);
        VectorSet(ent->r.maxs, 110, 170, 370);
        VectorCopy(ent->s.angles, ent->s.apos.trBase);
        ent->s.apos.trBase[YAW] -= 90;
        ent->think = DefenderThink;
        ent->nextthink = level.time + Q_irand(200, 800);
        //ent->classname = G_NewString("defender");
	trap_LinkEntity (ent);
}
