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
#include <cairo/cairo.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "interface.h"
#include "main.h"
#include "rules.h"

#define NORMAL_FONT_SCALE 0.9
#define PAINTED_FONT_SCALE 0.6
#define TAG_OFFSET 0.75
#define TAG_RADIUS 0.25
#define HINT_FLASHES 6
#define HINT_DISABLED 0
#define HINT_INTERVAL 500
#define CURSOR_MARGIN 3

static void kuro_cancel_hinting(Kuro *kuro);
static void board_theme_change_cb(GSettings *settings, const gchar *key,
                                  gpointer user_data);

static const KuroTheme theme_kuro = {
    {0.141, 0.122, 0.192, 1.0}, /* unpainted_bg: #241f31 */
    {0.102, 0.102, 0.102, 1.0}, /* painted_bg: #1a1a1a */
    {0.239, 0.220, 0.275, 1.0}, /* unpainted_border: #3d3846 */
    {0.102, 0.102, 0.102, 1.0}, /* painted_border: #1a1a1a */
    {0.965, 0.961, 0.957, 1.0}, /* unpainted_text: #f6f5f4 */
    {0.427, 0.427, 0.427, 1.0}, /* painted_text: #6d6d6d */
    {1.0, 0.482, 0.388, 1.0}    /* error_text: #ff7b63 */
};

static const KuroTheme theme_classic = {
    {0.929, 0.929, 0.929, 1.0}, /* unpainted_bg: #ededed */
    {0.957, 0.957, 0.957, 1.0}, /* painted_bg: #f4f4f4 */
    {0.180, 0.204, 0.212, 1.0}, /* unpainted_border: #2e3436 */
    {0.730, 0.737, 0.722, 1.0}, /* painted_border: #babcb8 */
    {0.180, 0.204, 0.212, 1.0}, /* unpainted_text: #2e3436 */
    {0.655, 0.671, 0.655, 1.0}, /* painted_text: #a7aba7 */
    {0.937, 0.161, 0.161, 1.0}  /* error_text: #ef2929 */
};

/* Declarations for GtkBuilder */
void kuro_draw_cb(GtkDrawingArea *drawing_area, cairo_t *cr, int width,
                  int height, gpointer user_data);
static void kuro_click_released_cb(GtkGestureClick *gesture, int n_press,
                                   double x, double y, gpointer user_data);
static gboolean kuro_key_pressed_cb(GtkEventControllerKey *controller,
                                    guint keyval, guint keycode,
                                    GdkModifierType state, gpointer user_data);
gboolean kuro_close_request_cb(GtkWindow *window, Kuro *kuro);
static void new_game_cb(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data);
static void hint_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data);
static void quit_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data);
static void undo_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data);
static void redo_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data);
static void help_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data);
static void about_cb(GSimpleAction *action, GVariant *parameter,
                     gpointer user_data);
static void pause_cb(GSimpleAction *action, GVariant *parameter,
                     gpointer user_data);
static void show_help_overlay_cb(GSimpleAction *action, GVariant *parameter,
                                 gpointer user_data);
static void board_size_cb(GSimpleAction *action, GVariant *parameter,
                          gpointer user_data);
static void board_theme_cb(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data);
static void board_size_change_cb(GSettings *settings, const gchar *key,
                                 gpointer user_data);
static void style_manager_dark_changed_cb(AdwStyleManager *style_manager,
                                          GParamSpec *pspec,
                                          gpointer user_data);

static void high_scores_cb(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data);

static GActionEntry app_entries[] = {
    {"new-game", new_game_cb, NULL, NULL, NULL},
    {"high-scores", high_scores_cb, NULL, NULL, NULL},
    {"about", about_cb, NULL, NULL, NULL},
    {"help", help_cb, NULL, NULL, NULL},
    {"quit", quit_cb, NULL, NULL, NULL},
    {"board-size", board_size_cb, "s", "'5'", NULL},
    {"board-theme", board_theme_cb, "s", "'kuro'", NULL},
};

static GActionEntry win_entries[] = {
    {"hint", hint_cb, NULL, NULL, NULL},
    {"undo", undo_cb, NULL, NULL, NULL},
    {"redo", redo_cb, NULL, NULL, NULL},
    {"pause", pause_cb, NULL, "false", NULL},
    {"show-help-overlay", show_help_overlay_cb, NULL, NULL, NULL},
};

static void on_new_high_score_done(GtkWidget *button, gpointer user_data) {
  AdwDialog *dialog = ADW_DIALOG(user_data);
  Kuro *kuro = g_object_get_data(G_OBJECT(dialog), "kuro");
  GtkWidget *entry = g_object_get_data(G_OBJECT(dialog), "entry");
  const char *name = gtk_editable_get_text(GTK_EDITABLE(entry));

  if (name && *name) {
    kuro_score_add(kuro, kuro->board_size, name, kuro->timer_value);
  }

  adw_dialog_close(dialog);
  /* Show win dialog after high score is handled */
  kuro_show_win_dialog(kuro);
}

static void win_dialog_response_cb(AdwAlertDialog *dialog, const char *response,
                                   gpointer user_data) {
  Kuro *kuro = (Kuro *)user_data;

  if (g_strcmp0(response, "quit") == 0) {
    kuro_quit(kuro);
  } else if (g_strcmp0(response, "play-again") == 0) {
    kuro_new_game(kuro, kuro->board_size);
  }
}

void kuro_show_win_dialog(Kuro *kuro) {
  AdwAlertDialog *dialog;
  gchar *message;

  message = g_strdup_printf(_("You’ve won in a time of %02u:%02u!"),
                            kuro->timer_value / 60, kuro->timer_value % 60);
  dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(_("You Won!"), message));
  g_free(message);

  adw_alert_dialog_add_responses(dialog, "quit", _("_Quit"), "play-again",
                                 _("_Play Again"), NULL);

  adw_alert_dialog_set_response_appearance(dialog, "play-again",
                                           ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "play-again");
  adw_alert_dialog_set_close_response(dialog, "play-again");

  g_signal_connect(dialog, "response", G_CALLBACK(win_dialog_response_cb),
                   kuro);

  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(kuro->window));
}

