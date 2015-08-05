/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "COPYING" for details.
 *
 */


/*
 * Code here is not optimized for performance and assumes
 * a 32 bits per pixel memory layout (direct color RGB memory model).
 */

#include <string.h>
#include <cpio/cpio.h>
#include "graphics.h"

/* pointer to base address of (linear) frame buffer */
typedef uint32_t* fb_t;

static seL4_VBEModeInfoBlock mib;
static fb_t fb = NULL;

/* linked in via archive.o; see Makefile */
extern char _cpio_archive[];


void
gfx_init_IA32BootInfo(seL4_IA32_BootInfo* bootinfo) {
    mib = bootinfo->vbeModeInfoBlock;
    assert(mib.bitsPerPixel == 32);
}

void
gfx_poke_fb(const uint32_t offset, const uint8_t val) {
    fb[offset] = val;
}

void
gfx_map_video_ram(ps_io_mapper_t *io_mapper) {
    size_t size = mib.yRes * mib.linBytesPerScanLine;
    fb = (fb_t) ps_io_map(io_mapper,
            mib.physBasePtr,
            size,
            0,
            PS_MEM_NORMAL);
    assert(fb != NULL);
}


void
gfx_display_testpic() {
    assert(fb != NULL);
    const size_t size = mib.yRes * mib.linBytesPerScanLine;
    for (int i = 0; i < size / 4; i++) {
        /* set pixel;
         * depending on color depth, one pixel is 1, 2, or 3 bytes */
        fb[i] = i; //generates some pattern
    }
}



void
gfx_print_IA32BootInfo(seL4_IA32_BootInfo* bootinfo) {
    seL4_VBEInfoBlock* ib      = &bootinfo->vbeInfoBlock;
    seL4_VBEModeInfoBlock* mib = &bootinfo->vbeModeInfoBlock;

    printf("\n");
    printf("=== VBE begin ===========\n");
    printf("vbeMode: 0x%x\n", bootinfo->vbeMode);
    printf("  VESA mode=%d; linear frame buffer=%d\n",
            (bootinfo->vbeMode & BIT(8)) != 0,
            (bootinfo->vbeMode & BIT(14)) != 0);
    printf("vbeInterfaceSeg: 0x%x\n", bootinfo->vbeInterfaceSeg);
    printf("vbeInterfaceOff: 0x%x\n", bootinfo->vbeInterfaceOff);
    printf("vbeInterfaceLen: 0x%x\n", bootinfo->vbeInterfaceLen);

    printf("ib->signature: %c%c%c%c\n",
            ib->signature[0],
            ib->signature[1],
            ib->signature[2],
            ib->signature[3]);
    printf("ib->version: %x\n", ib->version); //BCD

    printf("=== seL4_VBEModeInfoBlock:\n");
    printf("modeAttr: 0x%x\n", mib->modeAttr);
    printf("winAAttr: 0x%x\n", mib->winAAttr);
    printf("winBAttr: 0x%x\n", mib->winBAttr);
    printf("winGranularity: %d\n", mib->winGranularity);
    printf("winSize: %d\n", mib->winSize);
    printf("winASeg: 0x%x\n", mib->winASeg);
    printf("winBSeg: 0x%x\n", mib->winBSeg);
    printf("winFuncPtr: 0x%x\n", mib->winFuncPtr);
    printf("bytesPerScanLine: %d\n", mib->bytesPerScanLine);
    /* VBE 1.2+ */
    printf("xRes: %d\n", mib->xRes);
    printf("yRes: %d\n", mib->yRes);
    printf("xCharSize: %d\n", mib->xCharSize);
    printf("yCharSize: %d\n", mib->yCharSize);
    printf("planes: %d\n", mib->planes);
    printf("bitsPerPixel: %d\n", mib->bitsPerPixel);
    printf("banks: %d\n", mib->banks);
    printf("memoryModel: 0x%x\n", mib->memoryModel);
    printf("bankSize: %d\n", mib->bankSize);
    printf("imagePages: %d\n", mib->imagePages);
    printf("reserved1: 0x%x\n", mib->reserved1);

    printf("redLen: %d\n", mib->redLen);
    printf("redOff: %d\n", mib->redOff);
    printf("greenLen: %d\n", mib->greenLen);
    printf("greenOff: %d\n", mib->greenOff);
    printf("blueLen: %d\n", mib->blueLen);
    printf("blueOff: %d\n", mib->blueOff);
    printf("rsvdLen: %d\n", mib->rsvdLen);
    printf("rsvdOff: %d\n", mib->rsvdOff);
    printf("directColorInfo: %d\n", mib->directColorInfo);

    /* VBE 2.0+ */
    printf("physBasePtr: %x\n", mib->physBasePtr);
    //printf("reserved2: %d\n", mib->reserved2[6]);

    /* VBE 3.0+ */
    printf("linBytesPerScanLine: %d\n", mib->linBytesPerScanLine);
    printf("bnkImagePages: %d\n", mib->bnkImagePages);
    printf("linImagePages: %d\n", mib->linImagePages);
    printf("linRedLen: %d\n", mib->linRedLen);
    printf("linRedOff: %d\n", mib->linRedOff);
    printf("linGreenLen: %d\n", mib->linGreenLen);
    printf("linGreenOff: %d\n", mib->linGreenOff);
    printf("linBlueLen: %d\n", mib->linBlueLen);
    printf("linBlueOff: %d\n", mib->linBlueOff);
    printf("linRsvdLen: %d\n", mib->linRsvdLen);
    printf("linRsvdOff: %d\n", mib->linRsvdOff);
    printf("maxPixelClock: %d\n", mib->maxPixelClock);
    printf("modeId: %d\n", mib->modeId);
    printf("depth: %d\n", mib->depth);
    printf("=== VBE end ===========\n");
}

