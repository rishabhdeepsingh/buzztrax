// $Id: main-toolbar.c,v 1.56 2005-07-26 06:44:38 waffel Exp $
/**
 * SECTION:btmaintoolbar
 * @short_description: class for the editor main toolbar
 */ 

/* @todo: should we have separate the tolbars into several ones?
 * - common - load, save, ...
 * - volume - gain, levels
 * - load - cpu load
 */
#define BT_EDIT
#define BT_MAIN_TOOLBAR_C

#include "bt-edit.h"
#include "gtkvumeter.h"

enum {
  MAIN_TOOLBAR_APP=1,
};

#define MAX_VUMETER 4

struct _BtMainToolbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
  
	/* the toolbar widget */
	GtkWidget *toolbar;
	
  /* the level meters */
  GtkVUMeter *vumeter[MAX_VUMETER];
	
	/* the volume gain */
	GtkScale *volume;
	GstElement *gain;

	/* action buttons */
	GtkWidget *save_button;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	
	/* player variables */
	GThread* player_thread;
};

static GtkHandleBoxClass *parent_class=NULL;

//-- helper

static gint gst_caps_get_channels(GstCaps *caps) {
	GstStructure *structure;
	gint channels=0,size,i;

	g_assert(caps);
	
	if(GST_CAPS_IS_SIMPLE(caps)) {
		if((structure=gst_caps_get_structure(caps,0))) {
			gst_structure_get_int(structure,"channels",&channels);
			GST_DEBUG("---    simple caps with channels=%d",channels);
		}
	}
	else {
		size=gst_caps_get_size(caps);
		for(i=0;i<size;i++) {
			if((structure=gst_caps_get_structure(caps,i))) {
				gst_structure_get_int(structure,"channels",&channels);
				GST_DEBUG("---    caps %d with channels=%d",i,channels);
			}
		}
	}
	return(channels);
}

//-- event handler

static void on_song_stop(const BtSong *song, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
	gint i;

  g_assert(user_data);
  
  GST_INFO("song stop event occured");
	gdk_threads_try_enter();
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->play_button),FALSE);
	// reset level meters
	for(i=0;i<MAX_VUMETER;i++) {
    gtk_vumeter_set_levels(self->priv->vumeter[i], -900, -900);
	}
	// disable stop button
	gtk_widget_set_sensitive(GTK_WIDGET(self->priv->stop_button),FALSE);
	gdk_threads_try_leave();
	GST_INFO("song stop event handled");
}

static void on_toolbar_new_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar new event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_new_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_open_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_open_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_save_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_play_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);

  g_assert(user_data);

  if(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button))) {
    BtSong *song;
    GError *error=NULL;

    GST_INFO("toolbar play event occurred");
    // get song from app
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

    //-- start playing in a thread
    if(!(self->priv->player_thread=g_thread_create((GThreadFunc)&bt_song_play, (gpointer)song, TRUE, &error))) {
      GST_ERROR("error creating player thread : \"%s\"", error->message);
      g_error_free(error);
    }
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->stop_button),TRUE);
    // release the reference
    g_object_try_unref(song);
  }
}

static void on_toolbar_stop_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("toolbar stop event occurred");
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  bt_song_stop(song);
	//g_thread_join(self->priv->player_thread);
	//self->priv->player_thread=NULL;
  // release the reference
  g_object_try_unref(song);
}

static void on_toolbar_loop_toggled(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSequence *sequence;
  gboolean loop;

  g_assert(user_data);

  loop=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));
  GST_INFO("toolbar loop toggle event occurred, new-state=%d",loop);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  g_object_set(G_OBJECT(sequence),"loop",loop,NULL);
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
}

static void on_song_level_change(GstElement *element, gdouble time, gint channel, gdouble rms, gdouble peak, gdouble decay, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  
  g_assert(user_data);

  //GST_INFO("%d  %.3f  %.3f %.3f %.3f", channel,time,rms,peak,decay);
	gdk_threads_try_enter();// do we need this here?, input level is running in a thread and sending this event -> means yes
	gtk_vumeter_set_levels(self->priv->vumeter[channel], (gint)(rms*10.0), (gint)(peak*10.0));
	gdk_threads_try_leave();
}

