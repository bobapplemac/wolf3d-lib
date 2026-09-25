#ifndef WG_FILE_H
#define WG_FILE_H

#include <stddef.h>
#include <stdint.h>

typedef struct wg_file_buffer
{
    uint8_t *data;
    size_t size;
} wg_file_buffer_t;

int WG_FileExists(const char *path);
int WG_LoadFile(const char *path, wg_file_buffer_t *buffer);
int WG_WriteFile(const char *path, const void *data, size_t size);
void WG_FreeFile(wg_file_buffer_t *buffer);

#endif
