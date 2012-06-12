/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>

#include "ige-conf.h"
#include "dh-util.h"
#include "dh-app.h"
#include "dh-preferences.h"
#include "dh-window.h"
#include "dh-assistant.h"

struct _DhAppPrivate {
        DhBookManager *book_manager;
};

G_DEFINE_TYPE (DhApp, dh_app, GTK_TYPE_APPLICATION);

/******************************************************************************/

DhBookManager *
dh_app_peek_book_manager (DhApp *self)
{
        return self->priv->book_manager;
}

GtkWindow *
dh_app_peek_first_window (DhApp *self)
{
        GList *l;

        for (l = gtk_application_get_windows (GTK_APPLICATION (self));
             l;
             l = g_list_next (l)) {
                if (DH_IS_WINDOW (l->data)) {
                        return GTK_WINDOW (l->data);
                }
        }

        /* Create a new window */
        dh_app_new_window (self);

        /* And look for the newly created window again */
        return dh_app_peek_first_window (self);
}

GtkWindow *
dh_app_peek_assistant (DhApp *self)
{
        GList *l;

        for (l = gtk_application_get_windows (GTK_APPLICATION (self));
             l;
             l = g_list_next (l)) {
                if (DH_IS_ASSISTANT (l->data)) {
                        return GTK_WINDOW (l->data);
                }
        }

        return NULL;
}

/******************************************************************************/
/* Application action activators */

void
dh_app_new_window (DhApp *self)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "new-window", NULL);
}

void
dh_app_quit (DhApp *self)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "quit", NULL);
}

void
dh_app_search (DhApp *self,
               const gchar *keyword)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "search", g_variant_new_string (keyword));
}

void
dh_app_search_assistant (DhApp *self,
                         const gchar *keyword)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "search-assistant", g_variant_new_string (keyword));
}

void
dh_app_focus_search (DhApp *self)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "focus-search", NULL);
}

void
dh_app_raise (DhApp *self)
{
        g_action_group_activate_action (G_ACTION_GROUP (self), "raise", NULL);
}

/******************************************************************************/
/* Application actions setup */

static void
new_window_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GtkWidget *window;

        window = dh_window_new (self);
        gtk_application_add_window (GTK_APPLICATION (self), GTK_WINDOW (window));
        gtk_widget_show_all (window);
}

static void
preferences_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
        dh_preferences_show_dialog ();
}

static void
about_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        const gchar  *authors[] = {
                "Mikael Hallendal <micke@imendio.com>",
                "Richard Hult <richard@imendio.com>",
                "Johan Dahlin <johan@gnome.org>",
                "Ross Burton <ross@burtonini.com>",
                "Aleksander Morgado <aleksander@lanedo.com>",
                NULL
        };
        const gchar **documenters = NULL;
        const gchar  *translator_credits = _("translator_credits");

        /* i18n: Please don't translate "Devhelp" (it's marked as translatable
         * for transliteration only) */
        gtk_show_about_dialog (NULL,
                               "name", _("Devhelp"),
                               "version", PACKAGE_VERSION,
                               "comments", _("A developers' help browser for GNOME"),
                               "authors", authors,
                               "documenters", documenters,
                               "translator-credits",
                               (strcmp (translator_credits, "translator_credits") != 0 ?
                                translator_credits :
                                NULL),
                               "website", PACKAGE_URL,
                               "website-label", _("DevHelp Website"),
                               "logo-icon-name", PACKAGE_TARNAME,
                               NULL);
}

static void
quit_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GList *l;

        /* Remove all windows registered in the application */
        for (l = gtk_application_get_windows (GTK_APPLICATION (self));
             l;
             l = g_list_next (l)) {
                gtk_application_remove_window (GTK_APPLICATION (self),
                                               GTK_WINDOW (l->data));
        }
}

static void
search_cb (GSimpleAction *action,
           GVariant      *parameter,
           gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GtkWindow *window;
        const gchar *str;

        window = dh_app_peek_first_window (self);
        str = g_variant_get_string (parameter, NULL);
        if (str[0] == '\0') {
                g_warning ("Cannot search in application window: "
                           "No keyword given");
                return;
        }

        dh_window_search (DH_WINDOW (window), str);
        gtk_window_present (window);
}

