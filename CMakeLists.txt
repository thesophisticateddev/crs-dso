cmake_minimum_required(VERSION 3.16)

project(crs-dso VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Qt Quick library
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Charts Concurrent)

qt_standard_project_setup()

# --- libusb detection (cross-platform) ---
set(HAS_LIBUSB OFF)

if(UNIX AND NOT APPLE)
    # Linux: use pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUSB libusb-1.0)
    endif()
    if(LIBUSB_FOUND)
        set(HAS_LIBUSB ON)
    else()
        message(WARNING "libusb-1.0 not found. Building in mock-only mode. "
                        "Install libusb-1.0-0-dev for USB hardware support.")
    endif()
elseif(APPLE)
    # macOS: Homebrew or bundled
    find_path(LIBUSB_INCLUDE_DIRS libusb-1.0/libusb.h
        HINTS /usr/local/include /opt/homebrew/include)
    find_library(LIBUSB_LIBRARIES usb-1.0
        HINTS /usr/local/lib /opt/homebrew/lib)
    if(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
        set(HAS_LIBUSB ON)
    else()
        message(WARNING "libusb-1.0 not found. Building in mock-only mode. "
                        "Install with: brew install libusb")
    endif()
elseif(WIN32)
    # Windows: bundled libusb
    set(LIBUSB_DIR "${CMAKE_SOURCE_DIR}/third_party/libusb")
    if(EXISTS "${LIBUSB_DIR}/include")
        set(LIBUSB_INCLUDE_DIRS "${LIBUSB_DIR}/include")
        if(MSVC)
            set(LIBUSB_LIBRARIES "${LIBUSB_DIR}/MS64/dll/libusb-1.0.lib")
        else()
            set(LIBUSB_LIBRARIES "${LIBUSB_DIR}/MinGW64/dll/libusb-1.0.dll.a")
        endif()
        set(HAS_LIBUSB ON)
    else()
        message(WARNING "libusb not found in third_party/. Building in mock-only mode.")
    endif()
endif()

add_subdirectory(src)
