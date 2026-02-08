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
#include "main.h"

#ifndef KURO_RULES_H
#define KURO_RULES_H

G_BEGIN_DECLS

gboolean kuro_check_rule1 (Kuro *kuro);
gboolean kuro_check_rule2 (Kuro *kuro);
gboolean kuro_check_rule3 (Kuro *kuro);
gboolean kuro_check_win (Kuro *kuro);

G_END_DECLS

#endif /* KURO_RULES_H */
