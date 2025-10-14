#include "common.h"
#include "port/resources.h"
#include "types.h"

#include <sifdev.h>

#include <SDL3/SDL.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// sifdev

static int sce_to_unix_flags(int flags) {
    int result = 0;

    switch (flags & 0xF) {
    case SCE_RDONLY:
        result = O_RDONLY;
        break;

    case SCE_WRONLY:
        result = O_WRONLY;
        break;

    case SCE_RDWR:
        result = O_RDWR;
        break;
    }

    if (flags & SCE_APPEND) {
        result |= O_APPEND;
    }

    if (flags & SCE_CREAT) {
        result |= O_CREAT;
    }

    if (flags & SCE_TRUNC) {
        result |= O_TRUNC;
    }

    return result;
}

int sceOpen(const char* filename, int flag, ...) {
    const char expected_path[] = "cdrom0:\\THIRD\\IOP\\FONT8_8.TM2;1";

    if (strcmp(filename, expected_path) != 0) {
        fatal_error("Unexpected path: %s", filename);
    }

    char* font_path = Resources_GetPath("FONT8_8.TM2");
    const int flags = sce_to_unix_flags(flag);
    const int result = open(font_path, flags, 0644);
    SDL_free(font_path);
    return result;
}

int sceClose(int fd) {
    return close(fd);
}

int sceRead(int fd, void* buf, int nbyte) {
    return read(fd, buf, nbyte);
}

int sceWrite(int fd, const void* buf, int nbyte) {
    return write(fd, buf, nbyte);
}

int sceLseek(int fd, int offset, int whence) {
    return lseek(fd, offset, whence);
}
