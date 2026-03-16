# Platform-specific icon sources
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/icons/crs-dso.rc")
set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/icons/crs-dso.icns")

# Collect platform-specific sources
set(PLATFORM_SOURCES "")
if(WIN32)
    set(PLATFORM_SOURCES ${APP_ICON_RESOURCE_WINDOWS})
elseif(APPLE)
    set(PLATFORM_SOURCES ${APP_ICON_MACOS})
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

qt_add_executable(crs-dso
    main.cpp
    signalgenerator.h
    signalgenerator.cpp

    # Transport layer
    transport/ITransport.h
    transport/MockTransport.h
    transport/MockTransport.cpp

    # Protocol layer
    protocol/ProtocolDefs.h
    protocol/Crc16.h
    protocol/Crc16.cpp
    protocol/ProtocolCodec.h
    protocol/ProtocolCodec.cpp

    # Core layer
    core/WaveformBuffer.h
    core/DeviceSettings.h
    core/AcquisitionEngine.h
    core/AcquisitionEngine.cpp
    core/DeviceController.h
    core/DeviceController.cpp
    core/ScopeController.h
    core/ScopeController.cpp

    ${PLATFORM_SOURCES}
    "${CMAKE_SOURCE_DIR}/icons/icons.qrc"
)

qt_add_qml_module(crs-dso
    URI crs_dso
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ControlStepper.qml
    DEPENDENCIES
        QtQuick
        QtCharts    # Add this line
)

# Include paths for project sources
target_include_directories(crs-dso PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(crs-dso PRIVATE Qt6::Quick Qt6::Charts Qt6::Concurrent)

# --- Conditional libusb / UsbTransport support ---
if(HAS_LIBUSB)
    target_sources(crs-dso PRIVATE
        transport/UsbTransport.h
        transport/UsbTransport.cpp
    )
    target_include_directories(crs-dso PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(crs-dso PRIVATE ${LIBUSB_LIBRARIES})
    target_compile_definitions(crs-dso PRIVATE HAS_LIBUSB)

    # macOS: Link IOKit and CoreFoundation (required by libusb)
    if(APPLE)
        target_link_libraries(crs-dso PRIVATE
            "-framework IOKit"
            "-framework CoreFoundation"
        )
    endif()

    # Windows: Copy libusb DLL to output directory
    if(WIN32)
        set(LIBUSB_DIR "${CMAKE_SOURCE_DIR}/third_party/libusb")
        add_custom_command(TARGET crs-dso POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIBUSB_DIR}/MS64/dll/libusb-1.0.dll"
                $<TARGET_FILE_DIR:crs-dso>
        )
    endif()
endif()

# macOS bundle configuration
if(APPLE)
    set_target_properties(crs-dso PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/icons/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "CRS-DSO"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.crs.dso"
        MACOSX_BUNDLE_ICON_FILE "crs-dso.icns"
    )
endif()

# Windows executable properties
if(WIN32)
    set_target_properties(crs-dso PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install targets
install(TARGETS crs-dso DESTINATION bin)

# Linux: Install desktop file and icons
if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/icons/crs-dso.desktop"
        DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/icons/crs-dso.png"
        DESTINATION share/icons/hicolor/256x256/apps)
    install(FILES "${CMAKE_SOURCE_DIR}/icons/crs-dso.png"
        DESTINATION share/pixmaps)
endif()
