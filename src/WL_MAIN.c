/* Portable BuildTables, CalcProjection, and FinishSignon from WL_MAIN.C. */
#include "WL_MAIN.h"

#include <math.h>
#include <stddef.h>

#include "WG_FIXED.h"
#include "ID_VH.h"
#include "ID_VL.h"

#define WG_PI 3.141592657

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
    const float radians_to_fine = (float)(WG_FINE_ANGLES / 2 / WG_PI);
    const float angle_step = (float)(WG_PI / 2 / WG_ANGLE_QUADRANT);
    float angle = 0.0F;
    int i;

    if (tables == NULL)
    {
        return;
    }

    for (i = 0; i < WG_FINE_ANGLES / 8; ++i)
    {
        double tangent = tan((i + 0.5) / radians_to_fine);

        tables->fine_tangent[i] = (int32_t)(tangent * WG_FIXED_ONE);
        tables->fine_tangent[WG_FINE_ANGLES / 4 - 1 - i] =
            (int32_t)((1.0 / tangent) * WG_FIXED_ONE);
    }

    for (i = 0; i < WG_ANGLE_QUADRANT; ++i)
    {
        int32_t value = (int32_t)(WG_FIXED_ONE * sin((double)angle));

        tables->sine[i] = value;
        tables->sine[i + WG_ANGLES] = value;
        tables->sine[WG_ANGLES / 2 - i] = value;
        tables->sine[WG_ANGLES - i] = -value;
        tables->sine[WG_ANGLES / 2 + i] = -value;
        angle += angle_step;
    }
    tables->sine[WG_ANGLE_QUADRANT] = WG_FIXED_ONE;
    tables->sine[3 * WG_ANGLE_QUADRANT] = -WG_FIXED_ONE;
}

int WG_ViewCalculateProjection(wg_view_tables_t *tables, uint16_t view_width,
                               int32_t focal_length)
{
    const float radians_to_fine = (float)(WG_FINE_ANGLES / 2 / WG_PI);
    double face_distance;
    int half_view;
    int i;

    if (tables == NULL || view_width < 2U
        || view_width > WG_MAX_VIEW_WIDTH || (view_width & 1U) != 0U
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

    for (i = 0; i < half_view; ++i)
    {
        double tangent = ((int32_t)i * WG_VIEW_GLOBAL / view_width)
                         / face_distance;
        float angle = (float)atan(tangent);
        int angle_offset = (int)(angle * radians_to_fine);

        tables->pixel_angle[half_view - 1 - i] = (int16_t)angle_offset;
        tables->pixel_angle[half_view + i] = (int16_t)-angle_offset;
    }
    tables->max_slope = tables->fine_tangent[tables->pixel_angle[0]] / 256;
    return 1;
}

const int32_t *WG_ViewCosineTable(const wg_view_tables_t *tables)
{
    return tables == NULL ? NULL : &tables->sine[WG_ANGLE_QUADRANT];
}
