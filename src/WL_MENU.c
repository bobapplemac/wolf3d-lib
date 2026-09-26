/* Portable control-panel presentation derived from the original WL_MENU.C. */
#include "WL_MENU.h"

#include <stddef.h>
#include <string.h>

#include "ID_VH.h"
#include "ID_US_1.h"
#include "ID_VL.h"
#include "WL_GAME.h"

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

#define WL_SOUND_X 48
#define WL_SOUND_Y1 20
#define WL_SOUND_Y2 (WL_SOUND_Y1 + 5 * 13)
#define WL_SOUND_Y3 (WL_SOUND_Y2 + 5 * 13)
#define WL_SOUND_WIDTH 250

#define WL_CONTROL_X 24
#define WL_CONTROL_Y 70
#define WL_CONTROL_WIDTH 284
#define WL_CONTROL_HEIGHT (13 * 7 - 7)

#define WL_CUSTOM_X 8
#define WL_CUSTOM_Y (48 + 26)
#define WL_CUSTOM_START 60
#define WL_CUSTOM_SPACING 60

#define WL_LOAD_SAVE_X 85
#define WL_LOAD_SAVE_Y 55
#define WL_LOAD_SAVE_WIDTH 175
#define WL_LOAD_SAVE_HEIGHT (10 * 13 + 10)
#define WL_LOAD_SAVE_INDENT 24

typedef struct wl_menu_chunks
{
    size_t options;
    size_t cursor;
    size_t mouse_back;
    size_t baby_mode;
    size_t episode_one;
    size_t not_selected;
    size_t selected;
    size_t effects_title;
    size_t digitized_title;
    size_t music_title;
    size_t control_title;
    size_t customize_title;
    size_t load_title;
    size_t save_title;
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
        chunks->baby_mode = 19U;
        chunks->episode_one = 30U;
        chunks->not_selected = 13U;
        chunks->selected = 14U;
        chunks->effects_title = 15U;
        chunks->digitized_title = 16U;
        chunks->music_title = 17U;
        chunks->control_title = 26U;
        chunks->customize_title = 27U;
        chunks->load_title = 28U;
        chunks->save_title = 29U;
        return 1;
    }
    if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        /* Apogee 1.4 inserts H_SPEARADPIC after the help-window pieces. */
        chunks->options = 22U;
        chunks->cursor = 23U;
        chunks->mouse_back = 30U;
        chunks->baby_mode = 31U;
        chunks->episode_one = 42U;
        chunks->not_selected = 24U;
        chunks->selected = 25U;
        chunks->effects_title = 26U;
        chunks->digitized_title = 27U;
        chunks->music_title = 28U;
        chunks->control_title = 37U;
        chunks->customize_title = 38U;
        chunks->load_title = 39U;
        chunks->save_title = 40U;
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