void kuro_show_new_high_score_dialog(Kuro *kuro) {
  AdwDialog *dialog;
  GtkWidget *entry;
  GtkWidget *toolbar_view;
  GtkWidget *header_bar;
  GtkWidget *box;

  dialog = adw_dialog_new();
  g_object_set_data(G_OBJECT(dialog), "kuro", kuro);

  toolbar_view = adw_toolbar_view_new();
  header_bar = adw_header_bar_new();
  adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(header_bar), FALSE);

  char *size_str = g_strdup_printf(_("Grid Size: %d × %d"), kuro->board_size,
                                   kuro->board_size);
  GtkWidget *title_widget =
      adw_window_title_new(_("Congratulations!"), size_str);
  g_free(size_str);

  adw_header_bar_set_title_widget(ADW_HEADER_BAR(header_bar), title_widget);

  GtkWidget *done_btn = gtk_button_new_with_label(_("Done"));
  gtk_widget_add_css_class(done_btn, "suggested-action");
  adw_header_bar_pack_end(ADW_HEADER_BAR(header_bar), done_btn);

  g_signal_connect(done_btn, "clicked", G_CALLBACK(on_new_high_score_done),
                   dialog);

  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header_bar);

  gtk_widget_set_size_request(toolbar_view, 400, 520);

  /* Content */
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *msg_lbl = gtk_label_new(_("Your score has made the top ten."));
  gtk_widget_set_margin_top(msg_lbl, 24);
  gtk_widget_set_margin_bottom(msg_lbl, 24);
  gtk_box_append(GTK_BOX(box), msg_lbl);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append(GTK_BOX(box), separator);

  /* List of scores + New Score */
  GList *scores = kuro_score_get_top_scores(kuro, kuro->board_size);
  /* ... logic ... */

  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(list_box, "boxed-list");
  gtk_widget_set_margin_start(list_box, 24);
  gtk_widget_set_margin_end(list_box, 24);
  gtk_widget_set_margin_bottom(list_box, 24);

  /* Header Row */
  GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_top(header_row, 12);
  gtk_widget_set_margin_bottom(header_row, 6);
  gtk_widget_set_margin_start(header_row,
                              36); /* Align with list content padding roughly */

  GtkWidget *lbl_rank = gtk_label_new(_("Rank"));
  gtk_widget_set_size_request(lbl_rank, 40, -1);
  gtk_box_append(GTK_BOX(header_row), lbl_rank);

  GtkWidget *lbl_score = gtk_label_new(_("Score"));
  gtk_widget_set_size_request(lbl_score, 60, -1);
  gtk_box_append(GTK_BOX(header_row), lbl_score);

  GtkWidget *lbl_player = gtk_label_new(_("Player"));
  gtk_widget_set_hexpand(lbl_player, TRUE);
  gtk_widget_set_halign(lbl_player, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(header_row), lbl_player);

  gtk_box_append(GTK_BOX(box), header_row);
  gtk_box_append(GTK_BOX(box), list_box);

  /* Merge logic */
  KuroScore *new_s = g_new0(KuroScore, 1);
  new_s->board_size = kuro->board_size;
  new_s->time = kuro->timer_value;
  new_s->name = NULL;

  GList *l;
  gboolean inserted = FALSE;
  GList *display_list = NULL;

  for (l = scores; l != NULL; l = l->next) {
    KuroScore *s = (KuroScore *)l->data;
    if (!inserted && new_s->time < s->time) {
      display_list = g_list_append(display_list, new_s);
      inserted = TRUE;
    }
    display_list = g_list_append(display_list, s);
  }
  if (!inserted && g_list_length(display_list) < 10) {
    display_list = g_list_append(display_list, new_s);
  }

  while (g_list_length(display_list) > 10) {
    GList *last = g_list_last(display_list);
    display_list = g_list_delete_link(display_list, last);
  }

  /* Render */
  int rank = 1;
  entry = NULL;

  for (l = display_list; l != NULL; l = l->next) {
    KuroScore *s = (KuroScore *)l->data;
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(row_box, 12);
    gtk_widget_set_margin_bottom(row_box, 12);

    char *rank_str = g_strdup_printf("%d", rank++);
    GtkWidget *r_lbl = gtk_label_new(rank_str);
    g_free(rank_str);
    gtk_widget_set_size_request(r_lbl, 40, -1);
    gtk_box_append(GTK_BOX(row_box), r_lbl);

    char *time_str;
    if (s->time < 3600)
      time_str = g_strdup_printf("%02u:%02u", s->time / 60, s->time % 60);
    else
      time_str = g_strdup_printf("%u:%02u:%02u", s->time / 3600,
                                 (s->time % 3600) / 60, s->time % 60);

    GtkWidget *t_lbl = gtk_label_new(time_str);
    g_free(time_str);
    gtk_widget_set_size_request(t_lbl, 60, -1);
    gtk_box_append(GTK_BOX(row_box), t_lbl);

    if (s == new_s) {
      /* Input field */
      entry = gtk_entry_new();
      gtk_widget_set_hexpand(entry, TRUE);
      gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Your Name"));
      gtk_editable_set_text(GTK_EDITABLE(entry), g_get_real_name());
      gtk_box_append(GTK_BOX(row_box), entry);
    } else {
      /* Name label */
      GtkWidget *n_lbl = gtk_label_new(s->name);
      gtk_widget_set_hexpand(n_lbl, TRUE);
      gtk_widget_set_halign(n_lbl, GTK_ALIGN_START);
      gtk_box_append(GTK_BOX(row_box), n_lbl);
    }

    gtk_list_box_append(GTK_LIST_BOX(list_box), row_box);
  }

  g_list_free(display_list);
  g_list_free_full(scores, (GDestroyNotify)kuro_score_free);
  g_free(new_s);

  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), box);
  adw_dialog_set_child(dialog, toolbar_view);

  if (entry) {
    g_object_set_data(G_OBJECT(dialog), "entry", entry);
  }

  adw_dialog_present(dialog, GTK_WIDGET(kuro->window));
}

