cmake_minimum_required(VERSION 3.16)
project(JttyChat VERSION 1.0.0 LANGUAGES CXX Fortran)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HAMLIB REQUIRED IMPORTED_TARGET hamlib)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)
find_package(OpenMP REQUIRED COMPONENTS Fortran)

# JTTY mode codec (encode/decode DSP), copied from wsjtx-3.2.0-rc1's lib/jtty
# and lib/ (see thirdparty/jtty_codec/NOTICE.txt). GPLv3, same as the rest of
# this project's distribution.
file(GLOB jtty_codec_SOURCES thirdparty/jtty_codec/*.f90)
add_library(jtty_codec STATIC ${jtty_codec_SOURCES})
set_target_properties(jtty_codec PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)
target_compile_options(jtty_codec PRIVATE -std=legacy -ffree-form -fno-second-underscore)
target_link_libraries(jtty_codec PRIVATE PkgConfig::FFTW3F OpenMP::OpenMP_Fortran)

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
    src/JttyCodec.h
    src/JttyCodec.cpp
    src/JttyDecoder.h
    src/JttyDecoder.cpp
    src/AudioSpectrum.h
    src/AudioSpectrum.cpp
    src/SpectrumWidget.h
    src/SpectrumWidget.cpp
    src/TextCapitalization.h
    src/TextCapitalization.cpp
    src/MacroEditDialog.h
    src/MacroEditDialog.cpp
)

target_link_libraries(JttyChat PRIVATE Qt6::Widgets Qt6::Multimedia PkgConfig::HAMLIB
    jtty_codec PkgConfig::FFTW3F OpenMP::OpenMP_Fortran)

install(TARGETS JttyChat RUNTIME DESTINATION bin)
install(FILES packaging/jttychat.desktop DESTINATION share/applications)
install(FILES packaging/jttychat.png DESTINATION share/icons/hicolor/256x256/apps)
