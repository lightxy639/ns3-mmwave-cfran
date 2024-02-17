check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(source_files
    helper/cfran-helper.cc
    helper/system-info.cc
    helper/vr-server-helper.cc
    model/cf-unit.cc
    model/cf-model.cc
    model/vr-server.cc
    model/cf-application.cc
)

set(header_files
    helper/cfran-helper.h
    helper/system-info.h
    helper/vr-server-helper.h
    model/cf-unit.h
    model/cf-model.h
    model/vr-server.h
    model/cf-application.h
)


set(examples_as_tests_sources
    examples/cf-example.cc
    # examples/cf-mc-twoenbs.cc
)

if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/cfran-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME cfran
    SOURCE_FILES ${source_files}
    HEADER_FILES ${header_files}
    LIBRARIES_TO_LINK ${libmmwave}
    TEST_SOURCES test/cfran-test-suite.cc
                 ${examples_as_tests_sources}
)

