/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Kuro
 * Copyright (C) Philip Withnall 2007-2009 <philip@tecnocode.co.uk>
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

#include <gtk/gtk.h>
#include <glib.h>

#ifndef KURO_MAIN_H
#define KURO_MAIN_H

G_BEGIN_DECLS

#define DEFAULT_BOARD_SIZE 5
#define MAX_BOARD_SIZE 10

typedef struct {
	guchar x;
	guchar y;
} KuroVector;

typedef enum {
	UNDO_NEW_GAME,
	UNDO_PAINT,
	UNDO_TAG1,
	UNDO_TAG2,
	UNDO_TAGS /* = UNDO_TAG1 and UNDO_TAG2 */
} KuroUndoType;

typedef struct _KuroUndo KuroUndo;
struct _KuroUndo {
	KuroUndoType type;
	KuroVector cell;
	KuroUndo *undo;
	KuroUndo *redo;
};

typedef enum {
	CELL_PAINTED = 1 << 1,
	CELL_SHOULD_BE_PAINTED = 1 << 2,
	CELL_TAG1 = 1 << 3,
	CELL_TAG2 = 1 << 4,
	CELL_ERROR = 1 << 5
} KuroCellStatus;

typedef struct {
	GdkRGBA unpainted_bg;
	GdkRGBA painted_bg;
	GdkRGBA unpainted_border;
	GdkRGBA painted_border;
	GdkRGBA unpainted_text;
	GdkRGBA painted_text;
	GdkRGBA error_text;
} KuroTheme;

typedef struct {
	guchar num;
	guchar status;
} KuroCell;

#define KURO_TYPE_APPLICATION (kuro_application_get_type ())
G_DECLARE_FINAL_TYPE (KuroApplication, kuro_application, KURO, APPLICATION, GtkApplication)

struct _KuroApplication {
	GtkApplication parent;

	/* FIXME: This should all be merged into priv. */
	GtkWidget *window;
	GtkWidget *preferences_dialog;
	GtkWidget *board_theme_row;
	GtkWidget *board_size_row;
	GtkWidget *drawing_area;
	GSimpleAction *undo_action;
	GSimpleAction *redo_action;
	GSimpleAction *hint_action;

	gdouble drawing_area_width;
	gdouble drawing_area_height;

	gdouble drawing_area_x_offset;
	gdouble drawing_area_y_offset;

	PangoFontDescription *normal_font_desc;
	PangoFontDescription *painted_font_desc;

	guchar board_size;
	KuroCell **board;

	gboolean debug;
	gboolean processing_events;
	gboolean made_a_move;
	KuroUndo *undo_stack;

	guint hint_status;
	KuroVector hint_position;
	guint hint_timeout_id;

	guint timer_value; /* seconds into the game */
	GtkLabel *timer_label;
	guint timeout_id;

	gboolean cursor_active;
	KuroVector cursor_position;
	
	gboolean is_paused;
	GtkWidget *pause_overlay;
	GtkWidget *pause_button;

	const KuroTheme *theme;
	GSettings *settings;
};

KuroApplication *kuro_application_new (void) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

/* FIXME: Backwards compatibility. This should be phased out eventually. */
typedef KuroApplication Kuro;

void kuro_new_game (Kuro *kuro, guint board_size);
void kuro_clear_undo_stack (Kuro *kuro);
void kuro_set_board_size (Kuro *kuro, guint board_size);
void kuro_print_board (Kuro *kuro);
void kuro_free_board (Kuro *kuro);
void kuro_enable_events (Kuro *kuro);
void kuro_disable_events (Kuro *kuro);
void kuro_start_timer (Kuro *kuro);
void kuro_pause_timer (Kuro *kuro);
void kuro_reset_timer (Kuro *kuro);
void kuro_set_error_position (Kuro *kuro, KuroVector position);
void kuro_quit (Kuro *kuro);

G_END_DECLS

#endif /* KURO_MAIN_H */
