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
#include <string.h>
#include <utils/attribute.h>

//extern direction_t dir_forward[];
//extern direction_t dir_back[];
//extern cell_t board[numCellsX][numCellsY];
//extern int deltax[];
//extern int deltay[];
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
    rule_t* cur = &rules[numRules++];
    strcpy(cur->cond, cond);
    cur->action = action;
    cur->weight = weight;
}


static void
init_rules() {
    add_rule("100", MoveForward, 1000);
    add_rule("010", MoveLeft, 1000);
    add_rule("001", MoveRight, 1000);

    add_rule("1##", MoveForward, 10);
    add_rule("#1#", MoveLeft, 3);
    add_rule("##1", MoveRight, 3);
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

static void
read_detectors(char *msg) {
    coord_t pos;
    pos = get_newpos(me->pos, me->direction, MoveForward);
    msg[0] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    pos = get_newpos(me->pos, me->direction, MoveLeft);
    msg[1] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    pos = get_newpos(me->pos, me->direction, MoveRight);
    msg[2] = get_cell(pos) == CELL_EMPTY ? '1':'0';

    msg[3] = 0;
}


static direction_t
get_direction(direction_t dir, action_t action) {
    int delta[] = {0, -1, 1};
    return ((dir + DirLength) + delta[action]) % DirLength;
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

static action_t
get_action(int* matches, int numMatches) {
    assert(numMatches < RULES_LEN);
    if (numMatches == 0) {
        return MoveForward;
    }

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