void kuro_show_high_scores_dialog(Kuro *kuro) {
  AdwDialog *dialog;
  GtkWidget *toolbar_view;
  GtkWidget *header_bar;
  GtkWidget *box;

  dialog = adw_dialog_new();

  toolbar_view = adw_toolbar_view_new();
  header_bar = adw_header_bar_new();

  char *size_str = g_strdup_printf(_("Grid Size: %d × %d"), kuro->board_size,
                                   kuro->board_size);
  GtkWidget *title_widget = adw_window_title_new(_("High Scores"), size_str);
  g_free(size_str);

  adw_header_bar_set_title_widget(ADW_HEADER_BAR(header_bar), title_widget);

  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header_bar);

  gtk_widget_set_size_request(toolbar_view, 400, 520);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append(GTK_BOX(box), separator);

  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(list_box, "boxed-list");
  gtk_widget_set_margin_start(list_box, 24);
  gtk_widget_set_margin_end(list_box, 24);
  gtk_widget_set_margin_bottom(list_box, 24);

  /* Header Row */
  GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_top(header_row, 12);
  gtk_widget_set_margin_bottom(header_row, 6);
  gtk_widget_set_margin_start(header_row, 36);

  GtkWidget *lbl_rank = gtk_label_new(_("Rank"));
  gtk_widget_set_size_request(lbl_rank, 40, -1);
  gtk_box_append(GTK_BOX(header_row), lbl_rank);

  GtkWidget *lbl_score = gtk_label_new(_("Score"));
  gtk_widget_set_size_request(lbl_score, 60, -1);
  gtk_box_append(GTK_BOX(header_row), lbl_score);

  GtkWidget *lbl_player = gtk_label_new(_("Player"));
  gtk_widget_set_hexpand(lbl_player, TRUE);
  gtk_widget_set_halign(lbl_player, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(header_row), lbl_player);

  gtk_box_append(GTK_BOX(box), header_row);
  gtk_box_append(GTK_BOX(box), list_box);

  GList *scores = kuro_score_get_top_scores(kuro, kuro->board_size);
  int rank = 1;
  GList *l;

  for (l = scores; l != NULL; l = l->next) {
    KuroScore *s = (KuroScore *)l->data;
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(row_box, 12);
    gtk_widget_set_margin_bottom(row_box, 12);

    char *rank_str = g_strdup_printf("%d", rank++);
    GtkWidget *r_lbl = gtk_label_new(rank_str);
    g_free(rank_str);
    gtk_widget_set_size_request(r_lbl, 40, -1);
    gtk_box_append(GTK_BOX(row_box), r_lbl);

    char *time_str;
    if (s->time < 3600)
      time_str = g_strdup_printf("%02u:%02u", s->time / 60, s->time % 60);
    else
      time_str = g_strdup_printf("%u:%02u:%02u", s->time / 3600,
                                 (s->time % 3600) / 60, s->time % 60);

    GtkWidget *t_lbl = gtk_label_new(time_str);
    g_free(time_str);
    gtk_widget_set_size_request(t_lbl, 60, -1);
    gtk_box_append(GTK_BOX(row_box), t_lbl);

    GtkWidget *n_lbl = gtk_label_new(s->name);
    gtk_widget_set_hexpand(n_lbl, TRUE);
    gtk_widget_set_halign(n_lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(row_box), n_lbl);

    gtk_list_box_append(GTK_LIST_BOX(list_box), row_box);
  }
  g_list_free_full(scores, (GDestroyNotify)kuro_score_free);

  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), box);
  adw_dialog_set_child(dialog, toolbar_view);
  adw_dialog_present(dialog, GTK_WIDGET(kuro->window));
}

static void high_scores_cb(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  Kuro *kuro = (Kuro *)user_data;
  kuro_show_high_scores_dialog(kuro);
}

static void kuro_window_unmap_cb(GtkWidget *window, gpointer user_data) {
  gboolean window_maximized;
  GdkRectangle geometry;
  KuroApplication *kuro;

  kuro = KURO_APPLICATION(user_data);

  window_maximized = gtk_window_is_maximized(GTK_WINDOW(window));
  g_settings_set_boolean(kuro->settings, "window-maximized", window_maximized);

  if (window_maximized)
    return;

  /* Note: gtk_window_get_position and gtk_window_get_size removed in GTK4 */
  /* Window size/position is now managed by the compositor */
  gtk_window_get_default_size(GTK_WINDOW(window), &geometry.width,
                              &geometry.height);

  g_settings_set(kuro->settings, "window-size", "(ii)", geometry.width,
                 geometry.height);
}

