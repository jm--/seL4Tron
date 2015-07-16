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


/*
 * Print the VBE (VESA Bios Extension) conveniently included in the
 * boot info available to the root task.
 *
 * The InfoBlock struc is returned by VBE function 00h.
 * The ModeInfoBlock struc is returned by VBE function 01h.
 * The VBE 2.0 protected mode interface (vbeMode, vbeInterfaceSeg,
 * vbeInterfaceOff, and vbeInterfaceLen) is returned by VBE function 0Ah.
 * See version 3.0 of the VBE standard and the
 * GNU "Multiboot Specification" for details.
 */
void
printVBE(seL4_IA32_BootInfo* bootinfo2);


#endif /* GRAPHICS_H_ */
