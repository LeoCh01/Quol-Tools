if(NOT Qt6Multimedia_FOUND AND NOT Qt5Multimedia_FOUND)
    message(FATAL_ERROR "Qt Multimedia is required for the musicPlayer plugin")
endif()

# Compute Qt root from Qt6_DIR (e.g. C:/Qt/6.11.0/mingw_64/lib/cmake/Qt6)
get_filename_component(QT6_ROOT "${Qt6_DIR}/../../.." ABSOLUTE)

add_library(musicplayer_plugin MODULE
    MusicPlayer.cpp
    MusicPlayer.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(musicplayer_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(musicplayer_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia quol_core)

set_target_properties(musicplayer_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "musicPlayer"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET musicplayer_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:musicplayer_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:musicplayer_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        $<TARGET_FILE_DIR:musicplayer_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/musicPlayer/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:musicplayer_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/musicPlayer/$<TARGET_FILE_NAME:musicplayer_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/musicPlayer/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/musicPlayer/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::Multimedia>"
        ${QUOL_RUNTIME_DIR}/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/multimedia
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_ROOT}/plugins/multimedia/windowsmediaplugin.dll"
        ${QUOL_RUNTIME_DIR}/plugins/multimedia/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_ROOT}/plugins/multimedia/ffmpegmediaplugin.dll"
        ${QUOL_RUNTIME_DIR}/plugins/multimedia/
)
