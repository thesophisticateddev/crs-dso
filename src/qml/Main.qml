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
                color: "#00ff00" // Green
                useOpenGL: true  // Better performance for live data
                Component.onCompleted: signalGenerator.updateSeries(0,chan1)
            }

            LineSeries {
                id: chan2
                name: "CH 2"
                axisX: axisX
                axisY: axisY
                color: "#00ffff" // Cyan
                useOpenGL: true
                Component.onCompleted: signalGenerator.updateSeries(1,chan2)
            }

            // The actual Trigger Line
            LineSeries {
                id: triggerLine
                name: "Trigger"
                axisX: axisX
                axisY: axisY
                color: "yellow"
                width: 1
                style: Qt.DashLine // Makes it look distinct from signals

                // Initial position (horizontal line at 0V)
                XYPoint { x: 0; y: 1.5 }
                XYPoint { x: 400; y: 1.5 }
            }

            // Overlay to handle dragging the trigger level
            MouseArea {
                anchors.fill: parent
                drag.target: triggerHandle
                drag.axis: Drag.YAxis

                // Visual handle on the right side
                Rectangle {
                    id: triggerHandle
                    width: 20; height: 20
                    x: parent.width - 25
                    y: chartView.toPixels(Qt.point(0, 1.5)).y - 10 // Start at 1.5V
                    color: "yellow"
                    rotation: 45 // Diamond shape

                    Text {
                        text: "T"
                        anchors.centerIn: parent
                        color: "black"
                        rotation: -45
                        font.bold: true
                    }

                    onYChanged: {
                        if (drag.active) {
                            // Convert pixel position back to Voltage value
                            var point = chartView.toValue(Qt.point(0, y + 10))
                            updateTriggerLevel(point.y)
                        }
                    }
                }
            }

            function updateTriggerLevel(val) {
                // Snap the trigger line to the new voltage value
                triggerLine.replace(0, 0, val)
                triggerLine.replace(1, axisX.max, val)

                // Push the new trigger level to C++ backend
                signalGenerator.setTriggerLevel(val)
            }

            Component.onCompleted: {
                // Assuming your backend updateSeries can be called twice
                // or you have a modified function for multiple channels
                signalGenerator.updateSeries(chan1)
                // signalGenerator.updateSeries2(chan2) // Add this to your C++ later
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
