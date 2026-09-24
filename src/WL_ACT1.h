#ifndef WL_ACT1_H
#define WL_ACT1_H

#include <stdint.h>

struct wg_level;

int WL_PushWall(struct wg_level *level, uint8_t tile_x, uint8_t tile_y,
                uint8_t direction);
int WL_MovePushWalls(struct wg_level *level, unsigned tics);

#endif
