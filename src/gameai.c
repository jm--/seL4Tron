/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include "tron.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/attribute.h>

#define CI_FORWARD_ISEMPTY    0
#define CI_LEFT_ISEMPTY       1
#define CI_RIGHT_ISEMPTY      2
#define CI_LEFT_ISOK          3
#define CI_RIGHT_ISOK         4
#define CI_LAST               5

extern player_t players[NUMPLAYERS];
//static player_t* other = players + 0;
static player_t* me = players + 1;



#define COND_LEN 10

typedef enum { MoveForward = 0, MoveLeft = 1, MoveRight = 2, ActionLen = 3} action_t;

typedef struct {
    char cond[COND_LEN];
    action_t action;
    int weight;
} rule_t;

#define RULES_LEN 20

static rule_t rules[RULES_LEN];

static int numRules = 0;



static void
add_rule(char* cond, action_t action, int weight) {
    assert(numRules < RULES_LEN);
    char* dst = rules[numRules].cond;
    int i;
    for (i = 0; cond[i]; i++) {
        assert(i < COND_LEN);
        dst[i] = cond[i];
    }
    for(; i < COND_LEN - 1; i++) {
        dst[i] = '#';
    }
    dst[i] = '\0';

    //strcpy(cur->cond, cond);
    rules[numRules].action = action;
    rules[numRules].weight = weight;
    numRules++;
}


static void
init_rules() {
    // the most desperate moves: there is only one option left,
    // so we better take it
    add_rule("100", MoveForward, 1000);
    add_rule("010", MoveLeft, 1000);
    add_rule("001", MoveRight, 1000);

    // it's game over; move forward to end it
    add_rule("000", MoveForward, 1);

    //moving forward (when possible) is not bad because
    //zigzag moves bear the risk of locking ourselves in
    add_rule("1##", MoveForward, 20);

    //occasionally, we should make a turn (if cell in move direction is empty)
    add_rule("#1#", MoveLeft, 2);
    add_rule("##1", MoveRight, 2);

    // forward is blocked; left and right is empty: we have to turn,
    // turn when directions looks promising
    add_rule("01#1#", MoveLeft, 20);
    add_rule("0#1#1", MoveRight, 20);
}

static coord_t
get_newpos(coord_t pos, direction_t dir, action_t action) {
    static coord_t delta[DirLength][ActionLen] = {
            {{-1,0}, {0,1},  {0,-1}},
            {{0,-1}, {-1,0}, {1,0}},
            {{1,0},  {0,-1}, {0,1}},
            {{0,1},  {1,0},  {-1,0}}
    };
    coord_t d = delta[dir][action];
    return (coord_t){pos.x + d.x, pos.y + d.y};
}

static direction_t
get_direction(direction_t dir, action_t action) {
    int delta[] = {0, -1, 1};
    return ((dir + DirLength) + delta[action]) % DirLength;
}

static int count = 0;
static int traceValue = CELL_LEN + 1;

static void
count_empty_fill(coord_t pos) {
    static int cutoff = 200;

    cell_t cell = get_cell(pos);
    if (cell == traceValue || cell == CELL_P0
    || cell == CELL_P1 || cell == CELL_WALL) {
        //cell at location "pos" is not empty
        return;
    }
    // put a "trace value" into cell leaving a trail and marking it non-empty
    put_board(pos, traceValue);
    if (++count >= cutoff) {
        return;
    }
    count_empty_fill((coord_t){pos.x - 1, pos.y});
    count_empty_fill((coord_t){pos.x, pos.y - 1});
    count_empty_fill((coord_t){pos.x + 1, pos.y});
    count_empty_fill((coord_t){pos.x, pos.y + 1});
}

static int
count_emptyCells(coord_t pos, direction_t dir, action_t action) {
    const int slen = 5;  //number of cells sideways
    const int tlen = 4;  //number of cells back
    int count = 0;

    assert(action == MoveLeft || action == MoveRight);
    direction_t dsideways = get_direction(dir, action);
    direction_t dback = get_direction(dsideways, action);

    for (int s = 0; s < slen; s++) {
        coord_t p = pos;
        for (int t = 0; t < tlen; t++) {
            if (get_cell(p) == CELL_EMPTY) {
                count++;
            }
            //printf("  > s=%d t=%d x=%d y=%d count=%d\n", s,t,p.x, p.y, count);
            //waitf();
            p = get_newpos(p, dback, MoveForward);
        }
        pos = get_newpos(pos, dsideways, MoveForward);
    }

    return count;
}


