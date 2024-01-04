check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(source_files
    helper/cfran-helper.cc
    model/cfran-net-device.cc
)

set(header_files
    helper/cfran-helper.h
    model/cfran-net-device.h
)


set(examples_as_tests_sources)
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

