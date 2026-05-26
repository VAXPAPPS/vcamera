#include "camera_engine.h"
#include "video_recorder.h"
#include <gst/app/gstappsink.h>

struct _CameraEngine {
    GObject parent_instance;
    GstElement *pipeline;
    GstElement *sink;
    
    GMutex recorder_mutex;
    VideoRecorder *recorder;
    gboolean is_recording_requested;
    gboolean record_failed;
    gchar *record_filepath;
};

G_DEFINE_TYPE(CameraEngine, camera_engine, G_TYPE_OBJECT)

enum {
    SIGNAL_FRAME_READY,
    LAST_SIGNAL
};

static guint camera_signals[LAST_SIGNAL] = { 0 };

static void camera_engine_dispose(GObject *object) {
    CameraEngine *self = CAMERA_ENGINE(object);
    
    if (self->pipeline) {
        g_mutex_lock(&self->recorder_mutex);
        if (self->recorder) {
            video_recorder_stop(self->recorder);
            self->recorder = NULL;
        }
        g_mutex_unlock(&self->recorder_mutex);

        gst_element_set_state(self->pipeline, GST_STATE_NULL);
        g_clear_object(&self->sink);
        g_clear_object(&self->pipeline);
    }
    
    G_OBJECT_CLASS(camera_engine_parent_class)->dispose(object);
}

static void camera_engine_finalize(GObject *object) {
    CameraEngine *self = CAMERA_ENGINE(object);
    g_mutex_clear(&self->recorder_mutex);
    g_free(self->record_filepath);
    G_OBJECT_CLASS(camera_engine_parent_class)->finalize(object);
}

static void camera_engine_class_init(CameraEngineClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = camera_engine_dispose;
    gobject_class->finalize = camera_engine_finalize;

    camera_signals[SIGNAL_FRAME_READY] = g_signal_new(
        "frame-ready",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL,
        NULL,
        G_TYPE_NONE,
        1, GDK_TYPE_TEXTURE);
}

typedef struct {
    CameraEngine *engine;
    GdkTexture *texture;
} FrameData;

static gboolean emit_frame_ready_idle(gpointer user_data) {
    FrameData *data = (FrameData *)user_data;
    g_signal_emit(data->engine, camera_signals[SIGNAL_FRAME_READY], 0, data->texture);
    g_object_unref(data->texture);
    g_object_unref(data->engine);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static GstFlowReturn on_new_sample(GstAppSink *sink, gpointer user_data) {
    CameraEngine *self = CAMERA_ENGINE(user_data);
    GstSample *sample = gst_app_sink_pull_sample(sink);
    if (!sample) return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    if (!caps) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    
    GstStructure *s = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        
        // Video Recording logic (Thread-Safe)
        g_mutex_lock(&self->recorder_mutex);
        if (self->is_recording_requested) {
            if (!self->recorder && !self->record_failed) {
                self->recorder = video_recorder_new(width, height, self->record_filepath);
                if (!self->recorder) {
                    self->record_failed = TRUE; 
                }
            }
            if (self->recorder) {
                video_recorder_push_frame(self->recorder, map.data, map.size);
            }
        }
        g_mutex_unlock(&self->recorder_mutex);

        // Live Feed UI logic
        GBytes *bytes = g_bytes_new(map.data, map.size);
        GdkTexture *texture = gdk_memory_texture_new(width, height, GDK_MEMORY_R8G8B8, bytes, width * 3);
        g_bytes_unref(bytes);
        gst_buffer_unmap(buffer, &map);

        if (texture) {
            FrameData *data = g_new(FrameData, 1);
            data->engine = g_object_ref(self);
            data->texture = texture;
            g_idle_add(emit_frame_ready_idle, data);
        }
    } else {
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

static void camera_engine_init(CameraEngine *self) {
    GError *error = NULL;
    g_mutex_init(&self->recorder_mutex);
    
    self->pipeline = gst_parse_launch("v4l2src ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink drop=true max-buffers=1 emit-signals=true sync=true", &error);
    
    if (error) {
        g_printerr("Failed to create pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }
    
    g_object_ref_sink(self->pipeline); // Take ownership

    self->sink = gst_bin_get_by_name(GST_BIN(self->pipeline), "sink");
    if (self->sink) {
        g_signal_connect(self->sink, "new-sample", G_CALLBACK(on_new_sample), self);
    }
}

CameraEngine *camera_engine_new(void) {
    return g_object_new(CAMERA_TYPE_ENGINE, NULL);
}

void camera_engine_start(CameraEngine *self) {
    if (self->pipeline) {
        gst_element_set_state(self->pipeline, GST_STATE_PLAYING);
    }
}

void camera_engine_stop(CameraEngine *self) {
    if (self->pipeline) {
        gst_element_set_state(self->pipeline, GST_STATE_NULL);
    }
}

void camera_engine_start_recording(CameraEngine *self, const char *filepath) {
    g_mutex_lock(&self->recorder_mutex);
    g_free(self->record_filepath);
    self->record_filepath = g_strdup(filepath);
    self->is_recording_requested = TRUE;
    self->record_failed = FALSE;
    g_mutex_unlock(&self->recorder_mutex);
}

void camera_engine_stop_recording(CameraEngine *self) {
    g_mutex_lock(&self->recorder_mutex);
    self->is_recording_requested = FALSE;
    if (self->recorder) {
        video_recorder_stop(self->recorder);
        self->recorder = NULL;
    }
    g_mutex_unlock(&self->recorder_mutex);
}
