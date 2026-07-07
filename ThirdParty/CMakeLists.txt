add_library(NekiraPipeThirdParty INTERFACE)
add_library(NekiraPipe::ThirdParty ALIAS NekiraPipeThirdParty)

add_subdirectory(nlohmann)
add_subdirectory(stduuid)

target_link_libraries(NekiraPipeThirdParty
    INTERFACE
    nlohmann_json::nlohmann_json
    stduuid::stduuid
)
