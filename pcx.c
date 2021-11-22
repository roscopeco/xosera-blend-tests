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

bool pcx_load_palette(uint8_t *buf) {
    if (*buf++ != 0x0C) {
        dprintf("ERROR: Palette indicator not present");
        return false;
    } else {
        xm_setw(XR_ADDR, XR_COLOR_MEM);

        for (int i = 0; i < 256; i++) {
            uint16_t entry = 0;

            entry |= ((*buf++ & 0xF0) << 4);
            entry |= (*buf++ & 0xF0);
            entry |= ((*buf++ & 0xF0) >> 4);

#ifdef TRACE_DEBUG
            dprintf("Palette %3d: 0x%04x\n", i, entry);
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
    uint16_t xpwords = xpos / 2;

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

    return buf;
}


/*

bool show_pcx(uint32_t buf_size, uint8_t *buf) {
    if (buf_size < 128) {
        dprintf("ERROR: Buffer is too small\n");
        return false;
    } else {
        PCXHeader *hdr = (PCXHeader*)buf;

        uint16_t width  = swap(hdr->max_x) - swap(hdr->min_x) + 1;
        uint16_t height = swap(hdr->max_y) - swap(hdr->min_y) + 1;  

        // *very* basic checks
        if (width != 320 || height != 240 || hdr->bpp != 8 || hdr->num_planes != 1 || hdr->encoding != 1) {
            dprintf("ERROR: Bad image\n");
            return false;
        }

        dprintf("Header      : 0x%02x\n", hdr->header);
        dprintf("Version     : 0x%02x\n", hdr->version);
        dprintf("Encoding    : 0x%02x\n", hdr->encoding);
        dprintf("BPP         : 0x%02x\n", hdr->bpp);
        dprintf("Plane count : 0x%02x\n", hdr->num_planes);
        dprintf("Mode        : %s\n", modes[swap(hdr->palette_mode)]);
        dprintf("Min X       : %d\n", swap(hdr->min_x));
        dprintf("Min Y       : %d\n", swap(hdr->min_y));
        dprintf("Max X       : %d\n", swap(hdr->max_x));
        dprintf("Max Y       : %d\n", swap(hdr->max_y));
        dprintf("Dimensions  : %dx%d\n", width, height);
        dprintf("H DPI       : %d\n", swap(hdr->h_dpi));
        dprintf("V DPI       : %d\n", swap(hdr->v_dpi));
        dprintf("\n");

        
        if (!load_palette(buf + (buf_size - 769))) {
            dprintf("ERROR: Palette load failed\n");
            return false;
        }

        if (!load_image(buf_size - 769 - 128, buf + 128)) {
            dprintf("ERROR: Image load failed\n");
            return false;
        }

        return true;
    }
}

*/
