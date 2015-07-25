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
#include <sel4platsupport/arch/io.h>
#include <sel4utils/vspace.h>
#include <sel4utils/stack.h>
#include <simple-stable/simple-stable.h>

#include "tron.h"
#include "graphics.h"
#include "inputqueue.h"


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

/* input character device (e.g. keyboard, COM1) */
static ps_chardevice_t inputdev;


// ======================================================================

direction_t dir_forward[] = { West, North, East, South};
direction_t dir_back[] = { East, South, West, North};

int deltax[] = {-1, 0 , 1, 0 };
int deltay[] = {0, -1 , 0, 1 };

static char *keymap[] = { "jilk", "awds"};

/* speed in cells per second */
static int speed = 10;

/* the board is made of cells; cell coordinate (0,0) is in top left corner */
cell_t board[numCellsX][numCellsY];

player_t players[NUMPLAYERS];
static player_t* p0 = players + 0;
static player_t* p1 = players + 1;

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

    /* initialize platsupport IO: virt memory */
    err = sel4platsupport_new_io_mapper(simple, vspace, vka, &io_ops.io_mapper);
    assert(err == 0);

    /* initialize platsupport IO: ports */
    err = sel4platsupport_get_io_port_ops(&io_ops.io_port_ops, &simple);
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

uint64_t
get_current_time() {
    return timer_get_time(tsc_timer->timer);
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


static void
init_cdev (enum chardev_id id,  ps_chardevice_t* dev) {
    ps_chardevice_t *ret;
    ret = ps_cdev_init(id, &io_ops, dev);
    assert(ret != NULL);
}


static int
handle_user_input(int numHumanPlayers) {
    for (;;) { // (1)
        int c = ps_cdev_getchar(&inputdev);
        switch (c) {
        case EOF:
            //read till we get EOF
            for (int pl = 0; pl < numHumanPlayers; pl++) {
                int newdir;
                while ((newdir = get_nextdir(pl)) >= 0) {
                    // skip over forward and backward moves
                    if (newdir != players[pl].direction
                    && newdir !=  dir_back[players[pl].direction]) {
                        players[pl].direction = newdir;
                        break;
                    }
                }
            }
            return 0;
        case 27:
            // ESC key was pressed - quit game
            return 1;
        case ' ':
            printf("-- PAUSE --\n");
            while (' ' != ps_cdev_getchar(&inputdev)) {
                // busy wait
            }
            break;
        default:
            // demultiplex input: check for all players
            for (int pl = 0; pl < numHumanPlayers; pl++) {
                // check all directions
                for (int dir = 0; dir < DirLength; dir++) {
                    if (c == keymap[pl][dir]) {
                        // add direction to respective input queue
                        put_nextdir(pl, dir);
                        break;
                    }
                }
            }
        } // switch
    } // (1)
}


void
put_cell(const coord_t pos, cell_t element) {
    put_board(pos, element);

    // Map "element" to a visual representation and put it on the screen.
    // This currently means to fill the (cx, cy) cell with a different color,
    // but this could be more elaborate.
    uint32_t colors[] = {0
            , gfx_map_color(0, 200, 0)
            , gfx_map_color(0, 0, 200)
            , gfx_map_color(200, 0, 0)};
    uint32_t color = colors[element];
    gfx_draw_rect(pos.x * cellWidth, pos.y * cellWidth, cellWidth, cellWidth, color);
}

void
put_board(const coord_t pos, cell_t element) {
    //put element onto board
    board[pos.x][pos.y] = element;
}


int
isempty_cell(const coord_t pos) {
//    if (pos.x < 0 || pos.x >= numCellsX || pos.y < 0 || pos.y >= numCellsY) {
//        return CELL_WALL;
//    }
    int cell = board[pos.x][pos.y];
    return (cell != CELL_P0 && cell != CELL_P1 && cell != CELL_WALL);
}


cell_t
get_cell(const coord_t pos) {
//    if (pos.x < 0 || pos.x >= numCellsX || pos.y < 0 || pos.y >= numCellsY) {
//        return CELL_WALL;
//    }
    return board[pos.x][pos.y];
}


void init_game() {
    *p0 = (player_t) {
            .pos.x = numCellsX * 3 / 4,
            .pos.y = numCellsY / 2,
            .direction = East,
            .entity = CELL_P0
    };
    *p1 = (player_t) {
            .pos.x = numCellsX * 1 / 4,
            .pos.y = numCellsY / 2,
            .direction = West,
            .entity = CELL_P1
    };

    // clear board and draw boarder walls
    for (int y = 0; y < numCellsY; y++) {
        for (int x = 0; x < numCellsX; x++) {
            if (x == 0 || x == numCellsX -1  || y == 0 || y == numCellsY - 1) {
                //coord_t p = {x,y};
                put_cell((coord_t){x,y}, CELL_WALL);
            } else {
                put_cell((coord_t){x,y}, CELL_EMPTY);
            }
        }
    }

    // draw players at start position
    for (int i = 0; i < NUMPLAYERS; i++) {
        put_cell(players[i].pos, players[i].entity);
    }

    // initialize input queues
    init_nextdir();
}

static int
update_world(player_t* p) {
    p->pos.x += deltax[p->direction];
    p->pos.y += deltay[p->direction];
    if (isempty_cell(p->pos)) {
        put_cell(p->pos, p->entity);
        return 0;
    }
    return 1;
}


void run_game_1player(direction_t startDir) {
    init_game();
    init_computer_move();
    p0->direction = startDir;

    for (;;) {
        // human player moves
        int crash = update_world(p0);
        if (crash) {
            printf("computer wins\n");
            break;
        }
        // computer player moves

        // get current time in ns
        uint64_t startTime = get_current_time();
        // time when computer has to stop "thinking"; use the same
        // formula as for IRQ timing: (IRQ timer period) * (number of loops)
        uint64_t endTime = startTime + (10 * NS_IN_MS) * 100 / speed;
        //get_computer_move(numCellsX, numCellsX, board, endTime, p1, p0);
        p1->direction = get_computer_move(endTime);

        while (get_current_time() < endTime) {
            // busy waiting
        }
        crash = update_world(p1);
        if (crash) {
            printf("player wins\n");
            break;
        }
        // get next human player move
        int quit = handle_user_input(1);
        if (quit) {
            break;
        }
    }
}

void run_game_2player() {
    init_game();

    start_periodic_timer();
    for (;;) {
        wait_for_timer();
        int quit = handle_user_input(2);
        if (quit) {
            break;
        }
        int crash0 = update_world(p0);
        int crash1 = update_world(p1);
        if ((crash0 && crash1)
        || (p0->pos.x == p1->pos.x && p0->pos.y == p1->pos.y)) {
            printf("both crash\n");
            break;
        }
        if (crash0) {
            printf("player 2 wins\n");
            break;
        }
        if (crash1) {
            printf("player 1 wins\n");
            break;
        }

    }
    stop_periodic_timer();
}


void *main_continued()
{
    printf("initialize keyboard\n");
    init_cdev(PC99_KEYBOARD_PS2, &inputdev);

    gfx_print_IA32BootInfo(bootinfo2);
    gfx_init_IA32BootInfo(bootinfo2);
    gfx_map_video_ram(&io_ops.io_mapper);
    gfx_display_testpic();

    printf("initialize timers\n");
    fflush(stdout);
    init_timers();
    printf("done\n");

    for (;;) {
        gfx_diplay_ppm(10, 10, "sel4.ppm");
        for (;;) { // (1)
            int c = ps_cdev_getchar(&inputdev);
            switch(c) {
            case 27:
                return NULL;
                break;
            case ' ':
            case '2':
                run_game_2player();
                break;
            case '1':
                run_game_1player(North);
                break;
            default:
                // game starts with "direction key" press
                for (int i = 0; i < DirLength; i++) {
                    if (keymap[0][i] == c) {
                        run_game_1player(dir_forward[i]);
                    }
                }
            }
        } // (1)
    }
}

int main()
{
    setup_system();

    /* enable serial driver */
    platsupport_serial_setup_simple(NULL, &simple, &vka);

    printf("\n\n========= starting ========= \n\n");

    // stack size is configurable via CONFIG_SEL4UTILS_STACK_SIZE
    int err = (int)sel4utils_run_on_stack(&vspace, main_continued, NULL);
    assert(err == 0);
    printf("Bye!\n\n");
    return 0;
}
