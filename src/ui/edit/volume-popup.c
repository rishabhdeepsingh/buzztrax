/* $Id$
 *
 * GNOME Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2008 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btvolumepopup
 * @short_description: volume popup widget
 *
 * Shows a popup window containing a vertical slider
 */
/* @todo: use gtk_scale_add_mark() in gtk-2.16 instead of the ruler.
 */
#define BT_EDIT
#define VOLUME_POPUP_C

#include "bt-edit.h"
#include "vruler.h"

//-- the class

G_DEFINE_TYPE (BtVolumePopup, bt_volume_popup, GTK_TYPE_WINDOW);

//-- event handler

static void
cb_scale_changed(GtkRange *range, gpointer  user_data)
{
  GtkLabel *label=GTK_LABEL(user_data);
  gchar str[6];

  g_sprintf(str,"%3d %%",(gint)(100.0*gtk_range_get_value(range)));
  gtk_label_set_text(label,str);
}

/*
 * hide popup when clicking outside
 */
static gboolean
cb_dock_press(GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  BtVolumePopup *self = BT_VOLUME_POPUP(data);

  //if(!gtk_widget_get_realized(GTK_WIDGET(self)) return FALSE;

  if (event->type == GDK_BUTTON_PRESS) {
    GdkEventButton *e;
    //GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",event->type,event->window,event->send_event,event->time);
    //GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",event->x,event->y,event->axes,event->state);
    //GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",event->button,event->device,event->x_root,event->y_root);

    /*
    GtkWidget *parent=GTK_WIDGET(gtk_window_get_transient_for(GTK_WINDOW(self)));
    //GtkWidget *parent=gtk_widget_get_parent(GTK_WIDGET(self));
    //gboolean retval;

    GST_INFO("FORWARD : popup=%p, widget=%p", self, widget);
    GST_INFO("FORWARD : parent=%p, parent->window=%p", parent, parent->window);
    */

    bt_volume_popup_hide(self);

    // forward event
    e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
    //GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
    //GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
    //GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
    //e->window = widget->window;
    //e->window = parent->window;
    //e->type = GDK_BUTTON_PRESS;

    gtk_main_do_event ((GdkEvent *) e);
    //retval=gtk_widget_event (widget, (GdkEvent *) e);
    //retval=gtk_widget_event (parent, (GdkEvent *) e);
    //retval=gtk_widget_event (self->parent_widget, (GdkEvent *) e);
    //GST_INFO("  result =%d", retval);
    //g_signal_emit_by_name(self->parent_widget, "event", 0, &retval, e);
    //g_signal_emit_by_name(parent, "event", 0, &retval, e);
    //GST_INFO("  result =%d", retval);
    //e->window = event->window;
    gdk_event_free ((GdkEvent *) e);

    return TRUE;
  }
  return FALSE;
}

//-- helper methods

//-- constructor methods

/**
 * bt_volume_popup_new:
 * @adj: the adjustment for the popup
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GtkWidget *
bt_volume_popup_new(GtkAdjustment *adj) {
  GtkWidget *table, *scale, *frame,*ruler/*,*rbox,*pad*/, *label;
  BtVolumePopup *self;

  self = g_object_new(BT_TYPE_VOLUME_POPUP, "type", GTK_WINDOW_POPUP, NULL);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

  table = gtk_table_new(2,2, FALSE);


  label=gtk_label_new("");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0,2, 0,1);

  scale=gtk_vscale_new(adj);
  self->scale=GTK_RANGE(scale);
  gtk_widget_set_size_request(scale, -1, 200);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_inverted(self->scale, TRUE);
  g_signal_connect(self->scale, "value-changed", G_CALLBACK(cb_scale_changed), label);
  cb_scale_changed(self->scale,label);
  gtk_table_attach_defaults(GTK_TABLE(table), scale, 1,2, 1,2);


  // add ruler
  ruler=bt_vruler_new();
  /* we use -X instead of 0.0 because of:
   * http://bugzilla.gnome.org/show_bug.cgi?id=465041
   * @todo: take slider knob size into account
   * gtk_widget_style_get(scale,"slider-length",slider_length,NULL);
   */
  bt_ruler_set_range(BT_RULER(ruler),435.0,-35.0,100.0,30.0);
  gtk_widget_set_size_request(ruler,30,-1);
  BT_RULER_GET_CLASS(ruler)->draw_pos = NULL;
  gtk_table_attach_defaults(GTK_TABLE(table), ruler, 0,1, 1,2);


  gtk_container_add(GTK_CONTAINER(frame), table);

  gtk_container_add(GTK_CONTAINER(self), frame);
  gtk_widget_show_all(frame);

  g_signal_connect(self, "button-press-event", G_CALLBACK (cb_dock_press), self);

  return GTK_WIDGET(self);
}

//-- methods

/**
 * bt_volume_popup_show:
 * @self: the popup widget
 *
 * Show and activate the widget
 */
void bt_volume_popup_show(BtVolumePopup *self) {
  GdkWindow *window;

  gtk_widget_show_all(GTK_WIDGET(self));
  //gtk_widget_realize(GTK_WIDGET(self)); // not needed (yet)
  window = gtk_widget_get_window(GTK_WIDGET(self));

  /* grab focus */
  gtk_widget_grab_focus_savely(GTK_WIDGET(self));
  gtk_grab_add(GTK_WIDGET(self));
  gdk_pointer_grab(window, TRUE,
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK,
        NULL, NULL, GDK_CURRENT_TIME);
  gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME);
}

/**
 * bt_volume_popup_hide:
 * @self: the popup widget
 *
 * Hide and deactivate the widget
 */
void bt_volume_popup_hide(BtVolumePopup *self) {
  /* ungrab focus */
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_grab_remove(GTK_WIDGET(self));

  gtk_widget_hide(GTK_WIDGET(self));
}

//-- wrapper

//-- class internals

static void
bt_volume_popup_dispose(GObject *object)
{
  BtVolumePopup *popup = BT_VOLUME_POPUP(object);

  if (popup->timeout) {
    g_source_remove(popup->timeout);
    popup->timeout = 0;
  }

  G_OBJECT_CLASS(bt_volume_popup_parent_class)->dispose (object);
}

static void
bt_volume_popup_class_init(BtVolumePopupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->dispose = bt_volume_popup_dispose;
}

static void
bt_volume_popup_init(BtVolumePopup *popup)
{
  popup->timeout = 0;
}

