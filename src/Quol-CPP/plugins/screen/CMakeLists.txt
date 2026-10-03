add_library(screen_plugin MODULE
    Screen.cpp
    Screen.hpp
    lib/ShaderWidget.cpp
    lib/ShaderWidget.hpp
    lib/SnipOverlay.cpp
    lib/SnipOverlay.hpp
    lib/SnipItem.cpp
    lib/SnipItem.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(screen_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(screen_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL Qt${QT_VERSION_MAJOR}::Network quol_core)

set_target_properties(screen_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "screen"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET screen_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:screen_plugin>/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:screen_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:screen_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img/setting.svg
        $<TARGET_FILE_DIR:screen_plugin>/res/img/setting.svg
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img/snip.svg
        $<TARGET_FILE_DIR:screen_plugin>/res/img/snip.svg
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:screen_plugin>/res/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders
        $<TARGET_FILE_DIR:screen_plugin>/res/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/screen/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:screen_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/screen/$<TARGET_FILE_NAME:screen_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/screen/res/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/screen/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img/setting.svg
        ${QUOL_RUNTIME_DIR}/plugins/screen/res/img/setting.svg
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img/snip.svg
        ${QUOL_RUNTIME_DIR}/plugins/screen/res/img/snip.svg
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/screen/res/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders
        ${QUOL_RUNTIME_DIR}/plugins/screen/res/shaders
)
