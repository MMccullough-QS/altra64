//
// nso_ui.h -- NSO-style 4x2 grid frontend for altra64
//
#ifndef NSO_UI_H
#define NSO_UI_H

#include <libdragon.h>
#include "direntry.h"

#define NSO_COLS      4
#define NSO_ROWS      2
#define NSO_PER_PAGE  (NSO_COLS * NSO_ROWS)   /* 8 */

#define CELL_W        (320 / NSO_COLS)         /* 80 */
#define CELL_H        (188 / NSO_ROWS)         /* 94 */
#define GRID_TOP      26                       /* y start of content area */

/* Draw the 4x2 grid view (called from display_dir when nso_mode==1) */
void nso_draw_grid(direntry_t *list, int cursor, int count, display_context_t disp);

/* Free cached box art sprites (call before entering a new directory) */
void nso_free_cache(void);

#endif /* NSO_UI_H */