static void WL_ColorOutline(uint8_t *framebuffer, int x, int y,
                            int width, int height,
                            uint8_t bottom_right, uint8_t top_left)
{
    WL_MenuBar(framebuffer, x, y, width + 1, 1, top_left);
    WL_MenuBar(framebuffer, x, y, 1, height + 1, top_left);
    WL_MenuBar(framebuffer, x, y + height, width + 1, 1, bottom_right);
    WL_MenuBar(framebuffer, x + width, y, 1, height + 1, bottom_right);
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
        items[7].text = "End Game";
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

int WL_DrawConfirm(uint8_t framebuffer[320 * 200],
                   const wg_graphics_t *graphics, const char *message)
{
    const char *line_start;
    const char *cursor;
    wg_font_t font;
    size_t widest = 0U;
    unsigned lines = 0U;
    int width;
    int height;
    int x;
    int y;

    if (framebuffer == NULL || graphics == NULL || message == NULL
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    line_start = message;
    for (cursor = message;; ++cursor)
    {
        if (*cursor == '\n' || *cursor == '\0')
        {
            char line[80];
            size_t length = (size_t)(cursor - line_start);
            size_t measured;

            if (length >= sizeof(line))
            {
                WG_FontClose(&font);
                return 0;
            }
            memcpy(line, line_start, length);
            line[length] = '\0';
            measured = WG_FontMeasure(&font, line);
            if (measured > widest)
            {
                widest = measured;
            }
            ++lines;
            if (*cursor == '\0')
            {
                break;
            }
            line_start = cursor + 1;
        }
    }
    width = (int)widest + 10;
    height = (int)font.height * (int)lines + 10;
    x = (320 - width) / 2;
    y = (160 - height) / 2;
    WL_MenuBar(framebuffer, x, y, width, height, WL_MENU_TEXT_COLOR);
    WL_ColorOutline(framebuffer, x, y, width, height,
                    0U, WL_MENU_HIGHLIGHT_COLOR);
    line_start = message;
    lines = 0U;
    for (cursor = message;; ++cursor)
    {
        if (*cursor == '\n' || *cursor == '\0')
        {
            char line[80];
            size_t length = (size_t)(cursor - line_start);

            memcpy(line, line_start, length);
            line[length] = '\0';
            WG_FontDraw(&font, framebuffer, x + 5,
                        y + 5 + (int)lines * (int)font.height, line, 0U);
            ++lines;
            if (*cursor == '\0')
            {
                break;
            }
            line_start = cursor + 1;
        }
    }
    WG_FontClose(&font);
    return 1;
}

int WL_DrawLoadSaveMenu(
    uint8_t framebuffer[320 * 200], const wg_graphics_t *graphics,
    int saving, unsigned selected,
    const uint8_t available[WL_SAVE_SLOTS],
    const char names[WL_SAVE_SLOTS][WL_SAVE_NAME_LENGTH + 1U],
    int editing, int confirm_overwrite)
{
    static const char empty[] = "      - EMPTY -";
    wl_menu_chunks_t chunks;
    wg_font_t font;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL
        || selected >= WL_SAVE_SLOTS || available == NULL || names == NULL
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 0U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, WL_LOAD_SAVE_X - 10, WL_LOAD_SAVE_Y - 5,
               WL_LOAD_SAVE_WIDTH, WL_LOAD_SAVE_HEIGHT,
               WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_LOAD_SAVE_X - 10,
                   WL_LOAD_SAVE_Y - 5, WL_LOAD_SAVE_WIDTH,
                   WL_LOAD_SAVE_HEIGHT);
    WL_MenuBar(framebuffer, 0, 10, 320, 24, 0U);
    WL_MenuBar(framebuffer, 0, 32, 320, 1, WL_MENU_STRIPE_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics,
                             saving ? chunks.save_title : chunks.load_title,
                             60, 0))
    {
        WG_FontClose(&font);
        return 0;
    }
    for (index = 0U; index < WL_SAVE_SLOTS; ++index)
    {
        uint8_t color = index == selected ? WL_MENU_HIGHLIGHT_COLOR
                                          : WL_MENU_TEXT_COLOR;
        int x = WL_LOAD_SAVE_X + WL_LOAD_SAVE_INDENT;
        int y = WL_LOAD_SAVE_Y + (int)index * 13;

        WL_ColorOutline(framebuffer, x, y,
                        WL_LOAD_SAVE_WIDTH - WL_LOAD_SAVE_INDENT - 15,
                        11, color, color);
        WG_FontDraw(&font, framebuffer, x + 2, y + 1,
                    available[index] ? names[index] : empty, color);
        if (editing && index == selected)
        {
            WG_FontDraw(&font, framebuffer,
                        x + 2 + (int)WG_FontMeasure(&font, names[index]),
                        y + 1, "_", color);
        }
    }
    WG_FontClose(&font);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor,
                             WL_LOAD_SAVE_X & ~7,
                             WL_LOAD_SAVE_Y - 2 + (int)selected * 13))
    {
        return 0;
    }
    if (confirm_overwrite)
    {
        static const char *const lines[] =
        {
            "There's already a game",
            "saved at this position.",
            "      Overwrite?"
        };
        size_t widest = 0U;
        unsigned line;
        int width;
        int height;
        int x;
        int y;

        if (!WG_FontOpen(&font, graphics, 1U))
        {
            return 0;
        }
        for (line = 0U; line < 3U; ++line)
        {
            size_t line_width = WG_FontMeasure(&font, lines[line]);
            if (line_width > widest)
            {
                widest = line_width;
            }
        }
        width = (int)widest + 10;
        height = (int)font.height * 3 + 10;
        x = (320 - width) / 2;
        y = (200 - height) / 2;
        WL_MenuBar(framebuffer, x, y, width, height,
                   WL_MENU_TEXT_COLOR);
        WL_ColorOutline(framebuffer, x, y, width, height,
                        0U, WL_MENU_HIGHLIGHT_COLOR);
        for (line = 0U; line < 3U; ++line)
        {
            WG_FontDraw(&font, framebuffer, x + 5,
                        y + 5 + (int)line * (int)font.height,
                        lines[line], 0U);
        }
        WG_FontClose(&font);
    }
    return 1;
}

