/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btrenderdialog
 * @short_description: class for the editor render dialog
 *
 * Provides UI to access the song recording
 */
/* TODO(ensonic):
 * - use chooserbutton to choose name too
 *   (arg, the chooser button cannot do save_as)
 * - use song-name based file-name by default
 */
/* TODO(ensonic): more options
 * - have dithering and noise shaping options here
 */
/* TODO(ensonic): project file export
 * - consider MXF as one option
 */

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"
#include <glib/gprintf.h>
#include <glib/gstdio.h>

struct _BtRenderDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* dialog widgets */
  GtkWidget *okay_button;
  GtkWidget *dir_chooser;
  GtkWidget *file_name_entry;
  GtkWidget *format_menu;
  GtkWidget *mode_menu;
  GtkProgressBar *track_progress;
  GtkLabel *info;

  /* dialog data */
  gchar *folder, *filename;
  BtSinkBinRecordFormat format;
  BtRenderMode mode;

  /* render state data (incl. save song state data) */
  BtSong *song;
  BtSinkBin *sink_bin;
  GstElement *convert;
  GList *list;
  gint track, tracks;
  gchar *song_name, *file_name;
  gboolean unsaved, has_error;
  gboolean saved_loop;
  gboolean saved_follow_playback;
};

static void on_format_menu_changed (GtkComboBox * menu, gpointer user_data);

//-- the class

G_DEFINE_TYPE (BtRenderDialog, bt_render_dialog, GTK_TYPE_DIALOG);


//-- enums

GType
bt_render_mode_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_RENDER_MODE_MIXDOWN, "BT_RENDER_MODE_MIXDOWN", "mix to one track"},
      {BT_RENDER_MODE_SINGLE_TRACKS, "BT_RENDER_MODE_SINGLE_TRACKS",
          "record one track for each source"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtRenderMode", values);
  }
  return type;
}

//-- helper

static void
bt_render_dialog_enable_level_meters (BtSetup * setup, gboolean enable)
{
  GList *machines, *node;
  BtMachine *machine;
  GstElement *in_pre_level, *in_post_level, *out_pre_level, *out_post_level;

  g_object_get (setup, "machines", &machines, NULL);
  for (node = machines; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    g_object_get (machine, "input-pre-level", &in_pre_level, "input-post-level",
        &in_post_level, "output-pre-level", &out_pre_level, "output-post-level",
        &out_post_level, NULL);
    if (in_pre_level) {
      g_object_set (in_pre_level, "post-messages", enable, NULL);
      gst_object_unref (in_pre_level);
    }
    if (in_post_level) {
      g_object_set (in_post_level, "post-messages", enable, NULL);
      gst_object_unref (in_post_level);
    }
    if (out_pre_level) {
      g_object_set (out_pre_level, "post-messages", enable, NULL);
      gst_object_unref (out_pre_level);
    }
    if (out_post_level) {
      g_object_set (out_post_level, "post-messages", enable, NULL);
      gst_object_unref (out_post_level);
    }
  }
  g_list_free (machines);
}

//-- event handler helper

static gchar *
bt_render_dialog_make_file_name (const BtRenderDialog * self, gint track)
{
  gchar *file_name =
      g_build_filename (self->priv->folder, self->priv->filename, NULL);
  gchar track_str[4];
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if (self->priv->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    g_sprintf (track_str, ".%03u", track);
  } else {
    track_str[0] = '\0';
  }

  // add file suffix if not yet there
  enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  enum_value = g_enum_get_value (enum_class, self->priv->format);
  if (!g_str_has_suffix (file_name, enum_value->value_name)) {
    gchar *tmp_file_name;

    // append extension
    tmp_file_name = g_strdup_printf ("%s%s%s",
        file_name, track_str, enum_value->value_name);
    g_free (file_name);
    file_name = tmp_file_name;
  } else {
    gchar *tmp_file_name;

    // reuse extension
    file_name[strlen (file_name) - strlen (enum_value->value_name)] = '\0';
    tmp_file_name = g_strdup_printf ("%s%s%s",
        file_name, track_str, enum_value->value_name);
    g_free (file_name);
    file_name = tmp_file_name;
  }
  GST_INFO ("record file template: '%s'", file_name);

  return (file_name);
}

