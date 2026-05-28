cmake_minimum_required(VERSION 3.16)
project(SimpleSpectro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ===== Qt Detection (Qt6 first, fallback to Qt5) =====
find_package(Qt6 QUIET COMPONENTS Widgets Network)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Network)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
else()
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
endif()

# ===== Sources =====
set(SOURCES
    main.cpp
    mainwindow.cpp
    udpreceiver.cpp
    analysis.cpp

    # Axis
    Axis/XYAxis.cpp
    Axis/XYView.cpp

    # DBI
    DBI/dbi.cpp
    DBI/carrier.cpp
    DBI/sync.cpp
    DBI/ffe.cpp
    DBI/conv_same.cpp
    DBI/xcorr.cpp
    DBI/circshift.cpp
    DBI/resample.cpp
    DBI/operate_file.cpp

    # Util
    Util/util.cpp
    Util/spline.cpp
)

set(HEADERS
    compat.h
    mainwindow.h
    udpreceiver.h
    analysis.h

    Axis/XYAxis.h
    Axis/XYView.h

    DBI/dbi.h
    DBI/carrier.h
    DBI/sync.h
    DBI/ffe.h
    DBI/conv_same.h
    DBI/xcorr.h
    DBI/circshift.h
    DBI/resample.h
    DBI/operate_file.h

    Util/util.h
    Util/spline.h
)

# ===== Target =====
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network)
endif()

# ===== Platform-specific =====
if(WIN32)
    # Windows: set console or GUI subsystem
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # MSVC: suppress deprecation warnings, define _CRT_SECURE_NO_WARNINGS for fopen
    if(MSVC)
        target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    endif()
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

# ===== Install =====
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY Config/ DESTINATION bin/Config)

message(STATUS "SimpleSpectro configured successfully")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
