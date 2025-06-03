project('asa-fg', 'cpp',
    license: 'Copyright © 2021-2023 ASA. All rights reserved.',
    default_options : [
        'c_std=c11',
        'cpp_std=c++17'
    ]
)

script = run_command('./version.sh', check: true)
version = script.stdout().strip()

buildtype = get_option('buildtype')

message('Building version:', version)
message('Building type:', buildtype)

# Dependencies

pkg = import('pkgconfig')

# 2nd party dependencies
mixr_dep             = dependency('mixr')
asa_libs_dep         = dependency('asa-libs')
asa_models_dep       = dependency('asa-models')
asa_models_r_dep       = dependency('asa-models-r')

# 3rd party dependencies
boost_chrono_dep     = dependency('boost-chrono')
boost_filesystem_dep = dependency('boost-filesystem')
boost_log_dep = dependency('boost-log')
boost_log_setup_dep = dependency('boost-log_setup')
fmt_dep              = dependency('fmt')
protobuf_dep         = dependency('protobuf')
rapidjson_dep        = dependency('rapidjson')
zlib_dep= dependency('zlib')

simgear_dep= dependency('simgear')

#CXX = meson.get_compiler('cpp')

#simgear_scene_dep        = CXX.find_library('libSimGearScene', dirs : ['/usr/lib'])
#simgear_core_dep        = CXX.find_library('libSimGearCore', dirs : ['/usr/lib'])

public_headers = include_directories('include')

subdir('./src')
subdir('./tests/xdr_client')

# Installation

conf_data = configuration_data({
    'prefix' : get_option('prefix'),
    'libdir' : get_option('prefix') / 'usr/extensions/AsaFG/bin',
    'description' : 'Asa FlightGear.',
    'version' : version,
})

configure_file(
    input : 'asa-fg.pc.in',
    output : 'asa-fg.pc',
    configuration : conf_data,
    install : true,
    install_dir : get_option('prefix') / 'lib/pkgconfig'
)

install_subdir(
    'include/asa/fg',
    install_dir : 'include/asa',
)

install_subdir(
    'resources/demos',
    install_dir : 'usr/debug',
)

install_subdir(
    'resources/templates',
    install_dir : 'usr/extensions/AsaFG',
)

install_data(
    'resources/config.json',
    install_dir : 'usr/extensions/AsaFG'
)
