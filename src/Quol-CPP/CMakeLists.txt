cmake_minimum_required(VERSION 3.16)
project(QuolPlugins LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

# Path to your original Quol app project (host app + quol_core).
set(QUOL_HOST_ROOT "" CACHE PATH "Path to Quol-CPP host project")

if(NOT QUOL_HOST_ROOT)
    message(FATAL_ERROR
        "QUOL_HOST_ROOT is not set. Define it in CMakeUserPresets.json (cacheVariables.QUOL_HOST_ROOT)."
    )
endif()

set(QUOL_HOST_SOURCE_DIR "${QUOL_HOST_ROOT}/src")
set(QUOL_RUNTIME_DIR "${QUOL_HOST_ROOT}/build/build-release")
set(QUOL_CORE_IMPORT_LIB "${QUOL_RUNTIME_DIR}/libquol_core.dll.a")
set(QUOL_CORE_DLL "${QUOL_RUNTIME_DIR}/libquol_core.dll")

if(NOT EXISTS "${QUOL_HOST_SOURCE_DIR}")
    message(FATAL_ERROR "QUOL_HOST_SOURCE_DIR does not exist: ${QUOL_HOST_SOURCE_DIR}")
endif()

if(NOT EXISTS "${QUOL_RUNTIME_DIR}")
    message(FATAL_ERROR "QUOL_RUNTIME_DIR does not exist: ${QUOL_RUNTIME_DIR}")
endif()

if(NOT EXISTS "${QUOL_CORE_IMPORT_LIB}")
    message(FATAL_ERROR "QUOL_CORE_IMPORT_LIB does not exist: ${QUOL_CORE_IMPORT_LIB}")
endif()

if(NOT EXISTS "${QUOL_CORE_DLL}")
    message(FATAL_ERROR "QUOL_CORE_DLL does not exist: ${QUOL_CORE_DLL}")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# Plugin sources include headers via paths like plugins/example/Example.hpp.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Link plugins against the already-built host core library from the Quol app project.
add_library(quol_core SHARED IMPORTED GLOBAL)
set_target_properties(quol_core PROPERTIES
    IMPORTED_IMPLIB "${QUOL_CORE_IMPORT_LIB}"
    IMPORTED_LOCATION "${QUOL_CORE_DLL}"
)

# Build plugins from this repo.
add_subdirectory(plugins/example)
add_subdirectory(plugins/chance)
add_subdirectory(plugins/colorPicker)
add_subdirectory(plugins/chat)
add_subdirectory(plugins/draw)
add_subdirectory(plugins/keymap)
add_subdirectory(plugins/macro)
add_subdirectory(plugins/api)
add_subdirectory(plugins/misc)
