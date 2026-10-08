#ifndef WL_STATE_H
#define WL_STATE_H

#include "WL_GAME.h"

int WL_TickActors(wg_level_t *level, unsigned tics);
int WL_UpdateAreaConnectivity(wg_level_t *level);
int WL_CheckLine(const wg_level_t *level, const wg_actor_t *actor);
int WL_CheckSight(const wg_level_t *level, const wg_actor_t *actor);
int WL_TickAwareness(wg_level_t *level, unsigned tics, int made_noise);
int WL_KillActor(wg_level_t *level, size_t actor_index);
int WL_DamageActor(wg_level_t *level, size_t actor_index, unsigned damage);

#endif
