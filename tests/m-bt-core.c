/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * core library unit tests
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

#define BT_CHECK

#include "bt-check.h"
#include "../src/lib/core/libbuzztard-core/core.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);

extern Suite *bt_core_suite(void);
extern Suite *bt_machine_suite(void);
extern Suite *bt_network_suite(void);
extern Suite *bt_pattern_suite(void);
extern Suite *bt_processor_machine_suite(void);
extern Suite *bt_sequence_suite(void);
extern Suite *bt_setup_suite(void);
extern Suite *bt_sink_machine_suite(void);
extern Suite *bt_song_suite(void);
extern Suite *bt_song_io_suite(void);
extern Suite *bt_song_io_native_suite(void);
extern Suite *bt_song_info_suite(void);
extern Suite *bt_source_machine_suite(void);
extern Suite *bt_wire_suite(void);
extern Suite *bt_wire_pattern_suite(void);
extern Suite *bt_settings_suite(void);

gchar *test_argv[] = { "check_buzzard" };
gchar **test_argvptr = test_argv;
gint test_argc=G_N_ELEMENTS(test_argv) - 1;

/* common setup and teardown code */
void bt_core_setup(void) {
}

void bt_core_teardown(void) {
}

/* start the test run */
int main(int argc, char **argv) {
  int nf;
  SRunner *sr;

#if !GLIB_CHECK_VERSION (2, 31, 0) 
  // initialize as soon as possible
  if(!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

  g_type_init();
  setup_log(argc,argv);
  setup_log_capture();

  bt_init(&test_argc,&test_argvptr);
  bt_check_init();
  
  // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_LOG);
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);

  sr=srunner_create(bt_core_suite());
  srunner_add_suite(sr, bt_machine_suite());
  srunner_add_suite(sr, bt_network_suite());
  srunner_add_suite(sr, bt_pattern_suite());
  srunner_add_suite(sr, bt_processor_machine_suite());
  srunner_add_suite(sr, bt_sequence_suite());
  srunner_add_suite(sr, bt_setup_suite());
  srunner_add_suite(sr, bt_sink_machine_suite());
  srunner_add_suite(sr, bt_song_suite());
  srunner_add_suite(sr, bt_song_io_suite());
  srunner_add_suite(sr, bt_song_io_native_suite());
  srunner_add_suite(sr, bt_song_info_suite());
  srunner_add_suite(sr, bt_source_machine_suite());
  srunner_add_suite(sr, bt_wire_suite());
  srunner_add_suite(sr, bt_wire_pattern_suite());
  srunner_add_suite(sr, bt_settings_suite());
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  bt_deinit();
  //g_mem_profile();

  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