static void on_song_volume_change(GtkRange *range,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  
  g_assert(user_data);
	g_assert(self->priv->gain);
	// get value from HScale and change volume
	GST_INFO("volume has changed : %f",gtk_range_get_value(GTK_RANGE(self->priv->volume)));
	g_object_set(self->priv->gain,"volume",gtk_range_get_value(GTK_RANGE(self->priv->volume)),NULL);
}

static void on_channels_negotiated(GstPad *pad,GParamSpec *arg,gpointer user_data) {
	BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
	GstCaps *caps;
	gint i,channels;
	
  g_assert(user_data);
	if((caps=(GstCaps *)gst_pad_get_negotiated_caps(pad))) {
		channels=gst_caps_get_channels(caps);
		GST_INFO("!!!  input level src has %d output channels",channels);

		gdk_threads_try_enter();
		for(i=0;i<channels;i++) {
			gtk_widget_show(GTK_WIDGET(self->priv->vumeter[i]));
		}
		for(i=channels;i<MAX_VUMETER;i++) {
			gtk_widget_hide(GTK_WIDGET(self->priv->vumeter[i]));
		}
		gdk_threads_try_leave();
	}
}

static void on_song_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
	gboolean unsaved;

	g_assert(user_data);
	
	GST_INFO("song.unsaved has changed : song=%p, toolbar=%p",song,user_data);
	
  g_object_get(G_OBJECT(song),"unsaved",&unsaved,NULL);
	gtk_widget_set_sensitive(self->priv->save_button,unsaved);
}


static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSinkMachine *master;
  GstElement *level;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);
  
  // get the audio_sink (song->master is a bt_sink_machine) if there is one already
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
	g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"master",&master,NULL);
	if(master) {
		GstPad *pad;
		gdouble volume;

		// get the input_level property from audio_sink
		g_object_get(G_OBJECT(master),"input-level",&level,"input-gain",&self->priv->gain,NULL);
		// connect to the level signal
		g_signal_connect(level, "level", G_CALLBACK(on_song_level_change), self);
		// get the pad from the input-level and listen there for channel negotiation
		if((pad=gst_element_get_pad(level,"src"))) {
			g_signal_connect(pad,"notify::caps",G_CALLBACK(on_channels_negotiated),(gpointer)self);
		}
		// release the references
		g_object_try_unref(level);
		
		// get the current input_gain and adjust volume widget
		g_object_get(self->priv->gain,"volume",&volume,NULL);
		gtk_range_set_value(GTK_RANGE(self->priv->volume),volume);
		// connect volumne event
		g_signal_connect(G_OBJECT(self->priv->volume),"value_changed",G_CALLBACK(on_song_volume_change),self);
	}
	g_signal_connect(G_OBJECT(song),"stop",G_CALLBACK(on_song_stop),(gpointer)self);
	on_song_unsaved_changed(song,NULL,self);
	g_signal_connect(G_OBJECT(song), "notify::unsaved", G_CALLBACK(on_song_unsaved_changed), (gpointer)self);
  g_object_try_unref(master);
  g_object_try_unref(song);

}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
	gchar *toolbar_style;
	
	g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
	
	GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
	gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
	g_free(toolbar_style);
}

//-- helper methods

