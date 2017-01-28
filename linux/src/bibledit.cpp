/*
** Copyright (©) 2003-2015 Teus Benschop.
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**  
*/


#include <config.h>
#include <src/bibledit.h>
#include <libgen.h>
#include <iostream>
#include <webkit2/webkit2.h>


int bibledit_window_width = 0;
int bibledit_window_height = 0;
bool bibledit_window_maximized = false;
bool bibledit_window_fullscreen = false;
const char * bibledit_window_state = "WindowState";
const char * bibledit_window_width_key = "Width";
const char * bibledit_window_height_key = "Height";
const char * bibledit_window_maximized_key = "Maximized";
const char * bibledit_window_fullscreen_key = "Fullscreen";
const char * bibledit_window_state_ini = "state.ini";


int main (int argc, char *argv[])
{
  application = gtk_application_new ("org.bibledit.linux", G_APPLICATION_FLAGS_NONE);

  g_signal_connect (application, "activate", G_CALLBACK (activate), NULL);

  // Derive the webroot from the $HOME environment variable.
  string webroot;
  const char * home = g_getenv ("HOME");
  if (home) webroot = home;
  if (!webroot.empty ()) webroot.append ("/");
  webroot.append ("bibledit");
  
  // Read the package directory from config.h.
  string package (PACKAGE_DATA_DIR);

  status = g_application_run (G_APPLICATION (application), argc, argv);

  g_object_unref (application);
  
  return status;
}


void activate (GtkApplication *app)
{
  GList *list = gtk_application_get_windows (app);

  if (list) {
    // Activate existing live app.
    gtk_window_present (GTK_WINDOW (list->data));
    return;
  }

  // The top-level window.
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Bibledit");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_application (GTK_WINDOW (window), application);

  // The icon.
  gchar * iconfile = g_build_filename ("bibledit.xpm", NULL);
  if (!g_file_test (iconfile, G_FILE_TEST_EXISTS)) {
    iconfile = g_build_filename (PACKAGE_DATA_DIR, "bibledit.xpm", NULL);
  }
  gtk_window_set_default_icon_from_file (iconfile, NULL);
  g_free (iconfile);

  // Create a browser instance.
  WebKitWebView * webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  
  // Put the browser area into the main window.
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (webview));
  
  // Load a web page into the browser instance
  webkit_web_view_load_uri (webview, "http://bibledit.org:8080");
  
  // Ensure it will get mouse and keyboard events.
  gtk_widget_grab_focus (GTK_WIDGET (webview));

  // Set window state and size.
  const char *appid = g_application_get_application_id (g_application_get_default ());
  char *file = g_build_filename (g_get_user_cache_dir (), appid, bibledit_window_state_ini, NULL);
  GKeyFile *keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, file, G_KEY_FILE_NONE, NULL)) {
    bibledit_window_width = g_key_file_get_integer (keyfile, bibledit_window_state, bibledit_window_width_key, NULL);
    bibledit_window_height = g_key_file_get_integer (keyfile, bibledit_window_state, bibledit_window_height_key, NULL);
    bibledit_window_maximized = g_key_file_get_boolean (keyfile, bibledit_window_state, bibledit_window_maximized_key, NULL);
    bibledit_window_fullscreen = g_key_file_get_boolean (keyfile, bibledit_window_state, bibledit_window_fullscreen_key, NULL);
    gtk_window_set_default_size (GTK_WINDOW (window), bibledit_window_width, bibledit_window_height);
    if (bibledit_window_maximized) gtk_window_maximize (GTK_WINDOW (window));
    if (bibledit_window_fullscreen) gtk_window_fullscreen (GTK_WINDOW (window));
  }
  g_key_file_unref (keyfile);
  g_free (file);
  
  // Signal handlers.
  g_signal_connect (window, "size-allocate", G_CALLBACK (on_window_size_allocate), NULL);
  g_signal_connect (window, "window-state-event", G_CALLBACK (on_window_state_event), NULL);
  g_signal_connect (window, "destroy", G_CALLBACK (on_signal_destroy), NULL);
  g_signal_connect (webview, "key-press-event", G_CALLBACK (on_key_press), NULL);

  // Make sure the main window and all its contents are visible
  gtk_widget_show_all (window);

  // Run the main GTK+ event loop.
  gtk_main();
}


void on_signal_destroy (gpointer user_data)
{
  (void) user_data;

  GKeyFile *keyfile = g_key_file_new ();
  
  g_key_file_set_integer (keyfile, bibledit_window_state, bibledit_window_width_key, bibledit_window_width);
  g_key_file_set_integer (keyfile, bibledit_window_state, bibledit_window_height_key, bibledit_window_height);
  g_key_file_set_boolean (keyfile, bibledit_window_state, bibledit_window_maximized_key, bibledit_window_maximized);
  g_key_file_set_boolean (keyfile, bibledit_window_state, bibledit_window_fullscreen_key, bibledit_window_fullscreen);

  const char *appid = g_application_get_application_id (g_application_get_default ());
  char *path = g_build_filename (g_get_user_cache_dir (), appid, NULL);
  g_mkdir_with_parents (path, 0700);
  char *file = g_build_filename (path, bibledit_window_state_ini, NULL);
  g_key_file_save_to_file (keyfile, file, NULL);
  
  g_free (file);
  g_key_file_unref (keyfile);
  g_free (path);
  
  gtk_main_quit ();
}


gboolean on_key_press (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  if (event->type == GDK_KEY_PRESS) {
    GdkEventKey * event_key = (GdkEventKey *) event;
    if ((event_key->keyval == GDK_KEY_z) || (event_key->keyval == GDK_KEY_Z)  ) {
      if (event_key->state & GDK_CONTROL_MASK) {
        const gchar *command;
        if (event_key->state & GDK_SHIFT_MASK) {
          command = WEBKIT_EDITING_COMMAND_REDO;
        } else {
          command = WEBKIT_EDITING_COMMAND_UNDO;
        }
        WebKitWebView * web_view = WEBKIT_WEB_VIEW (widget);
        webkit_web_view_execute_editing_command (web_view, command);
        return true;
      }
    }
  }
  (void) data;
  return false;
}


void on_window_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  // Save the window geometry only if the window is not maximized or fullscreen.
  if (!(bibledit_window_maximized || bibledit_window_fullscreen)) {
    gtk_window_get_size (GTK_WINDOW (widget), &bibledit_window_width, &bibledit_window_height);
  }
}


gboolean on_window_state_event (GtkWidget *widget, GdkEventWindowState *event)
{
  (void) widget;
  bibledit_window_maximized = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
  bibledit_window_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
  return true;
}
