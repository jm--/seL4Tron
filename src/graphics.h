/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "COPYING" for details.
 *
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdio.h>
#include <sel4/arch/bootinfo.h>
#include <sel4platsupport/io.h>



void
gfx_init_IA32BootInfo(seL4_IA32_BootInfo* bootinfo);


void
gfx_poke_fb(const uint32_t offset, const uint8_t val);


/*
 * Map complete linear frame buffer.
 */
void
gfx_map_video_ram(ps_io_mapper_t *io_mapper);


/*
 * Fill frame buffer with some values; i.e., display a test picture.
 * @param fb base address of frame buffer
 *           (this virtual address is not mapped 1:1)
 */
void
gfx_display_testpic();


/*
 * Print the VBE (VESA Bios Extension), which is conveniently included in the
 * IA32_BootInfo available to the root task.
 *
 * The InfoBlock struc is returned by VBE function 00h.
 * The ModeInfoBlock struc is returned by VBE function 01h.
 * The VBE 2.0 protected mode interface (vbeMode, vbeInterfaceSeg,
 * vbeInterfaceOff, and vbeInterfaceLen) is returned by VBE function 0Ah.
 * See version 3.0 of the VBE standard and the
 * GNU "Multiboot Specification" for details.
 */
void
gfx_print_IA32BootInfo(seL4_IA32_BootInfo* bootinfo);


/*
 * Map an RGB triple to a 32 bit pixel value. r, g, b are the red, green,
 * and blue components of the pixel in the range 0-255.
 */
uint32_t
gfx_map_color(uint8_t r, uint8_t g, uint8_t b);


/*
 * Draw a rectangle and fill it.
 * @param x,y: top left corner
 * @param w,h: widht and height
 * @param c:  fill color
 */
void
gfx_draw_rect(const int x, const int y, const int w , const int h, uint32_t c);


/*
 * Fill entire screen with color c.
 */
void
gfx_fill_screen(uint32_t c);


/*
 * Load PPM file "filename" from cpio archive and display it
 * at (startx, starty) coordinate.
 */
void
gfx_diplay_ppm(uint32_t startx, uint32_t starty, const char* filename, float transp);

#endif /* GRAPHICS_H_ */
