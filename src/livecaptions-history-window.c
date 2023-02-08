/* livecaptions-history-window.c
 *
 * Copyright 2022 abb128
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <glib/gi18n.h>
#include <april_api.h>
#include "livecaptions-history-window.h"
#include "history.h"

G_DEFINE_TYPE(LiveCaptionsHistoryWindow, livecaptions_history_window, GTK_TYPE_APPLICATION_WINDOW)

static bool force_bottom(gpointer userdata) {
    LiveCaptionsHistoryWindow *self = LIVECAPTIONS_HISTORY_WINDOW(userdata);

    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(self->scroll));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));

    return G_SOURCE_REMOVE;
}

static void add_text(LiveCaptionsHistoryWindow *self, char *text, bool is_text) {
    GtkWidget *label = gtk_label_new(text);

    if(is_text){
        PangoFontDescription *desc = pango_font_description_from_string(g_settings_get_string(self->settings, "font-name"));
        PangoAttribute *attr_font = pango_attr_font_desc_new(desc);
        PangoAttrList *attr = gtk_label_get_attributes(GTK_LABEL(label));
        if(attr == NULL){
            attr = pango_attr_list_new();
        }
        pango_attr_list_change(attr, attr_font);
        gtk_label_set_attributes(GTK_LABEL(label), attr);
        pango_font_description_free(desc);
    }

    gtk_label_set_selectable(GTK_LABEL(label), true);
    gtk_label_set_wrap(GTK_LABEL(label), true);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_add_css_class(label, is_text ? "history-label" : "timestamp-label");

    gtk_widget_set_hexpand(label, true);
    gtk_widget_set_halign(label, GTK_ALIGN_FILL);

    gtk_box_prepend(self->main_box, label);
}

static void add_time(LiveCaptionsHistoryWindow *self, time_t timestamp, bool date) {
    char text[64];

    struct tm *tm = localtime(&timestamp);
    strftime(text, 64, date ? "\n\nStart of session %F | %H:%M" : "%H:%M:%S", tm);

    add_text(self, text, false);
}

static void add_session(LiveCaptionsHistoryWindow *self, const struct history_session *session){
    if(session->entries_count == 0) return;

    bool use_lowercase = !g_settings_get_boolean(self->settings, "text-uppercase");
    GString *string = g_string_new(NULL);

    for(size_t i_1=0; i_1<session->entries_count; i_1++){
        size_t i = session->entries_count - i_1 - 1;

        const struct history_entry *entry = &session->entries[i];

        if(entry->tokens_count == 0){
            // Silence
            if((i + 1) >= session->entries_count) continue;
            const struct history_entry *next_entry = &session->entries[i+1];

            add_text(self, string->str, true);
            add_time(self, next_entry->timestamp, false);

            g_string_truncate(string, 0);
        } else {
            GString *entry_text = g_string_new(NULL);

            for(size_t j=0; j<entry->tokens_count; j++) {
                const char *token = entry->tokens[j].token;
                if((j == 0) && (*token == ' ')) token++;

                g_string_append(entry_text, token);
            }

            if(use_lowercase) g_string_ascii_down(entry_text);

            g_string_append_c(entry_text, '\n');
            g_string_prepend(string, entry_text->str);

            g_string_free(entry_text, true);
        }
    }

    add_text(self, string->str, true);
    add_time(self, session->entries[0].timestamp, true);
}

static void load_to(LiveCaptionsHistoryWindow *self, size_t idx){
    for(size_t i_1=0; i_1<1; i_1++){
        const struct history_session *session = get_history_session(idx - i_1 - 1);
        if(session == NULL) break;

        // TODO: text fading?
        add_session(self, session);
    }
}

static void load_more_cb(LiveCaptionsHistoryWindow *self) {
    // gets very laggy
    load_to(self, ++self->session_load);
}


static void on_save_response(GtkDialog *dialog,
                             int        response,
                             LiveCaptionsHistoryWindow *self)
{
    if(response == GTK_RESPONSE_ACCEPT){
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

        g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
        
        char *path = g_file_get_path(file);

        export_history_as_text(path);

        char *uri = g_file_get_uri(file);

        gtk_show_uri(GTK_WINDOW(self), uri, GDK_CURRENT_TIME);

        g_free(path);
        g_free(uri);
    }

    gtk_window_destroy (GTK_WINDOW (dialog));                  
}

static void export_cb(LiveCaptionsHistoryWindow *self) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

    dialog = gtk_file_chooser_dialog_new("Export History",
                                         GTK_WINDOW(self),
                                         action,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Export"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    chooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_current_name(chooser, _("Live Captions History.txt"));

    gtk_window_present(GTK_WINDOW(dialog));

    g_signal_connect(dialog, "response",
                     G_CALLBACK (on_save_response),
                     self);
}

static void livecaptions_history_window_class_init(LiveCaptionsHistoryWindowClass *klass) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/net/sapples/LiveCaptions/livecaptions-history-window.ui");

    gtk_widget_class_bind_template_child(widget_class, LiveCaptionsHistoryWindow, scroll);
    gtk_widget_class_bind_template_child(widget_class, LiveCaptionsHistoryWindow, main_box);

    gtk_widget_class_bind_template_callback(widget_class, load_more_cb);
    gtk_widget_class_bind_template_callback(widget_class, export_cb);
}

// TODO: ctrl+f search
static void livecaptions_history_window_init(LiveCaptionsHistoryWindow *self) {
    gtk_widget_init_template(GTK_WIDGET(self));

    self->settings = g_settings_new("net.sapples.LiveCaptions");


    self->session_load = 0;
    load_to(self, ++self->session_load);

    g_idle_add(force_bottom, self);
}
