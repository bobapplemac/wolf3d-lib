#include "WG_PLATFORM.h"

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const wchar_t wg_window_class[] = L"wolf3dgeneric-window";
static HWND wg_window;
static uint32_t wg_pixels[WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT];
static LARGE_INTEGER wg_counter_frequency;
static LARGE_INTEGER wg_counter_start;
static int wg_quit_pending;

#define WG_PCM_BUFFER_COUNT 4U
#define WG_PCM_BUFFER_FRAMES 512U
static HWAVEOUT wg_wave_out;
static WAVEHDR wg_wave_headers[WG_PCM_BUFFER_COUNT];
static int16_t wg_wave_samples[WG_PCM_BUFFER_COUNT]
                              [WG_PCM_BUFFER_FRAMES * 2U];
static uint8_t wg_wave_used[WG_PCM_BUFFER_COUNT];

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
            wg_event_t event;

            event.type = WG_EVENT_KEY;
            event.pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            event.key = (uint16_t)((lparam >> 16) & 0xff);
            event.x = 0;
            event.y = 0;
            event.button = 0;
            wg_queue_event(&event);
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

int WG_Init(void)
{
    WNDCLASSW window_class;
    RECT rectangle;
    HINSTANCE instance;

    instance = GetModuleHandleW(NULL);
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = wg_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.lpszClassName = wg_window_class;

    if (!RegisterClassW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        return 0;
    }

    rectangle.left = 0;
    rectangle.top = 0;
    rectangle.right = WG_SCREEN_WIDTH * 3;
    rectangle.bottom = WG_SCREEN_HEIGHT * 3;
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

    QueryPerformanceFrequency(&wg_counter_frequency);
    QueryPerformanceCounter(&wg_counter_start);
    wg_event_read = 0;
    wg_event_write = 0;
    wg_quit_pending = 0;
    ShowWindow(wg_window, SW_SHOW);
    return 1;
}

void WG_Shutdown(void)
{
    WG_PCMShutdown();
    if (wg_window != NULL)
    {
        DestroyWindow(wg_window);
        wg_window = NULL;
    }
    UnregisterClassW(wg_window_class, GetModuleHandleW(NULL));
}

void WG_Present(const uint8_t *pixels, const uint8_t *palette)
{
    BITMAPINFO bitmap_info;
    RECT client;
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
    device_context = GetDC(wg_window);
    SetStretchBltMode(device_context, COLORONCOLOR);
    StretchDIBits(device_context, 0, 0, client.right, client.bottom,
                  0, 0, WG_SCREEN_WIDTH, WG_SCREEN_HEIGHT, wg_pixels,
                  &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(wg_window, device_context);
}

uint32_t WG_GetTicksMs(void)
{
    LARGE_INTEGER now;
    uint64_t ticks;

    QueryPerformanceCounter(&now);
    ticks = (uint64_t)(now.QuadPart - wg_counter_start.QuadPart);
    return (uint32_t)((ticks * 1000U) / (uint64_t)wg_counter_frequency.QuadPart);
}

void WG_SleepMs(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

int WG_PollEvent(wg_event_t *event)
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

    if (wg_event_read != wg_event_write)
    {
        *event = wg_event_queue[wg_event_read];
        wg_event_read = (wg_event_read + 1U) % WG_EVENT_QUEUE_CAPACITY;
        return 1;
    }

    event->type = WG_EVENT_NONE;
    return 0;
}

int WG_IsInteractive(void)
{
    return 1;
}

void WG_SetWindowTitle(const char *title)
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

void WG_ReportError(const char *message)
{
    MessageBoxA(wg_window, message, "wolf3dgeneric", MB_OK | MB_ICONERROR);
}

int WG_PCMInit(uint32_t sample_rate, uint16_t channels)
{
    WAVEFORMATEX format;
    size_t index;

    if (sample_rate == 0U || channels != 2U)
    {
        return 0;
    }
    WG_PCMShutdown();
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
            WG_PCMShutdown();
            return 0;
        }
    }
    return 1;
}

void WG_PCMShutdown(void)
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

size_t WG_PCMWritableFrames(void)
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

int WG_PCMSubmit(const int16_t *samples, size_t frame_count)
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                    PWSTR command_line, int show_command)
{
    int argc;
    wchar_t **wide_argv;
    char **argv;
    int index;
    int exit_code;
    wg_result_t result;

    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    wide_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wide_argv == NULL)
    {
        return 1;
    }

    argv = (char **)LocalAlloc(LMEM_FIXED, (size_t)argc * sizeof(*argv));
    if (argv == NULL)
    {
        LocalFree(wide_argv);
        return 1;
    }

    for (index = 0; index < argc; ++index)
    {
        int bytes = WideCharToMultiByte(CP_UTF8, 0, wide_argv[index], -1,
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
        WideCharToMultiByte(CP_UTF8, 0, wide_argv[index], -1,
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
