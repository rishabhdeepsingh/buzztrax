/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btuiressources
 * @short_description: common shared ui ressources like icons and colors
 *
 * This class serves as a central storage for colors and icons.
 * It is implemented as a singleton.
 */
 
/* @todo: manage GdkColor entries and guint colors (via gdk_color.pixel)
 */

#define BT_EDIT
#define BT_UI_RESSOURCES_C

#include "bt-edit.h"

struct _BtUIRessourcesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* icons */
  GdkPixbuf *source_machine_pixbuf;
  GdkPixbuf *processor_machine_pixbuf;
  GdkPixbuf *sink_machine_pixbuf;

  /* colors */
  GdkColor colors[BT_UI_RES_COLOR_COUNT];
  
  /* the keyboard shortcut table for the window */
  GtkAccelGroup *accel_group;
};

static GObjectClass *parent_class=NULL;

static gpointer singleton=NULL;

//-- event handler

//-- helper methods

#define MAKE_COLOR_FROM_FLOATS(ix,r,g,b) \
  self->priv->colors[ix].red=  (guint16)(r*65535); \
  self->priv->colors[ix].green=(guint16)(g*65535); \
  self->priv->colors[ix].blue= (guint16)(b*65535)

#define MAKE_COLOR_FROM_HEX(ix,r,g,b) \
  self->priv->colors[ix].red=  (guint16)((r*256)); \
  self->priv->colors[ix].green=(guint16)((g*256)); \
  self->priv->colors[ix].blue= (guint16)((b*256))

#define MAKE_COLOR_FROM_HEX_MIX(ix,r1,g1,b1,r2,g2,b2) \
  self->priv->colors[ix].red=  (guint16)((r2+((r1-r2)>>1))*256); \
  self->priv->colors[ix].green=(guint16)((g2+((g1-g2)>>1))*256); \
  self->priv->colors[ix].blue= (guint16)((b2+((b1-b2)>>1))*256)

static gboolean bt_ui_ressources_init_colors(BtUIRessources *self) {
  GdkColormap *colormap;
  GtkSettings *settings;
  gboolean color_res[BT_UI_RES_COLOR_COUNT];
  gulong res;
  gchar *icon_theme_name;
  
  settings=gtk_settings_get_default();
  /* get the theme name - we need different machine colors for tango
   * http://tango.freedesktop.org/Tango_Icon_Theme_Guidelines
   * http://tango.freedesktop.org/static/cvs/tango-art-tools/palettes/Tango-Palette.gpl
   */
  g_object_get (settings, "gtk-icon-theme-name", &icon_theme_name, NULL);
  GST_INFO("Icon Theme: %s",icon_theme_name);
  
  /* @todo: can we get some colors from the theme ?
   * gtk_widget_style_get(widget,
   *   "cursor-color",&self->priv->colors[ix],
   *   "secondary-cursor-color",&self->priv->colors[ix],
   *   NULL);
   */
  
  // cursor
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_CURSOR,                    0.85,0.85,0.20);
  
  // selection background
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SELECTION1,                1.00,1.00,0.60);
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SELECTION2,                0.95,0.95,0.55);
  
  // tree view lines
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PLAYLINE,                  0.00,0.00,1.00);
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_LOOPLINE,                  1.00,0.75,0.00);
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_ENDLINE,                   1.00,0.30,0.20);
  
  // source machine
  if(!strcasecmp(icon_theme_name,"tango")) {
    /* #fcaf3e / #f57900 / #ce5c00 */
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SOURCE_MACHINE_BASE,          0xf5,0x79,0x00);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1,       0xfc,0xaf,0x3e);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2,   0xfc,0xaf,0x3e,0xf5,0x79,0x00);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1,     0xce,0x5c,0x00,0xf5,0x79,0x00);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2,         0xce,0x5c,0x00);
  }
  else {
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SOURCE_MACHINE_BASE,       1.00,0.60,0.60);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1,    1.00,0.90,0.90);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2,    1.00,0.80,0.80);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1,      0.60,0.40,0.40);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2,      0.50,0.20,0.20);
  }
  
  // processor machine
  if(!strcasecmp(icon_theme_name,"tango")) {
    /* #8ae234 / #73d216 / #4e9a06 */
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE,       0x73,0xd2,0x16);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1,    0x8a,0xe2,0x34);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2,0x8a,0xe2,0x34,0x73,0xd2,0x16);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1,  0x4e,0x9a,0x06,0x73,0xd2,0x16);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2,      0x4e,0x9a,0x06);
  }
  else {
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE,    0.60,1.00,0.60);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1, 0.90,1.00,0.90);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2, 0.80,1.00,0.80);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1,   0.40,0.60,0.40);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2,   0.20,0.50,0.20);
  }

  // sink machine
  if(!strcasecmp(icon_theme_name,"tango")) {
    /* #729fcf / #3465a4 / #204a87 */
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SINK_MACHINE_BASE,            0x34,0x65,0xa4);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1,         0x72,0x9f,0xcf);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2,     0x72,0x9f,0xcf,0x34,0x65,0xa4);
    MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SINK_MACHINE_DARK1,       0x20,0x4a,0x87,0x34,0x65,0xa4);
    MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SINK_MACHINE_DARK2,           0x20,0x4a,0x87);
  }
  else {
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SINK_MACHINE_BASE,         0.60,0.60,1.00);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1,      0.90,0.90,1.00);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2,      0.80,0.80,1.00);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SINK_MACHINE_DARK1,        0.40,0.40,0.60);
    MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_SINK_MACHINE_DARK2,        0.20,0.20,0.50);
  }
  
  // analyzer window
  MAKE_COLOR_FROM_FLOATS(BT_UI_RES_COLOR_ANALYZER_PEAK,             1.00,0.75,0.00);
  
  g_free(icon_theme_name);
   
  // now allocate colors
  colormap=gdk_colormap_get_system();
  if((res=gdk_colormap_alloc_colors(colormap,self->priv->colors,BT_UI_RES_COLOR_COUNT,FALSE,TRUE,color_res))) {
    gulong i;
    GST_WARNING("failed to allocate %d colors %d",res);
    for(i=0;i<BT_UI_RES_COLOR_COUNT;i++) {
      if(!color_res[i]) {
        GST_WARNING("failed to allocate color %2d : %04x,%04x,%04x",i,self->priv->colors[i].red,self->priv->colors[i].green,self->priv->colors[i].blue);
      }
    }
  }

  return(TRUE);
}

