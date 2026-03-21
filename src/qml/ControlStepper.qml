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
    property bool showTextField: false
    property int decimals: 2
    property real inputStep: step
    property int inputWidth: 88

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

    function _clamp(v) {
        return Math.max(root.min, Math.min(root.max, v))
    }

    function _snap(v) {
        if (root.steps.length > 0) {
            var nearest = root.steps[0]
            var bestDistance = Math.abs(v - nearest)
            for (var i = 1; i < root.steps.length; i++) {
                var distance = Math.abs(v - root.steps[i])
                if (distance < bestDistance) {
                    nearest = root.steps[i]
                    bestDistance = distance
                }
            }
            return nearest
        }

        var next = root._clamp(v)
        if (root.inputStep > 0)
            next = Math.round(next / root.inputStep) * root.inputStep
        return root._clamp(next)
    }

    function _numericText(v) {
        return Number(v).toFixed(root.decimals)
    }

    function commitText(text) {
        var next = Number(text)
        if (isNaN(next)) {
            if (root.showTextField)
                editor.text = root._numericText(root.value)
            return
        }

        root.value = root._snap(next)
        if (root.showTextField)
            editor.text = root._numericText(root.value)
    }

    onValueChanged: {
        if (root.showTextField && !editor.activeFocus)
            editor.text = root._numericText(root.value)
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

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                TextField {
                    id: editor
                    visible: root.showTextField
                    Layout.preferredWidth: root.inputWidth
                    Layout.fillWidth: root.showTextField
                    text: root._numericText(root.value)
                    color: root.accentColor
                    selectByMouse: true
                    horizontalAlignment: TextInput.AlignHCenter
                    inputMethodHints: Qt.ImhFormattedNumbersOnly

                    background: Rectangle {
                        color: "transparent"
                        border.width: 0
                    }

                    onAccepted: root.commitText(text)
                    onEditingFinished: root.commitText(text)
                }

                Text {
                    visible: root.showTextField && root.suffix.length > 0
                    text: root.suffix
                    color: root.accentColor
                    font.family: "Monospace"
                    font.pixelSize: 12
                }

                Text {
                    visible: !root.showTextField
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
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
