/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef TRON_H_
#define TRON_H_

#include <sel4/types.h>

#define NUMPLAYERS 2
#define xRes 640
#define yRes 480
#define cellWidth 10
#define numCellsX (xRes / cellWidth)
#define numCellsY (yRes / cellWidth)

typedef enum { West, North, East, South, DirLength} direction_t;

/* possible things that can be placed onto the board */
typedef enum { CELL_EMPTY, CELL_P0, CELL_P1, CELL_WALL} cell_t ;

typedef struct player {
    /* player's current x cell position on the board */
    int cellx;
    /* player's current y cell position on the board */
    int celly;
    /* direction player is currently facing and moving next */
    direction_t direction;
    /* entity of player: CELL_P0 or CELL_P1 */
    cell_t entity;
} player_t;


uint64_t get_current_time();
direction_t get_computer_move(uint64_t endTime);


#endif /* TRON_H_ */