static void
bt_render_dialog_record (const BtRenderDialog * self)
{
  gchar *info;

  GST_INFO ("recording to '%s'", self->priv->file_name);
  g_object_set (self->priv->sink_bin, "record-file-name", self->priv->file_name,
      NULL);
  info = g_strdup_printf (_("Recording to: %s"), self->priv->file_name);
  gtk_label_set_text (self->priv->info, info);
  g_free (info);

  bt_song_play (self->priv->song);
  bt_song_write_to_lowlevel_dot_file (self->priv->song);
}

/* Prepare the rendering. */
static void
bt_render_dialog_record_init (const BtRenderDialog * self)
{
  BtMainPageSequence *sequence_page;
  BtSetup *setup;
  BtSequence *sequence;
  BtMachine *machine;

  // get current settings and override
  bt_child_proxy_get (self->priv->app, "unsaved", &self->priv->unsaved,
      "main-window::pages::sequence-page", &sequence_page, NULL);
  g_object_get (self->priv->song, "setup", &setup, "sequence", &sequence, NULL);
  g_object_get (sequence, "loop", &self->priv->saved_loop, NULL);
  g_object_set (sequence, "loop", FALSE, NULL);
  g_object_unref (sequence);
  g_object_get (sequence_page, "follow-playback",
      &self->priv->saved_follow_playback, NULL);
  g_object_set (sequence_page, "follow-playback", FALSE, NULL);
  g_object_unref (sequence_page);

  // disable all level-meters
  bt_render_dialog_enable_level_meters (setup, FALSE);

  // lookup the audio-sink machine and change mode
  if ((machine = bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
    g_object_get (machine, "machine", &self->priv->sink_bin, "adder-convert",
        &self->priv->convert, NULL);

    g_object_set (self->priv->sink_bin, "mode", BT_SINK_BIN_MODE_RECORD,
        // TODO(ensonic): this hangs :/
        //"mode",BT_SINK_BIN_MODE_PLAY_AND_RECORD,
        "record-format", self->priv->format, NULL);

    /* TODO(ensonic): configure dithering/noise-shaping
     * - should sink-bin do it so that we get this also when recording from
     *   the commandline (need some extra cmdline options for it :/
     * - we could also put it to the options
     * - sink-machine could also set this (hard-coded) when going to record mode
     */
    g_object_set (self->priv->convert, "dithering", 2, "noise-shaping", 3,
        NULL);

    g_object_unref (machine);
  }
  if (self->priv->mode == BT_RENDER_MODE_MIXDOWN) {
    self->priv->track = -1;
    self->priv->tracks = 0;
  } else {
    self->priv->list =
        bt_setup_get_machines_by_type (setup, BT_TYPE_SOURCE_MACHINE);
    self->priv->track = 0;
    self->priv->tracks = g_list_length (self->priv->list);

    bt_child_proxy_get (self->priv->song, "song-info::name",
        &self->priv->song_name, NULL);
  }
  g_object_unref (setup);
}

/* Cleanup the rendering */
static void
bt_render_dialog_record_done (const BtRenderDialog * self)
{
  BtSetup *setup;

  /* reset play mode */
  if (self->priv->sink_bin) {
    g_object_set (self->priv->sink_bin, "mode", BT_SINK_BIN_MODE_PLAY, NULL);
    gst_object_replace ((GstObject **) & self->priv->sink_bin, NULL);
  }

  /* reset dithering/noise-shaping */
  if (self->priv->convert) {
    g_object_set (self->priv->convert, "dithering", 0, "noise-shaping", 0,
        NULL);
    gst_object_replace ((GstObject **) & self->priv->convert, NULL);
  }

  /* reset loop and follow-playback */
  bt_child_proxy_set (self->priv->song, "sequence::loop",
      self->priv->saved_loop, NULL);
  bt_child_proxy_set (self->priv->app,
      "main-window::pages::sequence-page::follow-playback",
      self->priv->saved_follow_playback, NULL);

  g_object_set (self->priv->app, "unsaved", self->priv->unsaved, NULL);

  // re-enable all level-meters
  g_object_get (self->priv->song, "setup", &setup, NULL);
  bt_render_dialog_enable_level_meters (setup, TRUE);
  g_object_unref (setup);

  if (self->priv->list) {
    g_list_free (self->priv->list);
    self->priv->list = NULL;
  }
  if (self->priv->song_name) {
    bt_child_proxy_set (self->priv->song, "song-info::name",
        self->priv->song_name, NULL);
    g_free (self->priv->song_name);
  }
  // close the dialog
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_NONE);
}

