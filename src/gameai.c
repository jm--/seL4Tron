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
#include <assert.h>
#include <utils/attribute.h>

enum {
    CI_FORWARD_ISEMPTY,
    CI_LEFT_ISEMPTY,
    CI_RIGHT_ISEMPTY,
    CI_FORWARD_ISOK,
    CI_LEFT_ISOK,
    CI_RIGHT_ISOK,
    CI_QUADRANT_D1,
    CI_QUADRANT_D0,
    CI_LAST
};




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

/* mapping from enum value to string */
static char* str_action[] = {"forward", "left", "right"};

/* mapping from enum value to string */
static char* str_direction[] = {"West", "North", "East", "South"};

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

    rules[numRules].action = action;
    rules[numRules].weight = weight;
    numRules++;
}


static void
init_rules() {
    // 2 of 3 is blocked: there is only one option left,
    // so we better take it
    add_rule("100", MoveForward, 1000);
    add_rule("010", MoveLeft, 1000);
    add_rule("001", MoveRight, 1000);

    // 3 of 3 is blocked: it's game over; move forward to end it
    add_rule("000", MoveForward, 1);

    //catch all: low weight because those could be fatal
    add_rule("1##", MoveForward, 1);
    add_rule("#1#", MoveLeft, 1);
    add_rule("##1", MoveRight, 1);

    //moving forward (when possible) is not bad because
    //zigzag moves bear the risk of locking ourselves in
    add_rule("111", MoveForward, 5);

    //forward is looking good, so favor forward move even more
    add_rule("1##1##", MoveForward, 20);

    // 1 of 3 is blocked: forward is blocked, left looks promising
    add_rule("011#1#", MoveLeft, 30);
    // 1 of 3 is blocked: forward is blocked, right looks promising
    add_rule("011##1", MoveRight, 30);

    //rules including quadrants: (those may not be helpful at all)
    //other is in Q1 (in front of me, right)
    add_rule("1##1##00", MoveForward, 20);
    //other is in Q2 (in front of me, left)
    add_rule("1##1##01", MoveForward, 20);

    //other is in Q2 (in front of me, left)
    add_rule("#1##1#01", MoveLeft, 5);
    //other is in Q3 (behind me, left)
    add_rule("#1##1#10", MoveLeft, 40);

    //other is in Q1 (in front of me, right)
    add_rule("##1##100", MoveRight, 5);
    //other is in Q4 (behind me, right)
    add_rule("##1##111", MoveRight, 40);
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

/* Value recursive functions leave to indicate a cell was visited.
 * The value is only used by functions that explore the board for
 * heuristics. Cells marked with this value must be treated
 * like CELL_EMPTY by the actual game play logic.
 */
static int traceValue;
/* Cutoff value at which recursion is terminated.
 * */
static int cutoff = 200;

/*
 * Count number of empty cells potentially reachable from position pos.
 * This count is an upper bound, as the allowed moves are more restrictive
 * than the moves exercised here.
 * @param pos: current location
 * @param count: number of empty cells found
 */
static void
count_emptyCells(coord_t pos, int* count) {
    cell_t cell = get_cell(pos);
    if (cell == traceValue || cell == CELL_P0
    || cell == CELL_P1 || cell == CELL_WALL) {
        //cell at location "pos" is not empty
        return;
    }
    // put a "trace value" into cell leaving a trail and marking it non-empty
    put_board(pos, traceValue);
    if (++(*count) >= cutoff) {
        //Count could end up being greater than cutoff because we keep
        //increasing the count as the recursion unwinds. This is on purpose
        //as it makes count more accurate.
        return;
    }
    count_emptyCells((coord_t){pos.x - 1, pos.y}, count);
    count_emptyCells((coord_t){pos.x, pos.y - 1}, count);
    count_emptyCells((coord_t){pos.x + 1, pos.y}, count);
    count_emptyCells((coord_t){pos.x, pos.y + 1}, count);
}


static void
read_detectors_direction(coord_t pos, int* count, char* isempty, char* isok) {
    *count = 0;
    if (isempty_cell(pos)) {
        *isempty = '1';
        traceValue++;
        assert(traceValue > CELL_LEN);
        count_emptyCells(pos, count);
        *isok = *count > cutoff ? '1' : '0';
    } else {
        *isempty = *isok = '0';
    }
}


static void
read_detectors(char *msg, player_t* me, player_t* you) {
    coord_t pos;
    int countf = 0; // number of empty cells in forward direction
    int countl = 0; // in left direction
    int countr = 0; // in right direction

    //-----forward
    pos = get_newpos(me->pos, me->direction, MoveForward);
    read_detectors_direction(pos, &countf,
            msg + CI_FORWARD_ISEMPTY, msg + CI_FORWARD_ISOK);
    //-----left
    pos = get_newpos(me->pos, me->direction, MoveLeft);
    read_detectors_direction(pos, &countl,
            msg + CI_LEFT_ISEMPTY, msg + CI_LEFT_ISOK);
    //-----right
    pos = get_newpos(me->pos, me->direction, MoveRight);
    read_detectors_direction(pos, &countr,
            msg + CI_RIGHT_ISEMPTY, msg + CI_RIGHT_ISOK);

    //-----check left/right
    if (countf == 0 && countl > 0 && countr > 0) {
        //forward is blocked; left and right are open
        //favor direction with more empty cells
        const int diff = 5;
        if (countl - countr > diff) {
            //left are more empty cells than right
            msg[CI_LEFT_ISOK] = '1';
            msg[CI_RIGHT_ISOK] = '0';
        } else if (countl - countr < -diff) {
            //right are more empty cells than left
            msg[CI_LEFT_ISOK] = '0';
            msg[CI_RIGHT_ISOK] = '1';
        } else {
            //there is roughly an equal number of empty cells on both sides
            //this could be that both are at or above cutoff
            msg[CI_LEFT_ISOK] = '1';
            msg[CI_RIGHT_ISOK] = '1';
        }
    }

    //------------ get the quadrant human player is in relative to "me"
    int dx = you->pos.x - me->pos.x;
    int dy = -(you->pos.y - me->pos.y);

    //imagine me->pos at the origin of a coordinate system with positive
    //y-axis in direction me->direction; find the quadrant in which
    //human player is in
    int quadrant = (dx >= 0 ? (dy >= 0 ? 3:2) : (dy >= 0 ? 0:1));
    quadrant = (quadrant + me-> direction) % 4 + 1;

    //binary encode quadrant 1=00b; 2=01b; 3=10b; 4=11b
    //lower order bit
    msg[CI_QUADRANT_D0] = quadrant == 2 || quadrant == 4 ? '1' : '0';
    //higher order bit
    msg[CI_QUADRANT_D1] = quadrant == 3 || quadrant == 4 ? '1' : '0';



    //------------
    msg[CI_LAST] = 0;

    printf ("me (player %d) : x=%d y=%d dir=%d (%s)\n",
            me->entity, me->pos.x, me->pos.y, me->direction,
            str_direction[me->direction]);
    printf ("you (player %d): x=%d y=%d dir=%d (%s)\n",
            you->entity, you->pos.x, you->pos.y,
            you->direction, str_direction[you->direction]);
    printf ("coutf=%d countl=%d countr=%d; ", countf, countl, countr);
    printf ("quadrant=%d\n", quadrant);
    printf (">> msg: %s\n", msg);
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
debug_print_rules(int* matches, int numMatches, int ruleid) {
    int* m = matches;
    for (int i = 0; i < numRules; i++) {
        printf("num=%2d cond=%s weight=%4d action=%d (%s)", i,
                rules[i].cond, rules[i].weight,
                rules[i].action, str_action[rules[i].action]);
        if ((m - matches) < numMatches && i == *m) {
            //rule i matched message
            printf(" <==");
            m++;
        }
        if (i == ruleid) {
            printf(" *selected*");
        }
        printf("\n");
    }
    printf("picking rule num: %d\n", ruleid);
}

static action_t
get_action(int* matches, int numMatches) {
    assert(numMatches < RULES_LEN);
    assert(numMatches > 0);

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

    debug_print_rules(matches, numMatches, matches[i]);

    return rules[matches[i]].action;
}


/*
 * Called at the beginning of a new single player game.
 */
void
init_computer_move() {
    if (numRules == 0) {
        init_rules();
        srandom(get_current_time());
    }
    // reset value as variable must not overflow
    traceValue = CELL_LEN + 1;
}


direction_t
get_computer_move(uint64_t endTime, player_t* me, player_t* you) {
    static char msg[COND_LEN];
    read_detectors(msg, me, you);

    int matches[RULES_LEN];
    int numMatches;
    match_rules(msg, matches, &numMatches);

    action_t action = get_action(matches, numMatches);

    direction_t newdir = get_direction(me->direction, action);
    printf("computer moves %d (%s)\n", newdir, str_direction[newdir]);
    printf("--------------------\n");
    return newdir;
}
