add_library(example_plugin MODULE
    Example.cpp
    Example.hpp
    lib/adder.cpp
)

target_include_directories(example_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(example_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(example_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "example"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET example_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:example_plugin>/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:example_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/example/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:example_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/example/$<TARGET_FILE_NAME:example_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/example/res/config.json
)
