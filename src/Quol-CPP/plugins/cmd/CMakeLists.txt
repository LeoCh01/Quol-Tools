add_library(cmd_plugin MODULE
    Cmd.cpp
    Cmd.hpp
    lib/CommandDialog.cpp
    lib/CommandDialog.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(cmd_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(cmd_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(cmd_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "cmd"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET cmd_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cmd_plugin>/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:cmd_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/commands.json
        $<TARGET_FILE_DIR:cmd_plugin>/res/commands.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/cmd/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cmd_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/cmd/$<TARGET_FILE_NAME:cmd_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/cmd/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/commands.json
        ${QUOL_RUNTIME_DIR}/plugins/cmd/res/commands.json
)
