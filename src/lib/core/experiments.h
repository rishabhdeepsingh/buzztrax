/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_EXPERIMENTS_H
#define BT_EXPERIMENTS_H

/**
 * BtExperimentFlags:
 * @BT_EXPERIMENT_AUDIO_MIXER: try audiomixer instead of adder
 *
 * Code experiemnts.
 */
typedef enum {
  BT_EXPERIMENT_AUDIO_MIXER = 1 << 0,
} BtExperimentFlags;

void bt_experiments_init(gchar **flags);
gboolean bt_experiments_check_active(BtExperimentFlags experiments);

#endif // BT_EXPERIMENTS_H
