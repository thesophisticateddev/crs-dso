import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import CRS.DSO 1.0

ApplicationWindow {
    id: window
    width: 1024
    height: 720
    visible: true
    title: qsTr("Dual Channel Digital Sampling Oscilloscope")
    color: "#1a1a1a"

    // --- Mode state: true = signal generator emulation, false = hardware device ---
    property bool testMode: true

    // Helpers that route to the active backend
    function activeRunning() {
        return testMode ? signalGenerator.running : deviceController.running
    }
    function activeTriggerLevel() {
        return testMode ? signalGenerator.triggerLevel : deviceController.triggerLevel / 1000.0
    }

    // --- DeviceController for hardware/mock mode ---
    DeviceController {
        id: deviceController

        onWaveformUpdated: {
            if (!testMode) {
                var d1 = deviceController.ch1Data
                var d2 = deviceController.ch2Data
                if (d1.length > 0) chan1.replace(d1)
                if (d2.length > 0) chan2.replace(d2)
            }
        }

        onErrorMessage: function(msg) {
            modeStatusText.text = "Error: " + msg
        }

        onStateChanged: {
            if (!testMode)
                modeStatusText.text = deviceController.state
        }
    }

    // --- Device scan handler ---
    Connections {
        target: deviceController
        function onDevicesScanned(devices) {
            if (!testMode) {
                if (devices.length === 0) {
                    modeStatusText.text = "No devices found — using mock"
                } else {
                    deviceController.connectToDevice(0)
                    modeStatusText.text = "Connecting..."
                }
            }
        }
        function onConnectionChanged() {
            if (deviceController.connected)
                modeStatusText.text = "Connected: " + deviceController.deviceName
            else if (!testMode)
                modeStatusText.text = "Disconnected"
        }
    }

    // --- Menu Bar ---
    menuBar: MenuBar {
        palette.window: "#2d2d2d"
        palette.windowText: "#ddd"
        palette.highlight: "#3d6fa5"
        palette.highlightedText: "#fff"

        Menu {
            title: qsTr("&Mode")

            MenuItem {
                text: qsTr("Test Mode (Signal Generator)")
                checkable: true
                checked: testMode
                onTriggered: {
                    if (!testMode) {
                        testMode = true
                        deviceController.stopAcquisition()
                        deviceController.disconnect()
                        signalGenerator.setRunning(true)
                        signalGenerator.registerSeries(0, chan1)
                        signalGenerator.registerSeries(1, chan2)
                        modeStatusText.text = "Emulated oscilloscope"
                    }
                }
            }

            MenuItem {
                text: qsTr("Device Mode (Hardware)")
                checkable: true
                checked: !testMode
                onTriggered: {
                    if (testMode) {
                        testMode = false
                        signalGenerator.setRunning(false)
                        modeStatusText.text = "Scanning for devices..."
                        deviceController.scanDevices()
                    }
                }
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("Scan for Devices")
                enabled: !testMode
                onTriggered: {
                    modeStatusText.text = "Scanning..."
                    deviceController.scanDevices()
                }
            }
        }

        Menu {
            title: qsTr("&Acquisition")

            MenuItem {
                text: qsTr("Start (Auto)")
                enabled: !testMode && deviceController.connected
                onTriggered: deviceController.startAcquisition(0)
            }
            MenuItem {
                text: qsTr("Start (Normal)")
                enabled: !testMode && deviceController.connected
                onTriggered: deviceController.startAcquisition(1)
            }
            MenuItem {
                text: qsTr("Start (Single)")
                enabled: !testMode && deviceController.connected
                onTriggered: deviceController.startAcquisition(2)
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Stop")
                enabled: !testMode && deviceController.running
                onTriggered: deviceController.stopAcquisition()
            }
            MenuItem {
                text: qsTr("Force Trigger")
                enabled: !testMode && deviceController.running
                onTriggered: deviceController.forceTrigger()
            }
            MenuItem {
                text: qsTr("Auto Set")
                enabled: !testMode && deviceController.connected
                onTriggered: deviceController.autoSet()
            }
        }
    }

    // --- Main layout ---
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Mode status bar ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: testMode ? "#1b3a1b" : "#1b2a3a"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 15

                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: testMode ? "#4caf50"
                                    : (deviceController.connected ? "#2196f3" : "#f44336")
                }

                Text {
                    text: testMode ? "TEST MODE" : "DEVICE MODE"
                    color: testMode ? "#81c784" : "#90caf9"
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    id: modeStatusText
                    text: "Emulated oscilloscope"
                    color: "#999"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        // --- Content area ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- 1. Waveform Display ---
            ChartView {
                id: chartView
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: ChartView.ChartThemeDark
                antialiasing: true
                legend.visible: true
                legend.alignment: Qt.AlignTop

                property real ch1CursorLevel: 1.0
                property real ch2CursorLevel: -1.0

                ValueAxis {
                    id: axisX
                    min: 0
                    max: 1000
                    labelFormat: "%.0f"
                    titleText: "Samples"
                }

                ValueAxis {
                    id: axisY
                    min: -5
                    max: 5
                    labelFormat: "%.1f V"
                    gridVisible: true
                }

                LineSeries {
                    id: chan1
                    name: "CH 1"
                    axisX: axisX
                    axisY: axisY
                    color: "#00ff00"
                    useOpenGL: true
                }

                LineSeries {
                    id: chan2
                    name: "CH 2"
                    axisX: axisX
                    axisY: axisY
                    color: "#00ffff"
                    useOpenGL: true
                }

                LineSeries {
                    id: triggerLine
                    name: "Trigger"
                    axisX: axisX
                    axisY: axisY
                    color: "yellow"
                    width: 1
                    style: Qt.DashLine
                    XYPoint { x: 0; y: signalGenerator.triggerLevel }
                    XYPoint { x: 1000; y: signalGenerator.triggerLevel }
                }

                LineSeries {
                    id: ch1CursorLine
                    name: "C1"
                    axisX: axisX
                    axisY: axisY
                    color: "#00ff00"
                    width: 1
                    style: Qt.DotLine
                    XYPoint { x: 0; y: 1.0 }
                    XYPoint { x: 1000; y: 1.0 }
                }

                LineSeries {
                    id: ch2CursorLine
                    name: "C2"
                    axisX: axisX
                    axisY: axisY
                    color: "#00ffff"
                    width: 1
                    style: Qt.DotLine
                    XYPoint { x: 0; y: -1.0 }
                    XYPoint { x: 1000; y: -1.0 }
                }

                // --- Trigger Handle (right side, yellow) ---
                Rectangle {
                    id: triggerHandle
                    width: 20; height: 20
                    x: parent.width - 25
                    y: chartView.mapToPosition(Qt.point(0, signalGenerator.triggerLevel)).y - 10
                    color: "yellow"
                    rotation: 45
                    z: 10

                    Text {
                        text: "T"
                        anchors.centerIn: parent
                        color: "black"
                        rotation: -45
                        font.bold: true
                    }

                    onYChanged: {
                        if (triggerDragArea.drag.active) {
                            var point = chartView.mapToValue(Qt.point(x + 10, y + 10))
                            chartView.updateTriggerLevel(point.y)
                        }
                    }

                    MouseArea {
                        id: triggerDragArea
                        anchors.fill: parent
                        drag.target: parent
                        drag.axis: Drag.YAxis
                    }
                }

                // --- CH1 Cursor Handle (left side, green) ---
                Rectangle {
                    id: ch1CursorHandle
                    width: 20; height: 20
                    x: 5
                    y: chartView.mapToPosition(Qt.point(0, chartView.ch1CursorLevel)).y - 10
                    color: "#00ff00"
                    rotation: 45
                    z: 10

                    Text {
                        text: "1"
                        anchors.centerIn: parent
                        color: "black"
                        rotation: -45
                        font.bold: true
                        font.pixelSize: 10
                    }

                    onYChanged: {
                        if (ch1CursorDragArea.drag.active) {
                            var point = chartView.mapToValue(Qt.point(x + 10, y + 10))
                            chartView.updateCh1Cursor(point.y)
                        }
                    }

                    MouseArea {
                        id: ch1CursorDragArea
                        anchors.fill: parent
                        drag.target: parent
                        drag.axis: Drag.YAxis
                    }
                }

                // --- CH2 Cursor Handle (left side, cyan, offset from CH1) ---
                Rectangle {
                    id: ch2CursorHandle
                    width: 20; height: 20
                    x: 28
                    y: chartView.mapToPosition(Qt.point(0, chartView.ch2CursorLevel)).y - 10
                    color: "#00ffff"
                    rotation: 45
                    z: 10

                    Text {
                        text: "2"
                        anchors.centerIn: parent
                        color: "black"
                        rotation: -45
                        font.bold: true
                        font.pixelSize: 10
                    }

                    onYChanged: {
                        if (ch2CursorDragArea.drag.active) {
                            var point = chartView.mapToValue(Qt.point(x + 10, y + 10))
                            chartView.updateCh2Cursor(point.y)
                        }
                    }

                    MouseArea {
                        id: ch2CursorDragArea
                        anchors.fill: parent
                        drag.target: parent
                        drag.axis: Drag.YAxis
                    }
                }

                function updateTriggerLevel(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    triggerLine.replace(0, 0, val)
                    triggerLine.replace(1, axisX.max, val)
                    if (testMode) {
                        signalGenerator.setTriggerLevel(val)
                    } else {
                        deviceController.triggerLevel = Math.round(val * 1000)
                    }
                }

                function updateCh1Cursor(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    ch1CursorLevel = val
                    ch1CursorLine.replace(0, 0, val)
                    ch1CursorLine.replace(1, axisX.max, val)
                    if (!ch1CursorDragArea.drag.active)
                        ch1CursorHandle.y = mapToPosition(Qt.point(0, val)).y - 10
                }

                function updateCh2Cursor(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    ch2CursorLevel = val
                    ch2CursorLine.replace(0, 0, val)
                    ch2CursorLine.replace(1, axisX.max, val)
                    if (!ch2CursorDragArea.drag.active)
                        ch2CursorHandle.y = mapToPosition(Qt.point(0, val)).y - 10
                }

                function repositionHandles() {
                    if (!triggerDragArea.drag.active)
                        triggerHandle.y = mapToPosition(Qt.point(0, signalGenerator.triggerLevel)).y - 10
                    if (!ch1CursorDragArea.drag.active)
                        ch1CursorHandle.y = mapToPosition(Qt.point(0, ch1CursorLevel)).y - 10
                    if (!ch2CursorDragArea.drag.active)
                        ch2CursorHandle.y = mapToPosition(Qt.point(0, ch2CursorLevel)).y - 10
                }

                onPlotAreaChanged: {
                    repositionHandles()
                }

                Connections {
                    target: signalGenerator
                    function onTriggerLevelChanged() {
                        if (!triggerDragArea.drag.active) {
                            triggerHandle.y = chartView.mapToPosition(Qt.point(0, signalGenerator.triggerLevel)).y - 10
                            triggerLine.replace(0, 0, signalGenerator.triggerLevel)
                            triggerLine.replace(1, axisX.max, signalGenerator.triggerLevel)
                        }
                    }
                }

                Component.onCompleted: {
                    signalGenerator.registerSeries(0, chan1)
                    signalGenerator.registerSeries(1, chan2)
                }
            }

            // --- 2. Control Panel ---
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "#2d2d2d"
                border.color: "#3d3d3d"

                ScrollView {
                    anchors.fill: parent
                    contentWidth: parent.width
                    clip: true

                    ColumnLayout {
                        width: parent.width - 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 15

                        // --- Run/Stop ---
                        Button {
                            id: runStopBtn
                            property bool isRunning: testMode ? signalGenerator.running
                                                              : deviceController.running
                            text: isRunning ? "STOP" : "RUN"
                            highlighted: isRunning
                            Layout.fillWidth: true
                            Layout.topMargin: 10
                            onClicked: {
                                if (testMode) {
                                    signalGenerator.setRunning(!signalGenerator.running)
                                } else {
                                    if (deviceController.running)
                                        deviceController.stopAcquisition()
                                    else
                                        deviceController.startAcquisition(0) // Auto mode
                                }
                            }

                            background: Rectangle {
                                color: runStopBtn.isRunning ? "#2e7d32" : "#c62828"
                                radius: 4
                            }

                            contentItem: Text {
                                text: runStopBtn.text
                                color: "white"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Trigger Controls ---
                        Text {
                            text: "TRIGGER"
                            color: "yellow"
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Text { text: "Source:"; color: "#aaa"; font.pixelSize: 11 }

                            ComboBox {
                                id: triggerSourceCombo
                                model: ["CH 1", "CH 2"]
                                currentIndex: signalGenerator.triggerSource
                                Layout.fillWidth: true
                                onCurrentIndexChanged: {
                                    if (testMode)
                                        signalGenerator.setTriggerSource(currentIndex)
                                    else
                                        deviceController.triggerSource = currentIndex
                                }

                                background: Rectangle {
                                    color: "#3d3d3d"
                                    border.color: triggerSourceCombo.currentIndex === 0 ? "#00ff00" : "#00ffff"
                                    border.width: 2
                                    radius: 4
                                }

                                contentItem: Text {
                                    text: triggerSourceCombo.displayText
                                    color: triggerSourceCombo.currentIndex === 0 ? "#00ff00" : "#00ffff"
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 8
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Text { text: "Edge:"; color: "#aaa"; font.pixelSize: 11 }

                            ComboBox {
                                id: triggerEdgeCombo
                                model: ["Rising", "Falling"]
                                currentIndex: signalGenerator.triggerEdge
                                Layout.fillWidth: true
                                onCurrentIndexChanged: {
                                    if (testMode)
                                        signalGenerator.setTriggerEdge(currentIndex)
                                    else
                                        deviceController.triggerEdge = currentIndex
                                }

                                background: Rectangle {
                                    color: "#3d3d3d"
                                    border.color: "yellow"
                                    border.width: 1
                                    radius: 4
                                }

                                contentItem: Text {
                                    text: triggerEdgeCombo.displayText
                                    color: "yellow"
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 8
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Level: " + signalGenerator.triggerLevel.toFixed(2) + " V"
                                color: "yellow"
                                font.pixelSize: 11
                            }
                            Slider {
                                id: triggerLevelSlider
                                from: -5; to: 5
                                value: signalGenerator.triggerLevel
                                Layout.fillWidth: true
                                onMoved: chartView.updateTriggerLevel(value)

                                background: Rectangle {
                                    x: triggerLevelSlider.leftPadding
                                    y: triggerLevelSlider.topPadding + triggerLevelSlider.availableHeight / 2 - height / 2
                                    width: triggerLevelSlider.availableWidth
                                    height: 4
                                    radius: 2
                                    color: "#3d3d3d"

                                    Rectangle {
                                        width: triggerLevelSlider.visualPosition * parent.width
                                        height: parent.height
                                        color: "yellow"
                                        radius: 2
                                    }
                                }

                                handle: Rectangle {
                                    x: triggerLevelSlider.leftPadding + triggerLevelSlider.visualPosition * (triggerLevelSlider.availableWidth - width)
                                    y: triggerLevelSlider.topPadding + triggerLevelSlider.availableHeight / 2 - height / 2
                                    width: 16; height: 16
                                    radius: 8
                                    color: "yellow"
                                }
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Master Timebase ---
                        Text {
                            text: "TIMEBASE"
                            color: "#fff"
                            font.bold: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Time/Div: " + hScale.value.toFixed(0) + " ms"; color: "#aaa"; font.pixelSize: 11 }
                            Slider {
                                id: hScale
                                from: 100; to: 5000; value: 1000
                                Layout.fillWidth: true
                                onValueChanged: {
                                    axisX.max = value
                                    triggerLine.replace(1, value, signalGenerator.triggerLevel)
                                    ch1CursorLine.replace(1, value, chartView.ch1CursorLevel)
                                    ch2CursorLine.replace(1, value, chartView.ch2CursorLevel)
                                }
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Channel 1 ---
                        ControlStepper {
                            id: vScale1
                            label: "CH1 VOLTS/DIV"
                            accentColor: "#00ff00"
                            value: 5.0
                            suffix: " V"
                            steps: [0.005, 0.010, 0.020, 0.050, 0.100, 0.200, 0.500,
                                    1.0, 2.0, 5.0, 10.0, 20.0]
                            onValueChanged: {
                                if (value > vScale2.value) axisY.max = value
                                axisY.min = -value
                                triggerLevelSlider.from = -value
                                triggerLevelSlider.to = value
                                axisY.labelFormat = (value < 1.0 || vScale2.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                                if (!testMode) deviceController.ch1VoltageRange = _stepIndex()
                            }
                        }

                        CheckBox {
                            id: ch1ShowCheck
                            text: "Show CH1"; checked: true; palette.windowText: "white"
                            onToggled: {
                                chan1.visible = checked
                                if (!testMode) deviceController.ch1Enabled = checked
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Channel 2 Controls ---
                        ControlStepper {
                            id: vScale2
                            label: "CH2 VOLTS/DIV"
                            accentColor: "#00ffff"
                            value: 5.0
                            suffix: " V"
                            steps: [0.005, 0.010, 0.020, 0.050, 0.100, 0.200, 0.500,
                                    1.0, 2.0, 5.0, 10.0, 20.0]
                            onValueChanged: {
                                if (value > vScale1.value) axisY.max = value
                                axisY.min = -value
                                triggerLevelSlider.from = -value
                                triggerLevelSlider.to = value
                                axisY.labelFormat = (value < 1.0 || vScale1.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                                if (!testMode) deviceController.ch2VoltageRange = _stepIndex()
                            }
                        }

                        CheckBox {
                            id: ch2ShowCheck
                            text: "Show CH2"; checked: true; palette.windowText: "white"
                            onToggled: {
                                chan2.visible = checked
                                if (!testMode) deviceController.ch2Enabled = checked
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Cursors ---
                        Text {
                            text: "CURSORS"
                            color: "#fff"
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "C1:"; color: "#00ff00"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 24 }
                            Text { text: chartView.ch1CursorLevel.toFixed(3) + " V"; color: "#aaa"; font.pixelSize: 11 }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "C2:"; color: "#00ffff"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 24 }
                            Text { text: chartView.ch2CursorLevel.toFixed(3) + " V"; color: "#aaa"; font.pixelSize: 11 }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "ΔV:"; color: "#fff"; font.pixelSize: 11; Layout.preferredWidth: 24 }
                            Text {
                                text: (chartView.ch1CursorLevel - chartView.ch2CursorLevel).toFixed(3) + " V"
                                color: "#aaa"
                                font.pixelSize: 11
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        Button {
                            text: "AUTOSET"
                            Layout.fillWidth: true
                            onClicked: {
                                vScale1.value = 5
                                vScale2.value = 5
                                hScale.value = 1000
                                if (testMode) {
                                    signalGenerator.setTriggerLevel(0)
                                    signalGenerator.setTriggerSource(0)
                                    signalGenerator.setTriggerEdge(0)
                                } else {
                                    deviceController.autoSet()
                                }
                                chartView.updateCh1Cursor(1.0)
                                chartView.updateCh2Cursor(-1.0)
                            }
                        }

                        Item { Layout.fillHeight: true; Layout.preferredHeight: 20 }
                    }
                }
            }
        }
    }
}