static gboolean bt_ui_ressources_init_icons(BtUIRessources *self) {
  /*
  self->priv->source_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_source_machine.png");
  self->priv->processor_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_processor_machine.png");
  self->priv->sink_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_sink_machine.png");
  */
  self->priv->source_machine_pixbuf=gdk_pixbuf_new_from_theme("menu_source_machine",GTK_ICON_SIZE_MENU);
  self->priv->processor_machine_pixbuf=gdk_pixbuf_new_from_theme("menu_processor_machine",GTK_ICON_SIZE_MENU);
  self->priv->sink_machine_pixbuf=gdk_pixbuf_new_from_theme("menu_sink_machine",GTK_ICON_SIZE_MENU);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_ui_ressources_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtUIRessources *bt_ui_ressources_new(void) {
  if(!singleton) {
    // create singleton
    if(!(singleton=(gpointer)(g_object_new(BT_TYPE_UI_RESSOURCES,NULL)))) {
      goto Error;
    }
    // initialise ressources
    if(!bt_ui_ressources_init_colors(BT_UI_RESSOURCES(singleton))) {
      goto Error;
    }
    if(!bt_ui_ressources_init_icons(BT_UI_RESSOURCES(singleton))) {
      goto Error;
    }
    BT_UI_RESSOURCES(singleton)->priv->accel_group=gtk_accel_group_new();

    g_object_add_weak_pointer(G_OBJECT(singleton),&singleton);
  }
  else {
    singleton=g_object_ref(G_OBJECT(singleton));
  } 
  return(BT_UI_RESSOURCES(singleton));
Error:
  g_object_try_unref(G_OBJECT(singleton));
  return(NULL);
}

//-- methods

/**
 * bt_ui_ressources_get_pixbuf_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GdkPixbuf image that matches the given machine type.
 *
 * Returns: a #GdkPixbuf image
 */
GdkPixbuf *bt_ui_ressources_get_pixbuf_by_machine(const BtMachine *machine) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);

  if(BT_IS_SOURCE_MACHINE(machine)) {
    return(g_object_ref(ui_ressources->priv->source_machine_pixbuf));
  }
  else if(BT_IS_PROCESSOR_MACHINE(machine)) {
    return(g_object_ref(ui_ressources->priv->processor_machine_pixbuf));
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
    return(g_object_ref(ui_ressources->priv->sink_machine_pixbuf));
  }
  return(NULL);
}