unsigned WL_LoadSaveMenuMove(unsigned selected, int direction)
{
    if (selected >= WL_SAVE_SLOTS || direction == 0)
    {
        return selected;
    }
    if (direction < 0)
    {
        return selected == 0U ? WL_SAVE_SLOTS - 1U : selected - 1U;
    }
    return selected + 1U == WL_SAVE_SLOTS ? 0U : selected + 1U;
}

static int WL_SoundMenuActive(unsigned item)
{
    return item == 0U || item == 2U || item == 5U || item == 7U
        || item == 10U || item == 11U;
}

unsigned WL_SoundMenuMove(unsigned selected, int direction)
{
    unsigned candidate;

    if (selected >= WL_SOUND_MENU_ITEMS || direction == 0)
    {
        return selected;
    }
    candidate = selected;
    do
    {
        if (direction < 0)
        {
            candidate = candidate == 0U ? WL_SOUND_MENU_ITEMS - 1U
                                        : candidate - 1U;
        }
        else
        {
            candidate = candidate + 1U == WL_SOUND_MENU_ITEMS
                            ? 0U : candidate + 1U;
        }
    } while (!WL_SoundMenuActive(candidate));
    return candidate;
}

int WL_DrawSoundMenu(uint8_t framebuffer[320 * 200],
                     const wg_graphics_t *graphics, unsigned selected,
                     int adlib_effects, int digitized, int music)
{
    static const char *const labels[WL_SOUND_MENU_ITEMS] =
    {
        "None", "PC Speaker", "AdLib/Sound Blaster", "", "",
        "None", "Disney Sound Source", "Sound Blaster", "", "",
        "None", "AdLib/Sound Blaster"
    };
    wl_menu_chunks_t chunks;
    wg_font_t font;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL
        || selected >= WL_SOUND_MENU_ITEMS
        || !WL_SoundMenuActive(selected)
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y1 - 3,
               WL_SOUND_WIDTH, 4 * 13 - 7, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y1 - 3,
                   WL_SOUND_WIDTH, 4 * 13 - 7);
    WL_MenuBar(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y2 - 3,
               WL_SOUND_WIDTH, 4 * 13 - 7, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y2 - 3,
                   WL_SOUND_WIDTH, 4 * 13 - 7);
    WL_MenuBar(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y3 - 3,
               WL_SOUND_WIDTH, 3 * 13 - 7, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_SOUND_X - 8, WL_SOUND_Y3 - 3,
                   WL_SOUND_WIDTH, 3 * 13 - 7);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.effects_title,
                             100, WL_SOUND_Y1 - 20)
        || !WG_VideoDrawPicture(framebuffer, graphics,
                                chunks.digitized_title,
                                100, WL_SOUND_Y2 - 20)
        || !WG_VideoDrawPicture(framebuffer, graphics, chunks.music_title,
                                100, WL_SOUND_Y3 - 20))
    {
        WG_FontClose(&font);
        return 0;
    }
    for (index = 0U; index < WL_SOUND_MENU_ITEMS; ++index)
    {
        int on;
        uint8_t color;

        if (labels[index][0] == '\0')
        {
            continue;
        }
        color = WL_SoundMenuActive(index)
                    ? (index == selected ? WL_MENU_HIGHLIGHT_COLOR
                                         : WL_MENU_TEXT_COLOR)
                    : WL_MENU_DEACTIVE_COLOR;
        WG_FontDraw(&font, framebuffer, WL_SOUND_X + 52,
                    WL_SOUND_Y1 + (int)index * 13, labels[index], color);
        on = (index == 0U && !adlib_effects)
             || (index == 2U && adlib_effects)
             || (index == 5U && !digitized)
             || (index == 7U && digitized)
             || (index == 10U && !music)
             || (index == 11U && music);
        if (!WG_VideoDrawPicture(framebuffer, graphics,
                                 on ? chunks.selected : chunks.not_selected,
                                 WL_SOUND_X + 24,
                                 WL_SOUND_Y1 + (int)index * 13 + 2))
        {
            WG_FontClose(&font);
            return 0;
        }
    }
    WG_FontClose(&font);
    return WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor,
                               WL_SOUND_X & ~7,
                               WL_SOUND_Y1 - 2 + (int)selected * 13);
}