/* Run the rendering. */
static void
bt_render_dialog_record_next (const BtRenderDialog * self)
{
  static BtMachine *machine = NULL;

  /* cleanup from previous run */
  if (self->priv->has_error && self->priv->file_name) {
    g_unlink (self->priv->file_name);
  }
  if (self->priv->tracks && machine) {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  if (self->priv->file_name) {
    g_free (self->priv->file_name);
    self->priv->file_name = NULL;
  }
  /* if there was an error or we're done */
  if (self->priv->has_error || self->priv->track == self->priv->tracks) {
    bt_render_dialog_record_done (self);
    return;
  }

  self->priv->file_name =
      bt_render_dialog_make_file_name (self, self->priv->track);

  if (self->priv->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    gchar *track_name, *id;

    machine =
        BT_MACHINE (g_list_nth_data (self->priv->list, self->priv->track));
    g_object_set (machine, "state", BT_MACHINE_STATE_SOLO, NULL);

    g_object_get (machine, "id", &id, NULL);
    track_name = g_strdup_printf ("%s : %s", self->priv->song_name, id);
    bt_child_proxy_set (self->priv->song, "song-info::name", track_name, NULL);
    GST_INFO ("recording to '%s'", track_name);
    g_free (track_name);
    g_free (id);
  }
  bt_render_dialog_record (self);
  self->priv->track++;
}

//-- event handler

static void
on_folder_changed (GtkFileChooser * chooser, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->priv->folder);
  self->priv->folder = gtk_file_chooser_get_current_folder (chooser);

  GST_DEBUG ("folder changed '%s'", self->priv->folder);
}

static void
on_filename_changed (GtkEditable * editable, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->priv->filename);
  self->priv->filename = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));

  // update format
  if (self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (self->priv->filename, enum_value->value_name)) {
          g_signal_handlers_block_matched (self->priv->format_menu,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_format_menu_changed, (gpointer) user_data);
          gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->format_menu), i);
          g_signal_handlers_unblock_matched (self->priv->format_menu,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_format_menu_changed, (gpointer) user_data);
          break;
        }
      }
    }
  }
}

static void
on_format_menu_changed (GtkComboBox * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->priv->format = gtk_combo_box_get_active (menu);

  // update filename
  if (self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;
    gchar *fn = self->priv->filename;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (fn, enum_value->value_name)) {
          GST_INFO ("found matching ext: %s", enum_value->value_name);
          // replace this suffix, with the new
          fn[strlen (fn) - strlen (enum_value->value_name)] = '\0';
          GST_INFO ("cut fn to: %s", fn);
          break;
        }
      }
    }
    enum_value = g_enum_get_value (enum_class, self->priv->format);
    self->priv->filename = g_strdup_printf ("%s%s", fn, enum_value->value_name);
    GST_INFO ("set new fn to: %s", self->priv->filename);
    g_free (fn);
    g_signal_handlers_block_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_filename_changed, (gpointer) user_data);
    gtk_entry_set_text (GTK_ENTRY (self->priv->file_name_entry),
        self->priv->filename);
    g_signal_handlers_unblock_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_filename_changed, (gpointer) user_data);
  }
}

