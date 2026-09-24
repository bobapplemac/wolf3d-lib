#ifndef WL_AGENT_H
#define WL_AGENT_H

#include <stddef.h>
#include <stdint.h>

#include "ID_VL.h"
#include "WG_GRAPHICS.h"

typedef struct wl_status
{
    uint32_t score;
    uint16_t health;
    uint16_t ammo;
    uint8_t map;
    uint8_t lives;
    uint8_t weapon;
    uint8_t keys;
    uint8_t face_frame;
} wl_status_t;

struct wg_level;
struct wg_view_tables;

typedef enum wg_weapon
{
    WG_WEAPON_KNIFE = 0,
    WG_WEAPON_PISTOL,
    WG_WEAPON_MACHINEGUN,
    WG_WEAPON_CHAINGUN
} wg_weapon_t;

void WL_StatusDefaults(wl_status_t *status);
void WL_TakeDamage(struct wg_level *level, unsigned points);
void WL_GivePoints(struct wg_level *level, uint32_t points);
int WL_GetBonus(struct wg_level *level, size_t static_index);
unsigned WL_CollectPlayerTileBonuses(struct wg_level *level);
int WL_TryMove(const struct wg_level *level, int32_t x, int32_t y);
int WL_ClipMove(struct wg_level *level, int32_t x_move, int32_t y_move);
int WL_Thrust(struct wg_level *level, const struct wg_view_tables *tables,
              uint16_t angle, int32_t speed);
int WL_ControlMovement(struct wg_level *level,
                       const struct wg_view_tables *tables,
                       int control_x, int control_y, int strafe);
int WL_CmdUse(struct wg_level *level);
int WL_SelectWeapon(struct wg_level *level, unsigned weapon);
int WL_GunAttack(struct wg_level *level);
int WL_KnifeAttack(struct wg_level *level);
int WL_StartAttack(struct wg_level *level);
int WL_TickPlayerAttack(struct wg_level *level, unsigned tics,
                        int attack_held);
int WL_DrawStatusBar(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_t *status);

#endif
