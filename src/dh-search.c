/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include <atk/atk.h>
#include <gtk/gtkaccessible.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <libgnome/gnome-i18n.h>
#include <string.h>

#include "dh-index-model.h"
#include "dh-html.h"
#include "dh-marshal.h"
#include "dh-reader.h"
#include "function-database.h"
#include "dh-search.h"

#define d(x)

static void  search_init                       (DhSearch       *view);
static void  search_class_init                 (DhSearchClass  *klass);
static void  search_search_selection_changed_cb (GtkTreeSelection    *selection,
					       DhSearch       *content);
static void  search_html_uri_selected_cb       (DhHtml            *html,
						const gchar       *uri,
					       gboolean             handled,
					       DhSearch       *view);
static void  search_entry_changed_cb           (GtkEntry            *entry,
					       DhSearch       *view);
static void  search_entry_activated_cb         (GtkEntry            *entry,
					       DhSearch       *view);
static void  search_entry_text_inserted_cb     (GtkEntry            *entry,
					       const gchar         *text,
					       gint                 length,
					       gint                *position,
					       DhSearch       *view);
static gboolean search_complete_idle           (DhSearch       *view);
static gboolean search_filter_idle             (DhSearch       *view);
static gchar * search_complete_func            (Function       *function);

#if 0
static void  search_reader_data_cb             (DhpReader          *reader,
						const gchar         *data,
						gint                 len,
					       DhSearch       *view);
static void  search_reader_finished_cb         (DevhelpReader          *reader,
					       DevhelpURI             *uri,
					       DhSearch       *view);

static void  search_show_uri                  (DevhelpView            *view,
					       DevhelpURI             *search_uri,
					       GError             **error);
#endif

struct _DhSearchPriv {
	GtkWidget    *entry;
	GtkWidget    *hitlist;
	DhIndexModel *model;

	/* Html view */
	GtkWidget    *html_view;

	GCompletion  *completion;

	guint         idle_complete;
	guint         idle_filter;

	gboolean      first;
};

GType
devhelp_search_get_type (void)
{
        static GType type = 0;

        if (!type)
        {
                static const GTypeInfo info =
                        {
                                sizeof (DhSearchClass),
                                NULL,
                                NULL,
                                (GClassInitFunc) search_class_init,
                                NULL,
                                NULL,
                                sizeof (DhSearch),
                                0,
                                (GInstanceInitFunc) search_init,
                        };
                
                type = g_type_register_static (GTK_TYPE_VBOX,
					       "DhSearch", 
					       &info, 0);
        }
        
        return type;
}

static void
search_init (DhSearch *view)
{
	DhSearchPriv *priv;
	
	priv = g_new0 (DhSearchPriv, 1);
	view->priv = priv;
	
	priv->idle_complete = 0;
	priv->idle_filter   = 0;

	priv->completion = 
		g_completion_new ((GCompletionFunc) search_complete_func);

	priv->hitlist = gtk_tree_view_new ();
	priv->model   = dh_index_model_new ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist),
				 GTK_TREE_MODEL (priv->model));

	priv->html_view = dh_html_new ();
	
	g_signal_connect (priv->html_view, "uri_selected",
			  G_CALLBACK (search_html_uri_selected_cb),
			  view);

#if 0
	priv->reader = devhelp_reader_new ();
	
	g_signal_connect (G_OBJECT (priv->reader), "data",
			  G_CALLBACK (search_reader_data_cb),
			  view);
	g_signal_connect (G_OBJECT (priv->reader), "finished",
			  G_CALLBACK (search_reader_finished_cb),
			  view);
#endif
}

static void
search_class_init (DhSearchClass *klass)
{
/* 	DevhelpViewClass *view_class = DEVHELP_VIEW_CLASS (klass); */
       
/* 	view_class->show_uri = search_show_uri; */
}

