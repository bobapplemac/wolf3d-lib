/* Portable BuildTables, CalcProjection, and FinishSignon from WL_MAIN.C. */
#include "WL_MAIN.h"

#include <stddef.h>
#include <string.h>

#include "WG_FIXED.h"
#include "ID_VH.h"
#include "ID_VL.h"

#include "WG_VIEW_TABLES.inc"

int WL_DrawSignonPrompt(uint8_t framebuffer[320 * 200],
                        const wg_graphics_t *graphics, const char *text,
                        uint8_t color)
{
    wg_font_t font;
    size_t width;

    if (framebuffer == NULL || graphics == NULL || text == NULL)
    {
        return 0;
    }
    if (!WG_FontOpen(&font, graphics, 0U))
    {
        return 0;
    }

    /* The original clears only the first 300 pixels, retaining the version
       text at the lower right. Its fill color is the upper-left pixel. */
    WG_VideoBar(framebuffer, 0, 189, 300, 11, framebuffer[0]);
    width = WG_FontMeasure(&font, text);
    WG_FontDraw(&font, framebuffer, (320 - (int)width) / 2, 190,
                text, color);
    WG_FontClose(&font);
    return 1;
}

void WG_ViewBuildTrigTables(wg_view_tables_t *tables)
{
    if (tables == NULL)
    {
        return;
    }
    memcpy(tables->fine_tangent, wg_fine_tangent,
           sizeof(tables->fine_tangent));
    memcpy(tables->sine, wg_sine, sizeof(tables->sine));
}

int WG_ViewCalculateProjection(wg_view_tables_t *tables, uint16_t view_width,
                               int32_t focal_length)
{
    int32_t face_distance;
    unsigned table_index;
    size_t table_offset;
    int half_view;

    if (tables == NULL || view_width < 64U
        || view_width > WG_MAX_VIEW_WIDTH || (view_width & 15U) != 0U
        || focal_length <= 0)
    {
        return 0;
    }

    tables->focal_length = focal_length;
    tables->view_width = view_width;
    face_distance = focal_length + WG_MIN_DISTANCE;
    half_view = view_width / 2;
    tables->scale = (int32_t)(half_view * face_distance
                              / (WG_VIEW_GLOBAL / 2));
    tables->height_numerator =
        (int32_t)(((int64_t)WG_FIXED_ONE * tables->scale) / 64);
    tables->min_height_divisor = tables->height_numerator / 0x7fff + 1;

    table_index = (view_width - 64U) / 16U;
    table_offset = (size_t)8U * table_index * (table_index + 7U);
    memcpy(tables->pixel_angle, wg_pixel_angles + table_offset,
           (size_t)view_width * sizeof(tables->pixel_angle[0]));
    tables->max_slope = tables->fine_tangent[tables->pixel_angle[0]] / 256;
    return 1;
}

const int32_t *WG_ViewCosineTable(const wg_view_tables_t *tables)
{
    return tables == NULL ? NULL : &tables->sine[WG_ANGLE_QUADRANT];
}

uint16_t WG_PointToAngle(int32_t x, int32_t y)
{
    uint64_t magnitude_x;
    uint64_t magnitude_y;
    unsigned acute = 0U;
    unsigned acute_ceiling;
    unsigned angle;
    int on_boundary;

    if (x == 0 && y == 0)
    {
        return 0U;
    }
    magnitude_x = x < 0 ? (uint64_t)(-(int64_t)x) : (uint64_t)x;
    magnitude_y = y < 0 ? (uint64_t)(-(int64_t)y) : (uint64_t)y;
    on_boundary = magnitude_y == 0U;
    if (magnitude_x == 0U)
    {
        acute = WG_ANGLE_QUADRANT;
    }
    else
    {
        unsigned candidate;

        for (candidate = 1U; candidate < WG_ANGLE_QUADRANT; ++candidate)
        {
            uint64_t left = magnitude_y
                * wg_angle_sine_q32[WG_ANGLE_QUADRANT - candidate];
            uint64_t right = magnitude_x * wg_angle_sine_q32[candidate];

            if (left < right)
            {
                break;
            }
            acute = candidate;
            on_boundary = left == right;
        }
    }
    acute_ceiling = acute;
    if (!on_boundary && acute_ceiling < WG_ANGLE_QUADRANT)
    {
        ++acute_ceiling;
    }
    if (x >= 0)
    {
        angle = y >= 0 ? acute
                       : (WG_ANGLES - acute_ceiling) % WG_ANGLES;
        if (y < 0 && on_boundary && acute != 0U
            && acute != WG_ANGLE_QUADRANT)
        {
            --angle;
        }
    }
    else
    {
        angle = y >= 0 ? WG_ANGLES / 2U - acute_ceiling
                       : WG_ANGLES / 2U + acute;
        if (y < 0 && on_boundary && acute != 0U
            && acute != WG_ANGLE_QUADRANT)
        {
            --angle;
        }
    }
    return (uint16_t)angle;
}
