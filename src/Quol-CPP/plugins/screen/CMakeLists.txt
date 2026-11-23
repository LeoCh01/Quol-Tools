quol_add_plugin(screen_plugin "screen"
    Screen.cpp
    Screen.hpp
    lib/ShaderWidget.cpp
    lib/ShaderWidget.hpp
    lib/SnipOverlay.cpp
    lib/SnipOverlay.hpp
    lib/SnipItem.cpp
    lib/SnipItem.hpp
    LIBRARIES Qt${QT_VERSION_MAJOR}::OpenGL Qt${QT_VERSION_MAJOR}::Network
)
