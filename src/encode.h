/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright 2018 David Jackson
 */

#ifndef ENCODE_H
#define ENCODE_H

#include "asm8.h"
#include <stdlib.h>

int encode_statement(struct statement *stmt,
	struct label labels[MAX_LABELS], size_t num_labels,
	unsigned short *asm_stmt_p);

#endif /* ENCODE_H */
