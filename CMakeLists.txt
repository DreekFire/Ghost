cmake_minimum_required(VERSION 3.0.2)
project(Ghost VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH /usr/local/Qt-5.13.0/lib/cmake/Qt5;${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(SOURCES src/Main.cpp src/MainWindow.cpp include/MainWindow.h src/Listener.cpp include/Listener.h src/Matcher.cpp include/Matcher.h src/Player.cpp include/Player.h src/Scanner.cpp include/Scanner.h src/Teacher.cpp include/Teacher.h src/Trainer.cpp include/Trainer.h)
add_executable(Ghost ${SOURCES})

target_include_directories(Ghost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_directories(Ghost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(rwqueue INTERFACE)
target_include_directories(rwqueue INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/readerwriterqueue>
        $<INSTALL_INTERFACE:include>)

add_library(tsf INTERFACE)
target_include_directories(tsf INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/tinysoundfont>
        $<INSTALL_INTERFACE:include>)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_library(SOUNDIO_LIB soundio)
find_library(ESSENTIA_LIB essentia)
find_library(MLPACK_LIB mlpack.3.1)
target_link_libraries(Ghost ${SOUNDIO_LIB} ${ESSENTIA_LIB} ${MLPACK_LIB} Qt5::Core Qt5::Widgets rwqueue)