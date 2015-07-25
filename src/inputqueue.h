/*
 * Copyright (c) 2015, Josef Mihalits
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef INPUTQUEUE_H_
#define INPUTQUEUE_H_

void init_nextdir();
int get_nextdir(int pl);
void put_nextdir(int pl, int dir);


#endif /* INPUTQUEUE_H_ */
