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

#include <stdint.h>

#define NUMPLAYERS 2
#define xRes 640
#define yRes 480
#define cellWidth 10
/* lineWidth must be less than or equal cellWidth and an even number */
#define lineWidth 6
#define numCellsX (xRes / cellWidth)
#define numCellsY (yRes / cellWidth)

typedef struct coord {
    int x;
    int y;
} coord_t;

typedef enum { West, North, East, South, DirLength} direction_t;

/* possible things that can be placed onto the board */
typedef enum { CELL_EMPTY, CELL_P0, CELL_P1, CELL_WALL, CELL_LEN} cell_t ;

typedef struct player {
    /* player's current x and y cell position on the board */
    coord_t pos;
    /* direction player is currently facing and moving next */
    direction_t direction;
    /* entity of player: CELL_P0 or CELL_P1 */
    cell_t entity;
    /* name of the player */
    char* name;
    /* player's current score */
    int score;
} player_t;


uint64_t get_current_time();
void init_computer_move();
direction_t get_computer_move(uint64_t endTime, player_t* me, player_t* you);
cell_t get_cell(const coord_t pos);
void put_board(const coord_t pos, cell_t element);
int isempty_cell(const coord_t pos);
void waitf();

#endif /* TRON_H_ */
