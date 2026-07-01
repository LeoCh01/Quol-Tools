get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../" ABSOLUTE)
set(DOWNLOADS_DIR "${PROJECT_ROOT}/downloads")
set(MINIAUDIO_URL "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h")
set(MINIAUDIO_PATH "${DOWNLOADS_DIR}/miniaudio.h")

file(MAKE_DIRECTORY ${DOWNLOADS_DIR})

if(NOT EXISTS ${MINIAUDIO_PATH})
    message(STATUS "Downloading miniaudio.h...")
    file(DOWNLOAD ${MINIAUDIO_URL} ${MINIAUDIO_PATH} STATUS download_status)
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download miniaudio.h (status ${download_status})")
    endif()
    message(STATUS "miniaudio.h downloaded successfully")
endif()

add_library(musicplayer_plugin MODULE
    MusicPlayer.cpp
    MusicPlayer.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(musicplayer_plugin PRIVATE
    ${DOWNLOADS_DIR}
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(musicplayer_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core ole32 winmm)

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
)