static void
search_search_selection_changed_cb (GtkTreeSelection *selection, 
				    DhSearch    *search)
{
	DhSearchPriv *priv;
 	GtkTreeIter   iter;
	Function     *func;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (DH_IS_SEARCH (search));

	priv = view->priv;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		DevhelpURI *search_uri;
		
		gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
				    DH_INDEX_MODEL_COL_FUNCTION, &function,
				    -1);

		d(g_print ("Index View: selection changed: %s\n", 
			   devhelp_uri_to_string (function->uri)));
		
		search_uri = devhelp_uri_to_index (section->uri);

  		g_signal_emit_by_name (view, "uri_selected", search_uri, FALSE);

		devhelp_uri_unref (search_uri);
	}
}

static void
search_html_uri_selected_cb (DhHtml      *html, 
			  DevhelpURI       *uri, 
			  gboolean       handled,
			  DhSearch *view)
{
	DevhelpURI *search_uri;

	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));

	d(g_print ("Index View: uri selected: %s\n", 
		   devhelp_uri_to_string (uri)));

	search_uri = devhelp_uri_to_index (uri);
	g_signal_emit_by_name (view, "uri_selected", search_uri, handled);
	devhelp_uri_unref (search_uri);
}

static void
search_entry_changed_cb (GtkEntry *entry, DhSearch *view)
{
	DhSearchPriv *priv;
	
	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));
	
	priv = view->priv;
 
	d(g_print ("Entry changed\n"));

	if (!priv->idle_filter) {
		priv->idle_filter =
			g_idle_add ((GSourceFunc) search_filter_idle, view);
	}
}

static void
search_entry_activated_cb (GtkEntry *entry, DhSearch *view)
{
	DhSearchPriv *priv;
	gchar             *str;
	
	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));

	priv = view->priv;
	
	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));
	
	devhelp_search_model_filter (view->priv->model, str);
}

static void
search_entry_text_inserted_cb (GtkEntry      *entry,
			    const gchar   *text,
			    gint           length,
			    gint          *position,
			    DhSearch *view)
{
	DhSearchPriv *priv;
	
 	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));
	
	priv = view->priv;
	
	if (!priv->idle_complete) {
		priv->idle_complete = 
			g_idle_add ((GSourceFunc) search_complete_idle, view);
	}
}

static gboolean
search_complete_idle (DhSearch *view)
{
	DhSearchPriv *priv;
	const gchar       *text;
	gchar             *completed = NULL;
	GList             *list;
	gint               text_length;
	
	g_return_val_if_fail (DEVHELP_IS_VIEW_INDEX (view), FALSE);
	
	priv = view->priv;
	
	text = gtk_entry_get_text (GTK_ENTRY (priv->entry));

	list = g_completion_complete (priv->completion, 
				      (gchar *)text,
				      &completed);

	if (completed) {
		text_length = strlen (text);
		
		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);
 		gtk_editable_set_position (GTK_EDITABLE (priv->entry),
 					   text_length);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
	}
	
	priv->idle_complete = 0;

	return FALSE;
}

static gboolean
search_filter_idle (DhSearch *view)
{
	DhSearchPriv *priv;
	gchar             *str;
	
	g_return_val_if_fail (DEVHELP_IS_VIEW_INDEX (view), FALSE);

	priv = view->priv;

	d(g_print ("Filter idle\n"));
	
	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));
	
	devhelp_search_model_filter (view->priv->model, str);

	priv->idle_filter = 0;

	return FALSE;
}

static gchar *
search_complete_func (Function *function)
{
	return function->name;
}


#if 0
static void
search_reader_data_cb (DevhelpReader    *reader,
		    const gchar   *data,
		    gint           len,
		    DhSearch *view)
{
	DhSearchPriv *priv;
	
	g_return_if_fail (DEVHELP_IS_READER (reader));
	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));
	
	priv = view->priv;

	if (priv->first) {
		devhelp_html_clear (priv->html_view);
		priv->first = FALSE;
	}

	if (len == -1) {
		len = strlen (data);
	}

	if (len <= 0) {
		return;
	}

	devhelp_html_write (priv->html_view, data, len);
}

static void
search_reader_finished_cb (DevhelpReader    *reader,
			DevhelpURI       *uri,
			DhSearch *view)
{
	DhSearchPriv *priv;
	
	g_return_if_fail (DEVHELP_IS_READER (reader));
	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));

	priv = view->priv;
 
	if (!priv->first) {
		devhelp_html_close (priv->html_view);
	}
	
	gdk_window_set_cursor (priv->html_widget->window, NULL);
	gtk_widget_grab_focus (priv->html_widget);
}

