#include "WG_PLATFORM.h"
#include "../WG_HOST.h"
#include "../WG_TEXT_OUTPUT.h"

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <xinput.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const wchar_t wg_window_class[] = L"wolf3dgeneric-window";
static HWND wg_window;
static uint32_t wg_pixels[WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT];
static LARGE_INTEGER wg_counter_frequency;
static LARGE_INTEGER wg_counter_start;
static int wg_quit_pending;
static uint8_t wg_text_screen[WG_TEXT_COLUMNS * WG_TEXT_ROWS
                              * WG_TEXT_CELL_BYTES];
static uint16_t wg_text_columns;
static uint16_t wg_text_rows;
static HANDLE wg_console_output;

typedef DWORD (WINAPI *wg_xinput_get_state_t)(DWORD, XINPUT_STATE *);
static HMODULE wg_xinput_module;
static wg_xinput_get_state_t wg_xinput_get_state;
static int16_t wg_joystick_x[WG_MAX_JOYSTICKS];
static int16_t wg_joystick_y[WG_MAX_JOYSTICKS];
static uint32_t wg_joystick_buttons[WG_MAX_JOYSTICKS];
static uint8_t wg_joystick_connected[WG_MAX_JOYSTICKS];

#define WG_PCM_BUFFER_COUNT 4U
#define WG_PCM_BUFFER_FRAMES 512U
#define WG_DISPLAY_ASPECT_WIDTH 4
#define WG_DISPLAY_ASPECT_HEIGHT 3
#define WG_INITIAL_SCALE 3
static HWAVEOUT wg_wave_out;
static WAVEHDR wg_wave_headers[WG_PCM_BUFFER_COUNT];
static int16_t wg_wave_samples[WG_PCM_BUFFER_COUNT]
                              [WG_PCM_BUFFER_FRAMES * 2U];
static uint8_t wg_wave_used[WG_PCM_BUFFER_COUNT];

static void WG_Win32PCMShutdown(void);

static HANDLE wg_console_output_handle(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (wg_console_output != NULL
        && wg_console_output != INVALID_HANDLE_VALUE)
    {
        return wg_console_output;
    }
    if (output != NULL && output != INVALID_HANDLE_VALUE
        && GetConsoleMode(output, &mode))
    {
        wg_console_output = output;
        return output;
    }
    if ((output == NULL || output == INVALID_HANDLE_VALUE)
        && AttachConsole(ATTACH_PARENT_PROCESS))
    {
        output = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
        if (output != INVALID_HANDLE_VALUE)
        {
            wg_console_output = output;
            return output;
        }
    }
    return NULL;
}

static void wg_write_text_screen(void)
{
    HANDLE output;
    CHAR_INFO characters[WG_TEXT_COLUMNS * WG_TEXT_ROWS];
    COORD size;
    COORD origin = { 0, 0 };
    SMALL_RECT rectangle;
    size_t index;

    if (wg_text_columns == 0U || wg_text_rows == 0U)
    {
        return;
    }
    output = wg_console_output_handle();
    if (output == NULL)
    {
        WG_WriteTextScreen(stdout, wg_text_screen, wg_text_columns,
                           wg_text_rows, 0);
        return;
    }
    for (index = 0U; index < (size_t)wg_text_columns * wg_text_rows; ++index)
    {
        characters[index].Char.AsciiChar = (CHAR)wg_text_screen[index * 2U];
        characters[index].Attributes = wg_text_screen[index * 2U + 1U];
    }
    size.X = (SHORT)wg_text_columns;
    size.Y = (SHORT)wg_text_rows;
    rectangle.Left = 0;
    rectangle.Top = 0;
    rectangle.Right = (SHORT)(wg_text_columns - 1U);
    rectangle.Bottom = (SHORT)(wg_text_rows - 1U);
    (void)SetConsoleOutputCP(437U);
    (void)WriteConsoleOutputA(output, characters, size, origin, &rectangle);
    origin.X = 0;
    origin.Y = (SHORT)wg_text_rows;
    (void)SetConsoleCursorPosition(output, origin);
    wg_text_columns = 0U;
    wg_text_rows = 0U;
}

