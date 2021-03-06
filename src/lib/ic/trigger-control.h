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

#ifndef BTIC_TRIGGER_CONTROL_H
#define BTIC_TRIGGER_CONTROL_H

#include <glib.h>
#include <glib-object.h>

#include "control.h"
#include "device.h"

/* type macros */

#define BTIC_TYPE_TRIGGER_CONTROL            (btic_trigger_control_get_type ())
#define BTIC_TRIGGER_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_TRIGGER_CONTROL, BtIcTriggerControl))
#define BTIC_TRIGGER_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_TRIGGER_CONTROL, BtIcTriggerControlClass))
#define BTIC_IS_TRIGGER_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_TRIGGER_CONTROL))
#define BTIC_IS_TRIGGER_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_TRIGGER_CONTROL))
#define BTIC_TRIGGER_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_TRIGGER_CONTROL, BtIcTriggerControlClass))

typedef struct _BtIcTriggerControl BtIcTriggerControl;
typedef struct _BtIcTriggerControlClass BtIcTriggerControlClass;
typedef struct _BtIcTriggerControlPrivate BtIcTriggerControlPrivate;

/**
 * BtIcTriggerControl:
 *
 * buzztraxs interaction controller single trigger control
 */
struct _BtIcTriggerControl {
  const BtIcControl parent;

  /*< private >*/
  BtIcTriggerControlPrivate *priv;
};

struct _BtIcTriggerControlClass {
  const BtIcControlClass parent;

};

GType btic_trigger_control_get_type(void) G_GNUC_CONST;

BtIcTriggerControl *btic_trigger_control_new(const BtIcDevice *device,const gchar *name,guint id);

#endif // BTIC_TRIGGER_CONTROL_H
