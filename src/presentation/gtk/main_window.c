#include "main_window.h"
#include "../../data/gstreamer/camera_engine.h"
#include "../../domain/use_cases/take_photo.h"

struct _VcameraMainWindow {
    GtkApplicationWindow parent_instance;
    CameraEngine *engine;
    GtkWidget *picture;
    GtkWidget *flash_overlay;
    GdkTexture *last_texture;
    
    GtkWidget *record_btn;
    gboolean is_recording;
    gulong frame_ready_handler_id;
};

G_DEFINE_TYPE(VcameraMainWindow, vcamera_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void on_frame_ready(CameraEngine *engine, GdkTexture *texture, gpointer user_data) {
    VcameraMainWindow *self = VCAMERA_MAIN_WINDOW(user_data);
    gtk_picture_set_paintable(GTK_PICTURE(self->picture), GDK_PAINTABLE(texture));
    
    if (self->last_texture) {
        g_object_unref(self->last_texture);
    }
    self->last_texture = g_object_ref(texture);
}

static gboolean remove_flash(gpointer user_data) {
    GtkWidget *flash = GTK_WIDGET(user_data);
    gtk_widget_set_visible(flash, FALSE);
    return G_SOURCE_REMOVE;
}

static void on_gallery_button_clicked(GtkButton *btn, gpointer user_data) {
    const char *pictures_dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    if (!pictures_dir) pictures_dir = g_get_home_dir();
    
    g_autofree char *uri = g_filename_to_uri(pictures_dir, NULL, NULL);
    if (uri) {
        GError *error = NULL;
        if (!g_app_info_launch_default_for_uri(uri, NULL, &error)) {
            g_printerr("Failed to open gallery: %s\n", error->message);
            g_clear_error(&error);
        }
    }
}

static void on_photo_button_clicked(GtkButton *btn, gpointer user_data) {
    VcameraMainWindow *self = VCAMERA_MAIN_WINDOW(user_data);
    if (!self->last_texture) return;
    
    gtk_widget_set_visible(self->flash_overlay, TRUE);
    g_timeout_add(100, remove_flash, self->flash_overlay);

    take_photo_execute(self->last_texture);
}

static void on_record_button_clicked(GtkButton *btn, gpointer user_data) {
    VcameraMainWindow *self = VCAMERA_MAIN_WINDOW(user_data);
    
    if (!self->is_recording) {
        self->is_recording = TRUE;
        gtk_widget_add_css_class(self->record_btn, "recording");
        
        const char *videos_dir = g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS);
        if (!videos_dir) videos_dir = g_get_home_dir();
        // Use webm extension as we will switch to vp8enc
        g_autofree char *filename = g_strdup_printf("%s/vcamera_vid_%ld.webm", videos_dir, g_get_real_time());
        
        camera_engine_start_recording(self->engine, filename);
        g_print("🎥 بدأ التسجيل: %s\n", filename);
    } else {
        self->is_recording = FALSE;
        gtk_widget_remove_css_class(self->record_btn, "recording");
        
        camera_engine_stop_recording(self->engine);
        g_print("⏹ تم إيقاف التسجيل.\n");
    }
}

static void vcamera_main_window_dispose(GObject *object) {
    VcameraMainWindow *self = VCAMERA_MAIN_WINDOW(object);
    if (self->engine) {
        if (self->frame_ready_handler_id > 0) {
            g_signal_handler_disconnect(self->engine, self->frame_ready_handler_id);
            self->frame_ready_handler_id = 0;
        }
        camera_engine_stop(self->engine);
        g_clear_object(&self->engine);
    }
    g_clear_object(&self->last_texture);
    G_OBJECT_CLASS(vcamera_main_window_parent_class)->dispose(object);
}

