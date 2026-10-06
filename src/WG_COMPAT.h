#ifndef WG_COMPAT_H
#define WG_COMPAT_H

#if defined(_MSC_VER) && _MSC_VER < 1900
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

static int WG_Snprintf(char *buffer, size_t buffer_size,
                       const char *format, ...)
{
    va_list arguments;
    int required;
    int written;

#if _MSC_VER >= 1300
    va_start(arguments, format);
    required = _vscprintf(format, arguments);
    va_end(arguments);
#else
    required = 0;
#endif

    if (buffer_size > 0U)
    {
        va_start(arguments, format);
        written = _vsnprintf(buffer, buffer_size, format, arguments);
        va_end(arguments);
        (void)written;

        /* Pre-VC9 _vsnprintf does not terminate truncated output. */
        buffer[buffer_size - 1U] = '\0';

        /* VC6 has no _vscprintf. Its -1 truncation result is converted to
           a standard snprintf-style value that still reports no fit. */
#if _MSC_VER < 1300
        if (written < 0)
        {
            required = buffer_size > (size_t)INT_MAX
                ? INT_MAX : (int)buffer_size;
        }
        else
        {
            required = written;
        }
#endif
    }

    return required;
}

#define snprintf WG_Snprintf
#endif

#endif