static int WL_ControlMenuActive(unsigned item, int mouse_enabled)
{
    return item == 0U || item == 5U || (item == 4U && mouse_enabled);
}

unsigned WL_ControlMenuMove(unsigned selected, int direction,
                            int mouse_enabled)
{
    unsigned candidate;

    if (selected >= WL_CONTROL_MENU_ITEMS || direction == 0)
    {
        return selected;
    }
    candidate = selected;
    do
    {
        if (direction < 0)
        {
            candidate = candidate == 0U ? WL_CONTROL_MENU_ITEMS - 1U
                                        : candidate - 1U;
        }
        else
        {
            candidate = candidate + 1U == WL_CONTROL_MENU_ITEMS
                            ? 0U : candidate + 1U;
        }
    } while (!WL_ControlMenuActive(candidate, mouse_enabled));
    return candidate;
}

int WL_DrawControlMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned selected,
                       int mouse_enabled)
{
    static const char *const labels[WL_CONTROL_MENU_ITEMS] =
    {
        "Mouse Enabled", "Joystick Enabled", "Use joystick port 2",
        "Gravis GamePad Enabled", "Mouse Sensitivity", "Customize controls"
    };
    wl_menu_chunks_t chunks;
    wg_font_t font;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL
        || selected >= WL_CONTROL_MENU_ITEMS
        || !WL_ControlMenuActive(selected, mouse_enabled)
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    WL_MenuBar(framebuffer, 0, 10, 320, 24, 0U);
    WL_MenuBar(framebuffer, 0, 32, 320, 1, WL_MENU_STRIPE_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.control_title,
                             80, 0)
        || !WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                                112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, WL_CONTROL_X - 8, WL_CONTROL_Y - 5,
               WL_CONTROL_WIDTH, WL_CONTROL_HEIGHT,
               WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, WL_CONTROL_X - 8, WL_CONTROL_Y - 5,
                   WL_CONTROL_WIDTH, WL_CONTROL_HEIGHT);
    for (index = 0U; index < WL_CONTROL_MENU_ITEMS; ++index)
    {
        uint8_t color = WL_ControlMenuActive(index, mouse_enabled)
                            ? (index == selected
                                   ? WL_MENU_HIGHLIGHT_COLOR
                                   : WL_MENU_TEXT_COLOR)
                            : WL_MENU_DEACTIVE_COLOR;

        WG_FontDraw(&font, framebuffer, WL_CONTROL_X + 56,
                    WL_CONTROL_Y + (int)index * 13, labels[index], color);
        if (index < 4U
            && !WG_VideoDrawPicture(
                framebuffer, graphics,
                index == 0U && mouse_enabled
                    ? chunks.selected : chunks.not_selected,
                WL_CONTROL_X + 32, WL_CONTROL_Y + 3 + (int)index * 13))
        {
            WG_FontClose(&font);
            return 0;
        }
    }
    WG_FontClose(&font);
    return WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor,
                               WL_CONTROL_X,
                               WL_CONTROL_Y - 2 + (int)selected * 13);
}

int WL_DrawMouseSensitivity(uint8_t framebuffer[320 * 200],
                            const wg_graphics_t *graphics,
                            unsigned adjustment)
{
    wl_menu_chunks_t chunks;
    wg_font_t font;
    size_t title_width;

    if (framebuffer == NULL || graphics == NULL || adjustment > 9U
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, 10, 80, 300, 30, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, 10, 80, 300, 30);
    title_width = WG_FontMeasure(&font, "Adjust Mouse Sensitivity");
    WG_FontDraw(&font, framebuffer, (320 - (int)title_width) / 2, 82,
                "Adjust Mouse Sensitivity", WL_MENU_READ_COLOR);
    WG_FontDraw(&font, framebuffer, 14, 95, "Slow", WL_MENU_TEXT_COLOR);
    WG_FontDraw(&font, framebuffer, 269, 95, "Fast", WL_MENU_TEXT_COLOR);
    WL_MenuBar(framebuffer, 60, 97, 200, 10, WL_MENU_TEXT_COLOR);
    WL_ColorOutline(framebuffer, 60, 97, 200, 10,
                    0U, WL_MENU_HIGHLIGHT_COLOR);
    WL_ColorOutline(framebuffer, 60 + (int)adjustment * 20, 97, 20, 10,
                    0U, WL_MENU_READ_COLOR);
    WL_MenuBar(framebuffer, 61 + (int)adjustment * 20, 98, 19, 9,
               WL_MENU_READ_HIGHLIGHT_COLOR);
    WG_FontClose(&font);
    return 1;
}

