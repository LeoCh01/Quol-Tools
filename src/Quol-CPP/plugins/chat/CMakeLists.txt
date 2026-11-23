quol_add_plugin(chat_plugin "chat"
    Chat.cpp
    Chat.hpp
    lib/ChatConfig.cpp
    lib/ChatConfig.hpp
    lib/ChatOutput.cpp
    lib/ChatOutput.hpp
    lib/LatexRenderer.cpp
    lib/LatexRenderer.hpp
    lib/OcrTextOverlay.cpp
    lib/OcrTextOverlay.hpp
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
    LIBRARIES Qt${QT_VERSION_MAJOR}::Network
)
