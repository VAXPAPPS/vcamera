#include "video_recorder.h"
#include <gst/app/gstappsrc.h>

struct _VideoRecorder {
    GstElement *pipeline;
    GstElement *appsrc;
};

VideoRecorder *video_recorder_new(int width, int height, const char *filepath) {
    VideoRecorder *self = g_new0(VideoRecorder, 1);

    GError *error = NULL;
    // Added autoaudiosrc, audioconvert, audiorate, and vorbisenc to capture microphone audio
    // They are multiplexed with the video stream into the webmmux
    g_autofree char *pipeline_str = g_strdup_printf(
        "webmmux name=mux ! filesink location=\"%s\" "
        "appsrc name=src is-live=true do-timestamp=true format=time ! "
        "video/x-raw,format=RGB,width=%d,height=%d,framerate=30/1 ! "
        "queue max-size-buffers=30 ! videoconvert ! vp8enc deadline=1 ! mux. "
        "autoaudiosrc ! queue ! audioconvert ! audiorate ! vorbisenc ! mux.",
        filepath, width, height);

    self->pipeline = gst_parse_launch(pipeline_str, &error);
    if (error) {
        g_printerr("Video recorder error: %s\n", error->message);
        g_clear_error(&error);
        g_free(self);
        return NULL;
    }

    g_object_ref_sink(self->pipeline);

    self->appsrc = gst_bin_get_by_name(GST_BIN(self->pipeline), "src");
    if (!self->appsrc) {
        g_printerr("Failed to find appsrc in pipeline\n");
    } else {
        GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGB",
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
        gst_app_src_set_caps(GST_APP_SRC(self->appsrc), caps);
        gst_caps_unref(caps);
    }

    gst_element_set_state(self->pipeline, GST_STATE_PLAYING);

    return self;
}

void video_recorder_push_frame(VideoRecorder *self, const guint8 *data, gsize size) {
    if (!self || !self->appsrc) return;

    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
    gst_buffer_fill(buffer, 0, data, size);
    
    gst_app_src_push_buffer(GST_APP_SRC(self->appsrc), buffer);
}

void video_recorder_stop(VideoRecorder *self) {
    if (!self) return;
    
    if (self->appsrc) {
        gst_app_src_end_of_stream(GST_APP_SRC(self->appsrc));
    }

    GstBus *bus = gst_element_get_bus(self->pipeline);
    if (bus) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 2 * GST_SECOND, GST_MESSAGE_EOS | GST_MESSAGE_ERROR);
        if (msg) gst_message_unref(msg);
        gst_object_unref(bus);
    }

    gst_element_set_state(self->pipeline, GST_STATE_NULL);
    
    g_clear_object(&self->appsrc);
    g_clear_object(&self->pipeline);
    g_free(self);
}