GtkWidget *kuro_create_interface(Kuro *kuro) {
  GtkBuilder *builder;
  GtkCssProvider *css_provider;
  GAction *action;

  gchar *resource_path =
      g_strconcat("/", g_strdelimit(g_strdup(APPLICATION_ID), ".", '/'),
                  "/ui/kuro.ui", NULL);
  builder = gtk_builder_new_from_resource(resource_path);
  g_free(resource_path);

  gtk_builder_set_translation_domain(builder, PACKAGE);

  /* Setup the main window */
  kuro->window =
      GTK_WIDGET(gtk_builder_get_object(builder, "kuro_main_window"));
  kuro->drawing_area =
      GTK_WIDGET(gtk_builder_get_object(builder, "kuro_drawing_area"));
  kuro->timer_label = GTK_LABEL(gtk_builder_get_object(builder, "kuro_timer"));
  kuro->timer_label = GTK_LABEL(gtk_builder_get_object(builder, "kuro_timer"));
  kuro->pause_overlay =
      GTK_WIDGET(gtk_builder_get_object(builder, "pause_overlay"));
  kuro->pause_button =
      GTK_WIDGET(gtk_builder_get_object(builder, "pause_button"));

  g_signal_connect(kuro->window, "unmap", G_CALLBACK(kuro_window_unmap_cb),
                   kuro);

  g_object_unref(builder);

  /* Set up actions */
  g_action_map_add_action_entries(G_ACTION_MAP(kuro), app_entries,
                                  G_N_ELEMENTS(app_entries), kuro);
  g_action_map_add_action_entries(G_ACTION_MAP(kuro->window), win_entries,
                                  G_N_ELEMENTS(win_entries), kuro);

  g_signal_connect(kuro->settings, "changed::board-size",
                   G_CALLBACK(board_size_change_cb), kuro);
  g_signal_connect(kuro->settings, "changed::board-theme",
                   G_CALLBACK(board_theme_change_cb), kuro);

  /* Listen for system color scheme changes for auto theme */
  AdwStyleManager *style_manager = adw_style_manager_get_default();
  g_signal_connect(style_manager, "notify::dark",
                   G_CALLBACK(style_manager_dark_changed_cb), kuro);

  /* Initialize action state from settings */
  GVariant *state;
  action = g_action_map_lookup_action(G_ACTION_MAP(kuro), "board-size");
  state = g_settings_get_value(kuro->settings, "board-size");
  g_simple_action_set_state(G_SIMPLE_ACTION(action), state);
  g_variant_unref(state);

  action = g_action_map_lookup_action(G_ACTION_MAP(kuro), "board-theme");
  state = g_settings_get_value(kuro->settings, "board-theme");
  g_simple_action_set_state(G_SIMPLE_ACTION(action), state);
  g_variant_unref(state);

  /* Initial callback trigger */
  board_theme_change_cb(kuro->settings, "board-theme", kuro);

  kuro->undo_action = G_SIMPLE_ACTION(
      g_action_map_lookup_action(G_ACTION_MAP(kuro->window), "undo"));
  kuro->redo_action = G_SIMPLE_ACTION(
      g_action_map_lookup_action(G_ACTION_MAP(kuro->window), "redo"));
  kuro->hint_action = G_SIMPLE_ACTION(
      g_action_map_lookup_action(G_ACTION_MAP(kuro->window), "hint"));

  const gchar *vaccels_help[] = {"F1", NULL};
  const gchar *vaccels_hint[] = {"<Primary>h", NULL};
  const gchar *vaccels_new[] = {"<Primary>n", NULL};
  const gchar *vaccels_quit[] = {"<Primary>q", "<Primary>w", NULL};
  const gchar *vaccels_redo[] = {"<Primary><Shift>z", NULL};
  const gchar *vaccels_undo[] = {"<Primary>z", NULL};
  const gchar *vaccels_shortcuts[] = {"<Primary>question", NULL};
  const gchar *vaccels_about[] = {"<Primary><Shift>a", NULL};
  const gchar *vaccels_pause[] = {"<Primary>p", NULL};

  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "app.help",
                                        vaccels_help);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "win.hint",
                                        vaccels_hint);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "app.new-game",
                                        vaccels_new);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "app.quit",
                                        vaccels_quit);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "win.redo",
                                        vaccels_redo);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "win.undo",
                                        vaccels_undo);
  gtk_application_set_accels_for_action(
      GTK_APPLICATION(kuro), "win.show-help-overlay", vaccels_shortcuts);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "app.about",
                                        vaccels_about);
  gtk_application_set_accels_for_action(GTK_APPLICATION(kuro), "win.pause",
                                        vaccels_pause);

  /* Set up font descriptions for the drawing area */
  /* Note: In GTK4, we create default font descriptions instead of querying
   * style context */
  kuro->normal_font_desc = pango_font_description_from_string("Sans 12");
  kuro->painted_font_desc = pango_font_description_copy(kuro->normal_font_desc);

  /* Load CSS for the drawing area */
  css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource(
      css_provider,
      g_strconcat("/", g_strdelimit(g_strdup(APPLICATION_ID), ".", '/'),
                  "/ui/kuro.css", NULL));
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  /* Reset the timer */
  kuro_reset_timer(kuro);

  /* Disable undo/redo until a cell has been clicked. */
  g_simple_action_set_enabled(kuro->undo_action, FALSE);
  g_simple_action_set_enabled(kuro->redo_action, FALSE);

  /* Set up drawing callback */
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(kuro->drawing_area),
                                 kuro_draw_cb, kuro, NULL);

  /* Set up mouse input */
  GtkGesture *click_gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture),
                                GDK_BUTTON_PRIMARY);
  g_signal_connect(click_gesture, "released",
                   G_CALLBACK(kuro_click_released_cb), kuro);
  gtk_widget_add_controller(kuro->drawing_area,
                            GTK_EVENT_CONTROLLER(click_gesture));

  /* Set up keyboard input */
  GtkEventController *key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed",
                   G_CALLBACK(kuro_key_pressed_cb), kuro);
  gtk_widget_add_controller(kuro->drawing_area, key_controller);

  /* Cursor is initially not active as playing with the mouse is more common */
  kuro->cursor_active = FALSE;

  return kuro->window;
}

#define BORDER_LEFT 2.0

/* Generate the text for a given cell, potentially localised to the current
 * locale. */
static const gchar *localise_cell_digit(Kuro *kuro, const KuroVector *pos) {
  guchar value = kuro->board[pos->x][pos->y].num;

  G_STATIC_ASSERT(MAX_BOARD_SIZE < 11);

  switch (value) {
  /* Translators: This is a digit rendered in a cell on the game board.
   * Translate it to your locale’s number system if you wish the game
   * board to be rendered in those digits. Otherwise, leave the digits as
   * Arabic numerals. */
  case 1:
    return C_("Board cell", "1");
  case 2:
    return C_("Board cell", "2");
  case 3:
    return C_("Board cell", "3");
  case 4:
    return C_("Board cell", "4");
  case 5:
    return C_("Board cell", "5");
  case 6:
    return C_("Board cell", "6");
  case 7:
    return C_("Board cell", "7");
  case 8:
    return C_("Board cell", "8");
  case 9:
    return C_("Board cell", "9");
  case 10:
    return C_("Board cell", "10");
  case 11:
    return C_("Board cell", "11");
  default:
    g_assert_not_reached();
  }
}

