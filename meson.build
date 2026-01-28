project(
  'compare-images-inator',
  'cpp',
  meson_version: '>= 1.3.0',
  version: '0.0.0',
  default_options: ['warning_level=3', 'cpp_std=c++20', 'default_library=static'],
)

sfml_proj = subproject('sfml', default_options: ['miniaudio:c_std=c11'])

executable(
  'compare-images-inator',
  'src/main.cpp',
  dependencies: [
    sfml_proj.get_variable('sfml_dep'),
    subproject('imgui').get_variable('imgui_dep'),
    subproject('imgui-sfml').get_variable('imgui_sfml_dep')
  ],
  include_directories: [
    include_directories('subprojects/imgui-sfml-3.0'),
    include_directories('subprojects/imgui-1.91.6'),
    include_directories('subprojects/imgui-1.91.6/backends')
  ],
  install: true,
  link_args: ['-ObjC']
)
