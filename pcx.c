/*
 * vim: set et ts=4 sw=4
 *------------------------------------------------------------
 *                                  ___ ___ _
 *  ___ ___ ___ ___ ___       _____|  _| . | |_
 * |  _| . |_ -|  _| . |     |     | . | . | '_|
 * |_| |___|___|___|___|_____|_|_|_|___|___|_,_|
 *                     |_____|
 * ------------------------------------------------------------
 * Copyright (c) 2021 Ross Bamford
 * MIT License
 *
 * Simple PCX loader for Xosera
 * ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>

#include "xosera_m68k_api.h"
#include "dprint.h"

bool pcx_load_palette(uint8_t *palette_buf) {
    uint8_t *buf = palette_buf;
    if (*buf++ != 0x0C) {
        dprintf("ERROR: Palette indicator not present");
        return false;
    } else {
        xm_setw(XR_ADDR, XR_COLOR_MEM);

        // PA Palette
        for (int i = 0; i < 256; i++) {
            uint16_t entry = 0;

            entry |= ((*buf++ & 0xF0) << 4);
            entry |= (*buf++ & 0xF0);
            entry |= ((*buf++ & 0xF0) >> 4);

#ifdef PALETTE_DEBUG
            dprintf("PA Palette %3d: 0x%04x\n", i, entry);
#endif

            xm_setw(XR_DATA, entry);
        }

        buf = palette_buf + 1; /* +1 to skip identifier byte */
        // PB Palette
        for (int i = 0; i < 256; i++) {            
            uint16_t entry = i == 0 ? 0x0 : 0xC000;

            entry |= ((*buf++ & 0xF0) << 4);
            entry |= (*buf++ & 0xF0);
            entry |= ((*buf++ & 0xF0) >> 4);

#ifdef PALETTE_DEBUG
            dprintf("PB Palette %3d: 0x%04x\n", i, entry);
#endif

            xm_setw(XR_DATA, entry);
        }

        return true;
    }
}

static bool high = false;
static uint16_t save_pix = 0;

static inline void draw(uint8_t pix) {
    if (high) {
        // send to Xosera
        xm_setw(DATA, save_pix << 8 | pix);
        high = false;
    } else {
        save_pix = pix;
        high = true;
    }
}

/**
 * NOTE: Image width and height must be even, and buffer is not bounds-checked!
 * Only supports 320x240
 * 
 * Returns pointer to next pixel *after* end of the drawn image.
 * 
 * line_length is given in words!
 */
uint8_t* pcx_draw_image(uint16_t xpos, uint16_t ypos, uint16_t width, uint16_t height, uint16_t vram_base, uint8_t *buf) {
    uint16_t ystart = vram_base + (ypos * 160);
    uint16_t xpwords = xpos >> 1;
    uint8_t *start_buf = buf;

    dprintf("   YSTART: %d\n", ystart);

    for (uint16_t y = 0; y < height; y++) {
        uint16_t linestart = ystart + (y * 160) + xpwords;
        dprintf("   LSTART: %d (Line %d)\n", linestart, y);

        if (high) {
            dprintf("WARN: Last pixel not sent!\n");
            high = false;
        }

        xm_setw(WR_ADDR, linestart);

        for (int x = 0; x < width; x++) {
            uint8_t pix = *buf++;

            if ((pix & 0xc0) == 0xc0) {
                // Do a run
                uint8_t len = pix & 0x3f;
                pix = *buf++;

#ifdef TRACE_DEBUG
                dprintf("(X,Y) = (%d,%d) : Start run value %d (length %d)\n",
                    x, y, pix, len);
#endif

                for (uint8_t j = 0; j < len; j++) {
                    draw(pix);
                }

                x += (len - 1);
            } else {
                // Single pixel
                draw(pix);
            }
        }
    }

    dprintf("Drew %ld pixels", ((uint32_t)(buf - start_buf)));

    high = false; /* reset this for next draw */

    return buf;
}
