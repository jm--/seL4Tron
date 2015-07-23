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
#define CI_FORWARD_ISOK       3
#define CI_LEFT_ISOK          4
#define CI_RIGHT_ISOK         5
#define CI_LAST               6

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
    add_rule("011#1#", MoveLeft, 20);
    // 1 of 3 is blocked: forward is blocked, right looks promising
    add_rule("011##1", MoveRight, 20);
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
static int cutoff = 200;

static void
count_empty_fill(coord_t pos) {


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


static void
count_emptyCells(coord_t pos, int* _count) {
    count = 0;
    traceValue++;
    count_empty_fill(pos);
    *_count = count;
}


static void
read_detectors_direction(coord_t pos, int* count, char* isempty, char* isok) {
    if (isempty_cell(pos)) {
        *isempty = '1';
        count_emptyCells(pos, count);
        *isok = *count > cutoff ? '1' : '0';
    } else {
        *count = 0;
        *isempty = *isok = '0';
    }
}


static void
read_detectors(char *msg) {
    coord_t pos;
    int countf = 0;
    int countl = 0;
    int countr = 0;

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

    //------------
    msg[CI_LAST] = 0;

    printf ("current pos: x=%d y=%d current dir=%d\n", me->pos.x, me->pos.y, me->direction);
    printf (">> msg: %s\n", msg);
    printf ("coutf=%d countl=%d countr=%d\n", countf, countl, countr);
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
