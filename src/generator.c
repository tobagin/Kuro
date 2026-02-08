/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Kuro
 * Copyright (C) Philip Withnall 2007-2008 <philip@tecnocode.co.uk>
 * 
 * Kuro is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kuro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kuro.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <stdlib.h>

#include "main.h"
#include "generator.h"
#include "rules.h"

void
kuro_generate_board (Kuro *kuro, guint new_board_size, guint seed)
{
	guchar i;
	guint total, old_total;
	KuroVector iter;
	gboolean *accum, **horiz_accum;

	g_return_if_fail (kuro != NULL);
	g_return_if_fail (new_board_size > 0);

	/* Seed the random number generator */
	if (seed == 0)
		seed = g_get_real_time ();

	if (kuro->debug)
		g_debug ("Seed value: %u", seed);

	srand (seed);

	/* Deallocate any previous board */
	kuro_free_board (kuro);

	kuro->board_size = new_board_size;

	accum = g_new0 (gboolean, kuro->board_size + 2); /* Stores which numbers have been used in the current column */
	horiz_accum = g_new (gboolean*, kuro->board_size); /* Stores which numbers have been used in each row */
	for (iter.x = 0; iter.x < kuro->board_size; iter.x++)
		horiz_accum[iter.x] = g_slice_alloc0 (sizeof (gboolean) * (kuro->board_size + 2));

	/* Allocate the board */
	kuro->board = g_new (KuroCell*, kuro->board_size);
	for (i = 0; i < kuro->board_size; i++)
		kuro->board[i] = g_slice_alloc0 (sizeof (KuroCell) * kuro->board_size);

	/* Generate some randomly-placed painted cells */
	total = rand () % 5 + 13; /* Total number of painted cells (between 14 and 18 inclusive) */
	/* For the moment, I'm hardcoding the range in the number of painted
	 * cells, and only specifying it for 8x8 grids. This will change in the
	 * future. */
	for (i = 0; i < total; i++) {
		/* Generate pairs of coordinates until we find one which lies between unpainted cells (or at the edge of the board) */
		do {
			iter.x = rand () % kuro->board_size;
			iter.y = rand () % kuro->board_size;

			if ((iter.y < 1 || (kuro->board[iter.x][iter.y-1].status & CELL_PAINTED) == FALSE) &&
			    (iter.y + 1 >= kuro->board_size || (kuro->board[iter.x][iter.y+1].status & CELL_PAINTED) == FALSE) &&
			    (iter.x < 1 || (kuro->board[iter.x-1][iter.y].status & CELL_PAINTED) == FALSE) &&
			    (iter.x + 1 >= kuro->board_size || (kuro->board[iter.x+1][iter.y].status & CELL_PAINTED) == FALSE))
				break;
		} while (TRUE);

		kuro->board[iter.x][iter.y].status |= (CELL_PAINTED | CELL_SHOULD_BE_PAINTED);
	}

	/* Check that the painted squares don't mess everything up */
	if (kuro_check_rule2 (kuro) == FALSE ||
	    kuro_check_rule3 (kuro) == FALSE) {
	    	g_free (accum);
		for (iter.x = 0; iter.x < kuro->board_size; iter.x++)
			g_slice_free1 (sizeof (gboolean) * (kuro->board_size + 2), horiz_accum[iter.x]);
		g_free (horiz_accum);

		kuro_generate_board (kuro, kuro->board_size, seed + 1);
		return;
	}

	/* Fill in the squares, leaving the painted ones blank,
	 * and making sure not to repeat any previous numbers. */
	for (iter.x = 0; iter.x < kuro->board_size; iter.x++) {
		/* Reset the vertical accumulator */
		for (iter.y = 1; iter.y < kuro->board_size + 2; iter.y++)
			accum[iter.y] = FALSE;

		i = 0;
		accum[0] = TRUE;
		total = kuro->board_size + 1;
		old_total = total;

		for (iter.y = 0; iter.y < kuro->board_size; iter.y++) {
			if ((kuro->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				while (accum[i] == TRUE || horiz_accum[iter.y][i] == TRUE) {
					if (horiz_accum[iter.y][i] == TRUE && accum[i] == FALSE)
						total--;

					if (total < 1) {
						g_free (accum);
						for (iter.x = 0; iter.x < kuro->board_size; iter.x++)
							g_slice_free1 (sizeof (gboolean) * (kuro->board_size + 2), horiz_accum[iter.x]);
						g_free (horiz_accum);

						kuro_generate_board (kuro, kuro->board_size, seed + 1); /* We're buggered */
						return;
					}

					i = rand () % (kuro->board_size + 1) + 1;
				}

				accum[i] = TRUE;
				horiz_accum[iter.y][i] = TRUE;
				total = old_total;
				total--;

				kuro->board[iter.x][iter.y].num = i;
			}
		}
	}

	g_free (accum);
	for (iter.x = 0; iter.x < kuro->board_size; iter.x++)
		g_slice_free1 (sizeof (gboolean) * (kuro->board_size + 2), horiz_accum[iter.x]);
	g_free (horiz_accum);

	/* Fill in the painted squares, making sure they duplicate a number
	 * already in the column/row. */
	for (iter.x = 0; iter.x < kuro->board_size; iter.x++) {
		for (iter.y = 0; iter.y < kuro->board_size; iter.y++) {
			if (kuro->board[iter.x][iter.y].status & CELL_PAINTED) {
				do {
					i = rand () % kuro->board_size;
					if (iter.x > iter.y)
						total = kuro->board[iter.x][i].num; /* Take a number from the row */
					else
						total = kuro->board[i][iter.y].num; /* Take a number from the column */
				} while (total == 0);

				kuro->board[iter.x][iter.y].num = total;
				kuro->board[iter.x][iter.y].status &= (~CELL_PAINTED & ~CELL_ERROR);
			}
		}
	}

	/* Update things */
	kuro_enable_events (kuro);
}
