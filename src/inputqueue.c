/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "COPYING" for details.
 *
 */

/*
 * The input from the keyboard is demultiplexed into separate input queues.
 * For example, suppose the input stream is P1_up, P1_left, P2_right.
 * (That is player one clicked the up key, then the left key; then player
 * two clicked the move right key.) Then the next move for P1 is to move up
 * and for player P2 to move right. P1_left remains in the input queue.
 */
#include <assert.h>
#include "tron.h"


#define INPUTQUEUE_LEN 10

typedef struct {
    int head;
    int tail;
    direction_t queue[INPUTQUEUE_LEN];
} inputqueue_t;


static inputqueue_t q[NUMPLAYERS];


/*
 * Get next direction of player "pl".
 */
int
get_nextdir(int pl) {
    assert(pl < NUMPLAYERS);
    int head  = q[pl].head;
    if (head == q[pl].tail) {
        // queue is empty
        return -1;
    }
    int dir = q[pl].queue[head];
    // advance head
    q[pl].head = (head + 1) % INPUTQUEUE_LEN;
    return dir;
}


/*
 * Add direction "dir" to input queue of player "pl".
 */
void
put_nextdir(int pl, int dir) {
    assert(pl < NUMPLAYERS);
    int nexttail = (q[pl].tail + 1) % INPUTQUEUE_LEN;
    if (q[pl].head == nexttail) {
        // queue is full; ignore input
        return;
    }
    q[pl].queue[q[pl].tail] = dir;
    q[pl].tail = nexttail;
}


/*
 * Initialize input queues.
 */
void
init_nextdir() {
    for (int i = 0; i < NUMPLAYERS; i++) {
        q[i].head = q[i].tail = 0;
    }
}
