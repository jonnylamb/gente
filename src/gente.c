/* main gente source file.
 *
 * Copyright (C) 2014 Jonny Lamb
 *
 * Massively based on the work done by the authors' of
 * gnome-control-center and its timezone dialog.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <unistd.h>
#include <sys/types.h>

#include <act/act.h>
#include <gtk/gtk.h>

#include "timedated.h"
#include "gente-resources.h"

#include "cc-timezone-map.h"

#define FILENAME "data.txt"
#define SECTION "people"

enum {
  COL_NAME,
  COL_TZ,
  NUM_COLS
};

typedef struct
{
  CcTimezoneMap *map;
  GtkWidget *button;
} PeopleData;

static void
load_people (GtkListStore *people)
{
  GKeyFile *keyfile;
  GError *error = NULL;
  gchar *path;
  gchar **names;

  keyfile = g_key_file_new ();

  path = g_build_filename (g_get_user_config_dir (), "gente", FILENAME, NULL);
  if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error))
    {
      g_debug ("Failed to load %s: %s", path, error->message);
      g_clear_error (&error);

      if (!g_key_file_load_from_file (keyfile, GENTE_DATA_DIR "/" FILENAME, G_KEY_FILE_NONE, &error))
        {
          g_error ("Failed to load " GENTE_DATA_DIR "/" FILENAME ": %s", error->message);
          g_clear_error (&error);
          g_free (path);
          return;
        }
    }
  g_free (path);

  names = g_key_file_get_keys (keyfile, SECTION, NULL, &error);
  if (names == NULL)
    {
      g_error ("Failed to load the list of people: %s", error->message);
      g_clear_error (&error);
      return;
    }

  for (; names != NULL && *names != NULL; names++)
    {
      gchar *tz = g_key_file_get_string (keyfile, SECTION, *names, &error);

      if (error != NULL)
        {
          g_error ("Failed to get timezone for '%s': %s", *names, error->message);
          g_clear_error (&error);
          continue;
        }

      if (g_quark_try_string (tz) == 0)
        {
          /* GQuarks should be created for every timezone name in tz.c. */
          g_warning ("Unsupported timezone '%s' for '%s'", tz, *names);
        }
      else
        {
          gtk_list_store_insert_with_values (people, NULL, 0,
              COL_NAME, *names,
              COL_TZ, tz,
              -1);
        }

      g_free (tz);
    }

  g_key_file_unref (keyfile);
}

static void
set_timezone (PeopleData *data,
    const gchar *name,
    const gchar *tz)
{
  GDateTime *now, *date;
  GTimeZone *timezone;
  gchar *str, *utc_label, *time_label;

  now = g_date_time_new_now_utc ();
  timezone = g_time_zone_new (tz);
  date = g_date_time_to_timezone (now, timezone);
  g_time_zone_unref (timezone);

  utc_label = g_date_time_format (date, "UTC%:::z");
  time_label = g_date_time_format (date, "%R");

  /* format the text */
  str = g_strdup_printf ("<b>%s</b>\n"
      "<small>%s (%s)</small>\n"
      "<b>%s</b>",
      name,
      g_date_time_get_timezone_abbreviation (date),
      utc_label,
      time_label);

  if (cc_timezone_map_set_timezone (data->map, tz))
    {
      cc_timezone_map_set_bubble_text (data->map, str);
    }
  else
    {
      cc_timezone_map_set_bubble_text (data->map, NULL);
    }

  g_date_time_unref (now);
  g_date_time_unref (date);
  g_free (str);
}