static RECT wg_presentation_rectangle(const RECT *client)
{
    RECT presentation = { 0, 0, 0, 0 };
    LONG client_width;
    LONG client_height;
    LONG width;
    LONG height;

    if (client == NULL)
    {
        return presentation;
    }
    client_width = client->right - client->left;
    client_height = client->bottom - client->top;
    if (client_width <= 0 || client_height <= 0)
    {
        return presentation;
    }

    if ((int64_t)client_width * WG_DISPLAY_ASPECT_HEIGHT
        > (int64_t)client_height * WG_DISPLAY_ASPECT_WIDTH)
    {
        height = client_height;
        width = (height * WG_DISPLAY_ASPECT_WIDTH)
              / WG_DISPLAY_ASPECT_HEIGHT;
    }
    else
    {
        width = client_width;
        height = (width * WG_DISPLAY_ASPECT_HEIGHT)
               / WG_DISPLAY_ASPECT_WIDTH;
    }
    presentation.left = client->left + (client_width - width) / 2;
    presentation.top = client->top + (client_height - height) / 2;
    presentation.right = presentation.left + width;
    presentation.bottom = presentation.top + height;
    return presentation;
}

#define WG_EVENT_QUEUE_CAPACITY 64U
static wg_event_t wg_event_queue[WG_EVENT_QUEUE_CAPACITY];
static size_t wg_event_read;
static size_t wg_event_write;

static void wg_queue_event(const wg_event_t *event)
{
    size_t next = (wg_event_write + 1U) % WG_EVENT_QUEUE_CAPACITY;

    if (next != wg_event_read)
    {
        wg_event_queue[wg_event_write] = *event;
        wg_event_write = next;
    }
}

static void wg_queue_mouse_button(uint8_t button, int pressed)
{
    wg_event_t event = { 0 };

    event.type = WG_EVENT_MOUSE_BUTTON;
    event.pressed = pressed;
    event.key = 0;
    event.x = 0;
    event.y = 0;
    event.button = button;
    wg_queue_event(&event);
}

static void wg_load_xinput(void)
{
    static const wchar_t *const libraries[] =
    {
        L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"
    };
    size_t index;

    for (index = 0U; index < sizeof(libraries) / sizeof(libraries[0]); ++index)
    {
        wg_xinput_module = LoadLibraryW(libraries[index]);
        if (wg_xinput_module != NULL)
        {
            wg_xinput_get_state = (wg_xinput_get_state_t)GetProcAddress(
                wg_xinput_module, "XInputGetState");
            if (wg_xinput_get_state != NULL)
            {
                return;
            }
            FreeLibrary(wg_xinput_module);
            wg_xinput_module = NULL;
        }
    }
}

static void wg_poll_joysticks(void)
{
    DWORD joystick;

    if (wg_xinput_get_state == NULL)
    {
        return;
    }
    for (joystick = 0U; joystick < WG_MAX_JOYSTICKS; ++joystick)
    {
        XINPUT_STATE state;
        int connected;
        int16_t x = 0;
        int16_t y = 0;
        uint32_t buttons = 0U;

        ZeroMemory(&state, sizeof(state));
        connected = wg_xinput_get_state(joystick, &state) == ERROR_SUCCESS;
        if (connected)
        {
            WORD source = state.Gamepad.wButtons;
            SHORT source_y = state.Gamepad.sThumbLY;

            x = state.Gamepad.sThumbLX;
            y = source_y == INT16_MIN ? INT16_MAX : (int16_t)-source_y;
            if ((source & XINPUT_GAMEPAD_DPAD_LEFT) != 0U)
            {
                x = INT16_MIN;
            }
            else if ((source & XINPUT_GAMEPAD_DPAD_RIGHT) != 0U)
            {
                x = INT16_MAX;
            }
            if ((source & XINPUT_GAMEPAD_DPAD_UP) != 0U)
            {
                y = INT16_MIN;
            }
            else if ((source & XINPUT_GAMEPAD_DPAD_DOWN) != 0U)
            {
                y = INT16_MAX;
            }
            buttons |= (source & XINPUT_GAMEPAD_A) != 0U ? 1U : 0U;
            buttons |= (source & XINPUT_GAMEPAD_B) != 0U ? 2U : 0U;
            buttons |= (source & XINPUT_GAMEPAD_X) != 0U ? 4U : 0U;
            buttons |= (source & XINPUT_GAMEPAD_Y) != 0U ? 8U : 0U;
        }
        if (connected != wg_joystick_connected[joystick]
            || x != wg_joystick_x[joystick]
            || y != wg_joystick_y[joystick]
            || buttons != wg_joystick_buttons[joystick])
        {
            wg_event_t event = { 0 };

            wg_joystick_connected[joystick] = (uint8_t)connected;
            wg_joystick_x[joystick] = x;
            wg_joystick_y[joystick] = y;
            wg_joystick_buttons[joystick] = buttons;
            event.type = WG_EVENT_JOYSTICK;
            event.joystick = (uint8_t)joystick;
            event.connected = (uint8_t)connected;
            event.x = x;
            event.y = y;
            event.buttons = buttons;
            wg_queue_event(&event);
        }
    }
}

