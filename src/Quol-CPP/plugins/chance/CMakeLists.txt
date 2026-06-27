add_library(chance_plugin MODULE
    Chance.cpp
    Chance.hpp
)

target_include_directories(chance_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(chance_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets quol_core)

set_target_properties(chance_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "chance"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET chance_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:chance_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:chance_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        $<TARGET_FILE_DIR:chance_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/chance/res/img
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:chance_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/chance/$<TARGET_FILE_NAME:chance_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/chance/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/chance/res/img
)
