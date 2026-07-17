// nso_ui.c -- NSO-style 4x2 grid view for altra64 (with box art caching)
//
#include <libdragon.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dir.h>
#include "types.h"
#include "direntry.h"
#include "nso_ui.h"
#include "ff.h"
#include "rom.h"
#include "constants.h"

/* ---- externs from main.c ---- */
extern char pwd[64];
extern int  nso_mode;
extern sprite_t *loadPng(u8 *path);
extern void drawBg(display_context_t disp);

/* ---- box art cache ---- */
static sprite_t *nso_art[NSO_PER_PAGE];
static int       nso_cached_page = -1;

void nso_free_cache(void)
{
    for (int i = 0; i < NSO_PER_PAGE; i++) {
        if (nso_art[i]) { free(nso_art[i]); nso_art[i] = NULL; }
    }
    nso_cached_page = -1;
}

/* Try to load box art for a single ROM file. Returns NULL if not available. */
static sprite_t *load_boxart_for(const char *dir, const char *filename)
{
    /* Check for N64 ROM extension */
    int flen = (int)strlen(filename);
    if (flen < 5) return NULL;
    const char *ext = filename + flen - 4;
    int is_rom = (strcmp(ext, ".z64") == 0 || strcmp(ext, ".v64") == 0 ||
                  strcmp(ext, ".n64") == 0 || strcmp(ext, ".Z64") == 0 ||
                  strcmp(ext, ".V64") == 0 || strcmp(ext, ".N64") == 0);
    if (!is_rom) return NULL;

    /* Build full path */
    char full_path[256];
    if (strcmp(dir, "/") == 0)
        snprintf(full_path, sizeof(full_path), "/%s", filename);
    else
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, filename);

    /* Read first 64 bytes for ROM header */
    FIL file;
    FRESULT res = f_open(&file, full_path, FA_READ);
    if (res != FR_OK) return NULL;

    unsigned char hdr[64];
    UINT bread;
    f_read(&file, hdr, 64, &bread);
    f_close(&file);
    if (bread < 64) return NULL;

    /* Byte-swap if needed */
    int sw = is_valid_rom(hdr);
    if (sw) swap_header(hdr, 64);

    /* Cart ID is at bytes 0x3C-0x3D */
    char box_path[48];
    snprintf(box_path, sizeof(box_path),
             "/" ED64_FIRMWARE_PATH "/boxart/lowres/%c%c.png",
             hdr[0x3C], hdr[0x3D]);

    sprite_t *spr = loadPng((u8 *)box_path);
    if (!spr) {
        /* Fallback to generic */
        snprintf(box_path, sizeof(box_path),
                 "/" ED64_FIRMWARE_PATH "/boxart/lowres/00.png");
        spr = loadPng((u8 *)box_path);
    }
    return spr;
}

/* Load (or reuse) the art cache for the given NSO page. */
static void ensure_cache(direntry_t *list, int nso_page, int count)
{
    if (nso_page == nso_cached_page) return;
    nso_free_cache();
    nso_cached_page = nso_page;
    for (int slot = 0; slot < NSO_PER_PAGE; slot++) {
        int idx = nso_page + slot;
        if (idx >= count) break;
        if (list[idx].type == DT_DIR) continue;
        nso_art[slot] = load_boxart_for(pwd, list[idx].filename);
    }
}

