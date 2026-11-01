// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import ChristmasReckoning

// A typeset formula: MathJax-rendered SVG generated at build time (tools/formulas), falling back to
// the plain-text version if an SVG is missing. Shrinks to fit narrow (phone) widths.
Item {
    id: root
    property string formulaId   // "<console>-<step>"
    property string plain
    property real zoom: 1.0

    readonly property url svgUrl: formulaId.length ? "qrc:/formulas/" + formulaId + ".svg" : ""
    readonly property bool ready: measure.status === Image.Ready
    readonly property real naturalWidth: measure.implicitWidth * zoom
    readonly property real naturalHeight: measure.implicitHeight * zoom
    readonly property real shownWidth: width > 0 ? Math.min(naturalWidth, width) : naturalWidth

    implicitWidth: ready ? naturalWidth : fallback.implicitWidth
    implicitHeight: ready ? naturalHeight * shownWidth / Math.max(1, naturalWidth) : fallback.implicitHeight
    Accessible.role: Accessible.StaticText
    Accessible.name: plain

    Image { // intrinsic size only
        id: measure
        source: root.svgUrl
        visible: false
    }

    Image {
        visible: root.ready
        source: root.svgUrl
        width: root.shownWidth
        height: root.implicitHeight
        sourceSize.width: Math.ceil(root.shownWidth * 2) // crisp on high-DPI screens
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    Label {
        id: fallback
        width: root.width
        visible: !root.ready
        text: root.plain
        color: Theme.text
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
    }
}
