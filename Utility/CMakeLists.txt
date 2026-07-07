# Utility/CMakeLists.txt

set(HEADER
    Public/NekiraPipeUtility.hpp
)

set(SOURCE
    Private/NekiraPipeUtility.cpp
)

add_library(NekiraPipeUtility SHARED ${HEADER} ${SOURCE})

target_include_directories(NekiraPipeUtility
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_link_libraries(NekiraPipeUtility
    PUBLIC
    stduuid::stduuid
)
