# Server/CMakeLists.txt

add_subdirectory(Lib)

add_executable(NekiraPipeServer Main/ServerMain.cpp Main/ServerApp.hpp Main/ServerApp.cpp)

target_link_libraries(NekiraPipeServer PRIVATE NekiraPipeServerLib NekiraPipeUtility nlohmann_json::nlohmann_json)
