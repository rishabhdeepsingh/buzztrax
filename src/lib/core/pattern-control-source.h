/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_PATTERN_CONTROL_SOURCE_H
#define BT_PATTERN_CONTROL_SOURCE_H

#include <glib.h>
#include <glib-object.h>
#include <gst/controller/gstcontrolsource.h>

#include "sequence.h"

#define BT_TYPE_PATTERN_CONTROL_SOURCE						(bt_pattern_control_source_get_type ())
#define BT_PATTERN_CONTROL_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSource))
#define BT_PATTERN_CONTROL_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSourceClass))
#define BT_IS_PATTERN_CONTROL_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE))
#define BT_IS_PATTERN_CONTROL_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN_CONTROL_SOURCE))
#define BT_PATTERN_CONTROL_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSourceClass))

/* type macros */

typedef struct _BtPatternControlSource BtPatternControlSource;
typedef struct _BtPatternControlSourceClass BtPatternControlSourceClass;
typedef struct _BtPatternControlSourcePrivate BtPatternControlSourcePrivate;

/**
 * BtPatternControlSource:
 *
 * A pattern based control source
 */
struct _BtPatternControlSource {
  const GstControlSource parent;

  /*< private > */
  BtPatternControlSourcePrivate *priv;
};

struct _BtPatternControlSourceClass {
  const GstControlSourceClass parent;
};

GType bt_pattern_control_source_get_type (void) G_GNUC_CONST;

BtPatternControlSource *bt_pattern_control_source_new (BtSequence * sequence, const BtMachine * machine, BtParameterGroup * param_group);

#endif // BT_PATTERN_CONTROL_SOURCE_H
