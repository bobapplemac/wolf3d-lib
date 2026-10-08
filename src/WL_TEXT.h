#ifndef WL_TEXT_H
#define WL_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "WG_GRAPHICS.h"

#define WL_ARTICLE_MAX_PAGES 64

typedef struct wl_article
{
    uint8_t *text;
    size_t text_size;
    size_t page_offsets[WL_ARTICLE_MAX_PAGES];
    size_t page_count;
} wl_article_t;

int WL_ArticleOpen(wl_article_t *article, const wg_graphics_t *graphics,
                   unsigned episode);
int WL_ArticleOpenHelp(wl_article_t *article,
                       const wg_graphics_t *graphics);
void WL_ArticleClose(wl_article_t *article);
int WL_ArticleRender(const wl_article_t *article,
                     const wg_graphics_t *graphics, size_t page,
                     uint8_t framebuffer[320 * 200]);

#endif
