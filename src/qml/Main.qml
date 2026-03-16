import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

ApplicationWindow {
    id: window
    width: 1024
    height: 720
    visible: true
    title: qsTr("Dual Channel Digital Sampling Oscilloscope")
    color: "#1a1a1a"

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
                checked: scope.testMode
                onTriggered: scope.testMode = true
            }

            MenuItem {
                text: qsTr("Device Mode (Hardware)")
                checkable: true
                checked: !scope.testMode
                onTriggered: scope.testMode = false
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("Scan for Devices")
                enabled: !scope.testMode
                onTriggered: scope.scanDevices()
            }
        }

        Menu {
            title: qsTr("&Acquisition")

            MenuItem {
                text: qsTr("Start")
                enabled: !scope.running
                onTriggered: scope.running = true
            }
            MenuItem {
                text: qsTr("Stop")
                enabled: scope.running
                onTriggered: scope.running = false
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
            color: scope.testMode ? "#1b3a1b" : "#1b2a3a"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 15

                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: scope.testMode ? "#4caf50" : "#2196f3"
                }

                Text {
                    text: scope.testMode ? "TEST MODE" : "DEVICE MODE"
                    color: scope.testMode ? "#81c784" : "#90caf9"
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    text: scope.statusText
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

                // Fix #3: X-axis shows time, not samples
                ValueAxis {
                    id: axisX
                    min: 0
                    max: {
                        var steps = scope.timebaseSteps()
                        return steps[scope.timebaseIndex] * 10
                    }
                    labelFormat: {
                        var total = max
                        if (total < 1e-5)      return "%.0f ns"
                        else if (total < 1e-2) return "%.0f us"
                        else if (total < 10.0) return "%.0f ms"
                        else                   return "%.1f s"
                    }
                    titleText: scope.timebaseLabel
                    tickCount: 11
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

                // Fix #7: Trigger line bound to scope (mode-agnostic)
                LineSeries {
                    id: triggerLine
                    name: "Trigger"
                    axisX: axisX
                    axisY: axisY
                    color: "yellow"
                    width: 1
                    style: Qt.DashLine
                    XYPoint { x: 0; y: scope.triggerLevel }
                    XYPoint { x: axisX.max; y: scope.triggerLevel }
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
                    XYPoint { x: axisX.max; y: 1.0 }
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
                    XYPoint { x: axisX.max; y: -1.0 }
                }

                // --- Trigger Handle (right side, yellow) ---
                Rectangle {
                    id: triggerHandle
                    width: 20; height: 20
                    x: parent.width - 25
                    y: chartView.mapToPosition(Qt.point(0, scope.triggerLevel)).y - 10
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

                // --- CH2 Cursor Handle (left side, cyan) ---
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

                // Fix #7: Mode-agnostic trigger update
                function updateTriggerLevel(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    triggerLine.replace(0, 0, val)
                    triggerLine.replace(1, axisX.max, val)
                    scope.triggerLevel = val
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
                        triggerHandle.y = mapToPosition(Qt.point(0, scope.triggerLevel)).y - 10
                    if (!ch1CursorDragArea.drag.active)
                        ch1CursorHandle.y = mapToPosition(Qt.point(0, ch1CursorLevel)).y - 10
                    if (!ch2CursorDragArea.drag.active)
                        ch2CursorHandle.y = mapToPosition(Qt.point(0, ch2CursorLevel)).y - 10
                }

                onPlotAreaChanged: {
                    repositionHandles()
                }

                Connections {
                    target: scope
                    function onTriggerChanged() {
                        if (!triggerDragArea.drag.active) {
                            triggerHandle.y = chartView.mapToPosition(Qt.point(0, scope.triggerLevel)).y - 10
                            triggerLine.replace(0, 0, scope.triggerLevel)
                            triggerLine.replace(1, axisX.max, scope.triggerLevel)
                        }
                    }
                    function onTimebaseChanged() {
                        // Update cursor/trigger line endpoints when timebase changes
                        triggerLine.replace(1, axisX.max, scope.triggerLevel)
                        ch1CursorLine.replace(1, axisX.max, chartView.ch1CursorLevel)
                        ch2CursorLine.replace(1, axisX.max, chartView.ch2CursorLevel)
                    }
                }

                Component.onCompleted: {
                    scope.registerSeries(0, chan1)
                    scope.registerSeries(1, chan2)
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

                        // --- Fix #6: Acquisition Mode Selector ---
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 10
                            spacing: 4

                            Repeater {
                                model: ["Auto", "Normal", "Single"]
                                Button {
                                    text: modelData
                                    checked: scope.acquisitionMode === index
                                    checkable: true
                                    autoExclusive: true
                                    Layout.fillWidth: true
                                    onClicked: scope.acquisitionMode = index

                                    background: Rectangle {
                                        color: checked ? "#3d6fa5" : "#2d2d2d"
                                        border.color: "#3d6fa5"
                                        border.width: 1
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: modelData
                                        color: checked ? "white" : "#90caf9"
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }

                        // --- Run/Stop ---
                        Button {
                            id: runStopBtn
                            text: scope.running ? "STOP" : "RUN"
                            highlighted: scope.running
                            Layout.fillWidth: true
                            onClicked: scope.running = !scope.running

                            background: Rectangle {
                                color: scope.running ? "#2e7d32" : "#c62828"
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
                                currentIndex: scope.triggerSource
                                Layout.fillWidth: true
                                onCurrentIndexChanged: scope.triggerSource = currentIndex

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
                                currentIndex: scope.triggerEdge
                                Layout.fillWidth: true
                                onCurrentIndexChanged: scope.triggerEdge = currentIndex

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
                                text: "Level: " + scope.triggerLevel.toFixed(2) + " V"
                                color: "yellow"
                                font.pixelSize: 11
                            }
                            Slider {
                                id: triggerLevelSlider
                                from: -5; to: 5
                                value: scope.triggerLevel
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

                        // --- Horizontal / Timebase ---
                        Text {
                            text: "HORIZONTAL"
                            color: "#fff"
                            font.bold: true
                        }

                        // Fix #2: Standard 1-2-5 timebase stepper
                        ControlStepper {
                            id: timebaseStepper
                            label: "TIME/DIV"
                            accentColor: "#ffffff"
                            value: scope.timebaseSteps()[scope.timebaseIndex]
                            suffix: ""
                            steps: scope.timebaseSteps()
                            displayFormatter: function(v) { return scope.timebaseLabel }
                            onValueChanged: scope.timebaseIndex = currentIndex
                        }

                        // Fix #6: Horizontal position
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "H. Position:"; color: "#aaa"; font.pixelSize: 11 }
                            Slider {
                                id: hPositionSlider
                                from: -5; to: 5; value: 0
                                Layout.fillWidth: true
                                onMoved: {
                                    var shift = value * scope.timebaseSteps()[scope.timebaseIndex]
                                    axisX.min = shift
                                    axisX.max = shift + scope.timebaseSteps()[scope.timebaseIndex] * 10
                                }
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Channel 1 ---
                        Text {
                            text: "CH1"
                            color: "#00ff00"
                            font.bold: true
                        }

                        // Fix #6: Coupling Selector (AC / DC / GND)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Repeater {
                                model: ["AC", "DC", "GND"]
                                Button {
                                    text: modelData
                                    checked: scope.ch1Coupling === index
                                    checkable: true
                                    autoExclusive: true
                                    Layout.fillWidth: true
                                    onClicked: scope.ch1Coupling = index

                                    background: Rectangle {
                                        color: checked ? "#00ff00" : "#2d2d2d"
                                        border.color: "#00ff00"
                                        border.width: 1
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: modelData
                                        color: checked ? "black" : "#00ff00"
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }

                        ControlStepper {
                            id: vScale1
                            label: "CH1 VOLTS/DIV"
                            accentColor: "#00ff00"
                            value: scope.voltsPerDivSteps()[scope.ch1VoltsIndex]
                            suffix: " V"
                            steps: scope.voltsPerDivSteps()
                            onValueChanged: {
                                scope.ch1VoltsIndex = currentIndex
                                var maxV = Math.max(value, vScale2.value)
                                axisY.max = maxV
                                axisY.min = -maxV
                                triggerLevelSlider.from = -maxV
                                triggerLevelSlider.to = maxV
                                axisY.labelFormat = (value < 1.0 || vScale2.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                            }
                        }

                        // Fix #6: Channel offset (vertical position)
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Position:"; color: "#aaa"; font.pixelSize: 11 }
                            Slider {
                                id: ch1OffsetSlider
                                from: -5.0; to: 5.0; value: scope.ch1Offset
                                Layout.fillWidth: true
                                onMoved: scope.ch1Offset = value
                            }
                        }

                        CheckBox {
                            id: ch1ShowCheck
                            text: "Show CH1"; checked: scope.ch1Enabled; palette.windowText: "white"
                            onToggled: {
                                chan1.visible = checked
                                scope.ch1Enabled = checked
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Channel 2 ---
                        Text {
                            text: "CH2"
                            color: "#00ffff"
                            font.bold: true
                        }

                        // Fix #6: Coupling Selector (AC / DC / GND)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Repeater {
                                model: ["AC", "DC", "GND"]
                                Button {
                                    text: modelData
                                    checked: scope.ch2Coupling === index
                                    checkable: true
                                    autoExclusive: true
                                    Layout.fillWidth: true
                                    onClicked: scope.ch2Coupling = index

                                    background: Rectangle {
                                        color: checked ? "#00ffff" : "#2d2d2d"
                                        border.color: "#00ffff"
                                        border.width: 1
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: modelData
                                        color: checked ? "black" : "#00ffff"
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }

                        ControlStepper {
                            id: vScale2
                            label: "CH2 VOLTS/DIV"
                            accentColor: "#00ffff"
                            value: scope.voltsPerDivSteps()[scope.ch2VoltsIndex]
                            suffix: " V"
                            steps: scope.voltsPerDivSteps()
                            onValueChanged: {
                                scope.ch2VoltsIndex = currentIndex
                                var maxV = Math.max(value, vScale1.value)
                                axisY.max = maxV
                                axisY.min = -maxV
                                triggerLevelSlider.from = -maxV
                                triggerLevelSlider.to = maxV
                                axisY.labelFormat = (value < 1.0 || vScale1.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                            }
                        }

                        // Fix #6: Channel offset (vertical position)
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Position:"; color: "#aaa"; font.pixelSize: 11 }
                            Slider {
                                id: ch2OffsetSlider
                                from: -5.0; to: 5.0; value: scope.ch2Offset
                                Layout.fillWidth: true
                                onMoved: scope.ch2Offset = value
                            }
                        }

                        CheckBox {
                            id: ch2ShowCheck
                            text: "Show CH2"; checked: scope.ch2Enabled; palette.windowText: "white"
                            onToggled: {
                                chan2.visible = checked
                                scope.ch2Enabled = checked
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
                                scope.timebaseIndex = 12  // 1 ms/div
                                scope.ch1VoltsIndex = 9   // 1 V/div
                                scope.ch2VoltsIndex = 9
                                scope.triggerLevel = 0
                                scope.triggerSource = 0
                                scope.triggerEdge = 0
                                scope.ch1Offset = 0
                                scope.ch2Offset = 0
                                hPositionSlider.value = 0
                                vScale1.value = scope.voltsPerDivSteps()[9]
                                vScale2.value = scope.voltsPerDivSteps()[9]
                                timebaseStepper.value = scope.timebaseSteps()[12]
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
