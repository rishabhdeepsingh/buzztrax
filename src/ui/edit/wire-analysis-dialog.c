// $Id: wire-analysis-dialog.c,v 1.3 2006-07-22 15:37:06 ensonic Exp $
/**
 * SECTION:btwireanalysisdialog
 * @short_description: audio analysis for this wire
 */ 

#define BT_EDIT
#define BT_WIRE_ANALYSIS_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  WIRE_ANALYSIS_DIALOG_APP=1,
  WIRE_ANALYSIS_DIALOG_WIRE
};

/* @todo: add more later:
 * waveform (oszilloscope)
 * panorama (spacescope)
 */
typedef enum {
  ANALYZER_LEVEL=0,
  ANALYZER_SPECTRUM,
  // this can be 'mis'used for an aoszilloscope by connecting to hand-off
  ANALYZER_FAKESINK,
  // how many elements are used
  ANALYZER_COUNT
} BtWireAnalyzer;

#define SPECT_BANDS 256
#define SPECT_WIDTH (SPECT_BANDS)
#define SPECT_HEIGHT 64
#define LEVEL_WIDTH (SPECT_BANDS)
#define LEVEL_HEIGHT 16

struct _BtWireAnalysisDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the wire we analyze */
  BtWire *wire;
  
  /* the paint handler source id */
  guint paint_handler_id;
  /* the analyzer-graphs */
  GtkWidget *spectrum_drawingarea, *level_drawingarea;
  
    /* the gstreamer elements that is used */
  GstElement *analyzers[ANALYZER_COUNT];
  GList *analyzers_list;
  
  /* the analyzer results */
  gdouble rms[2], peak[2];
  guchar spect[SPECT_BANDS]; 
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static gboolean on_wire_analyzer_change(GstBus *bus, GstMessage *message, gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  gboolean res=FALSE;
  g_assert(user_data);
  
  switch(GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *structure=gst_message_get_structure(message);
      const gchar *name = gst_structure_get_name(structure);
      
      if(!strcmp(name,"level")) {
        const GValue *l_rms,*l_peak;
        guint i;
  
        GST_INFO("get level data");
        l_rms=(GValue *)gst_structure_get_value(structure, "rms");
        l_peak=(GValue *)gst_structure_get_value(structure, "peak");
        //l_decay=(GValue *)gst_structure_get_value(structure, "decay");
        for(i=0;i<gst_value_list_get_size(l_rms);i++) {
          self->priv->rms[i]=g_value_get_double(gst_value_list_get_value(l_rms,i));
          self->priv->peak[i]=g_value_get_double(gst_value_list_get_value(l_peak,i));
          //GST_INFO("level.%d  %.3f %.3f", i, self->priv->rms[i], self->priv->peak[i]);
        }
        res=TRUE;
      }
      else if(!strcmp(name,"spectrum")) {
        const GValue *list;
        const GValue *value;
        guint i;
  
        GST_INFO("get spectrum data");
        list = gst_structure_get_value (structure, "spectrum");
        // SPECT_BANDS=gst_value_list_get_size(list)
        for (i = 0; i < SPECT_BANDS; ++i) {
          value = gst_value_list_get_value (list, i);
          self->priv->spect[i] = g_value_get_uchar (value);
        }
      }
    } break;
    default:
      //GST_INFO("received bus message: type=%s",gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
      break;
  }
  return(res);
}

//-- helper methods

static gboolean on_wire_analyzer_redraw(gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  
  // TODO: draw
  if (self->priv->level_drawingarea) {
    GdkRectangle rect = { 0, 0, LEVEL_WIDTH, LEVEL_HEIGHT };
    GtkWidget *da=self->priv->level_drawingarea;
    
    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, LEVEL_WIDTH, LEVEL_HEIGHT);
    //gtk_vumeter_set_levels(self->priv->vumeter[i], (gint)(rms[i]*10.0), (gint)(peak[i]*10.0));
    gdk_window_end_paint (da->window);
  }

  // draw spectrum
  if (self->priv->spectrum_drawingarea) {
    gint i;
    GdkRectangle rect = { 0, 0, SPECT_WIDTH, SPECT_HEIGHT };
    GtkWidget *da=self->priv->spectrum_drawingarea;
    
    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, SPECT_BANDS, SPECT_HEIGHT);
    for (i = 0; i < SPECT_BANDS; i++) {
      gdk_draw_rectangle (da->window, da->style->white_gc, TRUE, i, SPECT_HEIGHT - self->priv->spect[i], 1, self->priv->spect[i]);
    }
    gdk_window_end_paint (da->window);
  }
  return(TRUE);
}

/*
 * bt_wire_analysis_dialog_make_element:
 * @self: the wire-analysis dialog
 * @part: which analyzer element to create
 * @factory_name: the element-factories name (also used to build the elemnt name)
 *
 * Create analyzer elements. Store them in the analyzers array and link them into the list.
 */
static gboolean bt_wire_analysis_dialog_make_element(const BtWireAnalysisDialog *self,BtWireAnalyzer part, const gchar *factory_name) {
  gboolean res=FALSE;
  gchar *name;
  
  // add analyzer element
  name=g_alloca(strlen(factory_name)+16);g_sprintf(name,"%s_%p",factory_name,self->priv->wire);
  if(!(self->priv->analyzers[part]=gst_element_factory_make(factory_name,name))) {
    GST_ERROR("failed to create %s",factory_name);goto Error;
  }
  self->priv->analyzers_list=g_list_prepend(self->priv->analyzers_list,self->priv->analyzers[part]);
  res=TRUE;
Error:
  return(res);
}


