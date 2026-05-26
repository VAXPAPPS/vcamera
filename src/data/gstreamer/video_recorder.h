#pragma once
#include <gst/gst.h>

typedef struct _VideoRecorder VideoRecorder;

VideoRecorder *video_recorder_new(int width, int height, const char *filepath);
void video_recorder_push_frame(VideoRecorder *self, const guint8 *data, gsize size);
void video_recorder_stop(VideoRecorder *self);
