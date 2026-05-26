#pragma once

#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define CAMERA_TYPE_ENGINE (camera_engine_get_type())
G_DECLARE_FINAL_TYPE(CameraEngine, camera_engine, CAMERA, ENGINE, GObject)

CameraEngine *camera_engine_new(void);
void camera_engine_start(CameraEngine *self);
void camera_engine_stop(CameraEngine *self);

void camera_engine_start_recording(CameraEngine *self, const char *filepath);
void camera_engine_stop_recording(CameraEngine *self);

G_END_DECLS