static gboolean bt_wire_analysis_dialog_init_ui(const BtWireAnalysisDialog *self) {
  BtMainWindow *main_window;
  BtMachine *src_machine,*dst_machine;
  gchar *src_id,*dst_id,*title;
  //GdkPixbuf *window_icon=NULL;
  GtkWidget *vbox;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));
  
  /* @todo: create and set *proper* window icon (analyzer, scope)
  if((window_icon=bt_ui_ressources_get_pixbuf_by_wire(self->priv->wire))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  */
  
  // leave the choice of width to gtk
  gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
  // set a title
  g_object_get(self->priv->wire,"src",&src_machine,"dst",&dst_machine,NULL);
  g_object_get(src_machine,"id",&src_id,NULL);
  g_object_get(dst_machine,"id",&dst_id,NULL);
  title=g_strdup_printf(_("%s -> %s analysis"),src_id,dst_id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(src_id);g_free(dst_id);g_free(title);
  g_object_try_unref(src_machine);
  g_object_try_unref(dst_machine);
  
  vbox = gtk_vbox_new(FALSE, 6);
  
  /* @todo: add scales for spectrum analyzer drawable */
  self->priv->spectrum_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->spectrum_drawingarea, SPECT_WIDTH, SPECT_HEIGHT);
  gtk_container_add (GTK_CONTAINER (vbox), self->priv->spectrum_drawingarea);

  /* @todo: add big level meter (with scales)
   * @todo: if stereo add pan-meter (L-R, R-L)
   */
  self->priv->level_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->level_drawingarea, LEVEL_WIDTH, LEVEL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (vbox), self->priv->level_drawingarea);

  gtk_container_add (GTK_CONTAINER (self), vbox);
   
  // create fakesink
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_FAKESINK,"fakesink")) return(FALSE);
  // create spectrum analyzer
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_SPECTRUM,"spectrum")) return(FALSE);
  g_object_set (G_OBJECT(self->priv->analyzers[ANALYZER_SPECTRUM]),
      "bands", SPECT_BANDS, "threshold", -80, "message", TRUE,
      NULL);
  // create level meter
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_LEVEL,"level")) return(FALSE);
  g_object_set(G_OBJECT(self->priv->analyzers[ANALYZER_LEVEL]),
      "interval",(GstClockTime)(0.1*GST_SECOND),"message",TRUE,
      "peak-ttl",(GstClockTime)(0.2*GST_SECOND),"peak-falloff", 20.0,
      NULL);
  
  g_object_set(G_OBJECT(self->priv->wire),"analyzers",self->priv->analyzers_list,NULL);
   
  bt_application_add_bus_watch(BT_APPLICATION(self->priv->app),on_wire_analyzer_change,(gpointer)self);
  
  // add idle-handler that redraws gfx
  self->priv->paint_handler_id=g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_wire_analyzer_redraw,(gpointer)self,NULL);
  
  g_object_try_unref(main_window);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_wire_analysis_dialog_new:
 * @app: the application the dialog belongs to
 * @wire: the wire to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtWireAnalysisDialog *bt_wire_analysis_dialog_new(const BtEditApplication *app,const BtWire *wire) {
  BtWireAnalysisDialog *self;

  if(!(self=BT_WIRE_ANALYSIS_DIALOG(g_object_new(BT_TYPE_WIRE_ANALYSIS_DIALOG,"app",app,"wire",wire,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_wire_analysis_dialog_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  GST_DEBUG("dialog created and shown");
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_analysis_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_ANALYSIS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case WIRE_ANALYSIS_DIALOG_WIRE: {
      g_value_set_object(value, self->priv->wire);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given preferences for this object */
static void bt_wire_analysis_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_ANALYSIS_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case WIRE_ANALYSIS_DIALOG_WIRE: {
      g_object_try_unref(self->priv->wire);
      self->priv->wire = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_analysis_dialog_dispose(GObject *object) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  g_source_remove(self->priv->paint_handler_id);
  bt_application_remove_bus_watch(BT_APPLICATION(self->priv->app),on_wire_analyzer_change);

  // this destroys the analyzers too  
  g_object_set(G_OBJECT(self->priv->wire),"analyzers",NULL,NULL);
    
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->wire);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wire_analysis_dialog_finalize(GObject *object) {
  //BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wire_analysis_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialogPrivate);
}

static void bt_wire_analysis_dialog_class_init(BtWireAnalysisDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWireAnalysisDialogPrivate));
  
  gobject_class->set_property = bt_wire_analysis_dialog_set_property;
  gobject_class->get_property = bt_wire_analysis_dialog_get_property;
  gobject_class->dispose      = bt_wire_analysis_dialog_dispose;
  gobject_class->finalize     = bt_wire_analysis_dialog_finalize;

  g_object_class_install_property(gobject_class,WIRE_ANALYSIS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WIRE_ANALYSIS_DIALOG_WIRE,
                                  g_param_spec_object("wire",
                                     "wire construct prop",
                                     "Set wire object, the dialog handles",
                                     BT_TYPE_WIRE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_wire_analysis_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireAnalysisDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_analysis_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWireAnalysisDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_analysis_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtWireAnalysisDialog",&info,0);
  }
  return type;
}