static void
search_assistant_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GtkWindow *assistant;
        const gchar *str;

        str = g_variant_get_string (parameter, NULL);
        if (str[0] == '\0') {
                g_warning ("Cannot look for keyword in Search Assistant: "
                           "No keyword given");
                return;
        }

        /* Look for an already registered assistant */
        assistant = dh_app_peek_assistant (self);
        if (!assistant) {
                assistant = GTK_WINDOW (dh_assistant_new (self));
                gtk_application_add_window (GTK_APPLICATION (self), assistant);
        }

        dh_assistant_search (DH_ASSISTANT (assistant), str);
}

static void
focus_search_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GtkWindow *window;

        window = dh_app_peek_first_window (self);
        dh_window_focus_search (DH_WINDOW (window));
        gtk_window_present (window);
}

static void
raise_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhApp *self = DH_APP (user_data);
        GtkWindow *window;

        /* Look for the first application window available and show it */
        window = dh_app_peek_first_window (self);
        gtk_window_present (window);
}

static GActionEntry app_entries[] = {
        /* general  actions */
        { "new-window",       new_window_cb,       NULL, NULL, NULL },
        { "preferences",      preferences_cb,      NULL, NULL, NULL },
        { "about",            about_cb,            NULL, NULL, NULL },
        { "quit",             quit_cb,             NULL, NULL, NULL },
        /* additional commandline-specific actions */
        { "search",           search_cb,           "s",  NULL, NULL },
        { "search-assistant", search_assistant_cb, "s",  NULL, NULL },
        { "focus-search",     focus_search_cb,     NULL, NULL, NULL },
        { "raise",            raise_cb,            NULL, NULL, NULL },
};

static void
setup_actions (DhApp *self)
{
        g_action_map_add_action_entries (G_ACTION_MAP (self),
                                         app_entries, G_N_ELEMENTS (app_entries),
                                         self);
}

/******************************************************************************/

static void
load_config_defaults (void)
{
        IgeConf    *conf;
        gchar      *path;

        conf = ige_conf_get ();
        path = dh_util_build_data_filename ("devhelp", "devhelp.defaults", NULL);
        ige_conf_add_defaults (conf, path);
        g_free (path);
}

static void
create_application_menu (DhApp *self)
{
        GMenu *menu, *section;

        menu = g_menu_new ();

        section = g_menu_new ();
        g_menu_append (section, _("New window"), "app.new-window");
        g_menu_append_section (menu, NULL, G_MENU_MODEL (section));

        section = g_menu_new ();
        g_menu_append (section, _("Preferences"), "app.preferences");
        g_menu_append_section (menu, NULL, G_MENU_MODEL (section));

        section = g_menu_new ();
        g_menu_append (section, _("About Devhelp"), "app.about");
        g_menu_append (section, _("Quit"), "app.quit");
        g_menu_append_section (menu, NULL, G_MENU_MODEL (section));

        gtk_application_set_app_menu (GTK_APPLICATION (self),
                                      G_MENU_MODEL (menu));
}

static void
startup (GApplication *application)
{
        DhApp *self = DH_APP (application);

        /* Chain up parent's startup */
        G_APPLICATION_CLASS (dh_app_parent_class)->startup (application);

        /* Setup application level actions */
        setup_actions (self);

        /* Create application menu */
        create_application_menu (self);

        /* Setup default configuration */
        load_config_defaults ();

        /* Load the book manager */
        g_assert (self->priv->book_manager == NULL);
        self->priv->book_manager = dh_book_manager_new ();
        dh_book_manager_populate (self->priv->book_manager);
}

/******************************************************************************/

DhApp *
dh_app_new (void)
{
        DhApp *application;

        g_type_init ();

        /* i18n: Please don't translate "Devhelp" (it's marked as translatable
         * for transliteration only) */
        g_set_application_name (_("Devhelp"));
        gtk_window_set_default_icon_name ("devhelp");

        application = g_object_new (DH_TYPE_APP,
                                    "application-id",   "org.gnome.Devhelp",
                                    "flags",            G_APPLICATION_FLAGS_NONE,
                                    "register-session", TRUE,
                                    NULL);

        return application;
}

static void
dh_app_init (DhApp *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DH_TYPE_APP, DhAppPrivate);
}

static void
dispose (GObject *object)
{
        DhApp *self = DH_APP (object);

        g_clear_object (&self->priv->book_manager);

        G_OBJECT_CLASS (dh_app_parent_class)->dispose (object);
}

static void
dh_app_class_init (DhAppClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DhAppPrivate));

        application_class->startup = startup;

        object_class->dispose = dispose;
}
