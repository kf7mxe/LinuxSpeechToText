/* main.c
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

#include <pipewire/pipewire.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <april_api.h>

#include "livecaptions-config.h"
#include "livecaptions-application.h"
#include "audiocap.h"

int main (int argc, char *argv[]) {
    g_thread_init(NULL);
    gdk_threads_init();

    aam_api_init();

    pw_init(&argc, &argv);

    fprintf(stdout, "Compiled with libpipewire %s\n"
                    "Linked with libpipewire %s\n",
                        pw_get_headers_version(),
                        pw_get_library_version());


    audio_thread audio = create_audio_thread();

    g_autoptr(LiveCaptionsApplication) app = NULL;
    int ret;

    /* Set up gettext translations */
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    /*
    * Create a new GtkApplication. The application manages our main loop,
    * application windows, integration with the window manager/compositor, and
    * desktop features such as file opening and single-instance applications.
    */
    app = livecaptions_application_new("net.sapples.LiveCaptions", G_APPLICATION_FLAGS_NONE);


    app->audio = audio;

    /*
    * Run the application. This function will block until the application
    * exits. Upon return, we have our exit code to return to the shell. (This
    * is the code you see when you do `echo $?` after running a command in a
    * terminal.
    *
    * Since GtkApplication inherits from GApplication, we use the parent class
    * method "run". But we need to cast, which is what the "G_APPLICATION()"
    * macro does.
    */
    ret = g_application_run(G_APPLICATION(app), argc, argv);

    free_audio_thread(audio);

    return ret;
}
