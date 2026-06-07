add_library(draw_plugin MODULE
    Draw.cpp
    Draw.hpp
    lib/DrawOverlay.cpp
    lib/DrawOverlay.hpp
    lib/ColorWheel.cpp
    lib/ColorWheel.hpp
)

target_include_directories(draw_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(draw_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(draw_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "draw"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET draw_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/draw/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:draw_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/draw/$<TARGET_FILE_NAME:draw_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/draw/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/draw/res/img
)
