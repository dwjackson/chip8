#ifndef ENCODE_H
#define ENCODE_H

#include "asm8.h"
#include <stdlib.h>

int encode_statement(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels,
	unsigned short *asm_stmt_p);

#endif /* ENCODE_H */
