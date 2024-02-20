# build_lib_example(
#     NAME oran-interface-example
#     SOURCE_FILES ric-indication-messages.cc
#                  test-wrappers.cc
#                  oran-interface-example.cc
#                  l3-rrc-example.cc
#                  ric-control-function-desc.cc
#                  e2sim-integration-example.cc
#     LIBRARIES_TO_LINK ${liboran-interface}
# )

set(base_examples
    ric-indication-messages
    test-wrappers
    oran-interface-example
    l3-rrc-example
    ric-control-function-desc
    e2sim-integration-example
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${liboran-interface}
  )
endforeach()



