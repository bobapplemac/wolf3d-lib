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
    uint8_t face;
} wl_status_t;

typedef enum wl_status_face
{
    WL_STATUS_FACE_NORMAL = 0,
    WL_STATUS_FACE_DEAD,
    WL_STATUS_FACE_GATLING,
    WL_STATUS_FACE_GOD,
    WL_STATUS_FACE_WAITING_1,
    WL_STATUS_FACE_WAITING_2,
    WL_STATUS_FACE_OUCH
} wl_status_face_t;

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
void WL_TakeDamageFrom(struct wg_level *level, unsigned points,
                       int32_t attacker_x, int32_t attacker_y);
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
void WL_VictorySpin(struct wg_level *level, unsigned tics);
int WL_CmdUse(struct wg_level *level);
int WL_CmdUseHeld(struct wg_level *level, int button_held);
int WL_SelectWeapon(struct wg_level *level, unsigned weapon);
int WL_GunAttack(struct wg_level *level);
int WL_KnifeAttack(struct wg_level *level);
int WL_StartAttack(struct wg_level *level);
int WL_TickPlayerAttack(struct wg_level *level, unsigned tics,
                        int attack_held);
int WL_DrawStatusBar(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_t *status);
unsigned WL_StatusDisplayFloor(wg_game_variant_t variant, unsigned map);

#endif
