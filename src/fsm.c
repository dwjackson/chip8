#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "fsm.h"

#define COMMENT_CHAR ';'

void fsm_init(struct fsm *fsm, const char *in, struct statement *stmt)
{
	fsm->currstate = STATE_START;
	fsm->in = in;
	fsm->stmt = stmt;
}

char fsm_nextchar(struct fsm *fsm)
{
	char ch = *(fsm->in);
	fsm->in++;
	return ch;
}

enum state fsm_nextstate(struct fsm *fsm, char nextch)
{
	if (nextch == '\n') {
		return STATE_DONE;
	}
	if (nextch == COMMENT_CHAR) {
		return STATE_COMMENT;
	}
	switch (fsm->currstate) {
	case STATE_START:
		if (!isspace(nextch)) {
			return STATE_LABEL;
		}
		break;
	case STATE_LABEL:
		if (nextch == ':') {
			return STATE_AFTER_LABEL;
		}
		if (isspace(nextch)) {
			fsm->currstate = STATE_INSTRUCTION;
			return STATE_WHITESPACE;
		}
		break;
	case STATE_AFTER_LABEL:
		if (!isspace(nextch)) {
			return STATE_INSTRUCTION;
		}
		break;
	case STATE_INSTRUCTION:
		if (isspace(nextch)) {
			return STATE_WHITESPACE;
		}
		break;
	case STATE_WHITESPACE:
		if (nextch == ',') {
			return STATE_WHITESPACE;
		}
		if (!isspace(nextch)) {
			return STATE_ARGUMENT;
		}
		break;
	case STATE_ARGUMENT:
		if (nextch == ',') {
			return STATE_WHITESPACE;
		}
		if (isspace(nextch)) {
			return STATE_WHITESPACE;
		}
		break;
	case STATE_COMMENT:
		return STATE_COMMENT;
	case STATE_DONE:
		break;
	default:
		/* Weird state, do nothing */
		break;
	}
	return fsm->currstate;
}

void fsm_transition(struct fsm *fsm, enum state nextstate)
{
	fsm->currstate = nextstate;
}

int fsm_is_done(struct fsm *fsm)
{
	return fsm->currstate == STATE_DONE;
}