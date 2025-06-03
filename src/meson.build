source_files = [
            
            './ArmamentInFlightNotification.cpp',
            './ArmamentHitNotification.cpp',
            './Emesary.cpp',
            './export.cpp',
            './Factory.cpp',
            './ObjectInFlightNotification.cpp',
            './Processmsg.cpp',
            './StaticNotification.cpp',
            './Translator.cpp',
            './Utils.cpp',
            './Xdr.cpp',
            './Xdrclient.cpp',
            './AsaFGNetIO.cpp',
            './player/AsaFGMissile.cpp',
            './player/AsaFGFighter.cpp'

]

asa_fg = library(
    'AsaFG',
    [source_files],
    include_directories : public_headers,
    dependencies : [
        # 2nd party
        mixr_dep,
        asa_libs_dep,
        asa_models_dep,
        asa_models_r_dep,
        # 3rd party
        boost_chrono_dep,
        boost_filesystem_dep,
        fmt_dep,
        rapidjson_dep,
        zlib_dep,
        simgear_dep
    ],
    install : true,
    install_dir: get_option('prefix') / 'usr/extensions/AsaFG/bin',
)

asa_fg_dep = declare_dependency(
    link_with : asa_fg,
    include_directories : public_headers
)