static void
act_user_ready_cb (ActUser *user,
    GParamSpec *pspec,
    PeopleData *data)
{
  Timedate1 *dtm;
  GError *error = NULL;

  const gchar *name, *tz;

  /* user's name */
  name = act_user_get_real_name (user);

  /* timezone */
  dtm = timedate1_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      "org.freedesktop.timedate1",
      "/org/freedesktop/timedate1",
      NULL,
      &error);

  if (dtm == NULL)
    {
      g_warning ("could not get proxy for DateTimeMechanism: %s", error->message);
      g_clear_error (&error);
      return;
    }

  tz = timedate1_get_timezone (dtm);

  if (tz == NULL)
    {
      g_warning ("Failed to get the current timezone");
      return;
    }

  /* finally set */
  set_timezone (data, name, tz);

  g_object_unref (dtm);
}

static void
act_manager_ready_cb (ActUserManager *manager,
    GParamSpec *pspec,
    PeopleData *data)
{
  ActUser *user;

  user = act_user_manager_get_user_by_id (manager, getuid ());

  if (act_user_is_loaded (user))
    {
      act_user_ready_cb (user, NULL, data);
    }
  else
    {
      g_signal_connect (user, "notify::is-loaded",
          G_CALLBACK (act_user_ready_cb), data);
    }
}

static void
set_initial_timezone (PeopleData *data)
{
  ActUserManager *manager;
  gboolean loaded;

  /* user's name */
  manager = act_user_manager_get_default ();

  g_object_get (manager,
      "is-loaded", &loaded,
      NULL);

  if (loaded)
    {
      act_manager_ready_cb (manager, NULL, data);
    }
  else
    {
      g_signal_connect (manager, "notify::is-loaded",
          G_CALLBACK (act_manager_ready_cb), data);
    }
}

static gboolean
person_changed_cb (GtkEntryCompletion *entry_completion,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer user_data)
{
  PeopleData *data = user_data;
  GtkWidget *entry;
  gchar *name, *tz;

  gtk_tree_model_get (model, iter,
      COL_NAME, &name,
      COL_TZ, &tz,
      -1);

  set_timezone (data, name, tz);
  g_free (name);
  g_free (tz);

  entry = gtk_entry_completion_get_entry (GTK_ENTRY_COMPLETION (entry_completion));
  gtk_entry_set_text (GTK_ENTRY (entry), "");

  gtk_widget_grab_focus (data->button);

  return TRUE;
}

int
main (int argc,
    char **argv)
{
  PeopleData *data;

  GtkWidget *window, *map, *entry, *close;
  GtkBuilder *builder;
  GtkEntryCompletion *completion;
  GtkTreeModel *model;

  gtk_init (&argc, &argv);

  g_resources_register (gente_get_resource ());

  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder, "/com/jonnylamb/gente/gente.ui", NULL);

  /* first the dialog */
  window = (GtkWidget *) gtk_builder_get_object (builder, "timezone-dialog");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show (window);

  /* main map widget */
  map = (GtkWidget *) cc_timezone_map_new ();
  gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (builder, "aspectmap")), map);
  gtk_widget_show (map);

  /* close button */
  close = (GtkWidget *) gtk_builder_get_object (builder, "timezone-close-button");
  g_signal_connect (close, "clicked", G_CALLBACK (gtk_main_quit), NULL);

  data = g_slice_new0 (PeopleData);
  data->map = (CcTimezoneMap *) map;
  data->button = close;

  /* person search entry */
  entry = (GtkWidget *) gtk_builder_get_object (builder, "timezone-searchentry");
  completion = gtk_entry_completion_new ();
  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  g_signal_connect (completion, "match-selected", G_CALLBACK (person_changed_cb), data);
  g_object_unref (completion);

  /* list store for the entry */
  load_people (GTK_LIST_STORE (gtk_builder_get_object (builder,
              "person-liststore")));

  /* sorting for the list store */
  model = GTK_TREE_MODEL (gtk_builder_get_object (builder,
          "person-modelsort"));
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
      COL_NAME,
      GTK_SORT_ASCENDING);
  gtk_entry_completion_set_model (completion, model);
  gtk_entry_completion_set_text_column (completion, COL_NAME);

  /* set initial selection */
  set_initial_timezone (data);

  gtk_widget_grab_focus (entry);

  gtk_main ();

  return 0;
}

