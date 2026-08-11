cmake_minimum_required(VERSION 3.5)

project(FlatExtract VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(ZLIB REQUIRED)
find_library(QUAZIP_LIBRARY quazip5 REQUIRED)
find_path(QUAZIP_INCLUDE_DIR quazip5/quazip.h REQUIRED)
find_package(ISO9660_LIBRARY NAMES iso9660 libiso9660 PATHS /usr/lib usr/lib/x86_64-linux-gnu )
find_package(CDIO_LIBRARY NAMES cdio libcdio PATHS /usr/lib usr/lib/x86_64-linux-gnu)
find_package(PkgConfig REQUIRED)

pkg_check_modules(R2 REQUIRED r_core)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui


)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(FlatExtract
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET FlatExtract APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(FlatExtract SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        file(GLOB HEADERS_UTILITYXEX)

        add_executable(FlatExtract
            ${PROJECT_SOURCES}
            ${SOURCES}
            ${HEADERS}
            ${HEADERS_UTILITYXEX}
            resources.qrc
            tool.h
            tool.cpp
            multiarchive.h
            multiarchive.cpp
            debughelper.h
            debughelper.cpp
            progresshelper.h
            progresshelper.cpp
            isotool.h
            isotool.cpp
            Xtool.h
            Xtool.cpp
            iso_reader.h
            xex_parser.h
            stfs_reader.h
            analysis.c
            analysis.h
            genrate.cpp
            UtilityXex/genrated.h



        )
    endif()
endif()

target_link_libraries(FlatExtract PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FlatExtract)
endif()
set_target_properties(FlatExtract PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FlatExtract
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FlatExtract)
endif()

##new link
target_link_libraries(FlatExtract PRIVATE
    Qt5::Widgets
    Qt5::Core
    ZLIB::ZLIB
    ${QUAZIP_LIBRARIES}
    ${CDIO_LIBRARIES}
    ${CDIOPP_LIBRARIES}
    ${ISO9660_LIBRARIES}
    ${CMAKE_C_STANDARD_LIBRARIES}
    ${R2_LIBRARIES}
)

target_include_directories(FlatExtract PRIVATE
    ${QUAZIP_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
 /usr/include

 ${R2_INCLUDE_DIRS}

 "${CMAKE_CURRENT_DIR}/UtilityXex"
)

# تعريف مسار QuaZip5 يدوياً
set(QUAZIP_INCLUDE_DIR "/usr/include/quazip5")
set(QUAZIP_LIBRARY "/usr/lib/x86_64-linux-gnu/libquazip5.so")

# إضافتها إلى المشروع
target_include_directories(FlatExtract PRIVATE ${QUAZIP_INCLUDE_DIR})
target_link_libraries(FlatExtract PRIVATE ${QUAZIP_LIBRARY} z)

# تعريف مسار libcdio  يدوياً
set(CDIO_INCLUDE_DIR "/usr/include")
set(CDIO_LIBRARY "/usr/lib/x86_64-linux-gnu/libcdio.so")
set(CDIOPP_LIBRARY "/usr/lib/x86_64-linux-gnu/libcdio++.so")
set(ISO9660_LIBRARY "/usr/lib/x86_64-linux-gnu/libiso9660.so")
set(ISO9660_INCLUDE-DIR "/use/include")

# إضافتها إلى المشروع
target_include_directories(FlatExtract PRIVATE ${CDIO_INCLUDE_DIR})
target_link_libraries(FlatExtract PRIVATE cdio++ ${CDIO_LIBRARY})
target_include_directories(FlatExtract PRIVATE ${ISO9660_INCLUDE_DIR})
target_link_libraries(FlatExtract PRIVATE iso9660 ${iso9660_LIBRARY})

message(STATUS "CDIO_INCLUDE_DIR: ${CDIO_INCLUDE_DIR}")
message(STATUS "CDIO_LIBRARY: ${CDIO_LIBRARY}")
message(STATUS "CDIOPP_LIBRARY: ${CDIOPP_LIBRARY}")


if(CDIO_LIBRARY)
    message(STATUS "Found libcdio: ${CDIO_LIBRARY}")
else()
    message(STATUS "libcdio NOT Founf!")
endif()
if(ISO9660_LIBRARY)
    message(STATUS "Found libiso9660: ${ISO9660_LIBRARY}")
else()
    message(STATUS "libiso9660 NOT Found!")
endif()


##conecnt Folder to Project
include_directories(UtilityXex)

