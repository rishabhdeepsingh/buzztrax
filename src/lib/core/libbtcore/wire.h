/* $Id: wire.h,v 1.14 2006-07-22 15:37:06 ensonic Exp $
 * class for a machine to machine connection
 */

#ifndef BT_WIRE_H
#define BT_WIRE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE            (bt_wire_get_type ())
#define BT_WIRE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE, BtWire))
#define BT_WIRE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE, BtWireClass))
#define BT_IS_WIRE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE))
#define BT_IS_WIRE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE))
#define BT_WIRE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE, BtWireClass))

/* type macros */

typedef struct _BtWire BtWire;
typedef struct _BtWireClass BtWireClass;
typedef struct _BtWirePrivate BtWirePrivate;

/**
 * BtWire:
 *
 * connects two #BtMachine instances
 */
struct _BtWire {
  GObject parent;
  
  /*< private >*/
  BtWirePrivate *priv;
};

/* structure of the wire class */
struct _BtWireClass {
  GObjectClass parent;
};

/* used by WIRE_TYPE */
GType bt_wire_get_type(void) G_GNUC_CONST;

#endif // BT_WIRE_H