static void
on_mode_menu_changed (GtkComboBox * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->priv->mode = gtk_combo_box_get_active (menu);
}

static void
on_okay_clicked (GtkButton * button, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  // gray the okay button and the action area
  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  gtk_widget_set_sensitive (self->priv->dir_chooser, FALSE);
  gtk_widget_set_sensitive (self->priv->file_name_entry, FALSE);
  gtk_widget_set_sensitive (self->priv->format_menu, FALSE);
  gtk_widget_set_sensitive (self->priv->mode_menu, FALSE);
  // start the rendering
  bt_render_dialog_record_init (self);
  bt_render_dialog_record_next (self);
}

static void
on_song_play_pos_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);
  BtSongInfo *song_info;
  gulong pos, length;
  gdouble progress;
  // the +4 is not really needed, but I get a stack smashing error on ubuntu without
  gchar str[3 + 2 * (2 + 2 + 3 + 3) + 4];
  gulong cmsec, csec, cmin, tmsec, tsec, tmin;

  bt_child_proxy_get ((gpointer) song, "sequence::length", &length, "song-info",
      &song_info, "play-pos", &pos, NULL);

  progress = (gdouble) pos / (gdouble) length;
  if (progress >= 1.0) {
    progress = 1.0;
  }
  GST_INFO ("progress %ld/%ld=%lf", pos, length, progress);

  bt_song_info_tick_to_m_s_ms (song_info, length, &tmin, &tsec, &tmsec);
  bt_song_info_tick_to_m_s_ms (song_info, pos, &cmin, &csec, &cmsec);
  // format
  g_sprintf (str, "%02lu:%02lu.%03lu / %02lu:%02lu.%03lu", cmin, csec, cmsec,
      tmin, tsec, tmsec);

  g_object_set (self->priv->track_progress, "fraction", progress, "text", str,
      NULL);

  g_object_unref (song_info);
}

static void
on_song_error (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  GST_WARNING ("received %s bus message from %s",
      GST_MESSAGE_TYPE_NAME (message),
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));

  self->priv->has_error = TRUE;
  bt_render_dialog_record_next (self);
}

static void
on_song_eos (const GstBus * const bus, const GstMessage * const message,
    gconstpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  bt_song_stop (self->priv->song);
  // trigger next file / done
  bt_render_dialog_record_next (self);
}

//-- helper methods

