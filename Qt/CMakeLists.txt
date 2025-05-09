cmake_minimum_required(VERSION 3.16)
project(qt_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(qt_app
    main.cpp
    mainwindow.ui
)

target_link_libraries(qt_app PRIVATE Qt6::Widgets) 