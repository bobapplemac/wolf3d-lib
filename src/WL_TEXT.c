/* Portable article layout derived from the original WL_TEXT.C. */
#include "WL_TEXT.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ID_VH.h"
#include "ID_VL.h"

#define WL_TEXT_BACK_COLOR 0x11U
#define WL_TEXT_FONT_HEIGHT 10
#define WL_TEXT_TOP_MARGIN 16
#define WL_TEXT_BOTTOM_MARGIN 32
#define WL_TEXT_LEFT_MARGIN 16
#define WL_TEXT_RIGHT_MARGIN 16
#define WL_TEXT_PICTURE_MARGIN 8
#define WL_TEXT_ROWS ((200 - WL_TEXT_TOP_MARGIN - WL_TEXT_BOTTOM_MARGIN) \
                      / WL_TEXT_FONT_HEIGHT)
#define WL_TEXT_SPACE_WIDTH 7
#define WL_TEXT_WORD_LIMIT 80

typedef struct wl_article_layout
{
    const uint8_t *text;
    const uint8_t *end;
    unsigned left[WL_TEXT_ROWS];
    unsigned right[WL_TEXT_ROWS];
    unsigned row;
    int x;
    int y;
    uint8_t color;
    int done;
} wl_article_layout_t;

static int WL_ArticleChunks(wg_game_variant_t variant, size_t *top,
                            size_t *end_text)
{
    if (top == NULL || end_text == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        *top = 6U;
        *end_text = 143U;
        return 1;
    }
    if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        *top = 18U;
        *end_text = 155U;
        return 1;
    }
    return 0;
}