static void
bt_render_dialog_init_ui (const BtRenderDialog * self)
{
  GtkWidget *box, *label, *widget, *table;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;
  GstBin *bin;
  GstBus *bus;
  gchar *full_file_name = NULL;

  GST_DEBUG ("read settings");

  bt_child_proxy_get (self->priv->app, "settings::record-folder",
      &self->priv->folder, NULL);
  bt_child_proxy_get (self->priv->song, "song-info::file-name", &full_file_name,
      "bin", &bin, NULL);

  // set default base name
  if (full_file_name) {
    gchar *file_name, *ext;

    // cut off extension from file_name
    if ((ext = strrchr (full_file_name, '.')))
      *ext = '\0';
    if ((file_name = strrchr (full_file_name, G_DIR_SEPARATOR))) {
      self->priv->filename = g_strdup (&file_name[1]);
      g_free (full_file_name);
    } else {
      self->priv->filename = full_file_name;
    }
  } else {
    self->priv->filename = g_strdup ("");
  }
  GST_INFO ("initial base filename: '%s'", self->priv->filename);

  GST_DEBUG ("prepare render dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "song rendering");

  gtk_window_set_title (GTK_WINDOW (self), _("song rendering"));

  // add dialog commision widgets (okay, cancel)
  self->priv->okay_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG
              (self))), self->priv->okay_button);
  gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL,
      GTK_RESPONSE_REJECT);

  //gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_NONE);
  g_signal_connect (self->priv->okay_button, "clicked",
      G_CALLBACK (on_okay_clicked), (gpointer) self);

  // add widgets to the dialog content area
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG
              (self))), box);

  table =
      gtk_table_new ( /*rows= */ 7, /*columns= */ 2, /*homogenous= */ FALSE);
  gtk_container_add (GTK_CONTAINER (box), table);


  label = gtk_label_new (_("Folder"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->dir_chooser = widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
      self->priv->folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 0, 1,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);


  label = gtk_label_new (_("Filename"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->file_name_entry = widget = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (widget), self->priv->filename);
  gtk_entry_set_activates_default (GTK_ENTRY (self->priv->file_name_entry),
      TRUE);
  g_signal_connect (widget, "changed", G_CALLBACK (on_filename_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);


  label = gtk_label_new (_("Format"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_SHRINK,
      2, 1);

  // query supported formats from sinkbin
  self->priv->format_menu = widget = gtk_combo_box_text_new ();
  enum_class = g_type_class_ref (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
          enum_value->value_nick);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
      BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  g_signal_connect (widget, "changed", G_CALLBACK (on_format_menu_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 2, 3,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);
  // set initial filename:
  on_format_menu_changed (GTK_COMBO_BOX (widget), (gpointer) self);


  label = gtk_label_new (_("Mode"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_SHRINK,
      2, 1);

  // query supported formats from sinkbin
  self->priv->mode_menu = widget = gtk_combo_box_text_new ();
  enum_class = g_type_class_ref (BT_TYPE_RENDER_MODE);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
          enum_value->value_nick);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), BT_RENDER_MODE_MIXDOWN);
  g_signal_connect (widget, "changed", G_CALLBACK (on_mode_menu_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 3, 4,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  /* TODO(ensonic): add more widgets
     o write project file
     o none, jokosher, ardour, ...
   */

  self->priv->info = GTK_LABEL (gtk_label_new (""));
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (self->priv->info), 0, 2, 4,
      5, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  self->priv->track_progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (self->priv->track_progress),
      0, 2, 5, 6, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  // connect signal handlers    
  g_signal_connect (self->priv->song, "notify::play-pos",
      G_CALLBACK (on_song_play_pos_notify), (gpointer) self);
  bus = gst_element_get_bus (GST_ELEMENT (bin));
  g_signal_connect (bus, "message::error", G_CALLBACK (on_song_error),
      (gpointer) self);
  g_signal_connect (bus, "message::eos", G_CALLBACK (on_song_eos),
      (gpointer) self);

  gst_object_unref (bus);
  gst_object_unref (bin);
  GST_DEBUG ("done");
}

//-- constructor methods

/**
 * bt_render_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtRenderDialog *
bt_render_dialog_new (void)
{
  BtRenderDialog *self;

  self = BT_RENDER_DIALOG (g_object_new (BT_TYPE_RENDER_DIALOG, NULL));
  g_object_get (self->priv->app, "song", &self->priv->song, NULL);
  bt_render_dialog_init_ui (self);
  return (self);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_render_dialog_dispose (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->file_name) {
    GST_INFO ("canceled recording, removing partial file");
    g_unlink (self->priv->file_name);
    g_free (self->priv->file_name);
  }

  if (self->priv->song) {
    GstBin *bin;

    g_object_get (self->priv->song, "bin", &bin, NULL);
    if (bin) {
      GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
      g_signal_handlers_disconnect_by_data (bus, self);
      gst_object_unref (bus);
      gst_object_unref (bin);
    }
    g_signal_handlers_disconnect_by_func (self->priv->song,
        on_song_play_pos_notify, self);
    g_object_unref (self->priv->song);
  }
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->dispose (object);
}

static void
bt_render_dialog_finalize (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_free (self->priv->folder);
  g_free (self->priv->filename);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->finalize (object);
}

static void
bt_render_dialog_init (BtRenderDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_RENDER_DIALOG,
      BtRenderDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_render_dialog_class_init (BtRenderDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtRenderDialogPrivate));

  gobject_class->dispose = bt_render_dialog_dispose;
  gobject_class->finalize = bt_render_dialog_finalize;
}
