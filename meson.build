project('TM kit basic', 'cpp', default_options: ['cpp_std=c++17'])

inc = include_directories('.')
thread_dep = dependency('threads')
boost_dep = dependency('boost', modules : ['log','log_setup','system','thread','filesystem','iostreams'])
spd_dep = dependency('spdlog')
tm_infra_dep = dependency('tm_kit_infra')
nlohmann_json_dep = dependency('nlohmann-json')
simd_json_dep = dependency('simdjson')
common_deps = [thread_dep, boost_dep, spd_dep, tm_infra_dep, nlohmann_json_dep, simd_json_dep]

if build_machine.system() == 'windows'
  add_global_arguments('-DWINVER=0x0601', language: 'cpp')
  add_global_arguments('-D_WIN32_WINNT=0x0601', language: 'cpp')
  add_global_arguments('-DBOOST_USE_WINAPI_VERSION=BOOST_WINAPI_VERSION_WIN7', language: 'cpp')
endif

pkg = import('pkgconfig')

subdir('tm_kit/basic')
subdir('src')
