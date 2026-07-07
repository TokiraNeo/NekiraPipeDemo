add_library(stduuid INTERFACE)
add_library(stduuid::stduuid ALIAS stduuid)

target_include_directories(stduuid
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
