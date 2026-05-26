#pragma once
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VCAMERA_TYPE_MAIN_WINDOW (vcamera_main_window_get_type())
G_DECLARE_FINAL_TYPE(VcameraMainWindow, vcamera_main_window, VCAMERA, MAIN_WINDOW, GtkApplicationWindow)

VcameraMainWindow *vcamera_main_window_new(GtkApplication *app);

G_END_DECLS
