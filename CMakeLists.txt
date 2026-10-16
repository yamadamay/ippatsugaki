cmake_minimum_required(VERSION 3.24)

project(ippatsugaki VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
qt_standard_project_setup()

add_library(ippatsugaki_core STATIC
    src/core/text_utils.cpp
    src/core/embedded_prompts.cpp
    src/core/prompt_source.cpp
    src/core/clock.cpp
    src/core/session_timer.cpp
    src/core/draft_store.cpp
    src/core/session_controller.cpp
)
target_include_directories(ippatsugaki_core PUBLIC src)
target_link_libraries(ippatsugaki_core PUBLIC Qt6::Core)

qt_add_executable(ippatsugaki
    src/main.cpp
    src/ui/plain_editor.cpp
    src/ui/main_window.cpp
)
target_link_libraries(ippatsugaki PRIVATE ippatsugaki_core Qt6::Widgets)

add_custom_command(TARGET ippatsugaki POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ippatsugaki>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/data/prompts.json"
        "$<TARGET_FILE_DIR:ippatsugaki>/data/prompts.json"
)

enable_testing()
add_subdirectory(tests)

install(TARGETS ippatsugaki RUNTIME DESTINATION bin)
install(FILES data/prompts.json DESTINATION bin/data)

set(CPACK_PACKAGE_NAME "ippatsugaki")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ")
include(CPack)
