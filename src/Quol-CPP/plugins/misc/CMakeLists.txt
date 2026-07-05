add_library(misc_plugin MODULE
    Misc.cpp
    Misc.hpp
    lib/ToolBase.hpp
    lib/StopwatchWidget.cpp
    lib/StopwatchWidget.hpp
    lib/DiceWidget.cpp
    lib/DiceWidget.hpp
)

target_include_directories(misc_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(misc_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(misc_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "misc"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET misc_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:misc_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/stopwatch/img
        $<TARGET_FILE_DIR:misc_plugin>/res/stopwatch/img
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/dice/img
        $<TARGET_FILE_DIR:misc_plugin>/res/dice/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/misc/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:misc_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/misc/$<TARGET_FILE_NAME:misc_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/misc/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/stopwatch/img
        ${QUOL_RUNTIME_DIR}/plugins/misc/res/stopwatch/img
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/dice/img
        ${QUOL_RUNTIME_DIR}/plugins/misc/res/dice/img
)
