cmake_minimum_required(VERSION 3.16)
project(JttyChat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HAMLIB REQUIRED IMPORTED_TARGET hamlib)

add_executable(JttyChat
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ChatBubble.h
    src/ChatBubble.cpp
    src/SettingsDialog.h
    src/SettingsDialog.cpp
    src/HamlibRigs.h
    src/HamlibRigs.cpp
)

target_link_libraries(JttyChat PRIVATE Qt6::Widgets Qt6::Multimedia PkgConfig::HAMLIB)

install(TARGETS JttyChat RUNTIME DESTINATION bin)
install(FILES packaging/jttychat.desktop DESTINATION share/applications)
install(FILES packaging/jttychat.png DESTINATION share/icons/hicolor/256x256/apps)
