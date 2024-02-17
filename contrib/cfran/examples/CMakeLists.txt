set(
    source_files
    cf-example.cc
    # cf-mc-twoenbs.cc
)
build_lib_example(
    NAME cf-example
    SOURCE_FILES ${source_files}
    LIBRARIES_TO_LINK ${libcfran}
)

