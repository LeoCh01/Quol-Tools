add_library(macro_plugin MODULE
    Macro.cpp
    Macro.hpp
)

target_include_directories(macro_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(macro_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(macro_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "macro"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET macro_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:macro_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:macro_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        $<TARGET_FILE_DIR:macro_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/macro/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:macro_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/macro/$<TARGET_FILE_NAME:macro_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/macro/res/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/macro/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/macro/res/img
)
