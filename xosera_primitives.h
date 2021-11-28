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
 * Some primitives for Xosera
 * ------------------------------------------------------------
 */

#include <stdbool.h>
#include <stdint.h>

typedef void (*PlotFunc)(uint16_t, uint16_t, uint8_t, uint16_t);

void plot_320x200_8bpp(uint16_t x, uint16_t y, uint8_t color, uint16_t vram_base);
void xosera_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, uint16_t vram_base, PlotFunc plot_func);