static void draw_cell(Kuro *kuro, cairo_t *cr, gdouble cell_size, gdouble x_pos,
                      gdouble y_pos, KuroVector iter) {
  const gchar *text;
  PangoLayout *layout;
  gint text_width, text_height;
  gboolean painted = FALSE;
  PangoFontDescription *font_desc;
  GdkRGBA colour = {0.0, 0.0, 0.0, 1.0};

  if (kuro->board[iter.x][iter.y].status & CELL_PAINTED) {
    painted = TRUE;
  }

  /* Draw the fill */
  if (painted) {
    colour = kuro->theme->painted_bg;
  } else {
    colour = kuro->theme->unpainted_bg;
  }

  gdk_cairo_set_source_rgba(cr, &colour);
  cairo_rectangle(cr, x_pos, y_pos, cell_size, cell_size);
  cairo_fill(cr);

  /* If the cell is tagged, draw the tag dots */
  if (kuro->board[iter.x][iter.y].status & CELL_TAG1) {
    colour = (GdkRGBA){0.447, 0.624, 0.812, painted ? 0.7 : 1.0}; /* #729fcf */
    gdk_cairo_set_source_rgba(cr, &colour);

    cairo_move_to(cr, x_pos, y_pos + TAG_OFFSET);
    cairo_line_to(cr, x_pos, y_pos);
    cairo_line_to(cr, x_pos + TAG_OFFSET, y_pos);
    cairo_arc(cr, x_pos + TAG_OFFSET, y_pos + TAG_OFFSET,
              TAG_RADIUS * cell_size, 0.0, 0.5 * M_PI);
    cairo_fill(cr);
  }

  if (kuro->board[iter.x][iter.y].status & CELL_TAG2) {
    colour = (GdkRGBA){0.541, 0.886, 0.204, painted ? 0.7 : 1.0}; /* #8ae234 */
    gdk_cairo_set_source_rgba(cr, &colour);

    cairo_move_to(cr, x_pos + cell_size - TAG_OFFSET, y_pos);
    cairo_line_to(cr, x_pos + cell_size, y_pos);
    cairo_line_to(cr, x_pos + cell_size, y_pos + TAG_OFFSET);
    cairo_arc(cr, x_pos + cell_size - TAG_OFFSET, y_pos + TAG_OFFSET,
              TAG_RADIUS * cell_size, 0.5 * M_PI, 1.0 * M_PI);
    cairo_fill(cr);
  }

  /* Draw the border */
  if (painted) {
    colour = kuro->theme->painted_border;
  } else {
    colour = kuro->theme->unpainted_border;
  }
  gdk_cairo_set_source_rgba(cr, &colour);
  cairo_set_line_width(cr, BORDER_LEFT);
  cairo_rectangle(cr, x_pos, y_pos, cell_size, cell_size);
  cairo_stroke(cr);

  /* Draw the text */
  text = localise_cell_digit(kuro, &iter);
  layout = pango_cairo_create_layout(cr);

  pango_layout_set_text(layout, text, -1);

  font_desc =
      (painted == TRUE) ? kuro->painted_font_desc : kuro->normal_font_desc;

  if (kuro->board[iter.x][iter.y].status & CELL_ERROR) {
    colour = kuro->theme->error_text;
    gdk_cairo_set_source_rgba(cr, &colour);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
  } else if (painted) {
    colour = kuro->theme->painted_text;
    gdk_cairo_set_source_rgba(cr, &colour);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
  } else {
    g_assert(!painted);
    colour = kuro->theme->unpainted_text;
    gdk_cairo_set_source_rgba(cr, &colour);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
  }

  pango_layout_set_font_description(layout, font_desc);

  pango_layout_get_pixel_size(layout, &text_width, &text_height);
  cairo_move_to(cr, x_pos + (cell_size - text_width) / 2,
                y_pos + (cell_size - text_height) / 2);

  /* Only draw text if not paused */
  if (!kuro->is_paused) {
    pango_cairo_show_layout(cr, layout);
  }

  g_object_unref(layout);

  if (kuro->cursor_active && kuro->cursor_position.x == iter.x &&
      kuro->cursor_position.y == iter.y &&
      gtk_widget_is_focus(kuro->drawing_area)) {
    /* Draw the cursor */
    colour = (GdkRGBA){0.208, 0.518, 0.894, 1.0}; /* #3584e4 */
    gdk_cairo_set_source_rgba(cr, &colour);
    cairo_set_line_width(cr, BORDER_LEFT);
    cairo_rectangle(cr, x_pos + CURSOR_MARGIN, y_pos + CURSOR_MARGIN,
                    cell_size - (2 * CURSOR_MARGIN),
                    cell_size - (2 * CURSOR_MARGIN));
    cairo_stroke(cr);
  }
}

void kuro_draw_cb(GtkDrawingArea *drawing_area, cairo_t *cr, int width,
                  int height, gpointer user_data) {
  Kuro *kuro = (Kuro *)user_data;

  if (kuro->is_paused) {
    /* When paused, we still draw the board but hide the numbers */
    /* Continue drawing... */
  }

  gint area_width = width;
  gint area_height = height;
  KuroVector iter;
  guint board_width, board_height;
  gdouble cell_size;
  gdouble x_pos, y_pos;

  /* Clamp the width/height to the minimum */
  if (area_height < area_width) {
    board_width = area_height;
    board_height = area_height;
  } else {
    board_width = area_width;
    board_height = area_width;
  }

  board_width -= BORDER_LEFT;
  board_height -= BORDER_LEFT;

  /* Work out the cell size and scale all text accordingly */
  cell_size = (gdouble)board_width / (gdouble)kuro->board_size;
  pango_font_description_set_absolute_size(kuro->normal_font_desc,
                                           cell_size * NORMAL_FONT_SCALE * 0.8 *
                                               PANGO_SCALE);
  pango_font_description_set_absolute_size(kuro->painted_font_desc,
                                           cell_size * PAINTED_FONT_SCALE *
                                               0.8 * PANGO_SCALE);

  /* Centre the board */
  kuro->drawing_area_x_offset = (area_width - board_width) / 2.0;
  kuro->drawing_area_y_offset = (area_height - board_height) / 2.0;
  cairo_translate(cr, kuro->drawing_area_x_offset, kuro->drawing_area_y_offset);

  /* Draw the unpainted cells first. */
  for (iter.x = 0, x_pos = 0; iter.x < kuro->board_size;
       iter.x++, x_pos += cell_size) { /* columns (X) */
    for (iter.y = 0, y_pos = 0; iter.y < kuro->board_size;
         iter.y++, y_pos += cell_size) { /* rows (Y) */
      if (!(kuro->board[iter.x][iter.y].status & CELL_PAINTED)) {
        draw_cell(kuro, cr, cell_size, x_pos, y_pos, iter);
      }
    }
  }

  /* Next draw the painted cells (so that their borders are painted over those
   * of the unpainted cells).. */
  for (iter.x = 0, x_pos = 0; iter.x < kuro->board_size;
       iter.x++, x_pos += cell_size) { /* columns (X) */
    for (iter.y = 0, y_pos = 0; iter.y < kuro->board_size;
         iter.y++, y_pos += cell_size) { /* rows (Y) */
      if (kuro->board[iter.x][iter.y].status & CELL_PAINTED) {
        draw_cell(kuro, cr, cell_size, x_pos, y_pos, iter);
      }
    }
  }

  /* Draw a hint if applicable */
  if (kuro->hint_status % 2 == 1) {
    gdouble line_width = BORDER_LEFT * 2.5;
    GdkRGBA colour = {1.0, 0.0, 0.0, 1.0}; /* red */
    gdk_cairo_set_source_rgba(cr, &colour);
    cairo_set_line_width(cr, line_width);
    cairo_rectangle(cr, kuro->hint_position.x * cell_size + line_width / 2,
                    kuro->hint_position.y * cell_size + line_width / 2,
                    cell_size - line_width, cell_size - line_width);
    cairo_stroke(cr);
  }
}

