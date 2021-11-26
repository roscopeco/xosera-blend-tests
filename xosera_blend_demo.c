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
 * Portions reproduced from Xark's xosera_test_m68k. 
 * Copyright (c) 2021 Xark
 *
 * MIT License
 *
 * Xosera playfield blend mode experiments
 * ------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <basicio.h>
#include <machine.h>
#include <sdfat.h>

#include "xosera_m68k_api.h"
#include "dprint.h"
#include "pcx.h"

/*
 * Options - Change these to suit your tastes
 */

// Directory on SD card containing frames with filenames like "0001.xmb".
// The files must be consecutive, and start at 1. A maximum of MAX_FRAMES
// will be loaded.
//
// WARNING: Max 9 characters!
#define FRAME_DIR   "xotext"

// Sets the (starting, if effects are used) attribute to draw the animations
// in 1bpp modes. High nybble is foreground, low is background
#define ATTR        0x0F

// One of the following two may be defined for "effects"
//#define SLOW_CYCLE                // Define to slowly cycle normal/inverse
//#define PSYCHEDELIC               // Define to quickly cycle colours

/*
 * Probably leave the rest of the defines alone unless you know what you're doing...
 */
#define FRAME_SIZE  9600
#define MAX_FRAMES  32

/* Playfield A and B buffers for 8bpp mode - no backbuffers (no space) */
#define PA_8BPP     0
#define PB_8BPP     0x9600

/* Double buffers for PA in 1bpp mode */
#define PA_BUF_0    0
#define PA_BUF_1    0x4b00

extern void install_intr();
extern void remove_intr();

extern void* _end;

// Guards against things being optimized out...
volatile uint32_t opt_guard = 0;

// These are all accessed by the vblank handler
volatile uint32_t vblank_count = 0;
volatile uint16_t current_pa_buf = PA_BUF_0;
volatile uint16_t back_pa_buf = PA_BUF_1;
volatile bool pa_flip_needed = false;
volatile uint16_t pb_gfx_ctrl = 0x0000;

#if !defined(checkchar)        // newer rosco_m68k library addition, this is in case not present
bool checkchar() {
    int rc;
    __asm__ __volatile__(
        "move.l #6,%%d1\n"        // CHECKCHAR
        "trap   #14\n"
        "move.b %%d0,%[rc]\n"
        "ext.w  %[rc]\n"
        "ext.l  %[rc]\n"
        : [rc] "=d"(rc)
        :
        : "d0", "d1");
    return rc != 0;
}
#endif

// Clear a chunk of VRAM
static void xcls(uint16_t vaddr, uint16_t len) {
    // TODO blitter
    xm_setw(WR_ADDR, vaddr);
    xm_setw(WR_INCR, 1);
    xm_setbh(DATA, 0);

    for (uint16_t i = 0; i < len; i++) {
        xm_setbl(DATA, 0);
    }
    xm_setw(WR_ADDR, 0);
}

static uint32_t load_sd_file(const char *filename, uint8_t *buffer) {
    uint8_t *bufptr = buffer;
    int cnt = 0;

    dprintf("Try load: %s\n", filename);

    void * file = fl_fopen(filename, "r");

    if (file != NULL) {
        while ((cnt = fl_fread(bufptr, 1, 512, file)) > 0) {
            bufptr += cnt;
        }

        fl_fclose(file);

        return (uint32_t)bufptr - (uint32_t)buffer;
    }

    return 0;
}

static void draw_mono_bitmap(uint16_t vaddr, uint8_t *buffer, uint32_t size, uint8_t attr) {
    xm_setw(WR_ADDR, vaddr);
    xm_setbh(DATA, attr);

    if (size > 0x10000) {
        printf("FAILED; Buffer too large for VRAM...\n");
    } else {
        for (uint32_t i = 0; i < size; i++) {
            xm_setbl(DATA, *buffer++);
        }
    }
}

/*
 * Loads a maximum of MAX_FRAMES (default: 32) frames numbered 0001-NNNN.
 * If you change from the default max frames, ensure they will fit in
 * memory (without blowing the stack!)
 *
 * Actual number loaded is returned.
 */
static uint8_t load_frames(uint8_t *buffer) {
    uint8_t *bufptr = buffer;
    char strbuf[21];

    for (int i = 0; i < MAX_FRAMES; i++) {
        if ((xm_getbl(UNUSED_A) & 0xF) > 3) {
            pb_gfx_ctrl = 0x0065;
        } else {
            pb_gfx_ctrl = 0x00E5;
        }

        if (sprintf(strbuf, "/" FRAME_DIR "/%04d.xmb", i + 1) < 0) {
            dprintf("sprintf failed!\n");
            return i;
        }

        if (load_sd_file(strbuf, bufptr) != FRAME_SIZE) {
            return i;
        }
        
        uint8_t color = (i & 0xF);
        if (!color) {
            color = 8;
        }

        bufptr += FRAME_SIZE;
    }

    return MAX_FRAMES;
}

