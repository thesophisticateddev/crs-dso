import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

Window {
    id: window
    width: 1024
    height: 720
    visible: true
    title: qsTr("Dual Channel Digital Sampling Oscilloscope")
    color: "#1a1a1a"

    RowLayout {
        anchors.fill: parent
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
                XYPoint { id: pStart; x: 0; y: signalGenerator.triggerLevel }
                XYPoint { id: pEnd; x: 1000; y: signalGenerator.triggerLevel }
            }

            MouseArea {
                id: triggerMouseArea
                anchors.fill: parent
                drag.target: triggerHandle
                drag.axis: Drag.YAxis

                Rectangle {
                    id: triggerHandle
                    width: 20; height: 20
                    x: parent.width - 25
                    y: chartView.mapToPosition(Qt.point(0, signalGenerator.triggerLevel)).y - 10
                    color: "yellow"
                    rotation: 45

                    Text {
                        text: "T"
                        anchors.centerIn: parent
                        color: "black"
                        rotation: -45
                        font.bold: true
                    }

                    onYChanged: {
                        if (triggerMouseArea.drag.active) {
                            var point = chartView.mapToValue(Qt.point(0, y + 10))
                            chartView.updateTriggerLevel(point.y)
                        }
                    }
                }
            }

            function updateTriggerLevel(val) {
                if (isNaN(val)) return;

                // Clamp to axis range
                val = Math.max(axisY.min, Math.min(axisY.max, val))

                // Update trigger line
                pStart.y = val
                pEnd.y = val
                pEnd.x = axisX.max

                // Notify C++ and update UI
                signalGenerator.setTriggerLevel(val)
            }

            // Sync trigger handle position when level changes from control panel
            Connections {
                target: signalGenerator
                function onTriggerLevelChanged() {
                    if (!triggerMouseArea.drag.active) {
                        triggerHandle.y = chartView.mapToPosition(Qt.point(0, signalGenerator.triggerLevel)).y - 10
                        pStart.y = signalGenerator.triggerLevel
                        pEnd.y = signalGenerator.triggerLevel
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
                        text: signalGenerator.running ? "STOP" : "RUN"
                        highlighted: signalGenerator.running
                        Layout.fillWidth: true
                        Layout.topMargin: 10
                        onClicked: signalGenerator.setRunning(!signalGenerator.running)

                        background: Rectangle {
                            color: signalGenerator.running ? "#2e7d32" : "#c62828"
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
                            onCurrentIndexChanged: signalGenerator.setTriggerSource(currentIndex)

                            background: Rectangle {
                                color: "#3d3d3d"
                                border.color: currentIndex === 0 ? "#00ff00" : "#00ffff"
                                border.width: 2
                                radius: 4
                            }

                            contentItem: Text {
                                text: triggerSourceCombo.displayText
                                color: currentIndex === 0 ? "#00ff00" : "#00ffff"
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
                            onCurrentIndexChanged: signalGenerator.setTriggerEdge(currentIndex)

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
                                pEnd.x = value
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
                        step: 0.5
                        min: 0.5; max: 20.0
                        suffix: " V"
                        onValueChanged: {
                            if (value > vScale2.value) axisY.max = value
                            axisY.min = -value
                            // Update trigger slider range
                            triggerLevelSlider.from = -value
                            triggerLevelSlider.to = value
                        }
                    }

                    CheckBox {
                        text: "Show CH1"; checked: true; palette.windowText: "white"
                        onToggled: chan1.visible = checked
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                    // --- Channel 2 Controls ---
                    ControlStepper {
                        id: vScale2
                        label: "CH2 VOLTS/DIV"
                        accentColor: "#00ffff"
                        value: 5.0
                        step: 0.5
                        min: 0.5; max: 20.0
                        suffix: " V"
                        onValueChanged: {
                            if (value > vScale1.value) axisY.max = value
                            axisY.min = -value
                            // Update trigger slider range
                            triggerLevelSlider.from = -value
                            triggerLevelSlider.to = value
                        }
                    }

                    CheckBox {
                        text: "Show CH2"; checked: true; palette.windowText: "white"
                        onToggled: chan2.visible = checked
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: "#444" }

                    Button {
                        text: "AUTOSET"
                        Layout.fillWidth: true
                        onClicked: {
                            vScale1.value = 5
                            vScale2.value = 5
                            hScale.value = 1000
                            signalGenerator.setTriggerLevel(0)
                            signalGenerator.setTriggerSource(0)
                            signalGenerator.setTriggerEdge(0)
                        }
                    }

                    Item { Layout.fillHeight: true; Layout.preferredHeight: 20 }
                }
            }
        }
    }
}
