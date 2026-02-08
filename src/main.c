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

#include <adwaita.h>
#include <config.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <stdlib.h>

#include "generator.h"
#include "interface.h"
#include "main.h"

static void constructed(GObject *object);
static void get_property(GObject *object, guint property_id, GValue *value,
                         GParamSpec *pspec);
static void set_property(GObject *object, guint property_id,
                         const GValue *value, GParamSpec *pspec);

static void startup(GApplication *application);
static void activate(GApplication *application);

typedef struct {
  /* Command line parameters. */
  gboolean debug;
  guint seed;
} KuroApplicationPrivate;

typedef enum { PROP_DEBUG = 1, PROP_SEED } KuroProperty;

G_DEFINE_TYPE_WITH_PRIVATE(KuroApplication, kuro_application,
                           GTK_TYPE_APPLICATION)

static void shutdown(GApplication *application) {
  KuroApplication *self = KURO_APPLICATION(application);

  kuro_free_board(self);
  kuro_clear_undo_stack(self);
  g_free(self->undo_stack); /* Clear the new game element */

  if (self->normal_font_desc != NULL)
    pango_font_description_free(self->normal_font_desc);
  if (self->painted_font_desc != NULL)
    pango_font_description_free(self->painted_font_desc);

  if (self->settings)
    g_object_unref(self->settings);

  /* Chain up to the parent class */
  G_APPLICATION_CLASS(kuro_application_parent_class)->shutdown(application);
}

