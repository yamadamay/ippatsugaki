set(test_names
    tst_text_utils
    tst_prompts
    tst_timer
    tst_draft
    tst_controller
)

foreach(test_name IN LISTS test_names)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE ippatsugaki_core Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

target_compile_definitions(tst_prompts PRIVATE
    PROMPTS_JSON_PATH="${CMAKE_SOURCE_DIR}/data/prompts.json"
)
