/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <autoconf.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sel4/arch/bootinfo.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <platsupport/timer.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/plat/timer.h>
#include <sel4utils/vspace.h>
#include <simple-stable/simple-stable.h>
#include "graphics.h"




/* memory management: Virtual Kernel Allocator (VKA) interface and VSpace */
static vka_t vka;
static vspace_t vspace;

/*system abstraction */
static simple_t simple;

/* root task's BootInfo */
static seL4_BootInfo *bootinfo;

/* root task's IA32_BootInfo */
static seL4_IA32_BootInfo *bootinfo2;

/* platsupport I/O */
static ps_io_ops_t io_ops;

/* amount of virtual memory for the allocator to use */
#define VIRT_POOL_SIZE (BIT(seL4_PageBits) * 200)

/* static memory for the allocator to bootstrap with */
#define POOL_SIZE (BIT(seL4_PageBits) * 10)
static char memPool[POOL_SIZE];

/* for virtual memory bootstrapping */
static sel4utils_alloc_data_t allocData;

/* platsupport (periodic) timer */
static seL4_timer_t* timer;

/* platsupport TSC based timer */
static seL4_timer_t* tsc_timer;

/* async endpoint for periodic timer */
static vka_object_t timer_aep;

// ======================================================================
#define xRes 640
#define yRes 480
#define cellWidth 10
#define numCellsX (xRes / cellWidth)
#define numCellsY (yRes / cellWidth)

/* number of cells per second */
static int speed = 10;
// ======================================================================
/*
 * Initialize all main data structures.
 *
 * The code to initialize simple, allocman, vka, and vspace is modeled
 * after the "sel4test-driver" app:
 * https://github.com/seL4/sel4test/blob/master/apps/sel4test-driver/src/main.c
 */
static void
setup_system()
{
    /* initialize boot information */
    bootinfo  = seL4_GetBootInfo();
    bootinfo2 = seL4_IA32_GetBootInfo();
    assert(bootinfo2); // boot kernel in graphics mode

    /* initialize simple interface */
    simple_stable_init_bootinfo(&simple, bootinfo);
    //simple_default_init_bootinfo(simple, bootinfo);

    /* create an allocator */
    allocman_t *allocman;
    allocman = bootstrap_use_current_simple(&simple, POOL_SIZE, memPool);
    assert(allocman);

    /* create a VKA */
    allocman_make_vka(&vka, allocman);

    /* create a vspace */
    UNUSED int err;
    err = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace,
            &allocData, seL4_CapInitThreadPD, &vka, bootinfo);
    assert(err == 0);

    /* fill allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t vres;
    vres = vspace_reserve_range(&vspace, VIRT_POOL_SIZE, seL4_AllRights,
            1, &vaddr);
    assert(vres.res);
    bootstrap_configure_virtual_pool(allocman, vaddr, VIRT_POOL_SIZE,
            seL4_CapInitThreadPD);

    /* initialize io_ops */
    err = sel4platsupport_new_io_ops(simple, vspace, vka, &io_ops);
    assert(err == 0);
}


static void
init_timers()
{
    // get an endpoint for the timer IRQ (interrupt handler)
    UNUSED int err = vka_alloc_async_endpoint(&vka, &timer_aep);
    assert(err == 0);

    // get the timer
    timer = sel4platsupport_get_default_timer(&vka, &vspace, &simple, timer_aep.cptr);
    assert(timer != NULL);

    // get a TSC timer (forward marching time); use
    // timer_get_time(tsc_timer) to get current time (in ns)
    tsc_timer = sel4platsupport_get_tsc_timer(timer);
    assert(tsc_timer != NULL);
}


static void
start_periodic_timer() {
    //setup periodic timer
    UNUSED int err = timer_periodic(timer->timer, 10 * NS_IN_MS);
    assert(err == 0);

    //start timer (no-op for PIT)
    err = timer_start(timer->timer);
    assert(err == 0);

    //Ack IRQ for good measure
    sel4_timer_handle_single_irq(timer);
}


static void
stop_periodic_timer() {
    UNUSED int err = timer_stop(timer->timer);
    assert(err == 0);

    sel4_timer_handle_single_irq(timer);
}


static void
wait_for_timer()
{
    for (int i = 0; i < 100 / speed; i++) {
        //wait for timer interrupt to occur
        seL4_Wait(timer_aep.cptr, NULL);

        //Ack IRQ
        sel4_timer_handle_single_irq(timer);
    }
}



inline static void
draw_cell(const int cx, const int cy) {
    gfx_draw_rect(cx * cellWidth, cy * cellWidth, cellWidth, cellWidth, 11);
}

int main()
{
    setup_system();

    /* enable serial driver */
    platsupport_serial_setup_simple(NULL, &simple, &vka);

    printf("\n\n========= starting ========= \n\n");
    gfx_print_IA32BootInfo(bootinfo2);
    gfx_init_IA32BootInfo(bootinfo2);
    gfx_map_video_ram(&io_ops.io_mapper);
    //gfx_display_testpic();

    printf("initialize timers\n");
    fflush(stdout);
    init_timers();
    start_periodic_timer();

    for (int y = 0; y < numCellsY; y++) {
        for (int x = 0; x < numCellsX; x++) {
            wait_for_timer();
            draw_cell(x, y);
        }
    }
    stop_periodic_timer();

    return 0;
}