static void kuro_update_cell_state(Kuro *kuro, KuroVector pos, gboolean tag1,
                                   gboolean tag2) {
  KuroUndo *undo;
  gboolean recheck = FALSE;

  /* Update the undo stack */
  undo = g_new(KuroUndo, 1);
  undo->cell = pos;
  undo->undo = kuro->undo_stack;
  undo->redo = NULL;

  if (tag1 && tag2) {
    /* Update both tags' state */
    kuro->board[pos.x][pos.y].status ^= CELL_TAG1;
    kuro->board[pos.x][pos.y].status ^= CELL_TAG2;
    undo->type = UNDO_TAGS;
  } else if (tag1) {
    /* Update tag 1's state */
    kuro->board[pos.x][pos.y].status ^= CELL_TAG1;
    undo->type = UNDO_TAG1;
  } else if (tag2) {
    /* Update tag 2's state */
    kuro->board[pos.x][pos.y].status ^= CELL_TAG2;
    undo->type = UNDO_TAG2;
  } else {
    /* Update the paint overlay */
    kuro->board[pos.x][pos.y].status ^= CELL_PAINTED;
    undo->type = UNDO_PAINT;
    recheck = TRUE;
  }

  kuro->made_a_move = TRUE;

  if (kuro->undo_stack != NULL) {
    KuroUndo *i, *next = NULL;

    /* Free the redo stack after this point. */
    for (i = kuro->undo_stack->redo; i != NULL; i = next) {
      next = i->redo;
      g_free(i);
    }

    kuro->undo_stack->redo = undo;
  }
  kuro->undo_stack = undo;
  g_simple_action_set_enabled(kuro->undo_action, TRUE);
  g_simple_action_set_enabled(kuro->redo_action, FALSE);

  /* Stop any current hints */
  kuro_cancel_hinting(kuro);

  /* Redraw */
  gtk_widget_queue_draw(kuro->drawing_area);

  /* Check to see if the player's won */
  if (recheck == TRUE)
    kuro_check_win(kuro);
}

static void kuro_click_released_cb(GtkGestureClick *gesture, int n_press,
                                   double x, double y, gpointer user_data) {
  Kuro *kuro = (Kuro *)user_data;
  gint width, height;
  gdouble cell_size;
  KuroVector pos;
  GdkModifierType state;

  if (kuro->processing_events == FALSE)
    return;

  width = gtk_widget_get_width(kuro->drawing_area);
  height = gtk_widget_get_height(kuro->drawing_area);

  /* Clamp the width/height to the minimum */
  if (height < width)
    width = height;

  cell_size = (gdouble)width / (gdouble)kuro->board_size;

  /* Determine the cell in which the button was released */
  pos.x = (guchar)((x - kuro->drawing_area_x_offset) / cell_size);
  pos.y = (guchar)((y - kuro->drawing_area_y_offset) / cell_size);

  if (pos.x >= kuro->board_size || pos.y >= kuro->board_size)
    return;

  /* Move the cursor to the clicked cell and deactivate it
   * (assuming player will use the mouse for the next move) */
  kuro->cursor_position.x = pos.x;
  kuro->cursor_position.y = pos.y;
  kuro->cursor_active = FALSE;

  /* Grab focus for keyboard navigation */
  gtk_widget_grab_focus(kuro->drawing_area);

  state = gtk_event_controller_get_current_event_state(
      GTK_EVENT_CONTROLLER(gesture));

  kuro_update_cell_state(kuro, pos, state & GDK_SHIFT_MASK,
                         state & GDK_CONTROL_MASK);
}

static gboolean kuro_key_pressed_cb(GtkEventControllerKey *controller,
                                    guint keyval, guint keycode,
                                    GdkModifierType state, gpointer user_data) {
  Kuro *kuro = (Kuro *)user_data;
  gboolean did_something = TRUE;
  gint dx = 0, dy = 0;

  if (kuro->processing_events == FALSE)
    return FALSE;

  switch (keyval) {
  case GDK_KEY_Left:
  case GDK_KEY_h:
  case GDK_KEY_a:
    dx = -1;
    break;
  case GDK_KEY_Right:
  case GDK_KEY_l:
  case GDK_KEY_d:
    dx = 1;
    break;
  case GDK_KEY_Up:
  case GDK_KEY_k:
  case GDK_KEY_w:
    dy = -1;
    break;
  case GDK_KEY_Down:
  case GDK_KEY_j:
  case GDK_KEY_s:
    dy = 1;
    break;
  case GDK_KEY_space:
  case GDK_KEY_Return:
    if (!kuro->cursor_active) {
      kuro->cursor_active = TRUE;
    } else {
      kuro_update_cell_state(kuro, kuro->cursor_position,
                             state & GDK_SHIFT_MASK, state & GDK_CONTROL_MASK);
    }
    break;
  default:
    did_something = FALSE;
  }

  if (dx != 0 || dy != 0) {
    did_something = TRUE;
    if (!kuro->cursor_active) {
      kuro->cursor_active = TRUE;
    } else {
      gint new_x = (gint)kuro->cursor_position.x + dx;
      gint new_y = (gint)kuro->cursor_position.y + dy;

      if (new_x >= 0 && new_x < kuro->board_size)
        kuro->cursor_position.x = (guchar)new_x;
      if (new_y >= 0 && new_y < kuro->board_size)
        kuro->cursor_position.y = (guchar)new_y;
    }
  }

  if (did_something) {
    /* Redraw */
    gtk_widget_queue_draw(kuro->drawing_area);
  }

  return did_something;
}

gboolean kuro_close_request_cb(GtkWindow *window, Kuro *kuro) {
  return FALSE; /* Let default handler destroy the window */
}

static void quit_cb(GSimpleAction *action, GVariant *parameters,
                    gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  kuro_quit(self);
}

static void new_game_cb(GSimpleAction *action, GVariant *parameters,
                        gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  kuro_new_game(self, self->board_size);
}

static void kuro_cancel_hinting(Kuro *kuro) {
  if (kuro->debug)
    g_debug("Stopping all current hints.");

  kuro->hint_status = HINT_DISABLED;
  if (kuro->hint_timeout_id != 0)
    g_source_remove(kuro->hint_timeout_id);
  kuro->hint_timeout_id = 0;
}

static gboolean kuro_update_hint(Kuro *kuro) {

  /* Check to see if hinting's been stopped by a cell being changed (race
   * condition) */
  if (kuro->hint_status == HINT_DISABLED)
    return FALSE;

  kuro->hint_status--;

  if (kuro->debug)
    g_debug("Updating hint status to %u.", kuro->hint_status);

  /* Redraw the widget (GTK4 doesn't support partial redraws) */
  gtk_widget_queue_draw(kuro->drawing_area);

  if (kuro->hint_status == HINT_DISABLED) {
    kuro_cancel_hinting(kuro);
    return FALSE;
  }

  return TRUE;
}

