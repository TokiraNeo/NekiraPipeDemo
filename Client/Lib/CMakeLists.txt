# Client/Lib/CMakeLists.txt

set(HEADER
    Public/NekiraClientLib.hpp
)

set(SOURCE
    Private/NekiraClientLib.cpp
)

add_library(NekiraPipeClientLib SHARED ${HEADER} ${SOURCE})

target_include_directories(NekiraPipeClientLib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)