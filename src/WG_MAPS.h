#ifndef WG_MAPS_H
#define WG_MAPS_H

#include <stddef.h>
#include <stdint.h>

#include "WG_DATA.h"
#include "WG_FILE.h"

#define WG_MAP_PLANES 2

typedef struct wg_map
{
    uint16_t width;
    uint16_t height;
    char name[17];
    uint16_t *planes[WG_MAP_PLANES];
} wg_map_t;

typedef struct wg_maps
{
    uint16_t rlew_tag;
    uint32_t *header_offsets;
    size_t header_offset_count;
    wg_file_buffer_t data;
} wg_maps_t;

int WG_MapsOpen(wg_maps_t *maps, const wg_data_set_t *data_set);
void WG_MapsClose(wg_maps_t *maps);
int WG_MapsLoad(const wg_maps_t *maps, size_t map_number, wg_map_t *map);
void WG_MapFree(wg_map_t *map);

#endif
