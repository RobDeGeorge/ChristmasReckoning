// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// A scenario parameter: label, current value, slider (linear or logarithmic) and its explanation.
// Changes are committed on release (and on keyboard steps), so dragging stays smooth.
ColumnLayout {
    id: root
    required property var param   // one entry of Mission.parameters
    property bool showHelp: true

    readonly property bool isLog: param.logScale
    // Log sliders can't reach 0, so they use a small positive floor and snap the far left to the minimum.
    readonly property real logFloor: param.min > 0 ? param.min : (param.unit === "s" ? 1e-6 : 1)

    function toPosition(v) {
        if (!isLog)
            return (v - param.min) / (param.max - param.min);
        if (v <= logFloor)
            return 0;
        return Math.log(v / logFloor) / Math.log(param.max / logFloor);
    }
    function fromPosition(p) {
        if (!isLog) {
            const raw = param.min + p * (param.max - param.min);
            return Math.round(raw / param.step) * param.step;
        }
        if (p <= 0.001)
            return param.min;
        const v = logFloor * Math.pow(param.max / logFloor, p);
        const magnitude = Math.pow(10, Math.floor(Math.log10(v)) - 1);
        return Math.round(v / magnitude) * magnitude;
    }

    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: root.param.label
            color: Theme.text
            font.pixelSize: Theme.fontBody
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
        Label {
            text: Fmt.param(slider.pressed ? root.fromPosition(slider.value) : root.param.value, root.param.unit)
            color: root.param.value === root.param.defaultValue ? Theme.textMuted : Theme.goldBright
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontSmall
        }
        ToolButton {
            visible: root.param.value !== root.param.defaultValue
            text: "↺"
            implicitWidth: 26
            implicitHeight: 24
            font.pixelSize: 15
            onClicked: Mission.setParameter(root.param.key, root.param.defaultValue)
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Reset to %1").arg(Fmt.param(root.param.defaultValue, root.param.unit))
            Accessible.name: qsTr("Reset %1").arg(root.param.label)
            palette.buttonText: Theme.textMuted
        }
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: 0
        to: 1
        stepSize: root.isLog ? 0.005 : Math.max(0.001, root.param.step / (root.param.max - root.param.min))
        value: root.toPosition(root.param.value)
        live: true
        Accessible.name: root.param.label
        Accessible.description: Fmt.param(root.param.value, root.param.unit)
        onPressedChanged: {
            if (!pressed)
                Mission.setParameter(root.param.key, root.fromPosition(value));
        }
        onMoved: {
            if (!pressed) // keyboard
                Mission.setParameter(root.param.key, root.fromPosition(value));
        }

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 4
            radius: 2
            color: Theme.line
            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: Theme.gold
            }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 18
            height: 18
            radius: 9
            color: slider.pressed ? Theme.goldBright : Theme.gold
            border.color: slider.visualFocus ? Theme.text : Theme.sky
            border.width: 2
        }
    }

    Label {
        visible: root.showHelp && root.param.help.length > 0
        Layout.fillWidth: true
        text: root.param.help
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
}
