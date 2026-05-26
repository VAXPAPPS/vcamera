project('vcamera', 'c',
  version: '0.1.0',
  meson_version: '>= 0.59.0',
  default_options: ['warning_level=2', 'c_std=gnu11'],
)

# Dependencies
gtk_dep = dependency('gtk4', version: '>= 4.0.0')
gstreamer_dep = dependency('gstreamer-1.0', version: '>= 1.18.0')
gstreamer_video_dep = dependency('gstreamer-video-1.0')
gstreamer_app_dep = dependency('gstreamer-app-1.0')

# Source files and include directories
subdir('src')
