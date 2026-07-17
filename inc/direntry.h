//
// direntry.h -- direntry_t struct shared between main.c and nso_ui.c
//
#ifndef DIRENTRY_H
#define DIRENTRY_H

#include <libdragon.h>

typedef struct
{
    uint32_t type;
    uint32_t color;
    char filename[MAX_FILENAME_LEN + 1];
} direntry_t;

#endif /* DIRENTRY_H */
