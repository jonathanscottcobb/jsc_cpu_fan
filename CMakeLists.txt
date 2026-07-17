cmake_minimum_required(VERSION 3.16)
project(jsc_cpu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ensure OpenGL::GL exists before Qt6 Gui/Widgets pull in WrapOpenGL.
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(jsc_cpu
    src/main.cpp
    src/HwmonReader.cpp
    src/SensorModel.cpp
    src/TrayController.cpp
    src/DetailPopup.cpp
    resources/jsc_cpu.qrc
)

target_include_directories(jsc_cpu PRIVATE src)
target_link_libraries(jsc_cpu PRIVATE Qt6::Widgets)

# ---------------------------------------------------------------------------
# Installation (used by `cmake --install` and for building the AppImage/.deb)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS jsc_cpu RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES packaging/jsc-cpu.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES resources/icons/jsc-cpu.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/jsc-cpu)

# ---------------------------------------------------------------------------
# CPack: Debian package (.deb) for the Ubuntu/Mint (MATE/Cinnamon) audience
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "jsc-cpu")
set(CPACK_PACKAGE_VENDOR "JSC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "System-tray CPU temperature and fan speed monitor")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/jonathanscottcobb/jsc_cpu_fan")

set(CPACK_PACKAGE_CONTACT "Jonathan Scott-Cobb <cobbjonathan@hotmail.com>")

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_GENERATOR "DEB")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
# Auto-resolve shared library dependencies (Qt6, libc, libstdc++, ...).
# Requires dpkg-dev at package time; CI installs it via build-essential.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "JSC CPU Monitor shows CPU temperature and motherboard fan speeds in the\n\
 Linux system tray. It reads the kernel hwmon sysfs interface directly, so it\n\
 needs no custom driver and no privileged daemon.")

include(CPack)
