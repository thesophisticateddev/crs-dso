qt_add_executable(crs-dso
    main.cpp
    signalgenerator.h
    signalgenerator.cpp
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

# This part is already correct in your snippet
target_link_libraries(crs-dso PRIVATE Qt6::Quick Qt6::Charts)

install(TARGETS crs-dso DESTINATION bin)
