/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkhpaned.h>
#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkvbox.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include <libegg/menu/egg-menu.h>

#include "dh-book-tree.h"
#include "dh-history.h"
#include "dh-html.h"
#include "dh-window.h"


struct _DhWindowPriv {
	DhHistory *history;
	DhProfile *profile;

	GtkWidget *menu;
	GtkWidget *toolbar;
	GtkWidget *statusbar;

	GtkWidget *hpaned;
        GtkWidget *notebook;
        GtkWidget *book_tree;
	GtkWidget *search;
	GtkWidget *html_view;
};

static void window_class_init                (DhWindowClass *klass);
static void window_init                      (DhWindow      *window);
 
static void window_destroy                   (GtkObject          *object);

static void window_populate                  (DhWindow      *window);

static void window_activate_action           (EggAction          *action,
					      DhWindow           *window);
#if 0
static void window_uri_changed_cb            (BonoboListener     *listener,
					      const gchar        *event_name,
					      const CORBA_any    *arg,
					      CORBA_Environment  *ev,
					      gpointer            user_data);

#endif
static void window_delete_cb                 (GtkWidget          *widget,
					      GdkEventAny        *event,
					      gpointer            user_data);

static gboolean window_open_url              (DhWindow           *window,
					      const gchar        *url);

static void window_link_selected_cb          (GObject            *ignored,
					      DhLink             *link,
					      DhWindow           *window);

static void window_on_url_cb                 (DhWindow      *window,
					      gchar              *url,
					      gpointer            ignored);

static void window_note_change_page_cb       (GtkWidget          *child,
					      GtkNotebook        *notebook);

static void window_note_page_mapped_cb       (GtkWidget          *page, 
					      GtkAccelGroup      *accel_group);

static void window_note_page_unmapped_cb     (GtkWidget          *page, 
					      GtkAccelGroup      *accel_group);


static void window_note_page_setup_signals   (GtkWidget          *page, 
					      GtkAccelGroup      *accel);

static void 
window_notebook_append_page_with_accelerator (GtkNotebook        *notebook,
					      GtkWidget          *page,
					      gchar              *label_text,
					      GtkAccelGroup      *accel);


static GtkWindowClass *parent_class = NULL;

static EggActionGroupEntry actions[] = {
	{ "StockFileMenuAction", N_("_File"), NULL, NULL, NULL, NULL, NULL },
	{ "StockGoMenuAction", N_("_File"), NULL, NULL, NULL, NULL, NULL },
	{ "StockHelpMenuAction", N_("_File"), NULL, NULL, NULL, NULL, NULL },

	/* File menu */
	{ "QuitAction", NULL, GTK_STOCK_QUIT, "<control>Q", NULL,
	  G_CALLBACK (window_activate_action), NULL },

	/* Go menu */
	{ "BackAction", NULL, GTK_STOCK_GO_BACK, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL },
	{ "QuitAction", NULL, GTK_STOCK_GO_FORWARD, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL },

	/* About menu */
	{ "AboutAction", N_("_About"), NULL, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL }
};

GtkType
dh_window_get_type (void)
{
        static GtkType type = 0;

        if (!type) {
                static const GtkTypeInfo info = {
                        "DhWindow",
                        sizeof (DhWindow),
                        sizeof (DhWindowClass),
                        (GtkClassInitFunc)  window_class_init,
                        (GtkObjectInitFunc) window_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                type = gtk_type_unique (GTK_TYPE_WINDOW, &info);
        }

        return type;
}

static void
window_class_init (DhWindowClass *klass)
{
        GtkObjectClass *object_class;
        
        parent_class = gtk_type_class (GTK_TYPE_WINDOW);

        object_class = (GtkObjectClass *) klass;
        
        object_class->destroy = window_destroy;
}

static void
window_init (DhWindow *window)
{
        DhWindowPriv *priv;

        priv         = g_new0 (DhWindowPriv, 1);
	priv->history = dh_history_new ();
	
        window->priv = priv;
}

static void
window_destroy (GtkObject *object)
{
}

static void
window_note_change_page_cb (GtkWidget *child, GtkNotebook *notebook)
{
	gint page = gtk_notebook_page_num (notebook, child);

	gtk_notebook_set_page (notebook, page);
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv *priv;
	GtkWidget    *html_sw;
	GtkWidget    *frame;
	GtkWidget    *book_tree_sw;
	GNode        *contents_tree;
	GSList       *keywords;
	GtkWidget    *vbox;
	 
        g_return_if_fail (window != NULL);
        g_return_if_fail (DH_IS_WINDOW (window));
        
        priv = window->priv;

	vbox            = gtk_vbox_new (FALSE, 0);
	priv->statusbar = gtk_statusbar_new ();
/* 	gtk_box_pack_end_defaults (GTK_BOX (vbox), priv->statusbar); */

        priv->hpaned    = gtk_hpaned_new ();
        priv->notebook  = gtk_notebook_new ();
	priv->html_view = dh_html_new ();
	html_sw           = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (html_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	book_tree_sw      = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (book_tree_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (book_tree_sw),
					     GTK_SHADOW_IN);
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), priv->notebook);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

	gtk_paned_add1 (GTK_PANED (priv->hpaned), frame);
	
 	gtk_container_add (GTK_CONTAINER (html_sw), priv->html_view);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), html_sw);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

 	gtk_paned_add2 (GTK_PANED(priv->hpaned), frame);

 	gtk_paned_set_position (GTK_PANED (priv->hpaned), 250);

	contents_tree = dh_profile_open (priv->profile, &keywords, NULL);
	
	if (contents_tree) {
		priv->book_tree = dh_book_tree_new (contents_tree);
	
		gtk_container_add (GTK_CONTAINER (book_tree_sw), 
				   priv->book_tree);

		gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
					  book_tree_sw,
					  gtk_label_new_with_mnemonic (_("_Contents")));
		g_signal_connect (priv->book_tree, "link_selected", 
				  G_CALLBACK (window_link_selected_cb),
				  window);
	}
	