static int WL_CustomMenuActive(unsigned item, int mouse_enabled)
{
    return item == 6U || item == 8U || (item == 0U && mouse_enabled);
}

unsigned WL_CustomMenuMove(unsigned selected, int direction,
                           int mouse_enabled)
{
    unsigned candidate;

    if (selected >= WL_CUSTOM_MENU_ITEMS || direction == 0)
    {
        return selected;
    }
    candidate = selected;
    do
    {
        if (direction < 0)
        {
            candidate = candidate == 0U ? WL_CUSTOM_MENU_ITEMS - 1U
                                        : candidate - 1U;
        }
        else
        {
            candidate = candidate + 1U == WL_CUSTOM_MENU_ITEMS
                            ? 0U : candidate + 1U;
        }
    } while (!WL_CustomMenuActive(candidate, mouse_enabled));
    return candidate;
}

int WL_DrawChangeView(uint8_t framebuffer[320 * 200],
                      const wg_graphics_t *graphics, unsigned view_size)
{
    static const char *const lines[3] =
    {
        "Use arrows to size", "ENTER to accept", "ESC to cancel"
    };
    wg_font_t font;
    unsigned line;

    if (framebuffer == NULL || graphics == NULL
        || view_size < WL_VIEW_SIZE_MIN || view_size > WL_VIEW_SIZE_MAX
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    if (!WL_DrawPlayBorder(framebuffer, view_size * 16U))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, 0, 160, 320, 40, 0x7fU);
    for (line = 0U; line < 3U; ++line)
    {
        int width = (int)WG_FontMeasure(&font, lines[line]);

        WG_FontDraw(&font, framebuffer, (320 - width) / 2,
                    161 + (int)line * font.height,
                    lines[line], WL_MENU_HIGHLIGHT_COLOR);
    }
    WG_FontClose(&font);
    return 1;
}

static void WL_CustomLabels(const wg_font_t *font, uint8_t *framebuffer,
                            int y, const char *const labels[4])
{
    unsigned index;

    for (index = 0U; index < 4U; ++index)
    {
        WG_FontDraw(font, framebuffer,
                    WL_CUSTOM_START + (int)index * WL_CUSTOM_SPACING,
                    y, labels[index], WL_MENU_TEXT_COLOR);
    }
}

static void WL_CustomWindow(uint8_t *framebuffer, int y)
{
    WL_MenuBar(framebuffer, 5, y - 1, 310, 13,
               WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, 5, y - 1, 310, 13);
}

static void WL_CustomCentered(const wg_font_t *font, uint8_t *framebuffer,
                              int y, const char *text)
{
    size_t width = WG_FontMeasure(font, text);

    WG_FontDraw(font, framebuffer, (320 - (int)width) / 2, y, text,
                WL_MENU_READ_COLOR);
}

