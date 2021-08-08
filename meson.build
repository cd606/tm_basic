project('TM kit basic', 'cpp', default_options: ['cpp_std=c++17'])

inc = include_directories('.')
thread_dep = dependency('threads')
boost_dep = dependency('boost', modules : ['log','log_setup','system','thread','filesystem','iostreams'])
spd_dep = dependency('spdlog')
tm_infra_dep = dependency('tm_kit_infra')
common_deps = [thread_dep, boost_dep, spd_dep, tm_infra_dep]

pkg = import('pkgconfig')

subdir('tm_kit/basic')
subdir('src')
