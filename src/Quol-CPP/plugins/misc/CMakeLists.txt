quol_add_plugin(misc_plugin "misc"
    Misc.cpp
    Misc.hpp
    lib/ToolBase.hpp
    lib/StopwatchWidget.cpp
    lib/StopwatchWidget.hpp
    lib/DiceWidget.cpp
    lib/DiceWidget.hpp
    lib/AnimeWidget.cpp
    lib/AnimeWidget.hpp
    LIBRARIES Qt${QT_VERSION_MAJOR}::Network
)
