#ifndef ENCODE_H
#define ENCODE_H

#include "asm8.h"
#include <stdlib.h>

#define NO_INSTRUCTION 0x00AA

unsigned short encode_statement(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels);

#endif /* ENCODE_H */
