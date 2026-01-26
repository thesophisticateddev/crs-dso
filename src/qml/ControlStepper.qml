import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    // Properties that can be set when using the component
    property string label: "LABEL"
    property real value: 1.0
    property real step: 1.0
    property real min: 1.0
    property real max: 10.0
    property string suffix: ""
    property color accentColor: "white"

    spacing: 5
    Layout.fillWidth: true

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
            onClicked: if (root.value > root.min) root.value -= root.step
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "#1a1a1a"
            border.color: "#444"
            radius: 4

            Text {
                anchors.centerIn: parent
                text: root.value.toFixed(1) + root.suffix
                color: root.accentColor
                font.family: "Monospace"
                font.pixelSize: 14
            }
        }

        Button {
            text: "+"
            Layout.preferredWidth: 40
            onClicked: if (root.value < root.max) root.value += root.step
        }
    }
}
