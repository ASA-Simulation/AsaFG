source_files = [
    './main.cpp',
]

xdr_client = executable(
    'xdr_client',
    [source_files],
    include_directories : public_headers,
    dependencies : [
        # 1st party
        asa_fg_dep,
        # 2nd party
        asa_libs_dep,
        asa_models_dep,
        asa_models_r_dep,
        # 3rd party
        boost_filesystem_dep,
        boost_log_dep,
        boost_log_setup_dep,
        fmt_dep,
    ],
    install : true,
)
