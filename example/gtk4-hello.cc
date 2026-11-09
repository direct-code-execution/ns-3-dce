/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Minimal GTK4 application built as a DCE application: shows a window with
// a label for <seconds> (default 5) and exits with 0. Run by dce-x11-hello
// (--binary=gtk4-hello) to exercise GLib, GObject, GIO and GTK4 inside DCE:
// their event loops (eventfd, poll), threads, futex based locks and the X11
// connection through the host socket passthrough. Needs GSK_RENDERER=cairo
// (no OpenGL in the simulation) and GDK_BACKEND=x11.
//
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
static gboolean quit_cb (gpointer app) { g_application_quit (G_APPLICATION (app)); return G_SOURCE_REMOVE; }
static void activate (GtkApplication *app, gpointer seconds)
{
  GtkWidget *win = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (win), "GTK4 inside ns-3 DCE");
  gtk_window_set_default_size (GTK_WINDOW (win), 400, 200);
  GtkWidget *label = gtk_label_new ("Hello from a GTK4 process running inside the ns-3 simulation");
  gtk_window_set_child (GTK_WINDOW (win), label);
  gtk_window_present (GTK_WINDOW (win));
  g_timeout_add_seconds (GPOINTER_TO_INT (seconds), quit_cb, app);
}
int main (int argc, char **argv)
{
  int seconds = argc > 1 ? atoi (argv[1]) : 5;
  GtkApplication *app = gtk_application_new (NULL, G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), GINT_TO_POINTER (seconds));
  int status = g_application_run (G_APPLICATION (app), 1, argv);
  g_object_unref (app);
  printf ("gtk4-hello: done, status %d\n", status);
  return status;
}