/**
 * bt_ui_ressources_get_image_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *bt_ui_ressources_get_image_by_machine(const BtMachine *machine) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);

  if(BT_IS_SOURCE_MACHINE(machine)) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->source_machine_pixbuf));
  }
  else if(BT_IS_PROCESSOR_MACHINE(machine)) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->processor_machine_pixbuf));
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->sink_machine_pixbuf));
  }
  return(NULL);
}

/**
 * bt_ui_ressources_get_image_by_machine_type:
 * @machine_type: the machine_type to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *bt_ui_ressources_get_image_by_machine_type(GType machine_type) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);

  if(machine_type==BT_TYPE_SOURCE_MACHINE) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->source_machine_pixbuf));
  }
  else if(machine_type==BT_TYPE_PROCESSOR_MACHINE) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->processor_machine_pixbuf));
  }
  else if(machine_type==BT_TYPE_SINK_MACHINE) {
    return(gtk_image_new_from_pixbuf(ui_ressources->priv->sink_machine_pixbuf));
  }
  return(NULL);
}

/**
 * bt_ui_ressources_get_gdk_color:
 * @color_type: the color id
 *
 * Gets a prealocated color by id.
 *
 * Returns: the requested #GdkColor.
 */
GdkColor *bt_ui_ressources_get_gdk_color(BtUIRessourcesColors color_type) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);
  return(&ui_ressources->priv->colors[color_type]);
}

/**
 * bt_ui_ressources_get_color_by_machine:
 * @machine: the machine to get the color for
 * @color_type: a color shade
 *
 * Gets a colors shade depending on machine type in rgba format.
 *
 * Returns: a color depending on machine class and color_type
 */
guint32 bt_ui_ressources_get_color_by_machine(const BtMachine *machine,BtUIRessourcesMachineColors color_type) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);
  gulong ix=0;
  guint32 color=0;
  
  if(BT_IS_SOURCE_MACHINE(machine)) {
    ix=BT_UI_RES_COLOR_SOURCE_MACHINE_BASE+color_type;
  }
  else if(BT_IS_PROCESSOR_MACHINE(machine)) {
    ix=BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE+color_type;
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
    ix=BT_UI_RES_COLOR_SINK_MACHINE_BASE+color_type;
  }
  color=(((guint32)(ui_ressources->priv->colors[ix].red&0xFF00))<<16)|
    (((guint32)(ui_ressources->priv->colors[ix].green&0xFF00))<<8)|
    ((guint32)(ui_ressources->priv->colors[ix].blue&0xFF00))|
    0x000000FF;
  //GST_INFO("color[%2d/%1d] : 0x%08lx : %04x,%04x,%04x",ix,color_type,color,
  //  ui_ressources->priv->colors[ix].red,ui_ressources->priv->colors[ix].green,ui_ressources->priv->colors[ix].blue);
  return(color);
}

/**
 * bt_ui_ressources_get_accel_group:
 *
 * All windows share one accelerator map.
 *
 * Returns: the shared keyboard accelerator map
 */
GtkAccelGroup *bt_ui_ressources_get_accel_group(void) {
  BtUIRessources *ui_ressources=BT_UI_RESSOURCES(singleton);
  
  return(ui_ressources->priv->accel_group);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_ui_ressources_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  return_if_disposed();
  switch (property_id) {
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_ui_ressources_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_ui_ressources_dispose(GObject *object) {
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  GST_DEBUG("  pb->ref_counts: %d, %d, %d", 
    G_OBJECT(self->priv->source_machine_pixbuf)->ref_count,
    G_OBJECT(self->priv->processor_machine_pixbuf)->ref_count,
    G_OBJECT(self->priv->sink_machine_pixbuf)->ref_count);
  g_object_try_unref(self->priv->source_machine_pixbuf);
  g_object_try_unref(self->priv->processor_machine_pixbuf);
  g_object_try_unref(self->priv->sink_machine_pixbuf);
  
  g_object_try_unref(self->priv->accel_group);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_ui_ressources_finalize(GObject *object) {
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  
  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_ui_ressources_init(GTypeInstance *instance, gpointer g_class) {
  BtUIRessources *self = BT_UI_RESSOURCES(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_UI_RESSOURCES, BtUIRessourcesPrivate);
}

static void bt_ui_ressources_class_init(BtUIRessourcesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtUIRessourcesPrivate));

  gobject_class->set_property = bt_ui_ressources_set_property;
  gobject_class->get_property = bt_ui_ressources_get_property;
  gobject_class->dispose      = bt_ui_ressources_dispose;
  gobject_class->finalize     = bt_ui_ressources_finalize;
}

GType bt_ui_ressources_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtUIRessourcesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_ui_ressources_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtUIRessources),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_ui_ressources_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtUIRessources",&info,0);
  }
  return type;
}