int WL_DrawCustomizeMenu(
    uint8_t framebuffer[320 * 200], const wg_graphics_t *graphics,
    unsigned selected, int mouse_enabled,
    const uint8_t mouse_bindings[WL_CUSTOM_BINDINGS],
    const uint16_t action_keys[WL_CUSTOM_BINDINGS],
    const uint16_t movement_keys[WL_CUSTOM_BINDINGS],
    int edit_column, int capture)
{
    static const char *const action_labels[4] =
        { "Run", "Open", "Fire", "Strafe" };
    static const char *const movement_labels[4] =
        { "Left", "Right", "Frwd", "Bkwrd" };
    static const uint8_t joystick_bindings[4] = { 3U, 2U, 0U, 1U };
    wl_menu_chunks_t chunks;
    wg_font_t font;
    unsigned index;
    int edit_y = 0;

    if (framebuffer == NULL || graphics == NULL
        || mouse_bindings == NULL || action_keys == NULL
        || movement_keys == NULL || selected >= WL_CUSTOM_MENU_ITEMS
        || !WL_CustomMenuActive(selected, mouse_enabled)
        || edit_column < -1 || edit_column >= 4
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
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
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.customize_title,
                             80, 0))
    {
        WG_FontClose(&font);
        return 0;
    }

    WL_CustomCentered(&font, framebuffer, 48, "Mouse");
    WL_CustomLabels(&font, framebuffer, 61, action_labels);
    WL_CustomWindow(framebuffer, 74);
    for (index = 0U; index < 4U; ++index)
    {
        char binding[3];

        if (mouse_bindings[index] >= 4U)
        {
            continue;
        }
        binding[0] = 'b';
        binding[1] = (char)('0' + mouse_bindings[index]);
        binding[2] = '\0';
        WG_FontDraw(&font, framebuffer,
                    WL_CUSTOM_START + (int)index * WL_CUSTOM_SPACING,
                    74, binding,
                    mouse_enabled
                        ? (selected == 0U ? WL_MENU_HIGHLIGHT_COLOR
                                          : WL_MENU_TEXT_COLOR)
                        : WL_MENU_DEACTIVE_COLOR);
    }

    WL_CustomCentered(&font, framebuffer, 87,
                      "Joystick/Gravis GamePad");
    WL_CustomLabels(&font, framebuffer, 100, action_labels);
    WL_CustomWindow(framebuffer, 113);
    for (index = 0U; index < 4U; ++index)
    {
        char binding[3] = { 'b', (char)('0' + joystick_bindings[index]), '\0' };

        WG_FontDraw(&font, framebuffer,
                    WL_CUSTOM_START + (int)index * WL_CUSTOM_SPACING,
                    113, binding, WL_MENU_DEACTIVE_COLOR);
    }

    WL_CustomCentered(&font, framebuffer, 126, "Keyboard");
    WL_CustomLabels(&font, framebuffer, 139, action_labels);
    WL_CustomWindow(framebuffer, 152);
    for (index = 0U; index < 4U; ++index)
    {
        WG_FontDraw(&font, framebuffer,
                    WL_CUSTOM_START + (int)index * WL_CUSTOM_SPACING,
                    152, ID_US_ScanName(action_keys[index]),
                    selected == 6U ? WL_MENU_HIGHLIGHT_COLOR
                                   : WL_MENU_TEXT_COLOR);
    }

    WL_CustomLabels(&font, framebuffer, 165, movement_labels);
    WL_CustomWindow(framebuffer, 178);
    for (index = 0U; index < 4U; ++index)
    {
        WG_FontDraw(&font, framebuffer,
                    WL_CUSTOM_START + (int)index * WL_CUSTOM_SPACING,
                    178, ID_US_ScanName(movement_keys[index]),
                    selected == 8U ? WL_MENU_HIGHLIGHT_COLOR
                                   : WL_MENU_TEXT_COLOR);
    }

    if (edit_column >= 0)
    {
        int edit_x = WL_CUSTOM_START + edit_column * WL_CUSTOM_SPACING;

        edit_y = selected == 0U ? 74 : selected == 6U ? 152 : 178;
        WL_MenuBar(framebuffer, edit_x - 2, edit_y, WL_CUSTOM_SPACING, 11,
                   WL_MENU_TEXT_COLOR);
        WL_ColorOutline(framebuffer, edit_x - 2, edit_y,
                        WL_CUSTOM_SPACING, 11, 0U,
                        WL_MENU_HIGHLIGHT_COLOR);
        if (capture)
        {
            WG_FontDraw(&font, framebuffer, edit_x, edit_y + 1, "?", 0U);
        }
        else if (selected == 0U)
        {
            char binding[3] = { 'b', '?', '\0' };

            if (mouse_bindings[edit_column] < 4U)
            {
                binding[1] = (char)('0' + mouse_bindings[edit_column]);
                WG_FontDraw(&font, framebuffer, edit_x, edit_y + 1,
                            binding, 0U);
            }
        }
        else
        {
            const uint16_t *keys = selected == 6U
                                       ? action_keys : movement_keys;
            WG_FontDraw(&font, framebuffer, edit_x, edit_y + 1,
                        ID_US_ScanName(keys[edit_column]), 0U);
        }
    }
    WG_FontClose(&font);
    return edit_column >= 0
               || WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor,
                                      WL_CUSTOM_X,
                                      WL_CUSTOM_Y - 2
                                          + (int)selected * 13);
}

