#ifndef WG_VIEW_H
#define WG_VIEW_H

#include <stdint.h>

#define WG_ANGLES 360
#define WG_ANGLE_QUADRANT (WG_ANGLES / 4)
#define WG_FINE_ANGLES 3600
#define WG_MAX_VIEW_WIDTH 320
#define WG_FOCAL_LENGTH INT32_C(0x5700)
#define WG_MIN_DISTANCE INT32_C(0x5800)
#define WG_VIEW_GLOBAL INT32_C(0x10000)

typedef struct wg_view_tables
{
    int32_t fine_tangent[WG_FINE_ANGLES / 4];
    int32_t sine[WG_ANGLES + WG_ANGLE_QUADRANT];
    int16_t pixel_angle[WG_MAX_VIEW_WIDTH];
    int32_t focal_length;
    int32_t scale;
    int32_t height_numerator;
    int32_t min_height_divisor;
    int32_t max_slope;
    uint16_t view_width;
} wg_view_tables_t;

void WG_ViewBuildTrigTables(wg_view_tables_t *tables);
int WG_ViewCalculateProjection(wg_view_tables_t *tables, uint16_t view_width,
                               int32_t focal_length);
const int32_t *WG_ViewCosineTable(const wg_view_tables_t *tables);

#endif
