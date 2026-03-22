import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import Qt.labs.settings

ApplicationWindow {
    id: window
    width: 1024
    height: 720
    visible: true
    title: qsTr("Dual Channel Digital Sampling Oscilloscope")
    color: "#1a1a1a"

    Settings {
        id: controlPanelSettings
        category: "ControlPanel"
        property int width: 300
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
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 8
                implicitHeight: 8
                color: SplitHandle.pressed ? "#5a5a5a"
                     : SplitHandle.hovered ? "#4a4a4a"
                     : "#3d3d3d"

                Rectangle {
                    width: 2
                    height: 30
                    radius: 1
                    anchors.centerIn: parent
                    color: SplitHandle.pressed ? "#999"
                         : SplitHandle.hovered ? "#888"
                         : "#666"
                }
            }

            // --- 1. Waveform Display ---
            ChartView {
                id: chartView
                SplitView.fillWidth: true
                SplitView.minimumWidth: 200
                theme: ChartView.ChartThemeDark
                antialiasing: true
                legend.visible: true
                legend.alignment: Qt.AlignTop

                property real ch1CursorLevel: 1.0
                property real ch2CursorLevel: -1.0

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

                function updateTriggerLevel(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    if (triggerLine.count >= 2) {
                        triggerLine.replace(0, 0, val)
                        triggerLine.replace(1, axisX.max, val)
                    }
                    scope.triggerLevel = val
                }

                function updateCh1Cursor(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    ch1CursorLevel = val
                    if (ch1CursorLine.count >= 2) {
                        ch1CursorLine.replace(0, 0, val)
                        ch1CursorLine.replace(1, axisX.max, val)
                    }
                    if (!ch1CursorDragArea.drag.active)
                        ch1CursorHandle.y = mapToPosition(Qt.point(0, val)).y - 10
                }

                function updateCh2Cursor(val) {
                    if (isNaN(val)) return;
                    val = Math.max(axisY.min, Math.min(axisY.max, val))
                    ch2CursorLevel = val
                    if (ch2CursorLine.count >= 2) {
                        ch2CursorLine.replace(0, 0, val)
                        ch2CursorLine.replace(1, axisX.max, val)
                    }
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
                        if (!triggerDragArea.drag.active && triggerLine.count >= 2) {
                            triggerHandle.y = chartView.mapToPosition(Qt.point(0, scope.triggerLevel)).y - 10
                            triggerLine.replace(0, 0, scope.triggerLevel)
                            triggerLine.replace(1, axisX.max, scope.triggerLevel)
                        }
                    }
                    function onTimebaseChanged() {
                        if (triggerLine.count >= 2) {
                            triggerLine.replace(1, axisX.max, scope.triggerLevel)
                            ch1CursorLine.replace(1, axisX.max, chartView.ch1CursorLevel)
                            ch2CursorLine.replace(1, axisX.max, chartView.ch2CursorLevel)
                        }
                    }
                    function onHorizontalChanged() {
                        var timebase = scope.timebaseSteps()[scope.timebaseIndex]
                        var shift = scope.horizontalPosition * timebase
                        axisX.min = shift
                        axisX.max = shift + timebase * 10
                    }
                }

                Component.onCompleted: {
                    scope.registerSeries(0, chan1)
                    scope.registerSeries(1, chan2)
                }
            }

            // --- 2. Control Panel ---
            Rectangle {
                SplitView.preferredWidth: controlPanelSettings.width
                SplitView.minimumWidth: 240
                SplitView.maximumWidth: 420
                color: "#2d2d2d"
                border.color: "#3d3d3d"

                onWidthChanged: {
                    if (width >= 240 && width <= 420)
                        controlPanelSettings.width = width
                }

                ScrollView {
                    anchors.fill: parent
                    contentWidth: parent.width
                    clip: true

                    ColumnLayout {
                        width: parent.width - 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 15

                        // --- Acquisition Mode Selector ---
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
                            ControlStepper {
                                id: triggerLevelControl
                                label: "LEVEL"
                                accentColor: "yellow"
                                value: scope.triggerLevel
                                min: axisY.min
                                max: axisY.max
                                step: 0.05
                                inputStep: 0.05
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: chartView.updateTriggerLevel(value)
                            }
                        }

                        // --- Trigger Status ---
                        Text {
                            text: "Status: " + scope.triggerStatus
                            color: {
                                switch(scope.triggerStatus) {
                                    case "Triggered": return "#4caf50"
                                    case "Waiting": return "#ff9800"
                                    case "Stopped": return "#f44336"
                                    default: return "yellow"
                                }
                            }
                            font.pixelSize: 11
                        }

                        // --- Force Trigger ---
                        Button {
                            text: "FORCE TRIGGER"
                            Layout.fillWidth: true
                            visible: scope.supportsForceTrigger
                            onClicked: scope.forceTrigger()

                            background: Rectangle {
                                color: "#5d4037"
                                border.color: "yellow"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: "FORCE TRIGGER"
                                color: "yellow"
                                font.pixelSize: 10
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // =============================================
                        // --- SIMULATION CONTROLS (test mode only) ---
                        // =============================================
                        ColumnLayout {
                            visible: scope.supportsWaveformEditor
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "SIMULATION"
                                color: "#ff9800"
                                font.bold: true
                            }

                            // --- CH1 Waveform ---
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Text { text: "CH1:"; color: "#00ff00"; font.pixelSize: 11; font.bold: true }
                                ComboBox {
                                    id: ch1WaveCombo
                                    model: scope.waveformTypes()
                                    currentIndex: scope.ch1Waveform
                                    Layout.fillWidth: true
                                    onCurrentIndexChanged: scope.ch1Waveform = currentIndex

                                    background: Rectangle {
                                        color: "#3d3d3d"
                                        border.color: "#00ff00"
                                        border.width: 1
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: ch1WaveCombo.displayText
                                        color: "#00ff00"
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 8
                                    }
                                }
                            }

                            // CH1 Frequency
                            ControlStepper {
                                label: "CH1 FREQ"
                                accentColor: "#00ff00"
                                value: scope.ch1Frequency
                                min: 1
                                max: 100000
                                step: 100
                                inputStep: 1
                                decimals: 0
                                suffix: " Hz"
                                showTextField: true
                                onValueChanged: scope.ch1Frequency = value
                            }

                            // CH1 Amplitude
                            ControlStepper {
                                label: "CH1 AMPL"
                                accentColor: "#00ff00"
                                value: scope.ch1Amplitude
                                min: 0.01
                                max: 10.0
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch1Amplitude = value
                            }

                            // CH1 Noise
                            ControlStepper {
                                label: "CH1 NOISE"
                                accentColor: "#00ff00"
                                value: scope.ch1Noise
                                min: 0
                                max: 2.0
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch1Noise = value
                            }

                            // CH1 DC Offset
                            ControlStepper {
                                label: "CH1 DC OFFSET"
                                accentColor: "#00ff00"
                                value: scope.ch1DcOffset
                                min: -5
                                max: 5
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch1DcOffset = value
                            }

                            Rectangle { height: 1; Layout.fillWidth: true; color: "#555" }

                            // --- CH2 Waveform ---
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Text { text: "CH2:"; color: "#00ffff"; font.pixelSize: 11; font.bold: true }
                                ComboBox {
                                    id: ch2WaveCombo
                                    model: scope.waveformTypes()
                                    currentIndex: scope.ch2Waveform
                                    Layout.fillWidth: true
                                    onCurrentIndexChanged: scope.ch2Waveform = currentIndex

                                    background: Rectangle {
                                        color: "#3d3d3d"
                                        border.color: "#00ffff"
                                        border.width: 1
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: ch2WaveCombo.displayText
                                        color: "#00ffff"
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 8
                                    }
                                }
                            }

                            // CH2 Frequency
                            ControlStepper {
                                label: "CH2 FREQ"
                                accentColor: "#00ffff"
                                value: scope.ch2Frequency
                                min: 1
                                max: 100000
                                step: 100
                                inputStep: 1
                                decimals: 0
                                suffix: " Hz"
                                showTextField: true
                                onValueChanged: scope.ch2Frequency = value
                            }

                            // CH2 Amplitude
                            ControlStepper {
                                label: "CH2 AMPL"
                                accentColor: "#00ffff"
                                value: scope.ch2Amplitude
                                min: 0.01
                                max: 10.0
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch2Amplitude = value
                            }

                            // CH2 Noise
                            ControlStepper {
                                label: "CH2 NOISE"
                                accentColor: "#00ffff"
                                value: scope.ch2Noise
                                min: 0
                                max: 2.0
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch2Noise = value
                            }

                            // CH2 DC Offset
                            ControlStepper {
                                label: "CH2 DC OFFSET"
                                accentColor: "#00ffff"
                                value: scope.ch2DcOffset
                                min: -5
                                max: 5
                                step: 0.05
                                inputStep: 0.01
                                decimals: 2
                                suffix: " V"
                                showTextField: true
                                onValueChanged: scope.ch2DcOffset = value
                            }

                            Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }
                        }

                        // --- Horizontal / Timebase ---
                        Text {
                            text: "HORIZONTAL"
                            color: "#fff"
                            font.bold: true
                        }

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

                        // Horizontal position (wired through controller)
                        ControlStepper {
                            id: hPositionControl
                            label: "H. POSITION"
                            accentColor: "#ffffff"
                            value: scope.horizontalPosition
                            min: -5
                            max: 5
                            step: 0.25
                            inputStep: 0.01
                            decimals: 2
                            suffix: " div"
                            showTextField: true
                            onValueChanged: scope.horizontalPosition = value
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                        // --- Channel 1 ---
                        Text {
                            text: "CH1"
                            color: "#00ff00"
                            font.bold: true
                        }

                        // Coupling Selector (AC / DC / GND)
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
                                triggerLevelControl.min = -maxV
                                triggerLevelControl.max = maxV
                                axisY.labelFormat = (value < 1.0 || vScale2.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                            }
                        }

                        // Channel offset (vertical position)
                        ControlStepper {
                            id: ch1OffsetControl
                            label: "POSITION"
                            accentColor: "#00ff00"
                            value: scope.ch1Offset
                            min: -5.0
                            max: 5.0
                            step: 0.05
                            inputStep: 0.01
                            decimals: 2
                            suffix: " V"
                            showTextField: true
                            onValueChanged: scope.ch1Offset = value
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

                        // Coupling Selector (AC / DC / GND)
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
                                triggerLevelControl.min = -maxV
                                triggerLevelControl.max = maxV
                                axisY.labelFormat = (value < 1.0 || vScale1.value < 1.0) ? "%.3f V" : "%.1f V"
                                Qt.callLater(chartView.repositionHandles)
                            }
                        }

                        // Channel offset (vertical position)
                        ControlStepper {
                            id: ch2OffsetControl
                            label: "POSITION"
                            accentColor: "#00ffff"
                            value: scope.ch2Offset
                            min: -5.0
                            max: 5.0
                            step: 0.05
                            inputStep: 0.01
                            decimals: 2
                            suffix: " V"
                            showTextField: true
                            onValueChanged: scope.ch2Offset = value
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
                            Text { text: "\u0394V:"; color: "#fff"; font.pixelSize: 11; Layout.preferredWidth: 24 }
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
                                scope.timebaseIndex = 16  // 1 ms/div
                                scope.ch1VoltsIndex = 9   // 1 V/div
                                scope.ch2VoltsIndex = 9
                                scope.triggerLevel = 0
                                scope.triggerSource = 0
                                scope.triggerEdge = 0
                                scope.ch1Offset = 0
                                scope.ch2Offset = 0
                                scope.horizontalPosition = 0
                                hPositionControl.value = 0
                                vScale1.value = scope.voltsPerDivSteps()[9]
                                vScale2.value = scope.voltsPerDivSteps()[9]
                                timebaseStepper.value = scope.timebaseSteps()[16]
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
