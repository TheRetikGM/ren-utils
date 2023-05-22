project('ren-utils', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'default_library=static',
    'cpp_std=c++17'])

inc = include_directories('include')

src = files(
  'basic.cpp'
)

ren_utils_lib = library('ren_utils',
  src,
  include_directories : inc,
)

ren_utils_dep = declare_dependency(
  include_directories : inc,
  link_with : ren_utils_lib
)
