#include "wg_file.h"

#include <stdio.h>
#include <stdlib.h>

static FILE *WG_OpenFile(const char *path, const char *mode)
{
#ifdef _MSC_VER
    FILE *stream = NULL;
    if (fopen_s(&stream, path, mode) != 0)
    {
        return NULL;
    }
    return stream;
#else
    return fopen(path, mode);
#endif
}

int WG_FileExists(const char *path)
{
    FILE *stream;

    stream = WG_OpenFile(path, "rb");
    if (stream == NULL)
    {
        return 0;
    }
    fclose(stream);
    return 1;
}

int WG_LoadFile(const char *path, wg_file_buffer_t *buffer)
{
    FILE *stream;
    long length;
    size_t bytes_read;

    if (path == NULL || buffer == NULL)
    {
        return 0;
    }

    buffer->data = NULL;
    buffer->size = 0;
    stream = WG_OpenFile(path, "rb");
    if (stream == NULL)
    {
        return 0;
    }

    if (fseek(stream, 0, SEEK_END) != 0)
    {
        fclose(stream);
        return 0;
    }
    length = ftell(stream);
    if (length < 0 || fseek(stream, 0, SEEK_SET) != 0)
    {
        fclose(stream);
        return 0;
    }

    if ((unsigned long)length > (size_t)-1)
    {
        fclose(stream);
        return 0;
    }

    buffer->size = (size_t)length;
    buffer->data = (uint8_t *)malloc(buffer->size == 0 ? 1 : buffer->size);
    if (buffer->data == NULL)
    {
        buffer->size = 0;
        fclose(stream);
        return 0;
    }

    bytes_read = fread(buffer->data, 1, buffer->size, stream);
    fclose(stream);
    if (bytes_read != buffer->size)
    {
        WG_FreeFile(buffer);
        return 0;
    }
    return 1;
}

void WG_FreeFile(wg_file_buffer_t *buffer)
{
    if (buffer == NULL)
    {
        return;
    }
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}
