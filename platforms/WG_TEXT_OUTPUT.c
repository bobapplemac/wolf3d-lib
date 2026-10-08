#include "WG_TEXT_OUTPUT.h"

#include <stddef.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
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

uint16_t WG_TextScreenContentRows(const uint8_t *cells,
                                  uint16_t columns, uint16_t rows)
{
    uint16_t content_rows = 0U;
    uint16_t y;

    if (cells == NULL || columns == 0U)
    {
        return 0U;
    }
    for (y = 0U; y < rows; ++y)
    {
        uint16_t x;

        for (x = 0U; x < columns; ++x)
        {
            uint8_t character = cells[((size_t)y * columns + x) * 2U];

            if (character != 0U && character != ' ')
            {
                content_rows = (uint16_t)(y + 1U);
                break;
            }
        }
    }
    return content_rows;
}

#ifdef _WIN32
void WG_WriteWindowsTextScreen(const uint8_t *cells,
                               uint16_t columns, uint16_t rows)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    CHAR_INFO *characters;
    CHAR_INFO *prompt = NULL;
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    COORD size;
    COORD origin = { 0, 0 };
    SMALL_RECT rectangle;
    SHORT prompt_length = 0;
    uint16_t content_rows;
    UINT previous_output_code_page;
    size_t cell_count;
    size_t index;

    if (cells == NULL || columns == 0U || rows == 0U)
    {
        return;
    }
    if (output == NULL || output == INVALID_HANDLE_VALUE
        || !GetConsoleMode(output, &mode))
    {
        (void)AttachConsole(ATTACH_PARENT_PROCESS);
        output = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
    }
    if (output == NULL || output == INVALID_HANDLE_VALUE)
    {
        WG_WriteTextScreen(stdout, cells, columns, rows, 0);
        return;
    }
    if (GetConsoleScreenBufferInfo(output, &console_info)
        && console_info.dwCursorPosition.X > 0)
    {
        COORD prompt_size;
        COORD prompt_origin = { 0, 0 };
        SMALL_RECT prompt_rectangle;

        prompt_length = console_info.dwCursorPosition.X;
        prompt = (CHAR_INFO *)malloc((size_t)prompt_length * sizeof(*prompt));
        prompt_size.X = prompt_length;
        prompt_size.Y = 1;
        prompt_rectangle.Left = 0;
        prompt_rectangle.Top = console_info.dwCursorPosition.Y;
        prompt_rectangle.Right = (SHORT)(prompt_length - 1);
        prompt_rectangle.Bottom = console_info.dwCursorPosition.Y;
        if (prompt == NULL
            || !ReadConsoleOutputW(output, prompt, prompt_size, prompt_origin,
                                   &prompt_rectangle))
        {
            free(prompt);
            prompt = NULL;
            prompt_length = 0;
        }
    }
    cell_count = (size_t)columns * rows;
    characters = (CHAR_INFO *)malloc(cell_count * sizeof(*characters));
    if (characters == NULL)
    {
        free(prompt);
        return;
    }
    for (index = 0U; index < cell_count; ++index)
    {
        characters[index].Char.AsciiChar = (CHAR)cells[index * 2U];
        characters[index].Attributes = cells[index * 2U + 1U];
    }
    size.X = (SHORT)columns;
    size.Y = (SHORT)rows;
    rectangle.Left = 0;
    rectangle.Top = 0;
    rectangle.Right = (SHORT)(columns - 1U);
    rectangle.Bottom = (SHORT)(rows - 1U);
    previous_output_code_page = GetConsoleOutputCP();
    (void)SetConsoleOutputCP(437U);
    (void)WriteConsoleOutputA(output, characters, size, origin, &rectangle);
    free(characters);

    content_rows = WG_TextScreenContentRows(cells, columns, rows);
    origin.X = 0;
    origin.Y = (SHORT)(content_rows != 0U ? content_rows : 1U);
    if (GetConsoleScreenBufferInfo(output, &console_info)
        && origin.Y >= console_info.dwSize.Y)
    {
        origin.Y = (SHORT)(console_info.dwSize.Y - 1);
    }
    if (prompt != NULL)
    {
        COORD prompt_size;
        COORD prompt_origin = { 0, 0 };
        SMALL_RECT prompt_rectangle;

        prompt_size.X = prompt_length;
        prompt_size.Y = 1;
        prompt_rectangle.Left = 0;
        prompt_rectangle.Top = origin.Y;
        prompt_rectangle.Right = (SHORT)(prompt_length - 1);
        prompt_rectangle.Bottom = origin.Y;
        (void)WriteConsoleOutputW(output, prompt, prompt_size, prompt_origin,
                                  &prompt_rectangle);
        origin.X = prompt_length;
    }
    (void)SetConsoleCursorPosition(output, origin);
    if (previous_output_code_page != 0U)
    {
        (void)SetConsoleOutputCP(previous_output_code_page);
    }
    free(prompt);
}
#endif

void WG_WriteTextScreen(FILE *stream, const uint8_t *cells,
                        uint16_t columns, uint16_t rows, int color)
{
    uint16_t content_rows;
    uint16_t y;

    if (stream == NULL || cells == NULL || columns == 0U || rows == 0U)
    {
        return;
    }
    content_rows = WG_TextScreenContentRows(cells, columns, rows);
    if (content_rows == 0U)
    {
        content_rows = 1U;
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
        if (!color && y < content_rows)
        {
            (void)fputc('\n', stream);
        }
    }
    if (color)
    {
        (void)fprintf(stream, "\x1b[0m\x1b[%u;1H",
                      content_rows < rows ? (unsigned)content_rows + 1U
                                          : (unsigned)rows);
    }
    (void)fflush(stream);
}