static void load_css() {
    static gboolean css_loaded = FALSE;
    if (css_loaded) return;
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        ".controls-dock {"
        "  background: rgba(30, 30, 30, 0.65);"
        "  border-radius: 40px;"
        "  padding: 12px 24px;"
        "  margin-bottom: 30px;"
        "  box-shadow: 0 10px 40px rgba(0,0,0,0.6);"
        "}"
        ".camera-btn {"
        "  border-radius: 50%;"
        "  min-width: 68px;"
        "  min-height: 68px;"
        "  background: white;"
        "  color: black;"
        "  margin: 0 16px;"
        "  border: 4px solid #dddddd;"
        "  transition: all 0.2s cubic-bezier(0.25, 0.8, 0.25, 1);"
        "}"
        ".camera-btn:hover {"
        "  transform: scale(1.08);"
        "  border-color: white;"
        "}"
        ".camera-btn:active {"
        "  transform: scale(0.92);"
        "}"
        ".record-btn {"
        "  border-radius: 50%;"
        "  min-width: 68px;"
        "  min-height: 68px;"
        "  background: #ff3b30;"
        "  color: white;"
        "  margin: 0 16px;"
        "  border: 4px solid rgba(255, 59, 48, 0.3);"
        "  transition: all 0.3s cubic-bezier(0.25, 0.8, 0.25, 1);"
        "}"
        ".record-btn:hover {"
        "  transform: scale(1.08);"
        "}"
        ".record-btn.recording {"
        "  border-radius: 16px;"
        "  transform: scale(0.85);"
        "  border-color: rgba(255, 59, 48, 0.8);"
        "}"
        ".flash-effect {"
        "  background-color: white;"
        "  opacity: 0.85;"
        "}"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    css_loaded = TRUE;
}

static void vcamera_main_window_init(VcameraMainWindow *self) {
    load_css();
    gtk_window_set_title(GTK_WINDOW(self), "vcamera");
    gtk_window_set_default_size(GTK_WINDOW(self), 800, 600);
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL);

    self->engine = camera_engine_new();
    
    GtkWidget *overlay = gtk_overlay_new();
    gtk_window_set_child(GTK_WINDOW(self), overlay);
    
    self->picture = gtk_picture_new();
    gtk_picture_set_can_shrink(GTK_PICTURE(self->picture), TRUE);
    gtk_widget_set_valign(self->picture, GTK_ALIGN_FILL);
    gtk_widget_set_halign(self->picture, GTK_ALIGN_FILL);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), self->picture);
    
    self->flash_overlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(self->flash_overlay, "flash-effect");
    gtk_widget_set_visible(self->flash_overlay, FALSE);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), self->flash_overlay);

    GtkWidget *dock = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_add_css_class(dock, "controls-dock");
    
    GtkWidget *bottom_center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(bottom_center_box, GTK_ALIGN_END);
    gtk_widget_set_halign(bottom_center_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(bottom_center_box), dock);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), bottom_center_box);

    GtkWidget *btn_gallery = gtk_button_new_from_icon_name("folder-pictures-symbolic");
    gtk_widget_add_css_class(btn_gallery, "circular");
    gtk_widget_set_tooltip_text(btn_gallery, "المعرض (Gallery)");
    g_signal_connect(btn_gallery, "clicked", G_CALLBACK(on_gallery_button_clicked), self);

    GtkWidget *btn_photo = gtk_button_new_from_icon_name("camera-photo-symbolic");
    gtk_widget_add_css_class(btn_photo, "camera-btn");
    g_signal_connect(btn_photo, "clicked", G_CALLBACK(on_photo_button_clicked), self);
    gtk_widget_set_tooltip_text(btn_photo, "التقاط صورة (Take Photo)");

    self->record_btn = gtk_button_new_from_icon_name("media-record-symbolic");
    gtk_widget_add_css_class(self->record_btn, "record-btn");
    g_signal_connect(self->record_btn, "clicked", G_CALLBACK(on_record_button_clicked), self);
    gtk_widget_set_tooltip_text(self->record_btn, "تسجيل فيديو (Record Video)");
    
    gtk_box_append(GTK_BOX(dock), btn_gallery);
    gtk_box_append(GTK_BOX(dock), btn_photo);
    gtk_box_append(GTK_BOX(dock), self->record_btn);

    self->frame_ready_handler_id = g_signal_connect(self->engine, "frame-ready", G_CALLBACK(on_frame_ready), self);
    camera_engine_start(self->engine);
}

static void vcamera_main_window_class_init(VcameraMainWindowClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = vcamera_main_window_dispose;
}

VcameraMainWindow *vcamera_main_window_new(GtkApplication *app) {
    return g_object_new(VCAMERA_TYPE_MAIN_WINDOW, "application", app, NULL);
}
