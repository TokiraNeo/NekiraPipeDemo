# Server/Lib/CMakeLists.txt

set(HEADER
    Public/PipeSession.hpp
    Public/MessageItem.hpp
)

set(SOURCE
    Private/PipeSession.cpp
)

add_library(NekiraPipeServerLib SHARED ${HEADER} ${SOURCE})

target_include_directories(NekiraPipeServerLib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
