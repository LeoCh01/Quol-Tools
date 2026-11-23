quol_add_plugin(api_plugin "api"
    Api.cpp
    Api.hpp
    LIBRARIES Qt${QT_VERSION_MAJOR}::Network
)