static void hint_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  KuroVector iter;

  /* Bail if we're already hinting */
  if (self->hint_status != HINT_DISABLED)
    return;

  /* Find the first cell which should be painted, but isn't (or vice-versa) */
  for (iter.x = 0; iter.x < self->board_size; iter.x++) {
    for (iter.y = 0; iter.y < self->board_size; iter.y++) {
      guchar status = self->board[iter.x][iter.y].status &
                      (CELL_PAINTED | CELL_SHOULD_BE_PAINTED);

      if (status <= MAX(CELL_SHOULD_BE_PAINTED, CELL_PAINTED) && status > 0) {
        if (self->debug)
          g_debug("Beginning hinting in cell (%u,%u).", iter.x, iter.y);

        /* Set up the cell for hinting */
        self->hint_status = HINT_FLASHES;
        self->hint_position = iter;
        self->hint_timeout_id =
            g_timeout_add(HINT_INTERVAL, (GSourceFunc)kuro_update_hint, self);
        kuro_update_hint((gpointer)self);

        return;
      }
    }
  }
}

static void undo_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);

  if (self->undo_stack->undo == NULL)
    return;

  switch (self->undo_stack->type) {
  case UNDO_PAINT:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_PAINTED;
    break;
  case UNDO_TAG1:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG1;
    break;
  case UNDO_TAG2:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG2;
    break;
  case UNDO_TAGS:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG1;
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG2;
    break;
  case UNDO_NEW_GAME:
  default:
    /* This is just here to stop the compiler warning */
    g_assert_not_reached();
    break;
  }

  self->cursor_position = self->undo_stack->cell;
  self->undo_stack = self->undo_stack->undo;

  g_simple_action_set_enabled(self->redo_action, TRUE);
  if (self->undo_stack->undo == NULL || self->undo_stack->type == UNDO_NEW_GAME)
    g_simple_action_set_enabled(self->undo_action, FALSE);

  /* The player can't possibly have won, but we need to update the error
   * highlighting */
  kuro_check_win(self);

  /* Redraw */
  gtk_widget_queue_draw(self->drawing_area);
}

static void redo_cb(GSimpleAction *action, GVariant *parameter,
                    gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);

  if (self->undo_stack->redo == NULL)
    return;

  self->undo_stack = self->undo_stack->redo;
  self->cursor_position = self->undo_stack->cell;

  switch (self->undo_stack->type) {
  case UNDO_PAINT:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_PAINTED;
    break;
  case UNDO_TAG1:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG1;
    break;
  case UNDO_TAG2:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG2;
    break;
  case UNDO_TAGS:
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG1;
    self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^=
        CELL_TAG2;
    break;
  case UNDO_NEW_GAME:
  default:
    /* This is just here to stop the compiler warning */
    g_assert_not_reached();
    break;
  }

  g_simple_action_set_enabled(self->undo_action, TRUE);
  if (self->undo_stack->redo == NULL)
    g_simple_action_set_enabled(self->redo_action, FALSE);

  /* The player can't possibly have won, but we need to update the error
   * highlighting */
  kuro_check_win(self);

  /* Redraw */
  gtk_widget_queue_draw(self->drawing_area);
}

static void pause_cb(GSimpleAction *action, GVariant *parameter,
                     gpointer user_data) {
  KuroApplication *kuro = KURO_APPLICATION(user_data);
  GVariant *state;
  gboolean paused;

  state = g_action_get_state(G_ACTION(action));
  paused = g_variant_get_boolean(state);
  g_variant_unref(state);

  /* Toggle state */
  paused = !paused;
  g_simple_action_set_state(action, g_variant_new_boolean(paused));

  kuro->is_paused = paused;

  if (paused) {
    kuro_pause_timer(kuro);
    gtk_widget_set_visible(kuro->pause_overlay, TRUE);
    /* Change icon to play */
    gtk_button_set_icon_name(GTK_BUTTON(kuro->pause_button),
                             "media-playback-start-symbolic");
    gtk_widget_set_tooltip_text(GTK_WIDGET(kuro->pause_button),
                                _("Resume the game"));
  } else {
    kuro_start_timer(kuro);
    gtk_widget_set_visible(kuro->pause_overlay, FALSE);
    /* Change icon to pause */
    gtk_button_set_icon_name(GTK_BUTTON(kuro->pause_button),
                             "media-playback-pause-symbolic");
    gtk_widget_set_tooltip_text(GTK_WIDGET(kuro->pause_button),
                                _("Pause the game"));
  }

  gtk_widget_queue_draw(kuro->drawing_area);
}

static void help_cb(GSimpleAction *action, GVariant *parameters,
                    gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);

  gtk_show_uri(GTK_WINDOW(self->window), "help:kuro", GDK_CURRENT_TIME);
}

/* XML Parsing for Release Notes */

typedef struct {
  const char *target_version;
  gboolean in_release;
  gboolean in_description;
  GString *notes;
  gboolean found;
  int tag_depth;
} ReleaseNotesData;

static void start_element(GMarkupParseContext *context,
                          const gchar *element_name,
                          const gchar **attribute_names,
                          const gchar **attribute_values, gpointer user_data,
                          GError **error) {
  ReleaseNotesData *data = user_data;

  if (g_strcmp0(element_name, "release") == 0) {
    const gchar *version = NULL;
    int i;

    for (i = 0; attribute_names[i] != NULL; i++) {
      if (g_strcmp0(attribute_names[i], "version") == 0) {
        version = attribute_values[i];
        break;
      }
    }

    /* Strict initial match or fallback to major.minor match if needed */
    if (version && g_strcmp0(version, data->target_version) == 0) {
      data->in_release = TRUE;
    }
  } else if (data->in_release && g_strcmp0(element_name, "description") == 0) {
    data->in_description = TRUE;
  } else if (data->in_description) {
    /* Reconstruct inner tags for description */
    g_string_append_printf(data->notes, "<%s", element_name);
    int i;
    for (i = 0; attribute_names[i] != NULL; i++) {
      g_string_append_printf(data->notes, " %s=\"%s\"", attribute_names[i],
                             attribute_values[i]);
    }
    g_string_append(data->notes, ">");
  }
}

static void end_element(GMarkupParseContext *context, const gchar *element_name,
                        gpointer user_data, GError **error) {
  ReleaseNotesData *data = user_data;

  if (g_strcmp0(element_name, "release") == 0) {
    if (data->in_release) {
      data->in_release = FALSE;
      if (data->notes->len > 0)
        data->found = TRUE;
    }
  } else if (g_strcmp0(element_name, "description") == 0) {
    if (data->in_description) {
      data->in_description = FALSE;
    }
  } else if (data->in_description) {
    g_string_append_printf(data->notes, "</%s>", element_name);
  }
}

static void text(GMarkupParseContext *context, const gchar *text,
                 gsize text_len, gpointer user_data, GError **error) {
  ReleaseNotesData *data = user_data;

  if (data->in_description) {
    g_string_append_len(data->notes, text, text_len);
  }
}