static void
read_detectors(char *msg) {
    coord_t pos;
    pos = get_newpos(me->pos, me->direction, MoveForward);
    msg[CI_FORWARD_ISEMPTY] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    pos = get_newpos(me->pos, me->direction, MoveLeft);
    msg[CI_LEFT_ISEMPTY] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    pos = get_newpos(me->pos, me->direction, MoveRight);
    msg[CI_RIGHT_ISEMPTY] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    //--------- heuristic helping to choose between left/right
    // count empty cells in rectangle of width s and height t, left
    // and right of current position and direction
    // (this seems to work as badly as I thought it would:)

    int numEmptyLeft  = count_emptyCells(me->pos, me->direction, MoveLeft);
    int numEmptyRight = count_emptyCells(me->pos, me->direction, MoveRight);

    const int diff = 3;
    if (numEmptyLeft - numEmptyRight > diff) {
        //left are more empty cells than right
        msg[CI_LEFT_ISOK] = '1';
        msg[CI_RIGHT_ISOK] = '0';
    } else if (numEmptyLeft - numEmptyRight < -diff) {
        //right are more empty cells than left
        msg[CI_LEFT_ISOK] = '0';
        msg[CI_RIGHT_ISOK] = '1';
    } else {
        //there is roughly an equal number of empty cells on both sides
        msg[CI_LEFT_ISOK] = '1';
        msg[CI_RIGHT_ISOK] = '1';
    }
    //-----------
    count = 0;
    traceValue++;
    count_empty_fill(pos);
    printf("count_empty_fill(): %d %d\n", count,traceValue);

    //------------
    msg[CI_LAST] = 0;

    printf ("current pos: x=%d y=%d current dir=%d\n", me->pos.x, me->pos.y, me->direction);
    printf ("msg: %s\n", msg);
    printf ("numEmptyLeft : %d\n", numEmptyLeft);
    printf ("numEmptyRight: %d\n\n", numEmptyRight);
}




UNUSED static void
test_module() {
    assert(get_direction(West, MoveForward) == West);
}

static void
match_rules(char* msg, int* matches, int* numMatches) {

    *numMatches = 0;

    for (int i = 0; i < numRules; i++) {
        int j;
        for (j = 0; msg[j]; j++) {
            if ((msg[j] == '1' && rules[i].cond[j] == '0')
            ||  (msg[j] == '0' && rules[i].cond[j] == '1'))
            {
                //this condition does not match
                break;
            }
        }
        // we reached end of msg string, so condition rules[i].cond
        // is a match; so add rule to "matches"
        if (msg[j] == 0) {
            matches[(*numMatches)++] = i;
        }
    }
}

static void
debug_print_rules(int* matches, int numMatches) {
    for (int i = 0; i < numRules; i++) {
        printf("num=%d cond=%s weight=%d action=%d\n", i, rules[i].cond, rules[i].weight, rules[i].action);
    }
    printf("matches: ");
    for (int i = 0; i < numMatches; i++) {
        printf("%d ", matches[i]);
    }
    printf("\n");
}

static action_t
get_action(int* matches, int numMatches) {
    assert(numMatches < RULES_LEN);
    assert(numMatches > 0);

    debug_print_rules(matches, numMatches);
    // roulette wheel selection
    int totalWeight = 0;
    for (int i = 0; i < numMatches; i++) {
        totalWeight += rules[matches[i]].weight;
    }
    int selected = random() % totalWeight;

    int i;
    for(i = 0; i < numMatches; i++) {
        selected -= rules[matches[i]].weight;
        if(selected <= 0) {
            break;
        }
    }

    printf("picking matched rule i=%d, which is rule i=%d\n", i,matches[i]);

    return rules[matches[i]].action;
}


direction_t
get_computer_move(uint64_t endTime) {

    if (numRules == 0) {
        init_rules();
        srandom(endTime);
    }

    static char msg[COND_LEN];
    read_detectors(msg);

    int matches[RULES_LEN];
    int numMatches;
    match_rules(msg, matches, &numMatches);

    action_t action = get_action(matches, numMatches);

    return get_direction(me->direction, action);

}
