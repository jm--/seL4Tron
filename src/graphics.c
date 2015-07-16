/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include "graphics.h"

void
printVBE(seL4_IA32_BootInfo* bootinfo2) {
    seL4_VBEInfoBlock* ib      = &bootinfo2->vbeInfoBlock;
    seL4_VBEModeInfoBlock* mib = &bootinfo2->vbeModeInfoBlock;

    printf("\n");
    printf("=== VBE begin ===========\n");
    printf("vbeMode: 0x%x\n", bootinfo2->vbeMode);
    printf("  VESA mode=%d; linear frame buffer=%d\n",
            (bootinfo2->vbeMode & BIT(8)) != 0,
            (bootinfo2->vbeMode & BIT(14)) != 0);
    printf("vbeInterfaceSeg: 0x%x\n", bootinfo2->vbeInterfaceSeg);
    printf("vbeInterfaceOff: 0x%x\n", bootinfo2->vbeInterfaceOff);
    printf("vbeInterfaceLen: 0x%x\n", bootinfo2->vbeInterfaceLen);

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