unsigned WL_EpisodeMenuMove(unsigned episode, int direction)
{
    if (episode >= 6U || direction == 0)
    {
        return episode;
    }
    if (direction < 0)
    {
        return episode == 0U ? 5U : episode - 1U;
    }
    return episode == 5U ? 0U : episode + 1U;
}

unsigned WL_DifficultyMenuMove(unsigned difficulty, int direction)
{
    if (difficulty >= 4U || direction == 0)
    {
        return difficulty;
    }
    if (direction < 0)
    {
        return difficulty == 0U ? 3U : difficulty - 1U;
    }
    return difficulty == 3U ? 0U : difficulty + 1U;
}

int WL_DrawEpisodeMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned episode,
                       int shareware)
{
    static const char *const names[6] =
    {
        "Episode 1", "Episode 2", "Episode 3",
        "Episode 4", "Episode 5", "Episode 6"
    };
    static const char *const subtitles[6] =
    {
        "Escape from Wolfenstein", "Operation: Eisenfaust",
        "Die, Fuhrer, Die!", "A Dark Secret", "Trail of the Madman",
        "Confrontation"
    };
    wl_menu_chunks_t chunks;
    wg_font_t font;
    size_t title_width;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL || episode >= 6U
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WL_MenuBar(framebuffer, 6, 19, 308, 162, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, 6, 19, 308, 162);
    title_width = WG_FontMeasure(&font, "Which episode to play?");
    WG_FontDraw(&font, framebuffer, (320 - (int)title_width) / 2, 2,
                "Which episode to play?", WL_MENU_READ_HIGHLIGHT_COLOR);

    for (index = 0U; index < 6U; ++index)
    {
        uint8_t color = shareware && index != 0U
                            ? (index == episode ? 0x67U : 0x6bU)
                            : (index == episode ? WL_MENU_HIGHLIGHT_COLOR
                                                : WL_MENU_TEXT_COLOR);
        int y = 23 + (int)index * 26;

        WG_FontDraw(&font, framebuffer, 98, y, names[index], color);
        WG_FontDraw(&font, framebuffer, 98, y + (int)font.height,
                    subtitles[index], color);
        if (!WG_VideoDrawPicture(framebuffer, graphics,
                                 chunks.episode_one + index, 42, y))
        {
            WG_FontClose(&font);
            return 0;
        }
    }
    WG_FontClose(&font);
    return WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor, 8,
                               21 + (int)episode * 26);
}

int WL_DrawDifficultyMenu(uint8_t framebuffer[320 * 200],
                          const wg_graphics_t *graphics, unsigned difficulty)
{
    static const char *const labels[4] =
    {
        "Can I play, Daddy?", "Don't hurt me.",
        "Bring 'em on!", "I am Death incarnate!"
    };
    wl_menu_chunks_t chunks;
    wg_font_t font;
    unsigned index;

    if (framebuffer == NULL || graphics == NULL || difficulty >= 4U
        || !WL_MenuChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 1U))
    {
        return 0;
    }
    WG_VideoClear(framebuffer, WL_MENU_BORDER_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.mouse_back,
                             112, 184))
    {
        WG_FontClose(&font);
        return 0;
    }
    WG_FontDraw(&font, framebuffer, 70, 68, "How tough are you?",
                WL_MENU_READ_HIGHLIGHT_COLOR);
    WL_MenuBar(framebuffer, 45, 90, 225, 67, WL_MENU_BACKGROUND_COLOR);
    WL_MenuOutline(framebuffer, 45, 90, 225, 67);
    for (index = 0U; index < 4U; ++index)
    {
        WG_FontDraw(&font, framebuffer, 74, 100 + (int)index * 13,
                    labels[index], index == difficulty
                                   ? WL_MENU_HIGHLIGHT_COLOR
                                   : WL_MENU_TEXT_COLOR);
    }
    WG_FontClose(&font);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.cursor, 48,
                             98 + (int)difficulty * 13))
    {
        return 0;
    }
    return WG_VideoDrawPicture(framebuffer, graphics,
                               chunks.baby_mode + difficulty, 235, 107);
}
