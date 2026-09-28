#include "WG_TEXT_OUTPUT.h"

#include <stddef.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int WG_TextOutputSupportsColor(FILE *stream)
{
    if (stream == NULL)
    {
        return 0;
    }
#ifdef _WIN32
    return _isatty(_fileno(stream));
#else
    return isatty(fileno(stream));
#endif
}

static uint32_t WG_CP437CodePoint(uint8_t character)
{
    static const uint16_t controls[32] =
    {
        0x0020, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
        0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
        0x25ba, 0x25c4, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8,
        0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc
    };
    static const uint16_t extended[128] =
    {
        0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
        0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
        0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
        0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
        0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
        0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
        0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
        0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
        0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
        0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
        0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
        0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
        0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
        0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
    };

    if (character < 32U)
    {
        return controls[character];
    }
    if (character == 127U)
    {
        return 0x2302U;
    }
    return character < 128U ? character : extended[character - 128U];
}

static void WG_WriteUTF8(FILE *stream, uint32_t code_point)
{
    uint8_t bytes[3];
    size_t count;

    if (code_point < 0x80U)
    {
        bytes[0] = (uint8_t)code_point;
        count = 1U;
    }
    else if (code_point < 0x800U)
    {
        bytes[0] = (uint8_t)(0xc0U | (code_point >> 6));
        bytes[1] = (uint8_t)(0x80U | (code_point & 0x3fU));
        count = 2U;
    }
    else
    {
        bytes[0] = (uint8_t)(0xe0U | (code_point >> 12));
        bytes[1] = (uint8_t)(0x80U | ((code_point >> 6) & 0x3fU));
        bytes[2] = (uint8_t)(0x80U | (code_point & 0x3fU));
        count = 3U;
    }
    (void)fwrite(bytes, 1U, count, stream);
}

static unsigned WG_ANSIForeground(unsigned color)
{
    static const uint8_t normal[8] = { 30, 34, 32, 36, 31, 35, 33, 37 };
    return normal[color & 7U] + (color >= 8U ? 60U : 0U);
}

static unsigned WG_ANSIBackground(unsigned color)
{
    static const uint8_t normal[8] = { 40, 44, 42, 46, 41, 45, 43, 47 };
    return normal[color & 7U];
}

void WG_WriteTextScreen(FILE *stream, const uint8_t *cells,
                        uint16_t columns, uint16_t rows, int color)
{
    uint16_t y;

    if (stream == NULL || cells == NULL || columns == 0U || rows == 0U)
    {
        return;
    }
    if (color)
    {
        (void)fputs("\x1b[0m\x1b[2J\x1b[H", stream);
    }
    for (y = 0U; y < rows; ++y)
    {
        uint16_t limit = columns;
        uint16_t x;
        uint8_t old_attribute = 0xffU;

        if (color)
        {
            (void)fprintf(stream, "\x1b[%u;1H", (unsigned)y + 1U);
        }
        else
        {
            while (limit != 0U
                   && cells[((size_t)y * columns + limit - 1U) * 2U] == ' ')
            {
                --limit;
            }
        }
        for (x = 0U; x < limit; ++x)
        {
            size_t offset = ((size_t)y * columns + x) * 2U;
            uint8_t attribute = cells[offset + 1U];

            if (color && attribute != old_attribute)
            {
                (void)fprintf(stream, "\x1b[0;%u;%u%s",
                              WG_ANSIForeground(attribute & 15U),
                              WG_ANSIBackground((attribute >> 4) & 7U),
                              (attribute & 0x80U) != 0U ? ";5m" : "m");
                old_attribute = attribute;
            }
            WG_WriteUTF8(stream, WG_CP437CodePoint(cells[offset]));
        }
        if (!color)
        {
            (void)fputc('\n', stream);
        }
    }
    if (color)
    {
        (void)fprintf(stream, "\x1b[0m\x1b[%u;1H",
                      rows > 1U ? (unsigned)rows - 1U : 1U);
    }
    (void)fflush(stream);
}
