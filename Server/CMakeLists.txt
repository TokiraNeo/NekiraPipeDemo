# Server/CMakeLists.txt

add_subdirectory(Lib)

add_executable(NekiraPipeServer Main/ServerMain.cpp)

target_link_libraries(NekiraPipeServer PRIVATE NekiraPipeServerLib)
