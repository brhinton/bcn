// SPDX-License-Identifier: GPL-2.0-only
/*
 * utl.c
 *
 * Copyright (C) 2022 Bryan Hinton
 *
 */

#include <utl.h>

/* validate pointer */
uint8_t valid(void *t)
{

	extern char _etext;
	return (t != NULL) && ((char*) t > &_etext);
}