static LRESULT CALLBACK wg_window_proc(HWND window, UINT message,
                                       WPARAM wparam, LPARAM lparam)
{
    (void)wparam;

    switch (message)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            wg_event_t event = { 0 };

            event.type = WG_EVENT_KEY;
            event.pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            event.key = wparam == VK_PAUSE
                            ? WG_KEY_PAUSE
                            : (uint16_t)((lparam >> 16) & 0xff);
            event.x = 0;
            event.y = 0;
            event.button = 0;
            wg_queue_event(&event);
            return 0;
        }

        case WM_INPUT:
        {
            RAWINPUT input;
            UINT size = sizeof(input);

            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &input, &size,
                                sizeof(RAWINPUTHEADER)) == sizeof(input)
                && input.header.dwType == RIM_TYPEMOUSE)
            {
                LONG x = input.data.mouse.lLastX;
                LONG y = input.data.mouse.lLastY;
                USHORT buttons = input.data.mouse.usButtonFlags;

                if (x != 0 || y != 0)
                {
                    wg_event_t event = { 0 };

                    event.type = WG_EVENT_MOUSE_MOTION;
                    event.pressed = 0;
                    event.key = 0;
                    event.x = (int16_t)(x > INT16_MAX ? INT16_MAX
                                         : x < INT16_MIN ? INT16_MIN : x);
                    event.y = (int16_t)(y > INT16_MAX ? INT16_MAX
                                         : y < INT16_MIN ? INT16_MIN : y);
                    event.button = 0;
                    wg_queue_event(&event);
                }
                if ((buttons & RI_MOUSE_LEFT_BUTTON_DOWN) != 0U)
                {
                    wg_queue_mouse_button(1U, 1);
                }
                if ((buttons & RI_MOUSE_LEFT_BUTTON_UP) != 0U)
                {
                    wg_queue_mouse_button(1U, 0);
                }
                if ((buttons & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0U)
                {
                    wg_queue_mouse_button(2U, 1);
                }
                if ((buttons & RI_MOUSE_RIGHT_BUTTON_UP) != 0U)
                {
                    wg_queue_mouse_button(2U, 0);
                }
                if ((buttons & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0U)
                {
                    wg_queue_mouse_button(3U, 1);
                }
                if ((buttons & RI_MOUSE_MIDDLE_BUTTON_UP) != 0U)
                {
                    wg_queue_mouse_button(3U, 0);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            wg_quit_pending = 1;
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            wg_window = NULL;
            wg_quit_pending = 1;
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wparam, lparam);
    }
}

static int WG_Win32Init(void)
{
    RAWINPUTDEVICE mouse;
    WNDCLASSW window_class;
    RECT rectangle;
    HINSTANCE instance;

    instance = GetModuleHandleW(NULL);
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = wg_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = wg_window_class;

    if (!RegisterClassW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        return 0;
    }

    rectangle.left = 0;
    rectangle.top = 0;
    rectangle.right = WG_SCREEN_WIDTH * WG_INITIAL_SCALE;
    rectangle.bottom = (WG_SCREEN_WIDTH * WG_DISPLAY_ASPECT_HEIGHT
                        / WG_DISPLAY_ASPECT_WIDTH) * WG_INITIAL_SCALE;
    AdjustWindowRect(&rectangle, WS_OVERLAPPEDWINDOW, FALSE);

    wg_window = CreateWindowExW(0, wg_window_class, L"wolf3dgeneric",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                rectangle.right - rectangle.left,
                                rectangle.bottom - rectangle.top, NULL, NULL,
                                instance, NULL);
    if (wg_window == NULL)
    {
        return 0;
    }

    mouse.usUsagePage = 0x01U;
    mouse.usUsage = 0x02U;
    mouse.dwFlags = 0U;
    mouse.hwndTarget = wg_window;
    if (!RegisterRawInputDevices(&mouse, 1U, sizeof(mouse)))
    {
        DestroyWindow(wg_window);
        wg_window = NULL;
        return 0;
    }

    QueryPerformanceFrequency(&wg_counter_frequency);
    QueryPerformanceCounter(&wg_counter_start);
    wg_event_read = 0;
    wg_event_write = 0;
    wg_quit_pending = 0;
    ZeroMemory(wg_joystick_x, sizeof(wg_joystick_x));
    ZeroMemory(wg_joystick_y, sizeof(wg_joystick_y));
    ZeroMemory(wg_joystick_buttons, sizeof(wg_joystick_buttons));
    ZeroMemory(wg_joystick_connected, sizeof(wg_joystick_connected));
    wg_load_xinput();
    ShowWindow(wg_window, SW_SHOW);
    return 1;
}

static void WG_Win32Shutdown(void)
{
    WG_Win32PCMShutdown();
    wg_xinput_get_state = NULL;
    if (wg_xinput_module != NULL)
    {
        FreeLibrary(wg_xinput_module);
        wg_xinput_module = NULL;
    }
    if (wg_window != NULL)
    {
        DestroyWindow(wg_window);
        wg_window = NULL;
    }
    UnregisterClassW(wg_window_class, GetModuleHandleW(NULL));
    wg_write_text_screen();
}

static void WG_Win32Present(const uint8_t *pixels, const uint8_t *palette)
{
    BITMAPINFO bitmap_info;
    RECT client;
    RECT presentation;
    HDC device_context;
    size_t index;

    if (wg_window == NULL)
    {
        return;
    }

    for (index = 0; index < WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT; ++index)
    {
        size_t color = (size_t)pixels[index] * 3U;
        wg_pixels[index] = ((uint32_t)palette[color] << 16)
                         | ((uint32_t)palette[color + 1U] << 8)
                         | (uint32_t)palette[color + 2U];
    }

    ZeroMemory(&bitmap_info, sizeof(bitmap_info));
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = WG_SCREEN_WIDTH;
    bitmap_info.bmiHeader.biHeight = -WG_SCREEN_HEIGHT;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    GetClientRect(wg_window, &client);
    presentation = wg_presentation_rectangle(&client);
    device_context = GetDC(wg_window);
    PatBlt(device_context, client.left, client.top,
           client.right - client.left, presentation.top - client.top,
           BLACKNESS);
    PatBlt(device_context, client.left, presentation.bottom,
           client.right - client.left, client.bottom - presentation.bottom,
           BLACKNESS);
    PatBlt(device_context, client.left, presentation.top,
           presentation.left - client.left,
           presentation.bottom - presentation.top, BLACKNESS);
    PatBlt(device_context, presentation.right, presentation.top,
           client.right - presentation.right,
           presentation.bottom - presentation.top, BLACKNESS);
    SetStretchBltMode(device_context, COLORONCOLOR);
    StretchDIBits(device_context, presentation.left, presentation.top,
                  presentation.right - presentation.left,
                  presentation.bottom - presentation.top,
                  0, 0, WG_SCREEN_WIDTH, WG_SCREEN_HEIGHT, wg_pixels,
                  &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(wg_window, device_context);
}

static uint32_t WG_Win32GetTicksMs(void)
{
    LARGE_INTEGER now;
    uint64_t ticks;

    QueryPerformanceCounter(&now);
    ticks = (uint64_t)(now.QuadPart - wg_counter_start.QuadPart);
    return (uint32_t)((ticks * 1000U) / (uint64_t)wg_counter_frequency.QuadPart);
}

static void WG_Win32SleepMs(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

static int WG_Win32PollEvent(wg_event_t *event)
{
    MSG message;

    if (event == NULL)
    {
        return 0;
    }

    if (wg_quit_pending)
    {
        wg_quit_pending = 0;
        event->type = WG_EVENT_QUIT;
        return 1;
    }

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            event->type = WG_EVENT_QUIT;
            return 1;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    wg_poll_joysticks();

    if (wg_event_read != wg_event_write)
    {
        *event = wg_event_queue[wg_event_read];
        wg_event_read = (wg_event_read + 1U) % WG_EVENT_QUEUE_CAPACITY;
        return 1;
    }

    event->type = WG_EVENT_NONE;
    return 0;
}

static int WG_Win32IsInteractive(void)
{
    return 1;
}

static void WG_Win32SetWindowTitle(const char *title)
{
    wchar_t wide_title[256];

    if (wg_window == NULL)
    {
        return;
    }

    MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title,
                        (int)(sizeof(wide_title) / sizeof(wide_title[0])));
    SetWindowTextW(wg_window, wide_title);
}

static void WG_Win32ReportError(const char *message)
{
    HANDLE output = wg_console_output_handle();

    if (output != NULL)
    {
        DWORD written;
        static const char prefix[] = "wolf3dgeneric: ";
        static const char newline[] = "\r\n";

        (void)WriteFile(output, prefix, (DWORD)(sizeof(prefix) - 1U),
                        &written, NULL);
        (void)WriteFile(output, message, (DWORD)strlen(message),
                        &written, NULL);
        (void)WriteFile(output, newline, (DWORD)(sizeof(newline) - 1U),
                        &written, NULL);
    }
    MessageBoxA(wg_window, message, "wolf3dgeneric", MB_OK | MB_ICONERROR);
}

static void WG_Win32PrintMessage(const char *message)
{
    HANDLE output = wg_console_output_handle();

    if (output != NULL)
    {
        DWORD written;
        static const char newline[] = "\r\n";

        (void)WriteFile(output, message, (DWORD)strlen(message),
                        &written, NULL);
        (void)WriteFile(output, newline, (DWORD)(sizeof(newline) - 1U),
                        &written, NULL);
    }
    else
    {
        fprintf(stdout, "%s\n", message);
        fflush(stdout);
    }
}

static void WG_Win32PresentText(const uint8_t *cells, uint16_t columns,
                                uint16_t rows)
{
    size_t size = (size_t)columns * rows * WG_TEXT_CELL_BYTES;

    if (cells == NULL || columns > WG_TEXT_COLUMNS || rows > WG_TEXT_ROWS
        || size > sizeof(wg_text_screen))
    {
        return;
    }
    memcpy(wg_text_screen, cells, size);
    wg_text_columns = columns;
    wg_text_rows = rows;
}

static int WG_Win32PCMInit(uint32_t sample_rate, uint16_t channels)
{
    WAVEFORMATEX format;
    size_t index;

    if (sample_rate == 0U || channels != 2U)
    {
        return 0;
    }
    WG_Win32PCMShutdown();
    ZeroMemory(&format, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = channels;
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16U;
    format.nBlockAlign = (WORD)(channels * sizeof(int16_t));
    format.nAvgBytesPerSec = sample_rate * format.nBlockAlign;
    if (waveOutOpen(&wg_wave_out, WAVE_MAPPER, &format, 0, 0,
                    CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        wg_wave_out = NULL;
        return 0;
    }
    ZeroMemory(wg_wave_headers, sizeof(wg_wave_headers));
    ZeroMemory(wg_wave_used, sizeof(wg_wave_used));
    for (index = 0U; index < WG_PCM_BUFFER_COUNT; ++index)
    {
        wg_wave_headers[index].lpData = (LPSTR)wg_wave_samples[index];
        wg_wave_headers[index].dwBufferLength =
            (DWORD)sizeof(wg_wave_samples[index]);
        if (waveOutPrepareHeader(wg_wave_out, &wg_wave_headers[index],
                                 sizeof(wg_wave_headers[index]))
            != MMSYSERR_NOERROR)
        {
            WG_Win32PCMShutdown();
            return 0;
        }
    }
    return 1;
}

static void WG_Win32PCMShutdown(void)
{
    size_t index;

    if (wg_wave_out == NULL)
    {
        return;
    }
    waveOutReset(wg_wave_out);
    for (index = 0U; index < WG_PCM_BUFFER_COUNT; ++index)
    {
        if ((wg_wave_headers[index].dwFlags & WHDR_PREPARED) != 0U)
        {
            waveOutUnprepareHeader(wg_wave_out, &wg_wave_headers[index],
                                   sizeof(wg_wave_headers[index]));
        }
    }
    waveOutClose(wg_wave_out);
    wg_wave_out = NULL;
    ZeroMemory(wg_wave_headers, sizeof(wg_wave_headers));
    ZeroMemory(wg_wave_used, sizeof(wg_wave_used));
}

static size_t WG_Win32PCMWritableFrames(void)
{
    size_t index;

    if (wg_wave_out == NULL)
    {
        return 0U;
    }
    for (index = 0U; index < WG_PCM_BUFFER_COUNT; ++index)
    {
        if (wg_wave_used[index]
            && (wg_wave_headers[index].dwFlags & WHDR_DONE) != 0U)
        {
            wg_wave_used[index] = 0U;
        }
        if (!wg_wave_used[index])
        {
            return WG_PCM_BUFFER_FRAMES;
        }
    }
    return 0U;
}

static int WG_Win32PCMSubmit(const int16_t *samples, size_t frame_count)
{
    size_t index;

    if (wg_wave_out == NULL || samples == NULL || frame_count == 0U
        || frame_count > WG_PCM_BUFFER_FRAMES)
    {
        return 0;
    }
    for (index = 0U; index < WG_PCM_BUFFER_COUNT; ++index)
    {
        if (!wg_wave_used[index])
        {
            size_t byte_count = frame_count * 2U * sizeof(*samples);

            memcpy(wg_wave_samples[index], samples, byte_count);
            wg_wave_headers[index].dwBufferLength = (DWORD)byte_count;
            wg_wave_headers[index].dwFlags &= ~WHDR_DONE;
            if (waveOutWrite(wg_wave_out, &wg_wave_headers[index],
                             sizeof(wg_wave_headers[index]))
                != MMSYSERR_NOERROR)
            {
                return 0;
            }
            wg_wave_used[index] = 1U;
            return 1;
        }
    }
    return 0;
}

int WG_InstallPlatform(void)
{
    static const wg_platform_api_t platform =
    {
        WG_PLATFORM_API_VERSION,
        sizeof(wg_platform_api_t),
        WG_Win32Init,
        WG_Win32Shutdown,
        WG_Win32Present,
        WG_Win32GetTicksMs,
        WG_Win32SleepMs,
        WG_Win32PollEvent,
        WG_Win32IsInteractive,
        WG_Win32SetWindowTitle,
        WG_Win32PrintMessage,
        WG_Win32ReportError,
        WG_Win32PresentText,
        WG_Win32PCMInit,
        WG_Win32PCMShutdown,
        WG_Win32PCMWritableFrames,
        WG_Win32PCMSubmit
    };

    return wolf3dgeneric_SetPlatform(&platform) == WG_RESULT_OK;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                    PWSTR command_line, int show_command)
{
    int argc;
    wchar_t **wide_argv;
    wchar_t module_path[32768];
    const wchar_t *wide_argument;
    DWORD module_path_length;
    char **argv;
    int index;
    int exit_code;
    wg_result_t result;

    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    if (!WG_InstallPlatform())
    {
        return 1;
    }

    wide_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wide_argv == NULL)
    {
        return 1;
    }

    module_path_length = GetModuleFileNameW(NULL, module_path,
                                           (DWORD)(sizeof(module_path)
                                                   / sizeof(module_path[0])));

    argv = (char **)LocalAlloc(LMEM_FIXED, (size_t)argc * sizeof(*argv));
    if (argv == NULL)
    {
        LocalFree(wide_argv);
        return 1;
    }

    for (index = 0; index < argc; ++index)
    {
        int bytes;

        wide_argument = index == 0 && module_path_length > 0U
                                && module_path_length
                                       < (DWORD)(sizeof(module_path)
                                                 / sizeof(module_path[0]))
                            ? module_path : wide_argv[index];
        bytes = WideCharToMultiByte(CP_UTF8, 0, wide_argument, -1,
                                    NULL, 0, NULL, NULL);
        argv[index] = (char *)LocalAlloc(LMEM_FIXED, (size_t)bytes);
        if (argv[index] == NULL)
        {
            while (index-- > 0)
            {
                LocalFree(argv[index]);
            }
            LocalFree(argv);
            LocalFree(wide_argv);
            return 1;
        }
        WideCharToMultiByte(CP_UTF8, 0, wide_argument, -1,
                            argv[index], bytes, NULL, NULL);
    }

    result = wolf3dgeneric_Create(argc, argv);
    if (result == WG_RESULT_OK)
    {
        result = wolf3dgeneric_Run();
        wolf3dgeneric_Shutdown();
    }
    exit_code = result == WG_RESULT_NOT_IMPLEMENTED || result == WG_RESULT_QUIT
                    ? 0 : 1;

    for (index = 0; index < argc; ++index)
    {
        LocalFree(argv[index]);
    }
    LocalFree(argv);
    LocalFree(wide_argv);
    return exit_code;
}
