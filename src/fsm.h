/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#include <stdio.h>

enum state {
	STATE_START,
	STATE_LABEL,
	STATE_AFTER_LABEL,
	STATE_INSTRUCTION,
	STATE_WHITESPACE,
	STATE_ARGUMENT,
	STATE_COMMENT,
	STATE_DONE
};

struct fsm {
	enum state currstate;
	const char *in;
	struct statement *stmt;
};

void fsm_init(struct fsm *fsm, const char *in, struct statement *stmt);
char fsm_nextchar(struct fsm *fsm);
enum state fsm_nextstate(struct fsm *fsm, char nextch);
void fsm_transition(struct fsm *fsm, enum state nextstate);
int fsm_is_done(struct fsm *fsm);
