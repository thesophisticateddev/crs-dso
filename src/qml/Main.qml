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
                // Removed updateSeries here to keep threading clean
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
                // Match the axisX.max so it spans the whole screen
                XYPoint { id: pStart; x: 0; y: 1.5 }
                XYPoint { id: pEnd; x: 1000; y: 1.5 }
            }


            MouseArea {
                id: triggerMouseArea // Add an ID
                anchors.fill: parent
                drag.target: triggerHandle
                drag.axis: Drag.YAxis

                Rectangle {
                    id: triggerHandle
                    width: 20; height: 20
                    // Ensure it's on the right edge
                    x: parent.width - 25
                    y: chartView.mapToPosition(Qt.point(0, 1.5)).y - 10
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
                            // mapToValue is the modern, preferred method [cite: 23]
                            var point = chartView.mapToValue(Qt.point(0, y + 10)) /*[cite: 23]*/
                            updateTriggerLevel(point.y) /*[cite: 24]*/
                        }
                    }
                }
            }

            function updateTriggerLevel(val) {
                // 1. Check for valid number to prevent further errors
                if (isNaN(val)) return;

                // 2. Update properties directly. No 'replace' function needed!
                pStart.y = val
                pEnd.y = val

                // Ensure the line always touches the right edge even if timebase changes
                pEnd.x = axisX.max

                // 3. Notify C++ worker
                signalGenerator.setTriggerLevel(val)
            }

            Component.onCompleted: {
                // Register the series with the signal generator for direct C++ updates
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

                    Text {
                        text: "MASTER TIMEBASE"
                        color: "#fff"
                        font.bold: true
                        Layout.topMargin: 10
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Time/Div: " + hScale.value + " ms"; color: "#aaa"; font.pixelSize: 11 }
                        Slider {
                            id: hScale
                            from: 100; to: 5000; value: 1000
                            Layout.fillWidth: true
                            onValueChanged: axisX.max = value
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
                        }
                    }

                    CheckBox {
                        text: "Show CH2"; checked: true; palette.windowText: "white"
                        onToggled: chan2.visible = checked
                    }

                    Item { Layout.fillHeight: true; Layout.preferredHeight: 20 }

                    Button {
                        text: "RUN / STOP"
                        highlighted: true
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "AUTOSET"
                        Layout.fillWidth: true
                        onClicked: {
                            vScale1.value = 5; vScale2.value = 5; hScale.value = 1000
                        }
                    }
                }
            }
        }
    }
}
