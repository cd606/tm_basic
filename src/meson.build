tm_basic_sources = [
      'real_time_clock/Clock.cpp'
    , 'real_time_clock/ClockComponent.cpp'
    , 'single_pass_iteration_clock/Clock.cpp'
    , 'single_pass_iteration_clock/ClockComponent.cpp'
    , 'ChronoUtils_AddOn.cpp'
    , 'DateHolder.cpp'
]
if get_option('buildtype') == 'debug'
  tm_basic_lib_debug = static_library('tm_kit_basic_debug', tm_basic_sources, include_directories: inc, dependencies: [common_deps, dependency('date_tz')], install: true)
  pkg.generate(tm_basic_lib_debug)
else
  tm_basic_lib = static_library('tm_kit_basic', tm_basic_sources, include_directories: inc, dependencies: [common_deps, dependency('date_tz')], install: true)
  pkg.generate(tm_basic_lib)
endif
