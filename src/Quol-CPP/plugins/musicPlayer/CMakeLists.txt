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

quol_add_plugin(musicplayer_plugin "musicPlayer"
    MusicPlayer.cpp
    MusicPlayer.hpp
    LIBRARIES ole32 winmm
    INCLUDE_DIRS ${DOWNLOADS_DIR}
)