/* Wait until at least one vblank has run */
static void wait_vblank() {
    uint32_t vblank_start = vblank_count;
    do {
        opt_guard++;
    } while (vblank_count == vblank_start);
}

/**
 * Start the "Loading" sequence. Loads the "Loading" image
 * and the overlay, and draws them to the relevant playfields.
 * 
 * The buffer passed in is only needed for the duration of
 * the function, and can be reused after this returns.
 */
static bool start_loading(uint8_t *temp_buffer) {
    uint32_t size;
    if ((size = load_sd_file("/" FRAME_DIR "/Disk.pcx", temp_buffer))) {
        xcls(PA_8BPP, 38400);
        xcls(PB_8BPP, 27136);

        pcx_load_palette(PCX_PALETTE(size, temp_buffer));

        // Draw main image on PA
        uint8_t *overlay_start = pcx_draw_image(8, 85, 304, 70, PA_8BPP, PCX_PIXELS(temp_buffer));

        // Draw overlay on PB
        pcx_draw_image(8, 132, 304, 10, PB_8BPP, overlay_start);

        // Enable playfield displays
        xreg_setw(PA_GFX_CTRL, 0x0065);
        xreg_setw(PA_LINE_LEN, 160);

        pb_gfx_ctrl = 0x0065;
        xreg_setw(PB_LINE_LEN, 160);

        xreg_setw(PA_DISP_ADDR, PA_8BPP);
        xreg_setw(PB_DISP_ADDR, PB_8BPP);

        wait_vblank();

        return true;
    } else {
        dprintf("Failed to load loading!\n");
        return false;
    }
}

static void done_loading() {
    // Just blank PB for now...
    pb_gfx_ctrl = 0x00E5;
    wait_vblank();
}

void xosera_demo() {
    // flush any input charaters to avoid instant exit
    while (checkchar())
    {
        readchar();
    }

    dprintf("Xosera video demo (m68k)\n");

    if (SD_check_support())
    {
        dprintf("SD card supported: ");

        if (SD_FAT_initialize())
        {
            dprintf("card ready\n");
        }
        else
        {
            dprintf("no card; quitting\n");
            return;
        }
    }
    else
    {
        dprintf("No SD card support; quitting.\n");
        return;
    }

    dprintf("\nxosera_init(1)...");
    // wait for monitor to unblank
    bool success = xosera_init(0);
    dprintf("%s (%dx%d)\n", success ? "succeeded" : "FAILED", xreg_getw(VID_HSIZE), xreg_getw(VID_VSIZE));

    xm_setw(WR_INCR, 1);    

    uint8_t *buffer = (uint8_t*)&_end;
    uint8_t frame_count;

    install_intr();

    dprintf("Loading loading image\n");

    if (!start_loading(buffer)) {
        dprintf("WARN: Failed to load loading image\n");
    }

    dprintf("Loading frames...\n");

    if ((frame_count = load_frames(buffer))) {
        dprintf("Loaded %d frames\n", frame_count);

        done_loading();

        xreg_setw(PA_GFX_CTRL, 0x0045);
        xreg_setw(PA_LINE_LEN, 40);

        uint8_t current_frame = frame_count;
        uint8_t *bufptr = buffer;
#if defined SLOW_CYCLE || defined PSYCHEDELIC
        uint16_t counter = 0;
#endif
        uint8_t attr = ATTR;

        while (true) {     
            if (current_frame == frame_count) {
                current_frame = 0;
                bufptr = buffer;
            }

            draw_mono_bitmap(back_pa_buf, bufptr, FRAME_SIZE, attr);
            pa_flip_needed = true;

            int count = 0;
            while (pa_flip_needed) { 
                // wait for flip
                if (count++ == 100000) {
                    dprintf("Still waiting for flip (after %d vblanks)...\n", vblank_count);
                    count = 0;
                }        
            }

            count = 0;
            while (count++ < 10000) {
                // busywait...
                opt_guard++;
            }

            current_frame++;
            bufptr += FRAME_SIZE;

#ifdef SLOW_CYCLE
            switch (counter++) {
            case 20:
                attr = 0x87;
                break;
            case 21:
                attr = 0x78;
                break;
            case 22:
                attr = 0xF0;
                break;
            case 42:
                attr = 0x78;
                break;
            case 43:
                attr = 0x87;
                break;
            case 44:
                attr = 0x0F;
                counter = 0;
                break;
            }
#else
#ifdef PSYCHEDELIC
            if (++counter == 2) {
                attr += 1;
                counter = 0;
            }
#endif
#endif
        }
    } else {
        printf("Load failed; No frames :(\n");
        done_loading();
        remove_intr();
    }
}

