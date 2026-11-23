cmake_minimum_required(VERSION 3.16)
project(QuolPlugins LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
set(CMAKE_SHARED_MODULE_PREFIX "")

# Path to your original Quol app project (host app + quol_core).
set(QUOL_HOST_ROOT "" CACHE PATH "Path to Quol-CPP host project")

if(NOT QUOL_HOST_ROOT)
    message(FATAL_ERROR
        "QUOL_HOST_ROOT is not set. Define it in CMakeUserPresets.json (cacheVariables.QUOL_HOST_ROOT)."
    )
endif()

set(QUOL_HOST_SOURCE_DIR "${QUOL_HOST_ROOT}/src")
# Host app build dir. Read-only for this project: only the core import library and DLL come
# from it. Deploying plugins there is copy-plugins.ps1's job.
set(QUOL_RUNTIME_DIR "${QUOL_HOST_ROOT}/build/build-release")
set(QUOL_CORE_IMPORT_LIB "${QUOL_RUNTIME_DIR}/libquol_core.dll.a")
set(QUOL_CORE_DLL "${QUOL_RUNTIME_DIR}/libquol_core.dll")

foreach(_required "${QUOL_HOST_SOURCE_DIR}" "${QUOL_CORE_IMPORT_LIB}" "${QUOL_CORE_DLL}")
    if(NOT EXISTS "${_required}")
        message(FATAL_ERROR "Missing ${_required}. Check QUOL_HOST_ROOT and build the host in release.")
    endif()
endforeach()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network OpenGL)

# Plugin sources include headers via paths like plugins/example/Example.hpp.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Link plugins against the already-built host core library from the Quol app project.
add_library(quol_core SHARED IMPORTED GLOBAL)
set_target_properties(quol_core PROPERTIES
    IMPORTED_IMPLIB "${QUOL_CORE_IMPORT_LIB}"
    IMPORTED_LOCATION "${QUOL_CORE_DLL}"
)

# quol_add_plugin(<target> <outputName> <sources...> [LIBRARIES ...] [INCLUDE_DIRS ...])
# Output stays inside this repo; copy-plugins.ps1 pushes it to the host app build.
# res files are link dependencies so a res-only edit relinks the DLL and re-stages res/.
function(quol_add_plugin target output_name)
    cmake_parse_arguments(ARG "" "" "LIBRARIES;INCLUDE_DIRS" ${ARGN})

    add_library(${target} MODULE ${ARG_UNPARSED_ARGUMENTS})

    target_include_directories(${target} PRIVATE
        ${QUOL_HOST_SOURCE_DIR}
        ${QUOL_HOST_ROOT}
        ${ARG_INCLUDE_DIRS}
    )

    target_link_libraries(${target} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${ARG_LIBRARIES}
        quol_core
    )

    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output_name}")

    file(GLOB_RECURSE _res_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/*")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${_res_files})

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:${target}>/res"
    )
endfunction()

# Every plugins/<name>/ that has a CMakeLists.txt is built.
file(GLOB _plugin_dirs LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")
foreach(_dir ${_plugin_dirs})
    if(EXISTS "${_dir}/CMakeLists.txt")
        add_subdirectory("${_dir}")
    endif()
endforeach()
