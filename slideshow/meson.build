# Rules for building with Meson

project(
  'slideshow-drm',
  'c',
  default_options: [
    'c_std=c99',
    'warning_level=3',
    'buildtype=release',
    'b_ndebug=if-release',
  ],
  license: 'MIT',
  version: '1.0',
  meson_version: '>=0.60.0',
)

add_project_arguments(
  [
    '-D_POSIX_C_SOURCE=200809',
    '-DAPP_VERSION="@0@"'.format(meson.project_version()),
  ],
  language: 'c',
)

executable(
  'slideshow',
  sources: [
    'src/display.c',
    'src/image.c',
    'src/imglist.c',
    'src/main.c',
    'src/sshow.c',
  ],
  dependencies: [
    dependency('libdrm'),
    dependency('libjpeg'),
  ],
  install: true
)