#if 0
	gtk_box_pack_start (GTK_BOX (priv->search_box), 
			    priv->search_entry, 
			    FALSE, FALSE, 0); 

	gtk_box_pack_end_defaults (GTK_BOX (priv->search_box),
				   priv->search_list); 
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  priv->search_box,
				  gtk_label_new_with_mnemonic (_("_Search")));



#endif
	gtk_widget_show_all (priv->hpaned);

	gtk_box_pack_end (GTK_BOX (vbox), priv->hpaned,
			  TRUE, TRUE, 0);
	
	gtk_container_add (GTK_CONTAINER (window), vbox);

 	g_signal_connect_swapped (HTML_VIEW (priv->html_view),
				  "uri_selected", 
				  G_CALLBACK (window_open_url),
				  window);

	/* TODO: Look in gtkhtml2 code or ask jborg */
#if GNOME2_PORT_COMPLETE	
	g_signal_connect_object (G_OBJECT (HTML_VIEW (priv->html_view)->document),
				 "on_url",
				 G_CALLBACK (window_on_url_cb),
				 G_OBJECT (window),
				 0);
#endif	
}

static void
window_activate_action (EggAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	const gchar  *name = action->name;
	
	g_return_if_fail (DH_IS_WINDOW (window));
	
	if (strcmp (name, "QuitAction") == 0) {
		gtk_main_quit ();
		/* Quit */
	}
	else if (strcmp (name, "BackAction") == 0) {
	}
	else if (strcmp (name, "ForwardAction") == 0) {
	}
	else if (strcmp (name, "AboutAction") == 0) {
		GtkWidget *about;

		const gchar *authors[] = {
			"Mikael Hallendal <micke@codefactory.se>",
			"Richard Hult <rhult@codefactory.se>",
			"Johan Dahlin <jdahlin@telia.com>",
			NULL
		};
		
		about = gnome_about_new (PACKAGE, VERSION,
					 "",
					 _("A developer's help browser for GNOME 2"),
					 authors,
					 NULL,
					 NULL,
					 NULL);
		
		gtk_widget_show (about);
	} else {
		g_message ("Unhandled action '%s'", name);
	}
}

#if 0
static void
window_uri_changed_cb (BonoboListener    *listener,
		       const gchar       *event_name,
		       const CORBA_any   *any,
		       CORBA_Environment *ev,
		       gpointer           user_data)
{
	DhWindow       *window;
	DhWindowPriv   *priv;
	gchar               *uri;
	
	g_return_if_fail (user_data != NULL);
	g_return_if_fail (DH_IS_WINDOW (user_data));
	
	window = DH_WINDOW (user_data);
	priv   = window->priv;
	uri  = g_strdup (any->_value);

	if (uri) {
		dh_html_open_uri (DH_HTML (priv->html_view), uri);
	}
	
	g_free (uri);
}
#endif 

static void
window_delete_cb (GtkWidget   *widget,
		  GdkEventAny *event,
		  gpointer     user_data)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (DH_IS_WINDOW (widget));
	
	gtk_main_quit ();
}

static void
window_link_selected_cb (GObject *ignored, DhLink *link, DhWindow *window)
{
	DhWindowPriv   *priv;

	g_return_if_fail (link != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	if (window_open_url (window, link->uri)) {
		dh_history_goto (priv->history, link->uri);
	}
}

static gboolean 
window_open_url (DhWindow *window, const gchar *url)
{
	DhWindowPriv *priv;
	BookNode     *node;
	GnomeVFSURI  *uri;
	Document     *doc;
	gchar        *anchor;
	gchar        *str_uri;
	
	g_return_val_if_fail (DH_IS_WINDOW (window), FALSE);
	g_return_val_if_fail (url != NULL, FALSE);

	priv = window->priv;

	dh_html_open_uri (DH_HTML (priv->html_view), url);
	dh_book_tree_show_uri (DH_BOOK_TREE (priv->book_tree), url);

	return TRUE;
}

static void
window_on_url_cb (DhWindow *window, gchar *url, gpointer ignored)
{
	DhWindowPriv *priv;
	gchar        *status_text;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	if (url) {
		status_text = g_strdup_printf (_("Open %s"), url);
		g_free (status_text);
	}
}

GtkWidget *
dh_window_new (DhProfile *profile)
{
        DhWindow     *window;
        DhWindowPriv *priv;
	GdkPixbuf    *icon;
	
        window = gtk_type_new (DH_TYPE_WINDOW);
        priv   = window->priv;

	priv->profile = g_object_ref (profile);

        gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
        
        gtk_window_set_default_size (GTK_WINDOW (window), 700, 500);
	
	gtk_window_set_wmclass (GTK_WINDOW (window), "devhelp", "DevHelp");

	g_signal_connect (GTK_OBJECT (window), 
			  "delete_event",
			  G_CALLBACK (window_delete_cb),
			  NULL);
	
        window_populate (window);

	icon = gdk_pixbuf_new_from_file (DATA_DIR "/pixmaps/devhelp.png", 
					 NULL);
	if (icon) {
		gtk_window_set_icon (GTK_WINDOW (window), icon);
		g_object_unref (icon);
	}
	
	return GTK_WIDGET (window);
}

void
dh_window_search (DhWindow *window, const gchar *str)
{
	DhWindowPriv      *priv;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

#if 0	
	dh_search_set_search_string (priv->search, str);
#endif
}
