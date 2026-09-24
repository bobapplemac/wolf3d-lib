#ifndef WG_PAGES_H
#define WG_PAGES_H

#include <stddef.h>
#include <stdint.h>

#include "wg_data.h"
#include "wg_file.h"

typedef struct wg_pages
{
    wg_file_buffer_t file;
    const wg_data_set_t *data_set;
} wg_pages_t;

int WG_PagesOpen(wg_pages_t *pages, const wg_data_set_t *data_set);
void WG_PagesClose(wg_pages_t *pages);
int WG_PagesGet(const wg_pages_t *pages, size_t page,
                const uint8_t **data, size_t *size);

#endif
