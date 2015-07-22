/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include "tron.h"
#include <stdlib.h>

extern direction_t dir_forward[];
extern direction_t dir_back[];
extern cell_t board[numCellsX][numCellsY];
extern int deltax[];
extern int deltay[];
extern player_t players[NUMPLAYERS];
//static player_t* other = players + 0;
static player_t* me = players + 1;


void
get_computer_move(uint64_t endTime) {

    while (get_current_time() < endTime) {
        // TODO: find move
    }
    me->direction = rand() % DirLength;
}
