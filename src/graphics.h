/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdio.h>
#include <sel4/arch/bootinfo.h>
#include <sel4platsupport/io.h>


/* pointer to base address of (linear) frame buffer */
typedef uint8_t* fb_t;


void
gfx_init_IA32BootInfo(seL4_IA32_BootInfo* bootinfo);


void
gfx_poke_fbxy(const int x, const int y, const uint8_t val);


/*
 * Map complete linear frame buffer.
 */
fb_t
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


#endif /* GRAPHICS_H_ */