static void kuro_application_class_init(KuroApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GApplicationClass *gapplication_class = G_APPLICATION_CLASS(klass);

  gobject_class->constructed = constructed;
  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;

  gapplication_class->startup = startup;
  gapplication_class->shutdown = shutdown;
  gapplication_class->activate = activate;

  g_object_class_install_property(
      gobject_class, PROP_DEBUG,
      g_param_spec_boolean("debug", "Debugging Mode",
                           "Whether debugging mode is active.", FALSE,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_SEED,
      g_param_spec_uint("seed", "Generation Seed",
                        "Seed controlling generation of the board.", 0,
                        G_MAXUINT, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void kuro_application_init(KuroApplication *self) {
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(self);

  priv->debug = FALSE;
  priv->seed = 0;
}

static void constructed(GObject *object) {
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(KURO_APPLICATION(object));

  /* Set various properties up */
  g_application_set_application_id(G_APPLICATION(object), APPLICATION_ID);

  /* Localisation */
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);

  const GOptionEntry options[] = {
      {"debug", 0, 0, G_OPTION_ARG_NONE, &(priv->debug),
       N_("Enable debug mode"), NULL},
      /* Translators: This means to choose a number as the "seed" for random
         number generation used when creating a board */
      {"seed", 0, 0, G_OPTION_ARG_INT, &(priv->seed),
       N_("Seed the board generation"), NULL},
      {NULL}};

  g_application_add_main_option_entries(G_APPLICATION(object), options);
  g_application_set_option_context_parameter_string(G_APPLICATION(object),
                                                    _("- Play a game of Kuro"));

  g_set_application_name(_("Kuro"));
  g_set_prgname(APPLICATION_ID);

  /* Chain up to the parent class */
  G_OBJECT_CLASS(kuro_application_parent_class)->constructed(object);
}

static void get_property(GObject *object, guint property_id, GValue *value,
                         GParamSpec *pspec) {
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(KURO_APPLICATION(object));

  switch (property_id) {
  case PROP_DEBUG:
    g_value_set_boolean(value, priv->debug);
    break;
  case PROP_SEED:
    g_value_set_uint(value, priv->seed);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void set_property(GObject *object, guint property_id,
                         const GValue *value, GParamSpec *pspec) {
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(KURO_APPLICATION(object));

  switch (property_id) {
  case PROP_DEBUG:
    priv->debug = g_value_get_boolean(value);
    break;
  case PROP_SEED:
    priv->seed = g_value_get_uint(value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void debug_handler(const char *log_domain, GLogLevelFlags log_level,
                          const char *message, KuroApplication *self) {
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(self);

  /* Only display debug messages if we've been run with --debug */
  if (priv->debug == TRUE) {
    g_log_default_handler(log_domain, log_level, message, NULL);
  }
}

static void startup(GApplication *application) {
  /* Chain up. */
  G_APPLICATION_CLASS(kuro_application_parent_class)->startup(application);

  /* Initialize Libadwaita */
  adw_init();

  /* Debug log handling */
  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc)debug_handler,
                    application);
}

static void activate(GApplication *application) {
  KuroApplication *self = KURO_APPLICATION(application);
  KuroApplicationPrivate *priv;

  priv = kuro_application_get_instance_private(self);

  /* Create the interface. */
  if (self->window == NULL) {
    GdkRectangle geometry;
    KuroUndo *undo;
    gboolean window_maximized;
    gchar *size_str;

    /* Setup */
    self->debug = priv->debug;
    self->settings = g_settings_new(APPLICATION_ID);
    size_str = g_settings_get_string(self->settings, "board-size");
    self->board_size = g_ascii_strtoull(size_str, NULL, 10);
    g_free(size_str);

    if (self->board_size > MAX_BOARD_SIZE) {
      GVariant *default_size =
          g_settings_get_default_value(self->settings, "board-size");
      g_variant_get(default_size, "s", &size_str);
      g_variant_unref(default_size);
      self->board_size = g_ascii_strtoull(size_str, NULL, 10);
      g_free(size_str);
      g_assert(self->board_size <= MAX_BOARD_SIZE);
    }

    undo = g_new0(KuroUndo, 1);
    undo->type = UNDO_NEW_GAME;
    self->undo_stack = undo;

    /* Showtime! */
    kuro_create_interface(self);
    kuro_generate_board(self, self->board_size, priv->seed);

    /* Restore window position and size */
    window_maximized =
        g_settings_get_boolean(self->settings, "window-maximized");
    g_settings_get(self->settings, "window-position", "(ii)", &geometry.x,
                   &geometry.y);
    g_settings_get(self->settings, "window-size", "(ii)", &geometry.width,
                   &geometry.height);

    if (window_maximized) {
      gtk_window_maximize(GTK_WINDOW(self->window));
    } else {
      /* Note: gtk_window_move is removed in GTK4, position is managed by
       * compositor */

      if (geometry.width >= 0 && geometry.height >= 0) {
        gtk_window_set_default_size(GTK_WINDOW(self->window), geometry.width,
                                    geometry.height);
      }
    }

    gtk_window_set_application(GTK_WINDOW(self->window), GTK_APPLICATION(self));
    gtk_widget_set_visible(self->window, TRUE);
  }

  /* Bring it to the foreground */
  gtk_window_present(GTK_WINDOW(self->window));
}

KuroApplication *kuro_application_new(void) {
  return KURO_APPLICATION(g_object_new(KURO_TYPE_APPLICATION, NULL));
}

void kuro_new_game(Kuro *kuro, guint board_size) {
  kuro->made_a_move = FALSE;

  kuro_generate_board(kuro, board_size, 0);
  kuro_clear_undo_stack(kuro);
  gtk_widget_queue_draw(kuro->drawing_area);

  kuro_reset_timer(kuro);
  kuro_start_timer(kuro);

  /* Reset the cursor position */
  kuro->cursor_position.x = 0;
  kuro->cursor_position.y = 0;
}

void kuro_clear_undo_stack(Kuro *kuro) {
  /* Clear the undo stack */
  if (kuro->undo_stack != NULL) {
    while (kuro->undo_stack->redo != NULL)
      kuro->undo_stack = kuro->undo_stack->redo;

    while (kuro->undo_stack->undo != NULL) {
      kuro->undo_stack = kuro->undo_stack->undo;
      g_free(kuro->undo_stack->redo);
      if (kuro->undo_stack->type == UNDO_NEW_GAME)
        break;
    }

    /* Reset the "new game" item */
    kuro->undo_stack->undo = NULL;
    kuro->undo_stack->redo = NULL;
  }

  g_simple_action_set_enabled(kuro->undo_action, FALSE);
  g_simple_action_set_enabled(kuro->redo_action, FALSE);
}

static void board_size_dialog_response_cb(GObject *source, GAsyncResult *result,
                                          gpointer user_data) {
  guint board_size = GPOINTER_TO_UINT(user_data);
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(source);
  const char *response = adw_alert_dialog_choose_finish(dialog, result);
  Kuro *kuro = KURO_APPLICATION(g_object_get_data(G_OBJECT(dialog), "kuro"));

  if (g_strcmp0(response, "new-game") == 0) {
    /* Kill the current game and resize the board */
    kuro_new_game(kuro, board_size);
  }
}

void kuro_set_board_size(Kuro *kuro, guint board_size) {
  /* Ask the user if they want to stop the current game, if they're playing at
   * the moment */
  if (kuro->processing_events == TRUE && kuro->made_a_move == TRUE) {
    AdwAlertDialog *dialog;

    dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
        _("Stop Current Game?"), _("Do you want to stop the current game?")));

    adw_alert_dialog_add_responses(dialog, "keep-playing", _("Keep _Playing"),
                                   "new-game", _("_New Game"), NULL);

    adw_alert_dialog_set_response_appearance(dialog, "new-game",
                                             ADW_RESPONSE_SUGGESTED);
    adw_alert_dialog_set_default_response(dialog, "keep-playing");
    adw_alert_dialog_set_close_response(dialog, "keep-playing");

    g_object_set_data(G_OBJECT(dialog), "kuro", kuro);

    adw_alert_dialog_choose(dialog, GTK_WIDGET(kuro->window), NULL,
                            board_size_dialog_response_cb,
                            GUINT_TO_POINTER(board_size));
  } else {
    /* Kill the current game and resize the board */
    kuro_new_game(kuro, board_size);
  }
}

void kuro_print_board(Kuro *kuro) {
  if (kuro->debug) {
    KuroVector iter;

    for (iter.y = 0; iter.y < kuro->board_size; iter.y++) {
      for (iter.x = 0; iter.x < kuro->board_size; iter.x++) {
        if ((kuro->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE)
          g_printf("%u ", kuro->board[iter.x][iter.y].num);
        else
          g_printf("X ");
      }
      g_printf("\n");
    }
  }
}

void kuro_free_board(Kuro *kuro) {
  guint i;

  if (kuro->board == NULL)
    return;

  for (i = 0; i < kuro->board_size; i++)
    g_slice_free1(sizeof(KuroCell) * kuro->board_size, kuro->board[i]);
  g_free(kuro->board);
  kuro->board = NULL;
}

void kuro_enable_events(Kuro *kuro) {
  kuro->processing_events = TRUE;

  if (kuro->undo_stack->redo != NULL)
    g_simple_action_set_enabled(kuro->redo_action, TRUE);
  if (kuro->undo_stack->undo != NULL)
    g_simple_action_set_enabled(kuro->undo_action, TRUE);
  g_simple_action_set_enabled(kuro->hint_action, TRUE);

  kuro_start_timer(kuro);
}

void kuro_disable_events(Kuro *kuro) {
  kuro->processing_events = FALSE;
  g_simple_action_set_enabled(kuro->redo_action, FALSE);
  g_simple_action_set_enabled(kuro->undo_action, FALSE);
  g_simple_action_set_enabled(kuro->hint_action, FALSE);

  kuro_pause_timer(kuro);
}

static void set_timer_label(Kuro *kuro) {
  gchar *text = g_strdup_printf("%02u∶\xE2\x80\x8E%02u", kuro->timer_value / 60,
                                kuro->timer_value % 60);
  gtk_label_set_text(kuro->timer_label, text);
  g_free(text);
}

static gboolean update_timer_cb(Kuro *kuro) {
  kuro->timer_value++;
  set_timer_label(kuro);

  return TRUE;
}

void kuro_start_timer(Kuro *kuro) {
  // Remove any old timeout
  kuro_pause_timer(kuro);

  set_timer_label(kuro);
  kuro->timeout_id =
      g_timeout_add_seconds(1, (GSourceFunc)update_timer_cb, kuro);
}

void kuro_pause_timer(Kuro *kuro) {
  if (kuro->timeout_id > 0) {
    g_source_remove(kuro->timeout_id);
    kuro->timeout_id = 0;
  }
}

void kuro_reset_timer(Kuro *kuro) {
  kuro->timer_value = 0;
  set_timer_label(kuro);
}

void kuro_quit(Kuro *kuro) {
  static gboolean quitting = FALSE;

  if (quitting == TRUE)
    return;
  quitting = TRUE;

  kuro_free_board(kuro);
  kuro_clear_undo_stack(kuro);
  g_free(kuro->undo_stack); /* Clear the new game element */

  if (kuro->window != NULL)
    gtk_window_destroy(GTK_WINDOW(kuro->window));

  if (kuro->settings)
    g_object_unref(kuro->settings);

  g_application_quit(G_APPLICATION(kuro));
}

int main(int argc, char *argv[]) {
  KuroApplication *app;
  int status;

#if !GLIB_CHECK_VERSION(2, 35, 0)
  g_type_init();
#endif

  app = kuro_application_new();
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