static char *get_release_notes(const char *version) {
  GBytes *bytes;
  const void *data;
  gsize len;
  GMarkupParseContext *context;
  GMarkupParser parser = {start_element, end_element, text, NULL, NULL};
  ReleaseNotesData parse_data = {version, FALSE, FALSE, NULL, FALSE, 0};
  char *result = NULL;

  /* Note: Path must match GResource prefix */
  /* Prefix: /io/github/tobagin/Kuro/Devel/gtk */
  /* Alias: io.github.tobagin.Kuro.Devel.metainfo.xml */
  const gchar *resource_path =
      g_strconcat("/", g_strdelimit(g_strdup(APPLICATION_ID), ".", '/'),
                  "/gtk/", APPLICATION_ID, ".metainfo.xml", NULL);

  bytes = g_resources_lookup_data(resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE,
                                  NULL);
  if (!bytes) {
    g_warning("Failed to load metainfo from resource: %s", resource_path);
    return NULL;
  }
  g_debug("Loaded metainfo size: %lu", g_bytes_get_size(bytes));

  data = g_bytes_get_data(bytes, &len);
  parse_data.notes = g_string_new("");

  context = g_markup_parse_context_new(&parser, 0, &parse_data, NULL);
  if (g_markup_parse_context_parse(context, data, len, NULL)) {
    if (parse_data.found) {
      result = g_string_free(parse_data.notes, FALSE);
    } else {
      g_string_free(parse_data.notes, TRUE);
    }
  } else {
    g_string_free(parse_data.notes, TRUE);
  }

  g_markup_parse_context_free(context);
  g_bytes_unref(bytes);

  return result;
}

static void about_cb(GSimpleAction *action, GVariant *parameters,
                     gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  AdwDialog *about;

  const char *developers[] = {"Thiago Fernandes", NULL};

  const char *original_authors[] = {"Philip Withnall", "Ben Windsor", NULL};

  const char *acknowledgements[] = {"GTK Team", "Libadwaita Team", NULL};

  about = adw_about_dialog_new();

  adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), _("Kuro"));
  adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about),
                                        APPLICATION_ID);
  adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), VERSION);
  adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about),
                                      "Thiago Fernandes");
  adw_about_dialog_set_copyright(
      ADW_ABOUT_DIALOG(about),
      _("Copyright © 2007–2010 Philip Withnall\nCopyright © 2026 Thiago "
        "Fernandes"));
  adw_about_dialog_set_comments(
      ADW_ABOUT_DIALOG(about),
      _("A logic puzzle originally designed by Nikoli"));
  adw_about_dialog_set_developers(ADW_ABOUT_DIALOG(about), developers);

  adw_about_dialog_add_credit_section(
      ADW_ABOUT_DIALOG(about), _("Original Developers"), original_authors);
  adw_about_dialog_add_credit_section(ADW_ABOUT_DIALOG(about),
                                      _("Acknowledgements"), acknowledgements);

  adw_about_dialog_set_translator_credits(ADW_ABOUT_DIALOG(about),
                                          _("translator-credits"));
  adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about),
                                    GTK_LICENSE_GPL_3_0);
  adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), PACKAGE_URL);

  /* Add release notes */
  char *release_notes = get_release_notes(VERSION);
  if (release_notes) {
    adw_about_dialog_set_release_notes(ADW_ABOUT_DIALOG(about), release_notes);
    g_free(release_notes);
  }

  adw_dialog_present(ADW_DIALOG(about), GTK_WIDGET(self->window));
}

static void show_help_overlay_cb(GSimpleAction *action, GVariant *parameter,
                                 gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  GtkBuilder *builder;
  GObject *overlay;

  builder = gtk_builder_new_from_resource(
      g_strconcat("/", g_strdelimit(g_strdup(APPLICATION_ID), ".", '/'),
                  "/gtk/help-overlay.ui", NULL));
  overlay = gtk_builder_get_object(builder, "help_overlay");

  if (overlay && ADW_IS_DIALOG(overlay)) {
    adw_dialog_present(ADW_DIALOG(overlay), GTK_WIDGET(self->window));
  } else {
    g_warning("Failed to load help overlay");
  }

  g_object_unref(builder);
}

static void board_size_cb(GSimpleAction *action, GVariant *parameter,
                          gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  g_print("board_size_cb: %s\n", g_variant_get_string(parameter, NULL));
  g_settings_set_value(self->settings, "board-size", parameter);
  g_simple_action_set_state(action, parameter);
}

static void board_theme_cb(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  g_print("board_theme_cb: %s\n", g_variant_get_string(parameter, NULL));
  g_settings_set_value(self->settings, "board-theme", parameter);
  g_simple_action_set_state(action, parameter);
}

static void board_size_change_cb(GSettings *settings, const gchar *key,
                                 gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  gchar *size_str;
  guint64 size;

  g_print("board_size_change_cb triggered\n");
  size_str = g_settings_get_string(self->settings, "board-size");
  size = g_ascii_strtoull(size_str, NULL, 10);
  g_free(size_str);

  if (size > MAX_BOARD_SIZE) {
    GVariant *default_size =
        g_settings_get_default_value(self->settings, "board-size");
    g_variant_get(default_size, "s", &size_str);
    g_variant_unref(default_size);
    size = g_ascii_strtoull(size_str, NULL, 10);
    g_free(size_str);
    g_assert(size <= MAX_BOARD_SIZE);
  }

  kuro_set_board_size(self, size);
}

static void board_theme_change_cb(GSettings *settings, const gchar *key,
                                  gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  gchar *theme_str;

  theme_str = g_settings_get_string(self->settings, "board-theme");

  if (g_strcmp0(theme_str, "auto") == 0) {
    /* Auto: use classic for light mode, kuro for dark mode */
    AdwStyleManager *style_manager = adw_style_manager_get_default();
    gboolean is_dark = adw_style_manager_get_dark(style_manager);
    self->theme = is_dark ? &theme_kuro : &theme_classic;
  } else if (g_strcmp0(theme_str, "classic") == 0) {
    self->theme = &theme_classic;
  } else {
    self->theme = &theme_kuro;
  }

  g_free(theme_str);

  if (self->drawing_area != NULL) {
    gtk_widget_queue_draw(self->drawing_area);
  }
}

static void style_manager_dark_changed_cb(AdwStyleManager *style_manager,
                                          GParamSpec *pspec,
                                          gpointer user_data) {
  KuroApplication *self = KURO_APPLICATION(user_data);
  /* Re-trigger theme change to update if auto theme is active */
  board_theme_change_cb(self->settings, "board-theme", self);
}
