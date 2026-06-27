add_library(color_picker_plugin MODULE
    ColorPicker.cpp
    ColorPicker.hpp
)

target_include_directories(color_picker_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(color_picker_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(color_picker_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "colorPicker"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET color_picker_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:color_picker_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:color_picker_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        $<TARGET_FILE_DIR:color_picker_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/colorPicker/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:color_picker_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/colorPicker/$<TARGET_FILE_NAME:color_picker_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/colorPicker/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/colorPicker/res/img
)
