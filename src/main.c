#include <gtk/gtk.h>
#include <gst/gst.h>
#include "presentation/gtk/main_window.h"
#include "theme_manager.h"

static void on_activate(GtkApplication *app, gpointer user_data) {
    theme_manager_init();
    VcameraMainWindow *window = vcamera_main_window_new(app);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    // Initialize GStreamer before everything
    gst_init(&argc, &argv);
    
    // Start GTK4 App
    GtkApplication *app = gtk_application_new("com.aether.vcamera", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_object_unref(app);
    gst_deinit();
    
    return status;
}
