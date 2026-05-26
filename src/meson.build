vcamera_sources = [
  'main.c',
  'data/gstreamer/camera_engine.c',
  'data/gstreamer/video_recorder.c',
  'domain/use_cases/take_photo.c',
  'presentation/gtk/main_window.c',
  'theme_manager.c'
]

executable('vcamera',
  vcamera_sources,
  dependencies: [gtk_dep, gstreamer_dep, gstreamer_video_dep, gstreamer_app_dep, zbar_dep],
  install: true,
)
