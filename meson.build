project('ren-utils', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'default_library=static',
    'cpp_std=c++17'])

inc = include_directories('include')

src = files(
  'src/basic.cpp'
)

ren_utils_lib = library('ren_utils',
  src,
  include_directories : inc,
)

ren_utils_dep = declare_dependency(
  include_directories : inc,
  link_with : ren_utils_lib
)

# Build integration tests
if get_option('build_tests')
  test_bb_src = []
  subdir('tests')

  test_bb_exe = executable('bb_tests', test_bb_src,
    dependencies : [dependency('gtest_main')],
    link_with : ren_utils_lib,
    include_directories : inc)

  test('black_box', test_bb_exe, workdir : meson.build_root())
endif
