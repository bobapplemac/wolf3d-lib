/* Portable VSWAP page access corresponding to the original ID_PM.C. */
#include "ID_PM.h"

#include <stdio.h>
#include <string.h>

#include "WG_COMPAT.h"

#include "WG_ENDIAN.h"

static int WG_PagesPath(char *destination, size_t destination_size,
                        const wg_data_set_t *data_set)
{
    int result = snprintf(destination, destination_size, "%s/VSWAP%s",
                          data_set->root, data_set->extension);
    return result >= 0 && (size_t)result < destination_size;
}

int WG_PagesOpen(wg_pages_t *pages, const wg_data_set_t *data_set)
{
    char path[1200];

    if (pages == NULL || data_set == NULL || data_set->pages == NULL)
    {
        return 0;
    }
    memset(pages, 0, sizeof(*pages));
    if (!WG_PagesPath(path, sizeof(path), data_set)
        || !WG_LoadFile(path, &pages->file))
    {
        return 0;
    }
    if (pages->file.size < 6U
        || WG_ReadLE16(pages->file.data) != data_set->page_count
        || WG_ReadLE16(pages->file.data + 2U) != data_set->sprite_start
        || WG_ReadLE16(pages->file.data + 4U) != data_set->sound_start)
    {
        WG_PagesClose(pages);
        return 0;
    }
    pages->data_set = data_set;
    return 1;
}

void WG_PagesClose(wg_pages_t *pages)
{
    if (pages == NULL)
    {
        return;
    }
    WG_FreeFile(&pages->file);
    pages->data_set = NULL;
}

int WG_PagesGet(const wg_pages_t *pages, size_t page,
                const uint8_t **data, size_t *size)
{
    const wg_page_entry_t *entry;

    if (pages == NULL || pages->data_set == NULL || data == NULL || size == NULL
        || page >= pages->data_set->page_count)
    {
        return 0;
    }
    *data = NULL;
    *size = 0;
    entry = &pages->data_set->pages[page];
    if (entry->offset == 0 || entry->length == 0
        || (size_t)entry->offset > pages->file.size
        || (size_t)entry->length > pages->file.size - (size_t)entry->offset)
    {
        return 0;
    }
    *data = pages->file.data + entry->offset;
    *size = entry->length;
    return 1;
}
