include_directories(/usr/local/include/e2sim) # CXXFLAGS -I counterpart
# add_definitions(-DLAPACK -DLUSOLVER=LAPACK) # CXXDEFINES counterpart
link_directories(/usr/local/lib) # LINKFLAGS -L counterpart
link_libraries(e2sim) # LINKFLAGS -l or LIB counterpart

check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(source_files
    helper/cfran-helper.cc
    helper/system-info.cc
    model/cf-unit.cc
    model/cf-unit-ue-iso.cc
    model/cf-model.cc
    model/cf-application.cc
    # helper/vr-server-helper.cc
    # helper/vr-client-helper.cc
    # model/cf-unit-simple.cc
    # model/vr-server.cc
    # model/vr-client.cc
    model/ue-cf-application.cc
    model/task-request-header.cc
    model/cf-radio-header.cc
    model/cf-x2-header.cc
    model/ue-simple-client.cc
    model/multi-packet-header.cc
    helper/ue-cf-application-helper.cc
    helper/cf-application-helper.cc
    helper/multi-packet-manager.cc
    helper/cf-e2e-calculator.cc
    helper/cf-e2e-buffer.cc
)

set(header_files
    helper/cfran-helper.h
    helper/system-info.h
    model/cf-unit.h
    model/cf-unit-ue-iso.h
    # helper/vr-server-helper.h
    # helper/vr-client-helper.h
    # model/cf-unit-simple.h
    # model/vr-server.h
    # model/vr-client.h
    model/cf-model.h
    model/cf-application.h
    model/ue-cf-application.h
    model/task-request-header.h
    model/cf-radio-header.h
    model/cf-x2-header.h
    model/ue-simple-client.h
    model/multi-packet-header.h
    helper/ue-cf-application-helper.h
    helper/cf-application-helper.h
    helper/multi-packet-manager.h
    helper/cf-e2e-calculator.h
    helper/cf-e2e-buffer.h
)


set(examples_as_tests_sources
    # examples/cf-example.cc
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
    LIBRARIES_TO_LINK ${libmmwave} ${liboran-interface}
    TEST_SOURCES test/cfran-test-suite.cc
                 ${examples_as_tests_sources}
)

