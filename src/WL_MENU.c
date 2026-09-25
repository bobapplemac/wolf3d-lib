/* Portable control-panel presentation derived from the original WL_MENU.C. */
#include "WL_MENU.h"

#include <stddef.h>

#include "ID_VH.h"
#include "ID_VL.h"

#define WL_MENU_BORDER_COLOR 0x29U
#define WL_MENU_BORDER_2_COLOR 0x23U
#define WL_MENU_DEACTIVE_COLOR 0x2bU
#define WL_MENU_BACKGROUND_COLOR 0x2dU
#define WL_MENU_STRIPE_COLOR 0x2cU
#define WL_MENU_TEXT_COLOR 0x17U
#define WL_MENU_HIGHLIGHT_COLOR 0x13U
#define WL_MENU_READ_COLOR 0x4aU
#define WL_MENU_READ_HIGHLIGHT_COLOR 0x47U

#define WL_MENU_X 76
#define WL_MENU_Y 55
#define WL_MENU_WIDTH 178
#define WL_MENU_HEIGHT (13 * 10 + 6)
#define WL_MENU_INDENT 24

typedef struct wl_menu_chunks
{
    size_t options;
    size_t cursor;
    size_t mouse_back;
} wl_menu_chunks_t;

typedef struct wl_menu_item
{
    const char *text;
    uint8_t active;
} wl_menu_item_t;

static int WL_MenuChunks(wg_game_variant_t variant, wl_menu_chunks_t *chunks)
{
    if (chunks == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        chunks->options = 10U;
        chunks->cursor = 11U;
        chunks->mouse_back = 18U;
        return 1;
    }
    if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        /* Apogee 1.4 inserts H_SPEARADPIC after the help-window pieces. */
        chunks->options = 22U;
        chunks->cursor = 23U;
        chunks->mouse_back = 30U;
        return 1;
    }
    return 0;
}

static void WL_MenuBar(uint8_t *framebuffer, int x, int y,
                       int width, int height, uint8_t color)
{
    WG_VideoBar(framebuffer, x, y, width, height, color);
}

static void WL_MenuOutline(uint8_t *framebuffer, int x, int y,
                           int width, int height)
{
    WL_MenuBar(framebuffer, x, y, width + 1, 1, WL_MENU_DEACTIVE_COLOR);
    WL_MenuBar(framebuffer, x, y, 1, height + 1, WL_MENU_DEACTIVE_COLOR);
    WL_MenuBar(framebuffer, x, y + height, width + 1, 1,
               WL_MENU_BORDER_2_COLOR);
    WL_MenuBar(framebuffer, x + width, y, 1, height + 1,
               WL_MENU_BORDER_2_COLOR);
}

static void WL_MenuItems(wl_menu_item_t items[WL_MAIN_MENU_ITEMS],
                         int in_game)
{
    static const char *const labels[WL_MAIN_MENU_ITEMS] =
    {
        "New Game", "Sound", "Control", "Load Game", "Save Game",
        "Change View", "Read This!", "View Scores", "Back to Demo", "Quit"
    };
    unsigned index;

    for (index = 0U; index < WL_MAIN_MENU_ITEMS; ++index)
    {
        items[index].text = labels[index];
        items[index].active = index == 4U ? (uint8_t)(in_game ? 1U : 0U)
                            : index == 6U ? 2U : 1U;
    }
    if (in_game)
    {
        items[8].text = "Back to Game";
        items[8].active = 2U;
    }
}

int WL_DrawMainMenu(uint8_t framebuffer[320 * 200],
                    const wg_graphics_t *graphics, unsigned selected,
                    int in_game)
{
    wl_menu_chunks_t chunks;
    wl_menu_item_t items[WL_MAIN_MENU_ITEMS];
    wg_font_t font;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL
        || selected >= WL_MAIN_MENU_ITEMS
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WL_MenuItems(items, in_game);
    if (items[selected].active == 0U)
    {
        WG_FontClose(&font);
        return 0;
    }

    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, 0, 10, 320, 24, 0U);
    WL_MenuBar(framebuffer, 0, 32, 320, 1, WL_MENU_STRIPE_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.options, 84, 0))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, WL_MENU_X - 8, WL_MENU_Y - 3,
               WL_MENU_WIDTH, WL_MENU_HEIGHT, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_MENU_X - 8, WL_MENU_Y - 3,
                   WL_MENU_WIDTH, WL_MENU_HEIGHT);

    for (index = 0U; index < WL_MAIN_MENU_ITEMS; ++index)
    {
        uint8_t color;

        if (items[index].active == 0U)
        {
            color = WL_MENU_DEACTIVE_COLOR;
        }
        else if (items[index].active == 2U)
        {
            color = index == selected ? WL_MENU_READ_HIGHLIGHT_COLOR
                                      : WL_MENU_READ_COLOR;
        }
        else
        {
            color = index == selected ? WL_MENU_HIGHLIGHT_COLOR
                                      : WL_MENU_TEXT_COLOR;
        }
        WG_FontDraw(&font, framebuffer, WL_MENU_X + WL_MENU_INDENT,
                    WL_MENU_Y + (int)index * 13, items[index].text, color);
    }
    WG_FontClose(&font);
    return WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor,
                               WL_MENU_X & ~7,
                               WL_MENU_Y - 2 + (int)selected * 13);
}

unsigned WL_MainMenuMove(unsigned selected, int direction, int in_game)
{
    wl_menu_item_t items[WL_MAIN_MENU_ITEMS];
    unsigned candidate;

    if (selected >= WL_MAIN_MENU_ITEMS || direction == 0)
    {
        return selected;
    }
    WL_MenuItems(items, in_game);
    candidate = selected;
    do
    {
        if (direction < 0)
        {
            candidate = candidate == 0U ? WL_MAIN_MENU_ITEMS - 1U
                                        : candidate - 1U;
        }
        else
        {
            candidate = candidate + 1U == WL_MAIN_MENU_ITEMS
                            ? 0U : candidate + 1U;
        }
    } while (items[candidate].active == 0U);
    return candidate;
}
