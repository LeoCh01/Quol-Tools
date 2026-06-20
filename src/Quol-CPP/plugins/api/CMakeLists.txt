add_library(api_plugin MODULE
    Api.cpp
    Api.hpp
)

target_include_directories(api_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(api_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network quol_core)

set_target_properties(api_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "api"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET api_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:api_plugin>/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:api_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/api/res
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:api_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/api/$<TARGET_FILE_NAME:api_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/api/res/config.json
)
