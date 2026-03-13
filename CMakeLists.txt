cmake_minimum_required(VERSION 3.16)

project(LR_1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(OpenSSL REQUIRED)

set(PROJECT_SOURCES
    main.cpp

    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    pindialog.cpp
    pindialog.h
    pindialog.ui

    creditem.cpp
    creditem.h
    credentials.ui

    crypto_utils.cpp
    crypto_utils.h

    security_state.cpp
    security_state.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LR_1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(LR_1
        ${PROJECT_SOURCES}
    )
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/creds.enc
    ${CMAKE_CURRENT_BINARY_DIR}/creds.enc
    COPYONLY
)

target_link_libraries(LR_1 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    OpenSSL::Crypto
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LR_1)
endif()
