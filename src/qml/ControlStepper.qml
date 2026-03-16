import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string label: "LABEL"
    property real value: 1.0
    property real step: 1.0
    property real min: 1.0
    property real max: 10.0
    property string suffix: ""
    property color accentColor: "white"

    // When non-empty, +/- steps through this list instead of using min/max/step
    property var steps: []

    // Expose step index as a proper property (Fix #9)
    property int currentIndex: {
        for (var i = 0; i < steps.length; i++) {
            if (Math.abs(steps[i] - value) < 1e-9) return i
        }
        return 0
    }

    // Optional: custom display formatter function
    property var displayFormatter: null

    spacing: 5
    Layout.fillWidth: true

    function _stepIndex() {
        for (var i = 0; i < steps.length; i++) {
            if (Math.abs(steps[i] - value) < 1e-9) return i
        }
        return 0
    }

    Text {
        text: root.label
        color: "#aaa"
        font.pixelSize: 11
        font.bold: true
    }

    RowLayout {
        spacing: 2

        Button {
            text: "-"
            Layout.preferredWidth: 40
            onClicked: {
                if (root.steps.length > 0) {
                    var idx = root._stepIndex()
                    if (idx > 0) root.value = root.steps[idx - 1]
                } else {
                    if (root.value > root.min) root.value -= root.step
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "#1a1a1a"
            border.color: "#444"
            radius: 4

            Text {
                anchors.centerIn: parent
                text: {
                    if (root.displayFormatter)
                        return root.displayFormatter(root.value)
                    if (root.value < 1.0)
                        return (root.value * 1000).toFixed(0) + " mV"
                    return root.value.toFixed(1) + root.suffix
                }
                color: root.accentColor
                font.family: "Monospace"
                font.pixelSize: 14
            }
        }

        Button {
            text: "+"
            Layout.preferredWidth: 40
            onClicked: {
                if (root.steps.length > 0) {
                    var idx = root._stepIndex()
                    if (idx < root.steps.length - 1) root.value = root.steps[idx + 1]
                } else {
                    if (root.value < root.max) root.value += root.step
                }
            }
        }
    }
}
