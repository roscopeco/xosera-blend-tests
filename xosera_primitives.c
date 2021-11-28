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
 * 
 * MIT License
 *
 * Not-yet-optimized primitive drawing for Xosera
 * ------------------------------------------------------------
 */

#include <stdbool.h>
#include <stdint.h>

#include "xosera_m68k_api.h"
#include "xosera_primitives.h"
#include "dprint.h"

// TODO this is slow, slow, SLOW....
void plot_320x200_8bpp(uint16_t x, uint16_t y, uint8_t color, uint16_t vram_base)
{
    uint16_t addr = vram_base + (y * 160 + ((x & 0xfffe) >> 1));

    xm_setw(RD_ADDR, addr);
    uint16_t current = xm_getw(DATA);

    if (x & 1)
    {
        // odd pixel, low byte
        current &= 0xFF00;
        current |= color;
    }
    else
    {
        // even pixel, high byte
        current &= 0x00FF;
        current |= (color << 8);
    }

#ifdef LINE_TRACE
    dprintf("PLOT: (%d,%d) = 0x%04x [%s]\n", x, y, addr, x & 1 ? "LO" : "HI");
#endif

    xm_setw(WR_ADDR, addr);
    xm_setw(DATA, current);
}

static int abs(int n) {
    int mask = n >> 31;
    return (mask & -n) | (~mask & n);
}

// TODO optimize horiz and verical lines!
void xosera_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, uint16_t vram_base, PlotFunc plot_func) {
    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy; 

    while (true) {
        plot_func(x0, y0, color, vram_base);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = err << 1;

        if (e2 >= dy) { /* e_xy+e_x > 0 */
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) { /* e_xy+e_y < 0 */
            err += dx;
            y0 += sy;
        }   
    }
}