uint32_t
gfx_map_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << mib.linRedOff)
         | (g << mib.linGreenOff)
         | (b << mib.linBlueOff);
}


inline static void
gfx_draw_point(const int x, const int y, const uint32_t c) {
    fb[y * mib.xRes + x] = c;
}


inline static uint32_t
gfx_get_point(const int x, const int y) {
    return fb[y * mib.xRes + x];
}


void
gfx_draw_rect(const int x, const int y, const int w , const int h, uint32_t c) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            gfx_draw_point(x + i, y + j, c);
        }
    }
}


void
gfx_fill_screen(uint32_t c) {
    gfx_draw_rect(0, 0, mib.xRes, mib.yRes, c);
}


/*
 * "Load" a PPM image from the cpio archive and display it on the screen
 * at location (startx, starty) with given opacity level.
 * see https://en.wikipedia.org/wiki/Netpbm_format for PPM format
 */
void
gfx_diplay_ppm(uint32_t startx, uint32_t starty, const char* filename, float opacity) {
    assert(filename);
    assert(opacity >= 0 && opacity <= 1.0);
    unsigned long filesize;
    void * img = cpio_get_file(_cpio_archive, filename, &filesize);
    assert(img);

    int imgx = 0;  // image width (in pixel)
    int imgy = 0;  // image height
    //we cannot handle any comments in the header
    int n = sscanf(img, "P6\n%d %d\n255\n", &imgx, &imgy);
    assert (n == 2 && imgx > 0 && imgy > 0);

    // find first pixel; header and data are separated by "\n255\n"
    uint8_t* src = (uint8_t*) strstr(img, "\n255\n");
    assert(src);
    // skip over separator
    src += 5;

    //copy pixels to frame buffer
    for (int y = 0; y < imgy; y++) {
        for (int x = 0; x < imgx; x++) {
            uint8_t r = *src++;
            uint8_t g = *src++;
            uint8_t b = *src++;

            if (opacity < 1.0) {
                uint32_t backgrnd = gfx_get_point(startx + x, starty + y);
                uint8_t r2 = (backgrnd & 0xFF0000) >> 16;
                uint8_t g2 = (backgrnd & 0xFF00) >> 8;
                uint8_t b2 = (backgrnd & 0xFF);

                r = r * opacity + r2 * (1.0 - opacity);
                g = g * opacity + g2 * (1.0 - opacity);
                b = b * opacity + b2 * (1.0 - opacity);
            }

            uint32_t color = gfx_map_color(r, g, b);
            gfx_draw_point(startx + x, starty + y, color);
        }
    }
}