static int WL_ArticleHex(uint8_t character)
{
    character = (uint8_t)toupper((int)character);
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

static int WL_ArticleNumber(wl_article_layout_t *layout, unsigned *number)
{
    unsigned value = 0U;

    while (layout->text < layout->end
           && (*layout->text < '0' || *layout->text > '9'))
    {
        ++layout->text;
    }
    if (layout->text == layout->end)
    {
        return 0;
    }
    do
    {
        value = value * 10U + (unsigned)(*layout->text - '0');
        ++layout->text;
    } while (layout->text < layout->end
             && *layout->text >= '0' && *layout->text <= '9');
    *number = value;
    return 1;
}

static int WL_ArticleEndOfLine(wl_article_layout_t *layout)
{
    while (layout->text < layout->end && *layout->text != '\n')
    {
        ++layout->text;
    }
    if (layout->text == layout->end)
    {
        return 0;
    }
    ++layout->text;
    return 1;
}

static void WL_ArticleNewLine(wl_article_layout_t *layout)
{
    ++layout->row;
    if (layout->row >= WL_TEXT_ROWS)
    {
        layout->done = 1;
        return;
    }
    layout->x = (int)layout->left[layout->row];
    layout->y += WL_TEXT_FONT_HEIGHT;
}

static int WL_ArticlePicture(wl_article_layout_t *layout,
                             const wg_graphics_t *graphics,
                             uint8_t *framebuffer, int margins)
{
    unsigned picture_y;
    unsigned picture_x;
    unsigned picture_chunk;
    size_t picture;

    if (!WL_ArticleNumber(layout, &picture_y)
        || !WL_ArticleNumber(layout, &picture_x)
        || !WL_ArticleNumber(layout, &picture_chunk)
        || !WL_ArticleEndOfLine(layout)
        || !WG_VideoDrawPicture(framebuffer, graphics, picture_chunk,
                                (int)(picture_x & ~7U), (int)picture_y))
    {
        return 0;
    }
    picture = picture_chunk - 3U;
    if (margins && picture_chunk >= 3U && picture < graphics->picture_count)
    {
        int width = graphics->pictures[picture].width;
        int height = graphics->pictures[picture].height;
        int middle = (int)picture_x + width / 2;
        int top = ((int)picture_y - WL_TEXT_TOP_MARGIN)
                  / WL_TEXT_FONT_HEIGHT;
        int bottom = ((int)picture_y + height - WL_TEXT_TOP_MARGIN)
                     / WL_TEXT_FONT_HEIGHT;
        int row;

        if (top < 0)
        {
            top = 0;
        }
        if (bottom >= WL_TEXT_ROWS)
        {
            bottom = WL_TEXT_ROWS - 1;
        }
        for (row = top; row <= bottom; ++row)
        {
            if (middle > 160)
            {
                layout->right[row] = picture_x - WL_TEXT_PICTURE_MARGIN;
            }
            else
            {
                layout->left[row] = picture_x + (unsigned)width
                                  + WL_TEXT_PICTURE_MARGIN;
            }
        }
        if (layout->x < (int)layout->left[layout->row])
        {
            layout->x = (int)layout->left[layout->row];
        }
    }
    return 1;
}

static int WL_ArticleCommand(wl_article_layout_t *layout,
                             const wg_graphics_t *graphics,
                             uint8_t *framebuffer)
{
    int command;

    ++layout->text;
    if (layout->text == layout->end)
    {
        return 0;
    }
    command = toupper((int)*layout->text++);
    if (command == 'P' || command == 'E')
    {
        layout->done = 1;
        layout->text -= 2;
        return 1;
    }
    if (command == ';')
    {
        return WL_ArticleEndOfLine(layout);
    }
    if (command == 'C')
    {
        int high;
        int low;

        if ((size_t)(layout->end - layout->text) < 2U)
        {
            return 0;
        }
        high = WL_ArticleHex(*layout->text++);
        low = WL_ArticleHex(*layout->text++);
        if (high < 0 || low < 0)
        {
            return 0;
        }
        layout->color = (uint8_t)(high * 16 + low);
        return 1;
    }
    if (command == '>')
    {
        layout->x = 160;
        return 1;
    }
    if (command == 'G')
    {
        return WL_ArticlePicture(layout, graphics, framebuffer, 1);
    }
    if (command == 'T')
    {
        if (!WL_ArticlePicture(layout, graphics, framebuffer, 0))
        {
            return 0;
        }
        /* The portable renderer presents complete frames; the original delay
           preceded this picture and does not occur in the shipped EndText. */
        return 1;
    }
    if (command == 'L')
    {
        unsigned y;
        unsigned x;

        if (!WL_ArticleNumber(layout, &y)
            || !WL_ArticleNumber(layout, &x)
            || !WL_ArticleEndOfLine(layout))
        {
            return 0;
        }
        layout->row = y <= WL_TEXT_TOP_MARGIN ? 0U
                    : (y - WL_TEXT_TOP_MARGIN) / WL_TEXT_FONT_HEIGHT;
        if (layout->row >= WL_TEXT_ROWS)
        {
            layout->row = WL_TEXT_ROWS - 1U;
        }
        layout->y = WL_TEXT_TOP_MARGIN
                  + (int)layout->row * WL_TEXT_FONT_HEIGHT;
        layout->x = (int)x;
        return 1;
    }
    if (command == 'B')
    {
        unsigned y;
        unsigned x;
        unsigned width;
        unsigned height;

        if (!WL_ArticleNumber(layout, &y)
            || !WL_ArticleNumber(layout, &x)
            || !WL_ArticleNumber(layout, &width)
            || !WL_ArticleNumber(layout, &height)
            || !WL_ArticleEndOfLine(layout))
        {
            return 0;
        }
        WG_VideoBar(framebuffer, (int)x, (int)y, (int)width, (int)height,
                    WL_TEXT_BACK_COLOR);
        return 1;
    }
    return 0;
}

int WL_ArticleOpen(wl_article_t *article, const wg_graphics_t *graphics,
                   unsigned episode)
{
    size_t unused_top;
    size_t end_text;
    size_t index;

    if (article == NULL || graphics == NULL
        || !WL_ArticleChunks(graphics->variant, &unused_top, &end_text)
        || episode >= (graphics->variant == WG_GAME_WOLF3D_SHAREWARE_14
                       ? 1U : 6U))
    {
        return 0;
    }
    memset(article, 0, sizeof(*article));
    if (!WG_GraphicsDecodeChunk(graphics, end_text + episode,
                                &article->text, &article->text_size))
    {
        return 0;
    }
    for (index = 0U; index + 1U < article->text_size; ++index)
    {
        if (article->text[index] == '^'
            && toupper((int)article->text[index + 1U]) == 'P')
        {
            if (article->page_count == WL_ARTICLE_MAX_PAGES)
            {
                WL_ArticleClose(article);
                return 0;
            }
            article->page_offsets[article->page_count++] = index;
        }
        else if (article->text[index] == '^'
                 && toupper((int)article->text[index + 1U]) == 'E')
        {
            break;
        }
    }
    if (article->page_count == 0U)
    {
        WL_ArticleClose(article);
        return 0;
    }
    return 1;
}

void WL_ArticleClose(wl_article_t *article)
{
    if (article != NULL)
    {
        free(article->text);
        memset(article, 0, sizeof(*article));
    }
}

int WL_ArticleRender(const wl_article_t *article,
                     const wg_graphics_t *graphics, size_t page,
                     uint8_t framebuffer[320 * 200])
{
    wl_article_layout_t layout;
    wg_font_t font;
    size_t top;
    size_t unused_end_text;
    unsigned row;
    char page_number[32];

    if (article == NULL || graphics == NULL || framebuffer == NULL
        || article->text == NULL || page >= article->page_count
        || !WL_ArticleChunks(graphics->variant, &top, &unused_end_text)
        || !WG_FontOpen(&font, graphics, 0U))
    {
        return 0;
    }
    memset(&layout, 0, sizeof(layout));
    layout.text = article->text + article->page_offsets[page];
    layout.end = article->text + article->text_size;
    layout.x = WL_TEXT_LEFT_MARGIN;
    layout.y = WL_TEXT_TOP_MARGIN;
    for (row = 0U; row < WL_TEXT_ROWS; ++row)
    {
        layout.left[row] = WL_TEXT_LEFT_MARGIN;
        layout.right[row] = 320U - WL_TEXT_RIGHT_MARGIN;
    }
    WG_VideoClear(framebuffer, WL_TEXT_BACK_COLOR);
    if (!WG_VideoDrawPicture(framebuffer, graphics, top, 0, 0)
        || !WG_VideoDrawPicture(framebuffer, graphics, top + 1U, 0, 8)
        || !WG_VideoDrawPicture(framebuffer, graphics, top + 2U, 312, 8)
        || !WG_VideoDrawPicture(framebuffer, graphics, top + 3U, 8, 176))
    {
        WG_FontClose(&font);
        return 0;
    }
    while (layout.text < layout.end && *layout.text <= 32U)
    {
        ++layout.text;
    }
    if ((size_t)(layout.end - layout.text) < 2U
        || *layout.text != '^'
        || toupper((int)layout.text[1]) != 'P')
    {
        WG_FontClose(&font);
        return 0;
    }
    layout.text += 2;
    if (!WL_ArticleEndOfLine(&layout))
    {
        WG_FontClose(&font);
        return 0;
    }
    while (!layout.done && layout.text < layout.end)
    {
        uint8_t character = *layout.text;

        if (character == '^')
        {
            if (!WL_ArticleCommand(&layout, graphics, framebuffer))
            {
                WG_FontClose(&font);
                return 0;
            }
        }
        else if (character == '\t')
        {
            layout.x = (layout.x + 8) & ~7;
            ++layout.text;
        }
        else if (character <= 32U)
        {
            ++layout.text;
            if (character == '\n')
            {
                WL_ArticleNewLine(&layout);
            }
        }
        else
        {
            char word[WL_TEXT_WORD_LIMIT];
            size_t length = 0U;
            size_t width;

            while (layout.text < layout.end && *layout.text > 32U)
            {
                if (length + 1U >= sizeof(word))
                {
                    WG_FontClose(&font);
                    return 0;
                }
                word[length++] = (char)*layout.text++;
            }
            word[length] = '\0';
            width = WG_FontMeasure(&font, word);
            while (!layout.done
                   && layout.x + (int)width
                      > (int)layout.right[layout.row])
            {
                WL_ArticleNewLine(&layout);
            }
            if (!layout.done)
            {
                WG_FontDraw(&font, framebuffer, layout.x, layout.y,
                            word, layout.color);
                layout.x += (int)width;
                while (layout.text < layout.end && *layout.text == ' ')
                {
                    layout.x += WL_TEXT_SPACE_WIDTH;
                    ++layout.text;
                }
            }
        }
    }
    (void)snprintf(page_number, sizeof(page_number), "pg %u of %u",
                   (unsigned)(page + 1U), (unsigned)article->page_count);
    WG_FontDraw(&font, framebuffer, 213, 183, page_number, 0x4fU);
    WG_FontClose(&font);
    return 1;
}
