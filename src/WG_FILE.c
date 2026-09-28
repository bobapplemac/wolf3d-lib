#include "WG_FILE.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <dirent.h>

static int WG_FileASCIINameEqual(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0')
    {
        char left_character = *left;
        char right_character = *right;

        if (left_character >= 'a' && left_character <= 'z')
        {
            left_character = (char)(left_character - ('a' - 'A'));
        }
        if (right_character >= 'a' && right_character <= 'z')
        {
            right_character = (char)(right_character - ('a' - 'A'));
        }
        if (left_character != right_character)
        {
            return 0;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static FILE *WG_OpenFileCaseInsensitive(const char *path, const char *mode)
{
    const char *separator = strrchr(path, '/');
    const char *name = separator == NULL ? path : separator + 1;
    const char *directory_name = ".";
    char *directory_copy = NULL;
    DIR *directory;
    struct dirent *entry;
    FILE *stream = NULL;

    if (*name == '\0')
    {
        return NULL;
    }
    if (separator != NULL)
    {
        size_t directory_length = (size_t)(separator - path);

        if (directory_length == 0U)
        {
            directory_name = "/";
        }
        else
        {
            directory_copy = (char *)malloc(directory_length + 1U);
            if (directory_copy == NULL)
            {
                return NULL;
            }
            memcpy(directory_copy, path, directory_length);
            directory_copy[directory_length] = '\0';
            directory_name = directory_copy;
        }
    }
    directory = opendir(directory_name);
    if (directory != NULL)
    {
        while ((entry = readdir(directory)) != NULL)
        {
            if (WG_FileASCIINameEqual(entry->d_name, name))
            {
                size_t directory_length = strlen(directory_name);
                size_t entry_length = strlen(entry->d_name);
                int needs_separator = directory_length != 0U
                    && directory_name[directory_length - 1U] != '/';
                size_t resolved_size = directory_length
                    + (size_t)needs_separator + entry_length + 1U;
                char *resolved = (char *)malloc(resolved_size);

                if (resolved != NULL)
                {
                    (void)snprintf(resolved, resolved_size, "%s%s%s",
                                   directory_name,
                                   needs_separator ? "/" : "",
                                   entry->d_name);
                    stream = fopen(resolved, mode);
                    free(resolved);
                }
                break;
            }
        }
        closedir(directory);
    }
    free(directory_copy);
    return stream;
}
#endif

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
    FILE *stream = fopen(path, mode);

#ifndef _WIN32
    if (stream == NULL && mode[0] == 'r')
    {
        stream = WG_OpenFileCaseInsensitive(path, mode);
    }
#endif
    return stream;
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

#if ULONG_MAX > SIZE_MAX
    if ((unsigned long)length > SIZE_MAX)
    {
        fclose(stream);
        return 0;
    }
#endif

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

int WG_WriteFile(const char *path, const void *data, size_t size)
{
    FILE *stream;
    size_t bytes_written;

    if (path == NULL || (data == NULL && size != 0U))
    {
        return 0;
    }
    stream = WG_OpenFile(path, "wb");
    if (stream == NULL)
    {
        return 0;
    }
    bytes_written = fwrite(data, 1, size, stream);
    return fclose(stream) == 0 && bytes_written == size;
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
