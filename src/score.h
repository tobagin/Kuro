/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Kuro
 * Copyright (C) Thiago Fernandes 2026 <thiago@example.com>
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

#ifndef KURO_SCORE_H
#define KURO_SCORE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _KuroApplication Kuro;

typedef struct {
  guint board_size;
  gchar *name;
  guint time;
} KuroScore;

void kuro_score_free(KuroScore *score);
GList *kuro_score_get_top_scores(Kuro *kuro, guint board_size);
gboolean kuro_score_is_high_score(Kuro *kuro, guint board_size, guint time);
void kuro_score_add(Kuro *kuro, guint board_size, const gchar *name,
                    guint time);

G_END_DECLS

#endif /* KURO_SCORE_H */