static void
search_show_uri (DevhelpView *view, DevhelpURI *search_uri, GError **error)
{
	DhSearchPriv *priv;
	DevhelpURI           *uri;

	g_return_if_fail (DEVHELP_IS_VIEW_INDEX (view));
	g_return_if_fail (search_uri != NULL);
	
	priv = DEVHELP_VIEW_INDEX (view)->priv;

	d(g_print ("Index show Uri: %s\n", devhelp_uri_to_string (search_uri)));

	if (devhelp_uri_no_path (search_uri)) {
		return;
	}

	uri = devhelp_uri_from_index (search_uri);

	priv->first = TRUE;

	devhelp_html_set_base_uri (priv->html_view, uri);

	if (!devhelp_reader_start (priv->reader, uri)) {
		gchar     *loading = _("Loading...");

		devhelp_html_clear (priv->html_view);
		
		devhelp_html_printf (priv->html_view, 
				  "<html><meta http-equiv=\"Content-Type\" "
				  "content=\"text/html; charset=utf-8\">"
				  "<title>%s</title>"
				  "<body><center>%s</center></body>"
				  "</html>", 
				  loading, loading);

		devhelp_html_close (priv->html_view);
	}

	/* FIXME: Handle the GError */
/* 	devhelp_html_open_uri (priv->html_view, uri, error); */
}

#endif

GtkWidget *
devhelp_search_new (GList *index)
{
	DhSearch     *view;
	DhSearchPriv *priv;
	GtkTreeSelection  *selection;
        GtkWidget         *html_sw;
        GtkWidget         *list_sw;
	GtkWidget         *frame;
	GtkWidget         *box;
	GtkWidget         *hbox;
	GtkWidget         *label;
	GtkWidget         *hpaned;
		
	view = g_object_new (DEVHELP_TYPE_VIEW_INDEX, NULL);
	priv = view->priv;

	hpaned = DEVHELP_VIEW (view)->widget;

	/* Setup the index box */
	box = gtk_vbox_new (FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("_Search for:"));

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);

	priv->entry = gtk_entry_new ();

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->entry);

	g_signal_connect (priv->entry, "changed", 
			  G_CALLBACK (search_entry_changed_cb),
			  view);

	gtk_box_pack_end (GTK_BOX (hbox), priv->entry, FALSE, FALSE, 0);
	
	g_signal_connect (priv->entry, "activate",
			  G_CALLBACK (search_entry_activated_cb),
			  view);
	
	g_signal_connect (priv->entry, "insert-text",
			  G_CALLBACK (search_entry_text_inserted_cb),
			  view);

	gtk_box_pack_start (GTK_BOX (box), hbox, 
			    FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	
        list_sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_sw),
                                        GTK_POLICY_AUTOMATIC, 
                                        GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (frame), list_sw);
	
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->search_view), -1,
		_("Section"), gtk_cell_renderer_text_new (),
		"text", 0,
		NULL);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->search_view),
					   FALSE);

	selection = gtk_tree_view_get_selection (
		GTK_TREE_VIEW (priv->search_view));

	g_signal_connect (selection, "changed",
			  G_CALLBACK (search_search_selection_changed_cb),
			  view);
	
	gtk_container_add (GTK_CONTAINER (list_sw), priv->search_view);

	gtk_box_pack_end_defaults (GTK_BOX (box), frame);

        /* Setup the Html view */
 	html_sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (html_sw),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), html_sw);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	
        gtk_container_add (GTK_CONTAINER (html_sw), priv->html_widget);

	/* Add the tree and html view to the paned */
	gtk_paned_add1 (GTK_PANED (hpaned), box);
        gtk_paned_add2 (GTK_PANED (hpaned), frame);
        gtk_paned_set_position (GTK_PANED (hpaned), 250);
 
	d(g_print ("List length: %d\n", g_list_length (index)));
	
	g_completion_add_items (priv->completion, index);
	devhelp_search_model_set_words (priv->model, index);

	return DEVHELP_VIEW (view);
}


