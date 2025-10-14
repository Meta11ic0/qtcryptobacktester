cmake_minimum_required(VERSION 3.19)
project(qtbacktester2 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts)

qt_standard_project_setup()

qt_add_executable(qtbacktester2
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    downloaddialog.cpp
    downloaddialog.h
    downloaddialog.ui
    rec.qrc
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/strategies)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/strategies
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

target_link_libraries(qtbacktester2
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
)

include(GNUInstallDirs)

install(TARGETS qtbacktester2
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/scripts
        FILES_MATCHING PATTERN "*.py")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/strategies/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/strategies
        FILES_MATCHING PATTERN "*.py")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/data
        FILES_MATCHING PATTERN "*.csv")

qt_generate_deploy_app_script(
    TARGET qtbacktester2
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
