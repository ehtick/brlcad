/*                      R E A D N A M E . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file iges/readname.c
 *
 * This routine reads the next field in "card" buffer.  It expects the
 * field to contain a character string of the form "nHstring" where n
 * is the length of "string". If "id" is not the null string, then
 * "id" is printed followed by "string".  A pointer to the string is
 * returned in "ptr".
 *
 * "eofd" is the "end-of-field" delimiter
 * "eord" is the "end-of-record" delimiter
 *
 */

#include "./iges_struct.h"
#include "./iges_extern.h"

#define MAX_NUM 4096


void
Readname(char **ptr, char *id)
{
    int i = 0, length = 0, done = 0, lencard;
    char num[MAX_NUM] = {0};
    char *ch;

    if (card[counter] == eofd) {
	/* This is an empty field */
	*ptr = (char *)NULL;
	counter++;
	return;
    } else if (card[counter] == eord) {
	/* Up against the end of record */
	*ptr = (char *)NULL;
	return;
    }

    if (card[72] == 'P')
	lencard = PARAMLEN;
    else
	lencard = CARDLEN;

    if (counter > lencard)
	Readrec(++currec);

    if (*id != '\0')
	bu_log("%s", id);

    while (!done && i < MAX_NUM-1) {
	while (i < MAX_NUM-1 &&
	       (num[i] = card[counter++]) != 'H' &&
	       counter <= lencard)
	{
	    if (i >= MAX_NUM-1) {
		done = 1;
	    }
	    i++;
	}
	if (counter > lencard) {
	    Readrec(++currec);
	} else {
	    done = 1;
	}
    }

    length = atoi(num);

    /* we may tack on a letter to the name */
    *ptr = (char *)bu_malloc((length + 2)*sizeof(char), "Readname: name");
    ch = *ptr;
    for (i = 0; i < length; i++) {
	if (counter > lencard)
	    Readrec(++currec);
	ch[i] = card[counter++];
	if (*id != '\0')
	    bu_log("%c", ch[i]);
    }
    ch[length] = '\0';
    if (*id != '\0')
	bu_log("%c", '\n');

    done = 0;
    while (!done) {
	while (card[counter++] != eofd && card[counter] != eord && counter <= lencard)
	    ;
	if (counter > lencard && card[counter] != eord && card[counter] != eofd)
	    Readrec(++currec);
	else
	    done = 1;
    }

    if (card[counter-1] == eord)
	counter--;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
