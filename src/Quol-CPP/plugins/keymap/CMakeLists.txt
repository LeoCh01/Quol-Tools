add_library(keymap_plugin MODULE
    Keymap.cpp
    Keymap.hpp
    lib/KeymapGroupDialog.cpp
    lib/KeymapGroupDialog.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(keymap_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(keymap_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(keymap_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "keymap"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET keymap_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:keymap_plugin>/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:keymap_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/keymaps.json
        $<TARGET_FILE_DIR:keymap_plugin>/res/keymaps.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/keymap/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:keymap_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/keymap/$<TARGET_FILE_NAME:keymap_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/keymap/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/keymaps.json
        ${QUOL_RUNTIME_DIR}/plugins/keymap/res/keymaps.json
)