/* ---- helpers ---- */
static void trunc_name(const char *src, char *dst, int max_chars)
{
    /* Strip N64 extension first, then truncate */
    char tmp[256];
    strncpy(tmp, src, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    int tlen = (int)strlen(tmp);
    if (tlen > 4) {
        char *e = tmp + tlen - 4;
        if (strcmp(e, ".z64") == 0 || strcmp(e, ".v64") == 0 ||
            strcmp(e, ".n64") == 0 || strcmp(e, ".Z64") == 0 ||
            strcmp(e, ".V64") == 0 || strcmp(e, ".N64") == 0)
            *e = 0;
    }

    int len = (int)strlen(tmp);
    if (len <= max_chars) {
        strcpy(dst, tmp);
    } else {
        strncpy(dst, tmp, max_chars - 2);
        dst[max_chars - 2] = '.';
        dst[max_chars - 1] = '.';
        dst[max_chars]     = 0;
    }
}

static void draw_cell_border(display_context_t disp, int cx, int cy, uint32_t col)
{
    graphics_draw_box(disp, cx,          cy,          CELL_W, 2,      col);
    graphics_draw_box(disp, cx,          cy+CELL_H-2, CELL_W, 2,      col);
    graphics_draw_box(disp, cx,          cy,          2,      CELL_H, col);
    graphics_draw_box(disp, cx+CELL_W-2, cy,          2,      CELL_H, col);
}

/* ---- main renderer ---- */
void nso_draw_grid(direntry_t *list, int cursor, int count, display_context_t disp)
{
    int nso_page     = (cursor / NSO_PER_PAGE) * NSO_PER_PAGE;
    int total_pages  = (count + NSO_PER_PAGE - 1) / NSO_PER_PAGE;
    int cur_page_num = nso_page / NSO_PER_PAGE + 1;
    if (total_pages < 1) total_pages = 1;

    drawBg(disp);

    ensure_cache(list, nso_page, count);

    uint32_t c_cell_bg  = graphics_make_color(0x0C, 0x0E, 0x1A, 0xD0);
    uint32_t c_sel_bg   = graphics_make_color(0x18, 0x20, 0x48, 0xFF);
    uint32_t c_empty_bg = graphics_make_color(0x08, 0x09, 0x12, 0x80);
    uint32_t c_border   = graphics_make_color(0x00, 0x78, 0xD4, 0xFF);
    uint32_t c_sep      = graphics_make_color(0x00, 0x78, 0xD4, 0xC0);
    uint32_t c_fg_norm  = graphics_make_color(0xD0, 0xD0, 0xD0, 0xFF);
    uint32_t c_fg_dir   = graphics_make_color(0x40, 0xBA, 0xFF, 0xFF);
    uint32_t c_fg_sel   = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF);
    uint32_t c_black    = graphics_make_color(0x00, 0x00, 0x00, 0x00);

    /* Header: page indicator + path */
    char header[64];
    snprintf(header, sizeof(header), "%d/%d  SD:/%s", cur_page_num, total_pages, pwd);
    graphics_set_color(c_fg_norm, c_black);
    graphics_draw_text(disp, 24, 10, header);

    for (int slot = 0; slot < NSO_PER_PAGE; slot++) {
        int idx = nso_page + slot;
        int col = slot % NSO_COLS;
        int row = slot / NSO_COLS;
        int cx  = col * CELL_W;
        int cy  = GRID_TOP + row * CELL_H;

        if (idx >= count) {
            graphics_draw_box(disp, cx+2, cy+2, CELL_W-4, CELL_H-4, c_empty_bg);
            continue;
        }

        int is_sel = (idx == cursor);
        int is_dir = (list[idx].type == DT_DIR);
        uint32_t fg = is_sel ? c_fg_sel : (is_dir ? c_fg_dir : c_fg_norm);

        /* Cell background */
        graphics_draw_box(disp, cx+1, cy+1, CELL_W-2, CELL_H-2,
                          is_sel ? c_sel_bg : c_cell_bg);

        /* Selection border */
        if (is_sel) draw_cell_border(disp, cx, cy, c_border);

        sprite_t *art = nso_art[slot];
        if (art) {
            /* Draw box art centered in upper portion of cell */
            int art_x = cx + (CELL_W  - (int)art->width)  / 2;
            int art_y = cy + 4;
            if (art_x < cx + 2) art_x = cx + 2;
            if ((int)art->height > CELL_H - 16) art_y = cy + 2;
            graphics_draw_sprite(disp, art_x, art_y, art);
            /* Filename below art */
            char name[12];
            trunc_name(list[idx].filename, name, 9);
            int name_px = (int)strlen(name) * 8;
            int name_x  = cx + (CELL_W - name_px) / 2;
            if (name_x < cx + 2) name_x = cx + 2;
            graphics_set_color(fg, c_black);
            graphics_draw_text(disp, name_x, cy + CELL_H - 12, name);
        } else {
            /* No art: type label + filename */
            const char *type_label = is_dir ? "[DIR]" : " ROM ";
            graphics_set_color(fg, c_black);
            int lbl_px = (int)strlen(type_label) * 8;
            int lbl_x  = cx + (CELL_W - lbl_px) / 2;
            graphics_draw_text(disp, lbl_x, cy + 28, type_label);
            char name[12];
            trunc_name(list[idx].filename, name, 9);
            int name_px = (int)strlen(name) * 8;
            int name_x  = cx + (CELL_W - name_px) / 2;
            if (name_x < cx + 2) name_x = cx + 2;
            graphics_draw_text(disp, name_x, cy + 44, name);
        }
    }

    /* Grid separator lines */
    for (int c = 1; c < NSO_COLS; c++)
        graphics_draw_box(disp, c * CELL_W, GRID_TOP, 1, 188, c_sep);
    graphics_draw_box(disp, 0, GRID_TOP + CELL_H, 320, 1, c_sep);

    /* Footer: full name of selected item */
    if (cursor >= 0 && cursor < count) {
        char footer[44];
        trunc_name(list[cursor].filename, footer, 39);
        graphics_set_color(c_fg_sel, c_black);
        graphics_draw_text(disp, 24, 222, footer);
    }
}
