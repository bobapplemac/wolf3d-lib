#ifndef ID_PM_H
#define ID_PM_H

#include <stddef.h>
#include <stdint.h>

#include "WG_DATA.h"
#include "WG_FILE.h"

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
