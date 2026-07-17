add_library(chat_plugin MODULE
    Chat.cpp
    Chat.hpp
    lib/ChatConfig.cpp
    lib/ChatConfig.hpp
    lib/SnipOverlay.cpp
    lib/SnipOverlay.hpp
    lib/providers/ProviderTypes.hpp
    lib/providers/GeminiProvider.cpp
    lib/providers/GeminiProvider.hpp
    lib/providers/GroqProvider.cpp
    lib/providers/GroqProvider.hpp
    lib/providers/OllamaProvider.cpp
    lib/providers/OllamaProvider.hpp
    lib/providers/OpenAIProvider.cpp
    lib/providers/OpenAIProvider.hpp
    lib/providers/DeepSeekProvider.cpp
    lib/providers/DeepSeekProvider.hpp
    ${QUOL_HOST_SOURCE_DIR}/ui/QuolPopupWindow.cpp
    ${QUOL_HOST_SOURCE_DIR}/ui/TitleBar.cpp
)

target_include_directories(chat_plugin PRIVATE
    ${QUOL_HOST_SOURCE_DIR}
    ${QUOL_HOST_ROOT}
)

target_link_libraries(chat_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network quol_core)

set_target_properties(chat_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "chat"
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_custom_command(TARGET chat_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:chat_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:chat_plugin>/res/logs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        $<TARGET_FILE_DIR:chat_plugin>/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        $<TARGET_FILE_DIR:chat_plugin>/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/chat/res/img
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QUOL_RUNTIME_DIR}/plugins/chat/res/logs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:chat_plugin>
        ${QUOL_RUNTIME_DIR}/plugins/chat/$<TARGET_FILE_NAME:chat_plugin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${QUOL_RUNTIME_DIR}/plugins/chat/res/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/img
        ${QUOL_RUNTIME_DIR}/plugins/chat/res/img
)
