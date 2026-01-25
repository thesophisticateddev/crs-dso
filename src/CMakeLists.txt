qt_add_executable(crs-dso
    main.cpp
)

qt_add_qml_module(crs-dso
    URI crs_dso
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

# Use the QtQuick module from Qt 6.
target_link_libraries(crs-dso PRIVATE Qt6::Quick)

# Install the executable
install(TARGETS crs-dso DESTINATION bin)
