#ifndef WG_COMPAT_H
#define WG_COMPAT_H

#if defined(_MSC_VER) && _MSC_VER < 1900
#include <stdarg.h>
#include <stdio.h>

static int WG_Snprintf(char *buffer, size_t buffer_size,
                       const char *format, ...)
{
    va_list arguments;
    int required;

    va_start(arguments, format);
    required = _vscprintf(format, arguments);
    va_end(arguments);

    if (buffer_size > 0U)
    {
        va_start(arguments, format);
        (void)_vsnprintf_s(buffer, buffer_size, _TRUNCATE, format, arguments);
        va_end(arguments);
    }

    return required;
}

#define snprintf WG_Snprintf
#endif

#endif
