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
        # 3rd party
        boost_filesystem_dep,
        boost_log_dep,
        boost_log_setup_dep,
        fmt_dep,
    ],
    install : true,
)