static gboolean bt_main_toolbar_init_ui(const BtMainToolbar *self) {
	BtSettings *settings;
  GtkWidget *tool_item;
  GtkTooltips *tips;
  GtkWidget *box;
  gulong i;
   
  tips=gtk_tooltips_new();

  gtk_widget_set_name(GTK_WIDGET(self),_("handlebox for toolbar"));
	
  self->priv->toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->toolbar,_("tool bar"));
	
  //-- file controls

	tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
  gtk_widget_set_name(tool_item,_("New"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Prepare a new empty song"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
	g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_new_clicked),(gpointer)self);

	tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_OPEN));
  gtk_widget_set_name(tool_item,_("Open"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Load a new song"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);

	tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_SAVE));
  gtk_widget_set_name(tool_item,_("Save"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Save this song"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
	g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_save_clicked),(gpointer)self);
	self->priv->save_button=tool_item;

	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  //-- media controls
  
	tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name(tool_item,_("Play"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Play this song"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
	self->priv->play_button=tool_item;

	tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP));
  gtk_widget_set_name(tool_item,_("Stop"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Stop playback of this song"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
	gtk_widget_set_sensitive(tool_item,FALSE);
	self->priv->stop_button=tool_item;

	tool_item=GTK_WIDGET(gtk_toggle_tool_button_new());
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(tool_item),gtk_image_new_from_filename("stock_repeat.png"));
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item),_("Loop"));
  gtk_widget_set_name(tool_item,_("Loop"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Toggle looping of playback"),NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"toggled",G_CALLBACK(on_toolbar_loop_toggled),(gpointer)self);

	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);
  
  //-- volume level and control

	box=gtk_vbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(box),2);
  // add gtk_vumeter widgets and update from level_callback
  for(i=0;i<MAX_VUMETER;i++) {
    self->priv->vumeter[i]=GTK_VUMETER(gtk_vumeter_new(FALSE));
		// @idea have distinct tooltips
		gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),GTK_WIDGET(self->priv->vumeter[i]),_("playback volume"),NULL);
    gtk_vumeter_set_min_max(self->priv->vumeter[i], -900, 0);
    gtk_vumeter_set_scale(self->priv->vumeter[i], GTK_VUMETER_SCALE_LOG);
    gtk_vumeter_set_levels(self->priv->vumeter[i], -900, -900);
    gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->vumeter[i]),TRUE,TRUE,0);
  }

  gtk_widget_set_size_request(GTK_WIDGET(box),150,-1);
  // add gain-control
	self->priv->volume=GTK_SCALE(gtk_hscale_new_with_range(/*min=*/0.0,/*max=*/1.0,/*step=*/0.01));
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),GTK_WIDGET(self->priv->volume),_("Change playback volume"),NULL);
	gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->volume),TRUE,TRUE,0);
	gtk_scale_set_draw_value(self->priv->volume,FALSE);
	gtk_range_set_update_policy(GTK_RANGE(self->priv->volume),GTK_UPDATE_DELAYED);

	tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Volume"));
	gtk_container_add(GTK_CONTAINER(tool_item),box);
	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);

	gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  gtk_container_add(GTK_CONTAINER(self),self->priv->toolbar);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

	// let settings control toolbar style
	g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
	on_toolbar_style_changed(settings,NULL,(gpointer)self);
	g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);
	g_object_unref(settings);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_toolbar_new:
 * @app: the application the toolbar belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainToolbar *bt_main_toolbar_new(const BtEditApplication *app) {
  BtMainToolbar *self;

  if(!(self=BT_MAIN_TOOLBAR(g_object_new(BT_TYPE_MAIN_TOOLBAR,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_toolbar_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_toolbar_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_toolbar_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_toolbar: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_toolbar_dispose(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	
  g_object_try_weak_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_toolbar_finalize(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_toolbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(instance);
  self->priv = g_new0(BtMainToolbarPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_toolbar_class_init(BtMainToolbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_HANDLE_BOX);
  
  gobject_class->set_property = bt_main_toolbar_set_property;
  gobject_class->get_property = bt_main_toolbar_get_property;
  gobject_class->dispose      = bt_main_toolbar_dispose;
  gobject_class->finalize     = bt_main_toolbar_finalize;

  g_object_class_install_property(gobject_class,MAIN_TOOLBAR_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_toolbar_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainToolbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_toolbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainToolbar),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_toolbar_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_HANDLE_BOX,"BtMainToolbar",&info,0);
  }
  return type;
}
